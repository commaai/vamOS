#!/bin/bash
set -e

. /usr/comma/mdns_helpers.sh

HOSTNAME="$(get_serial_hostname)"
echo "hostname: '$HOSTNAME'"
SERIAL="$(get_device_serial)"
echo "serial: '$SERIAL'"
sysctl kernel.hostname="$HOSTNAME" >/dev/null
