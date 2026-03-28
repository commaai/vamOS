#!/bin/bash
set -e

SERIAL="$(/usr/comma/get-serial.sh)"
echo "serial: '$SERIAL'"
sysctl kernel.hostname="comma-$SERIAL"
