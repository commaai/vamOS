#!/usr/bin/env bash
set -e

# Rootfs profiling — collects size data from the system-builder container
# Usage:
#   ./profile_rootfs.sh                              # collect profile (needs container)
#   ./profile_rootfs.sh diff <baseline> <current>    # diff two profile JSONs (host only)

# ── Diff mode ──────────────────────────────────────────────────────────────────
if [ "${1:-}" = "diff" ]; then
  BASELINE="$2"
  CURRENT="$3"

  if [ ! -f "$BASELINE" ] || [ ! -f "$CURRENT" ]; then
    echo "**No baseline available for comparison.**"
    exit 0
  fi

  command -v jq &>/dev/null || { echo "jq required for diff"; exit 1; }

  # Image size delta
  OLD_RAW=$(jq -r '.image_size_used_bytes // 0' "$BASELINE")
  NEW_RAW=$(jq -r '.image_size_used_bytes // 0' "$CURRENT")
  OLD_SPARSE=$(jq -r '.image_size_sparse_bytes // 0' "$BASELINE")
  NEW_SPARSE=$(jq -r '.image_size_sparse_bytes // 0' "$CURRENT")

  fmt_delta() {
    local old=$1 new=$2
    local delta=$((new - old))
    local sign="+"
    [ $delta -lt 0 ] && sign=""
    local old_mb=$(echo "scale=1; $old / 1048576" | bc)
    local new_mb=$(echo "scale=1; $new / 1048576" | bc)
    local delta_mb=$(echo "scale=1; $delta / 1048576" | bc)
    echo "${old_mb}MB → ${new_mb}MB (${sign}${delta_mb}MB)"
  }

  echo "| Metric | Change |"
  echo "|--------|--------|"
  echo "| Used space | $(fmt_delta "$OLD_RAW" "$NEW_RAW") |"
  if [ "$OLD_SPARSE" != "0" ] && [ "$NEW_SPARSE" != "0" ]; then
    echo "| Sparse image | $(fmt_delta "$OLD_SPARSE" "$NEW_SPARSE") |"
  fi

  OLD_PKGS=$(jq -r '.package_count // 0' "$BASELINE")
  NEW_PKGS=$(jq -r '.package_count // 0' "$CURRENT")
  echo "| Package count | ${OLD_PKGS} → ${NEW_PKGS} |"
  echo ""

  # Added/removed packages
  ADDED=$(comm -23 <(jq -r '.packages[].name' "$CURRENT" | sort) <(jq -r '.packages[].name' "$BASELINE" | sort))
  REMOVED=$(comm -23 <(jq -r '.packages[].name' "$BASELINE" | sort) <(jq -r '.packages[].name' "$CURRENT" | sort))

  if [ -n "$ADDED" ]; then
    echo "**Added packages:** $(echo "$ADDED" | tr '\n' ',' | sed 's/,$//' | sed 's/,/, /g')"
    echo ""
  fi
  if [ -n "$REMOVED" ]; then
    echo "**Removed packages:** $(echo "$REMOVED" | tr '\n' ',' | sed 's/,$//' | sed 's/,/, /g')"
    echo ""
  fi

  # Directory size changes > 1MB
  echo "<details><summary>Directory size changes (>1MB)</summary>"
  echo ""
  echo "| Directory | Change |"
  echo "|-----------|--------|"
  jq -r '.top_directories[] | "\(.path)\t\(.bytes)"' "$BASELINE" > /tmp/prof_dirs_old 2>/dev/null || true
  jq -r '.top_directories[] | "\(.path)\t\(.bytes)"' "$CURRENT" > /tmp/prof_dirs_new 2>/dev/null || true
  while IFS=$'\t' read -r path new_bytes; do
    old_bytes=$(awk -F'\t' -v p="$path" '$1==p {print $2}' /tmp/prof_dirs_old)
    old_bytes=${old_bytes:-0}
    delta=$((new_bytes - old_bytes))
    abs_delta=${delta#-}
    if [ "$abs_delta" -gt 1048576 ]; then
      sign="+"
      [ $delta -lt 0 ] && sign=""
      delta_mb=$(echo "scale=1; $delta / 1048576" | bc)
      echo "| ${path} | ${sign}${delta_mb}MB |"
    fi
  done < /tmp/prof_dirs_new
  echo "</details>"
  rm -f /tmp/prof_dirs_old /tmp/prof_dirs_new

  exit 0
fi

# ── Profile mode ───────────────────────────────────────────────────────────────

if [ -z "${MOUNT_CONTAINER_ID:-}" ]; then
  echo "Error: profile collection can only be run during 'vamos build system'"
  exit 1
fi

: "${MOUNT_CONTAINER_ID:?MOUNT_CONTAINER_ID required}"
: "${ROOTFS_DIR:?ROOTFS_DIR required}"
: "${ROOTFS_IMAGE:?ROOTFS_IMAGE required}"
: "${OUTPUT_DIR:?OUTPUT_DIR required}"

exec_container() {
  docker exec "$MOUNT_CONTAINER_ID" "$@"
}

mkdir -p "$OUTPUT_DIR"

PROFILE_START=$(date +%s%N)
echo "Collecting rootfs profile data..."

# Used space on ext4

DF_LINE=$(exec_container df -B1 "$ROOTFS_DIR" | tail -1)
USED_BYTES=$(echo "$DF_LINE" | awk '{print $3}')
TOTAL_BYTES=$(echo "$DF_LINE" | awk '{print $2}')
AVAIL_BYTES=$(echo "$DF_LINE" | awk '{print $4}')


# Summary stats

FILE_COUNT=$(exec_container find "$ROOTFS_DIR" -xdev -type f | wc -l)
DIR_COUNT=$(exec_container find "$ROOTFS_DIR" -xdev -type d | wc -l)
SYMLINK_COUNT=$(exec_container find "$ROOTFS_DIR" -xdev -type l | wc -l)


# xbps packages — parse pkgdb plist directly (single XML file with all packages)

PACKAGES_JSON="[]"
PKG_COUNT=0
PKGDB="$ROOTFS_DIR/usr/lib/xbps-db/pkgdb-0.38.plist"
if exec_container test -f "$PKGDB" 2>/dev/null; then
  PACKAGES_RAW=$(exec_container awk '
    /<key>pkgver<\/key>/ { getline; gsub(/.*<string>|<\/string>.*/, ""); pkgver=$0 }
    /<key>installed_size<\/key>/ { getline; gsub(/.*<integer>|<\/integer>.*/, ""); if(pkgver) print $0 "\t" pkgver; pkgver="" }
  ' "$PKGDB")
  if [ -n "$PACKAGES_RAW" ]; then
    PACKAGES_JSON=$(echo "$PACKAGES_RAW" | sort -rn | jq -Rn '
      [inputs | split("\t") | {name: .[1], bytes: (.[0] | tonumber)}]
    ')
    PKG_COUNT=$(echo "$PACKAGES_JSON" | jq 'length')
  fi
fi

# Top 30 directories by size

TOP_DIRS_JSON=$(exec_container du -x --max-depth=3 "$ROOTFS_DIR" 2>/dev/null \
  | sort -rn | head -30 \
  | awk -v root="$ROOTFS_DIR" '{
      path=$2; sub(root, "", path); if(path=="") path="/";
      print $1 * 1024 "\t" path
    }' \
  | jq -Rn '[inputs | split("\t") | {path: .[1], bytes: (.[0] | tonumber)}]')

# Top 30 files by size

TOP_FILES_JSON=$(exec_container find "$ROOTFS_DIR" -xdev -type f -printf '%s %p\n' 2>/dev/null \
  | sort -rn | head -30 \
  | awk -v root="$ROOTFS_DIR" '{
      path=$2; sub(root, "", path);
      print $1 "\t" path
    }' \
  | jq -Rn '[inputs | split("\t") | {path: .[1], bytes: (.[0] | tonumber)}]')

# Python venv breakdown

VENV_JSON="[]"
VENV_TOTAL=0
VENV_DIR="$ROOTFS_DIR/usr/local/venv/lib"
if exec_container test -d "$VENV_DIR" 2>/dev/null; then
  VENV_JSON=$(exec_container du --max-depth=2 "$VENV_DIR" 2>/dev/null \
    | sort -rn \
    | awk -v root="$ROOTFS_DIR" '{
        path=$2; sub(root, "", path);
        print $1 * 1024 "\t" path
      }' \
    | jq -Rn '[inputs | split("\t") | {path: .[1], bytes: (.[0] | tonumber)}]')
  VENV_TOTAL=$(exec_container du -sb "$VENV_DIR" 2>/dev/null | awk '{print $1}')
  VENV_TOTAL=${VENV_TOTAL:-0}
fi

# Firmware sizes

FIRMWARE_BYTES=0
if exec_container test -d "$ROOTFS_DIR/lib/firmware" 2>/dev/null; then
  FIRMWARE_BYTES=$(exec_container du -sb "$ROOTFS_DIR/lib/firmware" 2>/dev/null | awk '{print $1}')
fi

# Shared libs (top 30)

SHARED_LIBS_JSON=$(exec_container find "$ROOTFS_DIR/usr/lib" -name '*.so*' -type f -printf '%s %p\n' 2>/dev/null \
  | sort -rn | head -30 \
  | awk -v root="$ROOTFS_DIR" '{
      path=$2; sub(root, "", path);
      print $1 "\t" path
    }' \
  | jq -Rn '[inputs | split("\t") | {path: .[1], bytes: (.[0] | tonumber)}]')

# Calculate "other" bytes (used - known categories)
XBPS_TOTAL=0
if [ "$PACKAGES_JSON" != "[]" ]; then
  XBPS_TOTAL=$(echo "$PACKAGES_JSON" | jq '[.[].bytes] | add // 0')
fi
OTHER_BYTES=$((USED_BYTES - XBPS_TOTAL - VENV_TOTAL - FIRMWARE_BYTES))
[ $OTHER_BYTES -lt 0 ] && OTHER_BYTES=0

# ── Assemble JSON ──────────────────────────────────────────────────────────────
jq -n \
  --arg used "$USED_BYTES" \
  --arg total "$TOTAL_BYTES" \
  --arg avail "$AVAIL_BYTES" \
  --arg files "$FILE_COUNT" \
  --arg dirs "$DIR_COUNT" \
  --arg symlinks "$SYMLINK_COUNT" \
  --arg pkgs "$PKG_COUNT" \
  --argjson packages "$PACKAGES_JSON" \
  --argjson top_directories "$TOP_DIRS_JSON" \
  --argjson top_files "$TOP_FILES_JSON" \
  --argjson python_venv "$VENV_JSON" \
  --arg venv_total "$VENV_TOTAL" \
  --arg firmware "$FIRMWARE_BYTES" \
  --argjson shared_libs "$SHARED_LIBS_JSON" \
  --arg xbps_total "$XBPS_TOTAL" \
  --arg other "$OTHER_BYTES" \
  '{
    image_size_used_bytes: ($used | tonumber),
    image_size_total_bytes: ($total | tonumber),
    image_size_avail_bytes: ($avail | tonumber),
    file_count: ($files | tonumber),
    dir_count: ($dirs | tonumber),
    symlink_count: ($symlinks | tonumber),
    package_count: ($pkgs | tonumber),
    packages: $packages,
    top_directories: $top_directories,
    top_files: $top_files,
    python_venv: $python_venv,
    python_venv_bytes: ($venv_total | tonumber),
    firmware_bytes: ($firmware | tonumber),
    shared_libs: $shared_libs,
    categories: {
      xbps_packages: ($xbps_total | tonumber),
      python_venv: ($venv_total | tonumber),
      firmware: ($firmware | tonumber),
      other: ($other | tonumber)
    }
  }' > "$OUTPUT_DIR/rootfs-profile.json"

# ── Generate Markdown ──────────────────────────────────────────────────────────
USED_MB=$(echo "scale=1; $USED_BYTES / 1048576" | bc)
TOTAL_MB=$(echo "scale=1; $TOTAL_BYTES / 1048576" | bc)

fmt_mb() { echo "scale=1; $1 / 1048576" | bc; }
fmt_pct() { echo "scale=1; $1 * 100 / $USED_BYTES" | bc; }

{
  echo "| Metric | Value |"
  echo "|--------|-------|"
  echo "| Used space | ${USED_MB}MB / ${TOTAL_MB}MB |"
  echo "| Files | ${FILE_COUNT} |"
  echo "| Directories | ${DIR_COUNT} |"
  echo "| Symlinks | ${SYMLINK_COUNT} |"
  echo "| Packages | ${PKG_COUNT} |"
  echo ""

  echo "### Top 10 Directories"
  echo ""
  echo "| Directory | Size |"
  echo "|-----------|------|"
  echo "$TOP_DIRS_JSON" | jq -r '.[1:11][] | "| \(.path) | \(.bytes / 1048576 | . * 10 | floor / 10)MB |"'
  echo ""

  echo "### Category Breakdown"
  echo ""
  echo "| Category | Size | % |"
  echo "|----------|------|---|"
  echo "| xbps packages | $(fmt_mb "$XBPS_TOTAL")MB | $(fmt_pct "$XBPS_TOTAL")% |"
  echo "| Python venv | $(fmt_mb "$VENV_TOTAL")MB | $(fmt_pct "$VENV_TOTAL")% |"
  echo "| Firmware | $(fmt_mb "$FIRMWARE_BYTES")MB | $(fmt_pct "$FIRMWARE_BYTES")% |"
  echo "| Other | $(fmt_mb "$OTHER_BYTES")MB | $(fmt_pct "$OTHER_BYTES")% |"
  echo ""

  echo "### Top 10 Packages by Size"
  echo ""
  echo "| Package | Size |"
  echo "|---------|------|"
  echo "$PACKAGES_JSON" | jq -r '.[:10][] | "| \(.name) | \(.bytes / 1048576 | . * 10 | floor / 10)MB |"'
  echo ""

  echo "<details><summary><h3>Top 30 Files by Size</h3></summary>"
  echo ""
  echo "| File | Size |"
  echo "|------|------|"
  echo "$TOP_FILES_JSON" | jq -r '.[] | "| \(.path) | \(.bytes / 1048576 | . * 10 | floor / 10)MB |"'
  echo "</details>"
} > "$OUTPUT_DIR/rootfs-profile.md"

# Print summary to terminal
echo ""
echo "=== Rootfs Profile ==="
echo "Used: ${USED_MB}MB / ${TOTAL_MB}MB"
echo "Files: ${FILE_COUNT} | Dirs: ${DIR_COUNT} | Symlinks: ${SYMLINK_COUNT} | Packages: ${PKG_COUNT}"
echo "Categories: xbps=$(fmt_mb "$XBPS_TOTAL")MB  venv=$(fmt_mb "$VENV_TOTAL")MB  firmware=$(fmt_mb "$FIRMWARE_BYTES")MB  other=$(fmt_mb "$OTHER_BYTES")MB"
echo "Profiling completed in $(( ($(date +%s%N) - PROFILE_START) / 1000000 ))ms"
echo "Output: $OUTPUT_DIR/rootfs-profile.json, $OUTPUT_DIR/rootfs-profile.md"
