#!/bin/sh
set -e

. /usr/comma/serial_helpers.sh

SERIAL="$(get_device_serial)"
ADDR="$(/usr/comma/get-bt-address.sh)"

if [ -z "$SERIAL" ]; then
  echo "bluetooth: missing androidboot.serialno"
  exit 0
fi

for _ in $(seq 1 30); do
  INFO="$(btmgmt --index 0 info 2>/dev/null || true)"
  CURRENT_ADDR="$(printf '%s\n' "$INFO" | sed -n 's/^[[:space:]]*addr \([0-9A-F:]*\) .*/\1/p' | tr '[:lower:]' '[:upper:]')"
  if [ "$CURRENT_ADDR" = "$ADDR" ]; then
    echo "bluetooth: public address already set to $ADDR"
    exit 0
  fi

  btmgmt --index 0 power off >/dev/null 2>&1 || true
  if btmgmt --index 0 public-addr "$ADDR" >/dev/null 2>&1; then
    echo "bluetooth: set public address to $ADDR from serial $SERIAL"
    exit 0
  fi

  sleep 1
done

echo "bluetooth: failed to set public address to $ADDR"
