#!/bin/sh
set -e

SERIAL="$(/usr/comma/get-serial.sh)"
HOSTNAME="comma"

if [ -n "$SERIAL" ] && [ "$SERIAL" != "(none)" ]; then
  HOSTNAME="comma-$SERIAL"
fi

echo "hostname: '$HOSTNAME'"
hostname "$HOSTNAME"
