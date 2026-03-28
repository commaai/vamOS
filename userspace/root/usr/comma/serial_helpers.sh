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
