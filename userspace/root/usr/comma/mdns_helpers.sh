#!/bin/sh

get_device_serial() {
  if [ -n "${COMMA_SERIAL:-}" ]; then
    printf '%s\n' "$COMMA_SERIAL"
    return 0
  fi

  sed -n 's/.*androidboot.serialno=\([^ ]*\).*/\1/p' /proc/cmdline 2>/dev/null | head -n 1
}

get_serial_hostname() {
  local serial

  serial="$(get_device_serial)"
  if [ -n "$serial" ]; then
    printf 'comma-%s\n' "$serial"
  else
    printf 'comma\n'
  fi
}

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
