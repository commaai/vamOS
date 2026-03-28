#!/bin/bash
set -e

. /usr/comma/mdns_helpers.sh

HOSTNAME="$(get_serial_hostname)"
echo "hostname: '$HOSTNAME'"
sysctl kernel.hostname="$HOSTNAME" >/dev/null
