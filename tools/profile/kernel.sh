#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./tools/profile/kernel.sh collect [kernel_out_dir] [output_dir]
  ./tools/profile/kernel.sh diff <baseline_json> <current_json>
EOF
}

fmt_binary_bytes() {
  local bytes=${1:-0}
  local abs=$bytes
  local sign=""
  if [ "$abs" -lt 0 ]; then
    abs=$(( -abs ))
    sign="-"
  elif [ "$abs" -gt 0 ]; then
    sign="+"
  fi

  awk -v bytes="$abs" -v sign="$sign" '
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

fmt_bytes() {
  local bytes=${1:-0}
  awk -v bytes="$bytes" '
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
    BEGIN { print human(bytes) }
  '
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
  echo "$rows_json" | jq -r '.[] | [.label, (.delta | tostring)] | @tsv' | while IFS=$'\t' read -r label delta; do
    printf '{"label":%s,"display_delta":%s}\n' \
      "$(printf '%s' "$label" | jq -R .)" \
      "$(printf '%s' "$(fmt_binary_bytes "$delta")" | jq -R .)"
  done | jq -s '.'
}

if [ "${1:-}" = "collect" ]; then
  KERNEL_OUT_DIR=${2:-}
  OUTPUT_DIR=${3:-}

  if [ -z "$KERNEL_OUT_DIR" ] || [ -z "$OUTPUT_DIR" ]; then
    usage
    exit 1
  fi

  command -v jq >/dev/null || { echo "jq is required"; exit 1; }
  command -v nm >/dev/null || { echo "nm is required"; exit 1; }
  command -v size >/dev/null || { echo "size is required"; exit 1; }

  mkdir -p "$OUTPUT_DIR"

  IMAGE="$KERNEL_OUT_DIR/arch/arm64/boot/Image"
  IMAGE_GZ="$KERNEL_OUT_DIR/arch/arm64/boot/Image.gz"
  VMLINUX="$KERNEL_OUT_DIR/vmlinux"
  CONFIG="$KERNEL_OUT_DIR/.config"
  DTB_DIR="$KERNEL_OUT_DIR/arch/arm64/boot/dts/qcom"

  for required in "$IMAGE" "$IMAGE_GZ" "$VMLINUX" "$CONFIG"; do
    if [ ! -f "$required" ]; then
      echo "Missing required kernel build output: $required"
      exit 1
    fi
  done

  image_size=$(wc -c < "$IMAGE" | tr -d '[:space:]')
  image_gz_size=$(wc -c < "$IMAGE_GZ" | tr -d '[:space:]')
  vmlinux_size=$(wc -c < "$VMLINUX" | tr -d '[:space:]')

  text_size=$(read_section_size "$VMLINUX" ".text")
  rodata_size=$(read_section_size "$VMLINUX" ".rodata")
  data_size=$(read_section_size "$VMLINUX" ".data")
  bss_size=$(read_section_size "$VMLINUX" ".bss")

  DTB_JSON="[]"
  dtb_total_size=0
  if [ -d "$DTB_DIR" ]; then
    DTB_JSON=$(find "$DTB_DIR" -maxdepth 1 -type f -name '*.dtb' -printf '%P\t%s\n' \
      | sort \
      | jq -Rn '
          [inputs
           | select(length > 0)
           | split("\t")
           | {path: .[0], bytes: (.[1] | tonumber)}]
        ')
    dtb_total_size=$(echo "$DTB_JSON" | jq '[.[].bytes] | add // 0')
  fi

  OBJECTS_JSON=$(find "$KERNEL_OUT_DIR" -type f -name '*.o' -printf '%P\t%s\n' \
    | sort \
    | jq -Rn '
        [inputs
         | select(length > 0)
         | split("\t")
         | {path: .[0], bytes: (.[1] | tonumber)}]
      ' \
    | filter_meaningful_objects)

  SYMBOLS_JSON=$(nm -S --size-sort --radix=d --defined-only "$VMLINUX" \
    | awk '
        NF >= 4 {
          addr=$1
          size=$2
          type=$3
          name=$4
          for (i = 5; i <= NF; i++) {
            name = name " " $i
          }
          if (size != "0") {
            printf("%s\t%s\t%s\t%s\n", name, size, type, addr)
          }
        }
      ' \
    | jq -Rn '
        [inputs
         | select(length > 0)
         | split("\t")
         | {
             name: .[0],
             size: (.[1] | tonumber),
             type: .[2],
             address: (.[3] | tonumber)
           }]
      ')

  config_json_file=$(mktemp)
  sections_json_file=$(mktemp)
  dtbs_json_file=$(mktemp)
  trap 'rm -f "$config_json_file" "$sections_json_file" "$dtbs_json_file"' RETURN

  normalize_config_to_json "$CONFIG" > "$config_json_file"
  jq -n \
    --arg text "$text_size" \
    --arg rodata "$rodata_size" \
    --arg data "$data_size" \
    --arg bss "$bss_size" \
    '{text: ($text | tonumber), rodata: ($rodata | tonumber), data: ($data | tonumber), bss: ($bss | tonumber)}' \
    > "$sections_json_file"
  printf '%s\n' "$DTB_JSON" > "$dtbs_json_file"

  cp "$CONFIG" "$OUTPUT_DIR/kernel-config.txt"
  printf '%s\n' "$OBJECTS_JSON" > "$OUTPUT_DIR/kernel-objects.json"
  printf '%s\n' "$SYMBOLS_JSON" > "$OUTPUT_DIR/kernel-symbols.json"

  jq -n \
    --slurpfile dtbs "$dtbs_json_file" \
    --slurpfile sections "$sections_json_file" \
    --slurpfile config "$config_json_file" \
    --arg generated_at "$(date -u +"%Y-%m-%dT%H:%M:%SZ")" \
    --arg image_size "$image_size" \
    --arg image_gz_size "$image_gz_size" \
    --arg vmlinux_size "$vmlinux_size" \
    --arg dtb_total_size "$dtb_total_size" \
    --arg text_size "$text_size" \
    --arg rodata_size "$rodata_size" \
    --arg data_size "$data_size" \
    --arg bss_size "$bss_size" \
    --arg object_count "$(echo "$OBJECTS_JSON" | jq 'length')" \
    --arg symbol_count "$(echo "$SYMBOLS_JSON" | jq 'length')" \
    '{
      generated_at: $generated_at,
      image_size: ($image_size | tonumber),
      image_gz_size: ($image_gz_size | tonumber),
      vmlinux_size: ($vmlinux_size | tonumber),
      dtb_total_size: ($dtb_total_size | tonumber),
      sections: $sections[0],
      dtbs: $dtbs[0],
      config: $config[0],
      object_count: ($object_count | tonumber),
      symbol_count: ($symbol_count | tonumber)
    }' > "$OUTPUT_DIR/kernel-profile.json"

  {
    echo "| Metric | Value |"
    echo "|--------|-------|"
    echo "| Image | $(fmt_bytes "$image_size") |"
    echo "| Image.gz | $(fmt_bytes "$image_gz_size") |"
    echo "| vmlinux | $(fmt_bytes "$vmlinux_size") |"
    echo "| DTBs total | $(fmt_bytes "$dtb_total_size") |"
    echo "| .text | $(fmt_bytes "$text_size") |"
    echo "| .rodata | $(fmt_bytes "$rodata_size") |"
    echo "| .data | $(fmt_bytes "$data_size") |"
    echo "| .bss | $(fmt_bytes "$bss_size") |"
    echo "| Objects tracked | $(echo "$OBJECTS_JSON" | jq 'length') |"
    echo ""
    echo "### DTBs"
    echo ""
    echo "| DTB | Size |"
    echo "|-----|------|"
    echo "$DTB_JSON" | jq -r '.[] | [.path, (.bytes | tostring)] | @tsv' | while IFS=$'\t' read -r name bytes; do
      echo "| \`$name\` | $(fmt_bytes "$bytes") |"
    done
    echo ""
    echo "### Largest Objects"
    echo ""
    echo "| Object | Size |"
    echo "|--------|------|"
    echo "$OBJECTS_JSON" | jq -r 'sort_by(.bytes) | reverse | .[:15][] | [.path, (.bytes|tostring)] | @tsv' | while IFS=$'\t' read -r path bytes; do
      echo "| \`$path\` | $(fmt_bytes "$bytes") |"
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

  command -v jq >/dev/null || { echo "jq required for diff"; exit 1; }

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
      metric_row("DTBs total"; "dtb_total_size"),
      section_row(".text"; "text"),
      section_row(".rodata"; "rodata"),
      section_row(".data"; "data"),
      section_row(".bss"; "bss")
    ]
  ')

  echo "| Metric | Change |"
  echo "|--------|--------|"
  echo "$metric_rows" | jq -r '.[] | [.label, (.old|tostring), (.new|tostring)] | @tsv' | while IFS=$'\t' read -r label old new; do
    delta=$((new - old))
    delta_human=$(fmt_binary_bytes "$delta")
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
          | select(.delta != 0)
        ]
      | sort_by((.delta | if . < 0 then -. else . end), .delta) | reverse
    ')

  echo "$object_rows" | jq 'length' >/dev/null
  if [ "$(echo "$object_rows" | jq 'length')" -gt 0 ]; then
    object_rows_with_display=$(rows_with_display_delta "$object_rows")
    emit_top_deltas_table "Largest Object Deltas" 10 "$object_rows_with_display"
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
    echo "### Config Changes"
    echo ""
    echo "| Option | Change |"
    echo "|--------|--------|"
    echo "$config_rows" | jq -r '.[0:20][] | [.key, "\(.old) -> \(.new)"] | @tsv' | while IFS=$'\t' read -r key change; do
      echo "| \`$key\` | $change |"
    done
    if [ "$(echo "$config_rows" | jq 'length')" -gt 20 ]; then
      echo ""
      echo "_Showing first 20 config changes._"
    fi
    echo ""
  fi

  exit 0
fi

usage
exit 1
