#!/usr/bin/env bash
set -euo pipefail

SOURCE="${BASH_SOURCE[0]}"
while [ -L "$SOURCE" ]; do
  DIR="$(cd "$(dirname "$SOURCE")/../.." >/dev/null && pwd)"
  SOURCE="$(readlink "$SOURCE")"
  [[ "$SOURCE" != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$(cd "$(dirname "$SOURCE")/../.." >/dev/null && pwd)"

TOP_OBJECT_DELTAS_LIMIT=10
TOP_SUBTREE_DELTAS_LIMIT=8
INLINE_CONFIG_CHANGES_LIMIT=20
OBJECT_DELTAS_TITLE="Largest Object Deltas"
SUBTREE_DELTAS_TITLE="Largest Subtree Deltas"
CONFIG_CHANGES_TITLE="Config Changes"

usage() {
  cat <<'EOF'
Usage:
  ./tools/profile/kernel.sh
  ./tools/profile/kernel.sh diff <baseline_json> <current_json>
EOF
}

collect_from_volume() {
  local linux_volume

  linux_volume="vamos-kernel-linux"

  if ! docker image inspect vamos-builder >/dev/null 2>&1; then
    echo "Building vamos-builder docker image"
    export DOCKER_BUILDKIT=1
    docker build -f "$DIR/tools/build/Dockerfile.builder" -t vamos-builder "$DIR" \
      --build-arg UNAME="$(id -nu)" \
      --build-arg UID="$(id -u)" \
      --build-arg GID="$(id -g)"
  fi

  if ! docker volume inspect "$linux_volume" >/dev/null 2>&1; then
    echo "Missing Docker volume $linux_volume. Run ./vamos build kernel first."
    exit 1
  fi

  if ! docker run --rm --entrypoint sh -v "$DIR:$DIR" -v "$linux_volume:$DIR/kernel/linux" -w "$DIR" vamos-builder -lc "test -f '$DIR/kernel/linux/out/vmlinux'" >/dev/null 2>&1; then
    echo "Missing kernel build output in Docker volume. Run ./vamos build kernel first."
    exit 1
  fi

  docker run --rm \
    --entrypoint bash \
    -u "$(id -u):$(id -g)" \
    -v "$DIR:$DIR" \
    -v "$linux_volume:$DIR/kernel/linux" \
    -w "$DIR" \
    vamos-builder \
    "$DIR/tools/profile/kernel.sh"
}

format_bytes() {
  local bytes=${1:-0}
  local sign=${2:-}
  awk -v bytes="$bytes" -v sign="$sign" '
    function human(v,    units, idx, out) {
      split("B KiB MiB GiB TiB", units, " ")
      idx = 1
      while (v >= 1024 && idx < 5) {
        v /= 1024
        idx++
      }
      if (idx == 1) {
        out = sprintf("%d %s", v, units[idx])
      } else {
        out = sprintf("%.1f %s", v, units[idx])
        sub(/\.0 /, " ", out)
      }
      return out
    }
    BEGIN { print sign human(bytes) }
  '
}

format_delta_bytes() {
  local bytes=${1:-0}
  local abs=$bytes
  local sign=""
  if [ "$abs" -lt 0 ]; then
    abs=$(( -abs ))
    sign="-"
  elif [ "$abs" -gt 0 ]; then
    sign="+"
  fi

  format_bytes "$abs" "$sign"
}

fmt_percent_change() {
  local old=${1:-0}
  local new=${2:-0}

  if [ "$old" -eq 0 ]; then
    if [ "$new" -eq 0 ]; then
      echo "0.0%"
    else
      echo "new"
    fi
    return
  fi

  awk -v old="$old" -v new="$new" '
    BEGIN {
      delta = new - old
      pct = (delta * 100.0) / old
      if (pct > 0) {
        printf("+%.1f%%", pct)
      } else {
        printf("%.1f%%", pct)
      }
    }
  '
}

read_section_size() {
  local binary=$1
  local section=$2
  size -A -d "$binary" 2>/dev/null | awk -v section="$section" '$1 == section { print $2; found=1 } END { if (!found) print 0 }'
}

normalize_config_to_json() {
  local config_path=$1
  awk '
    /^CONFIG_/ {
      split($0, a, "=")
      key=a[1]
      value=substr($0, length(key) + 2)
      printf("%s\t%s\n", key, value)
      next
    }
    /^# CONFIG_[A-Za-z0-9_]+ is not set$/ {
      key=$2
      printf("%s\tn\n", key)
    }
  ' "$config_path" | jq -Rn '
    reduce inputs as $line ({}; ($line | split("\t")) as $parts | .[$parts[0]] = $parts[1])
  '
}

filter_meaningful_objects() {
  jq '
    map(select(
      (.path | test("(^|/)(vmlinux\\.o|\\.vmlinux\\.export\\.o|\\.tmp_vmlinux[0-9]+\\.kallsyms\\.o|\\.tmp_vmlinux\\.kallsyms[0-9]*\\.o|\\.tmp_vmlinux\\.btf$|\\.btf\\.vmlinux\\.bin\\.o$)") | not)
    ))
  '
}

emit_top_deltas_table() {
  local title=$1
  local limit=$2
  local rows_json=$3

  if [ "$(echo "$rows_json" | jq 'length')" -eq 0 ]; then
    return
  fi

  echo "### $title"
  echo ""
  echo "| Item | Change |"
  echo "|------|--------|"
  echo "$rows_json" | jq -r --argjson limit "$limit" '
    .[:$limit][] | [(.label | gsub("\\|"; "\\\\|")), .display_delta] | @tsv
  ' | while IFS=$'\t' read -r label delta; do
    echo "| \`$label\` | $delta |"
  done
  echo ""
}

rows_with_display_delta() {
  local rows_json=$1
  local limit=$2
  echo "$rows_json" | jq -r --argjson limit "$limit" '.[:$limit][] | [.label, (.delta | tostring)] | @tsv' | while IFS=$'\t' read -r label delta; do
    printf '{"label":%s,"display_delta":%s}\n' \
      "$(printf '%s' "$label" | jq -R .)" \
      "$(printf '%s' "$(format_delta_bytes "$delta")" | jq -R .)"
  done | jq -s '.'
}

if [ "${1:-}" != "diff" ]; then
  KERNEL_OUT_DIR="$DIR/kernel/linux/out"
  OUTPUT_DIR="$DIR/build"

  if [ "$#" -ne 0 ]; then
    usage
    exit 1
  fi

  if [ "$(uname)" = "Darwin" ]; then
    collect_from_volume
    exit 0
  fi

  command -v jq >/dev/null || { echo "jq is required"; exit 1; }
  command -v size >/dev/null || { echo "size is required"; exit 1; }

  mkdir -p "$OUTPUT_DIR"

  IMAGE="$KERNEL_OUT_DIR/arch/arm64/boot/Image"
  IMAGE_GZ="$KERNEL_OUT_DIR/arch/arm64/boot/Image.gz"
  VMLINUX="$KERNEL_OUT_DIR/vmlinux"
  CONFIG="$KERNEL_OUT_DIR/.config"
  BOOT_IMG="$OUTPUT_DIR/boot.img"
  for required in "$IMAGE" "$IMAGE_GZ" "$VMLINUX" "$CONFIG"; do
    if [ ! -f "$required" ]; then
      echo "Missing required kernel build output: $required"
      exit 1
    fi
  done

  image_size=$(wc -c < "$IMAGE" | tr -d '[:space:]')
  image_gz_size=$(wc -c < "$IMAGE_GZ" | tr -d '[:space:]')
  vmlinux_size=$(wc -c < "$VMLINUX" | tr -d '[:space:]')
  boot_img_size=""
  if [ -f "$BOOT_IMG" ]; then
    boot_img_size=$(wc -c < "$BOOT_IMG" | tr -d '[:space:]')
  fi

  text_size=$(read_section_size "$VMLINUX" ".text")
  rodata_size=$(read_section_size "$VMLINUX" ".rodata")
  data_size=$(read_section_size "$VMLINUX" ".data")
  bss_size=$(read_section_size "$VMLINUX" ".bss")

  OBJECTS_JSON=$(find "$KERNEL_OUT_DIR" -type f -name '*.o' -printf '%P\t%s\n' \
    | sort \
    | jq -Rn '
        [inputs
         | select(length > 0)
         | split("\t")
         | {path: .[0], bytes: (.[1] | tonumber)}]
      ' \
    | filter_meaningful_objects)

  config_json_file=$(mktemp)
  sections_json_file=$(mktemp)
  trap 'rm -f "$config_json_file" "$sections_json_file"' RETURN

  normalize_config_to_json "$CONFIG" > "$config_json_file"
  jq -n \
    --arg text "$text_size" \
    --arg rodata "$rodata_size" \
    --arg data "$data_size" \
    --arg bss "$bss_size" \
    '{text: ($text | tonumber), rodata: ($rodata | tonumber), data: ($data | tonumber), bss: ($bss | tonumber)}' \
    > "$sections_json_file"
  printf '%s\n' "$OBJECTS_JSON" > "$OUTPUT_DIR/kernel-objects.json"

  jq -n \
    --slurpfile sections "$sections_json_file" \
    --slurpfile config "$config_json_file" \
    --arg generated_at "$(date -u +"%Y-%m-%dT%H:%M:%SZ")" \
    --arg image_size "$image_size" \
    --arg image_gz_size "$image_gz_size" \
    --arg vmlinux_size "$vmlinux_size" \
    --arg boot_img_size "$boot_img_size" \
    --arg object_count "$(echo "$OBJECTS_JSON" | jq 'length')" \
    '({
      generated_at: $generated_at,
      image_size: ($image_size | tonumber),
      image_gz_size: ($image_gz_size | tonumber),
      vmlinux_size: ($vmlinux_size | tonumber),
      sections: $sections[0],
      config: $config[0],
      object_count: ($object_count | tonumber)
    } + if $boot_img_size == "" then {} else {boot_img_size: ($boot_img_size | tonumber)} end)' > "$OUTPUT_DIR/kernel-profile.json"

  {
    echo "| Metric | Value |"
    echo "|--------|-------|"
    echo "| Image | $(format_bytes "$image_size") |"
    echo "| Image.gz | $(format_bytes "$image_gz_size") |"
    echo "| vmlinux | $(format_bytes "$vmlinux_size") |"
    if [ -n "$boot_img_size" ]; then
      echo "| boot.img | $(format_bytes "$boot_img_size") |"
    fi
    echo "| .text | $(format_bytes "$text_size") |"
    echo "| .rodata | $(format_bytes "$rodata_size") |"
    echo "| .data | $(format_bytes "$data_size") |"
    echo "| .bss | $(format_bytes "$bss_size") |"
    echo "| Objects tracked | $(echo "$OBJECTS_JSON" | jq 'length') |"
    echo ""
    echo "### Largest Objects"
    echo ""
    echo "| Object | Size |"
    echo "|--------|------|"
    echo "$OBJECTS_JSON" | jq -r 'sort_by(.bytes) | reverse | .[:15][] | [.path, (.bytes|tostring)] | @tsv' | while IFS=$'\t' read -r path bytes; do
      echo "| \`$path\` | $(format_bytes "$bytes") |"
    done
  } > "$OUTPUT_DIR/kernel-profile.md"

  echo "Kernel profile written to $OUTPUT_DIR"
  exit 0
