#!/bin/bash
# Temporary solution until openpilot is running (which owns panda bringup).
source /usr/comma/gpio_base.sh

STM_RST_N=124
STM_BOOT0=134

case "$1" in
  start)
    echo "Resetting panda..."
    gpio $STM_RST_N 1
    gpio $STM_BOOT0 0
    sleep 0.01
    gpio $STM_RST_N 0

    touch /run/panda.ready
    echo " Panda reset done"
    ;;
  *)
    echo "Specify start as first argument!"
    exit 1
    ;;
esac
