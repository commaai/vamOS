#!/bin/sh
set -e

. /usr/comma/serial_helpers.sh

SERIAL="$(get_device_serial)"

if [ -z "$SERIAL" ]; then
  exit 0
fi

HASH="$(printf '%s' "$SERIAL" | sha256sum | awk '{print $1}')"
printf '%s\n' "$HASH" \
  | sed 's/^\(..\)\(..\)\(..\)\(..\)\(..\).*/02:\1:\2:\3:\4:\5/' \
  | tr '[:lower:]' '[:upper:]'
