#!/bin/sh

. /usr/comma/serial_helpers.sh

get_device_model() {
  local raw_model

  if [ -n "${COMMA_DEVICE_MODEL:-}" ]; then
    raw_model="$COMMA_DEVICE_MODEL"
  else
    raw_model="$(tr -d '\0' < /sys/firmware/devicetree/base/model 2>/dev/null || true)"
  fi

  case "$raw_model" in
    *tizi*)
      printf 'tizi\n'
      ;;
    *mici*)
      printf 'mici\n'
      ;;
    *)
      return 1
      ;;
  esac
}

get_model_alias() {
  local model

  model="$(get_device_model)" || return 1
  printf 'comma-%s\n' "$model"
}
