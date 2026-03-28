#!/bin/bash
set -e

. /usr/comma/mdns_helpers.sh

HOSTNAME="$(get_serial_hostname)"
echo "hostname: '$HOSTNAME'"
SERIAL="$(/usr/comma/get-serial.sh)"
echo "serial: '$SERIAL'"
sysctl kernel.hostname="$HOSTNAME" >/dev/null