fi

if [ "${1:-}" = "diff" ]; then
  BASELINE=${2:-}
  CURRENT=${3:-}

  if [ ! -f "$BASELINE" ] || [ ! -f "$CURRENT" ]; then
    echo "**No baseline available for comparison.**"
    exit 0
  fi

  command -v jq >/dev/null || { echo "jq is required"; exit 1; }

  metric_rows=$(jq -n --slurpfile old "$BASELINE" --slurpfile new "$CURRENT" '
    def metric_row($label; $key):
      {
        label: $label,
        old: ($old[0][$key] // 0),
        new: ($new[0][$key] // 0)
      };
    def section_row($label; $key):
      {
        label: $label,
        old: ($old[0].sections[$key] // 0),
        new: ($new[0].sections[$key] // 0)
      };
    [
      metric_row("Image"; "image_size"),
      metric_row("Image.gz"; "image_gz_size"),
      metric_row("vmlinux"; "vmlinux_size"),
      metric_row("boot.img"; "boot_img_size"),
      section_row(".text"; "text"),
      section_row(".rodata"; "rodata"),
      section_row(".data"; "data"),
      section_row(".bss"; "bss")
    ]
  ')

  echo "### Size Changes"
  echo ""
  echo "| Metric | Change |"
  echo "|--------|--------|"
  echo "$metric_rows" | jq -r '.[] | [.label, (.old|tostring), (.new|tostring)] | @tsv' | while IFS=$'\t' read -r label old new; do
    delta=$((new - old))
    delta_human=$(format_delta_bytes "$delta")
    pct=$(fmt_percent_change "$old" "$new")
    echo "| $label | $delta_human ($pct) |"
  done
  echo ""

  object_rows=$(jq -n \
    --slurpfile oldObjs "$(dirname "$BASELINE")/kernel-objects.json" \
    --slurpfile newObjs "$(dirname "$CURRENT")/kernel-objects.json" '
      def rows($src):
        $src[0] // [];
      def mapify($rows):
        reduce $rows[] as $row ({}; .[$row.path] = ($row.bytes // 0));
      def union_keys($a; $b):
        (($a | keys_unsorted) + ($b | keys_unsorted) | unique);
      (mapify(rows($oldObjs))) as $oldMap
      | (mapify(rows($newObjs))) as $newMap
      | [union_keys($oldMap; $newMap)[] as $path
          | {
              label: $path,
              old: ($oldMap[$path] // 0),
              new: ($newMap[$path] // 0),
              delta: (($newMap[$path] // 0) - ($oldMap[$path] // 0))
            }
          | select((.delta | if . < 0 then -. else . end) >= 1024)
        ]
      | sort_by((.delta | if . < 0 then -. else . end), .delta) | reverse
    ')

  if [ "$(echo "$object_rows" | jq 'length')" -gt 0 ]; then
    object_rows_with_display=$(rows_with_display_delta "$object_rows" "$TOP_OBJECT_DELTAS_LIMIT")
    emit_top_deltas_table "$OBJECT_DELTAS_TITLE" "$TOP_OBJECT_DELTAS_LIMIT" "$object_rows_with_display"
  else
    echo "### $OBJECT_DELTAS_TITLE"
    echo ""
    echo "No object changes."
    echo ""
  fi

  subtree_rows=$(jq -n \
    --slurpfile oldObjs "$(dirname "$BASELINE")/kernel-objects.json" \
    --slurpfile newObjs "$(dirname "$CURRENT")/kernel-objects.json" '
      def rows($src):
        $src[0] // [];
      def subtree($path):
        ($path | split("/")) as $parts
        | if ($parts | length) >= 2 and $parts[0] == "arch" then
            ($parts[0] + "/" + $parts[1])
          elif ($parts | length) >= 1 then
            $parts[0]
          else
            "(root)"
          end;
      def mapify($rows):
        reduce $rows[] as $row ({}; .[subtree($row.path)] = ((.[subtree($row.path)] // 0) + ($row.bytes // 0)));
      def union_keys($a; $b):
        (($a | keys_unsorted) + ($b | keys_unsorted) | unique);
      (mapify(rows($oldObjs))) as $oldMap
      | (mapify(rows($newObjs))) as $newMap
      | [union_keys($oldMap; $newMap)[] as $path
          | {
              label: $path,
              old: ($oldMap[$path] // 0),
              new: ($newMap[$path] // 0),
              delta: (($newMap[$path] // 0) - ($oldMap[$path] // 0))
            }
          | select((.delta | if . < 0 then -. else . end) >= 1024)
        ]
      | sort_by((.delta | if . < 0 then -. else . end), .delta) | reverse
    ')

  if [ "$(echo "$subtree_rows" | jq 'length')" -gt 0 ]; then
    subtree_rows_with_display=$(rows_with_display_delta "$subtree_rows" "$TOP_SUBTREE_DELTAS_LIMIT")
    emit_top_deltas_table "$SUBTREE_DELTAS_TITLE" "$TOP_SUBTREE_DELTAS_LIMIT" "$subtree_rows_with_display"
  else
    echo "### $SUBTREE_DELTAS_TITLE"
    echo ""
    echo "No subtree changes."
    echo ""
  fi

  config_rows=$(jq -n --slurpfile old "$BASELINE" --slurpfile new "$CURRENT" '
    ($old[0].config // {}) as $oldCfg
    | ($new[0].config // {}) as $newCfg
    | (($oldCfg | keys_unsorted) + ($newCfg | keys_unsorted) | unique) as $keys
    | [$keys[] as $key
        | {
            key: $key,
            old: ($oldCfg[$key] // "n"),
            new: ($newCfg[$key] // "n")
          }
        | select(.old != .new)
      ]
    | sort_by(.key)
  ')

  if [ "$(echo "$config_rows" | jq 'length')" -gt 0 ]; then
    config_count=$(echo "$config_rows" | jq 'length')
    if [ "$config_count" -gt "$INLINE_CONFIG_CHANGES_LIMIT" ]; then
      echo "<details><summary><h3>$CONFIG_CHANGES_TITLE ($config_count total)</h3></summary>"
    else
      echo "### $CONFIG_CHANGES_TITLE ($config_count total)"
    fi
    echo ""
    echo "| Option | Before | After |"
    echo "|--------|--------|-------|"
    echo "$config_rows" | jq -r '.[] | [.key, .old, .new] | @tsv' | while IFS=$'\t' read -r key old new; do
      echo "| \`$key\` | $old | $new |"
    done
    if [ "$config_count" -gt "$INLINE_CONFIG_CHANGES_LIMIT" ]; then
      echo "</details>"
    fi
    echo ""
  else
    echo "### $CONFIG_CHANGES_TITLE"
    echo ""
    echo "No config changes."
    echo ""
  fi

  exit 0
fi

usage
exit 1
