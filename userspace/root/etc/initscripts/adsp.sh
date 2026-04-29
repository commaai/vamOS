#!/bin/sh
###############################################################################
#
# This script is used for System V init scripts to start adsp
#
# Copyright (c) 2012-2016 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################

set -e

case "$1" in
  start)
        echo -n "Starting adsp: "
        /usr/local/qr-linux/adsp-start.sh 
        echo "done"
        ;;
  stop)
        echo -n "Stopping adsp: "
        if [ -d /sys/class/remoteproc ]; then
          for d in /sys/class/remoteproc/remoteproc*; do
            [ -d "$d" ] || continue
            if [ "$(cat "$d/name" 2>/dev/null)" = "adsp" ]; then
              echo stop > "$d/state"
              break
            fi
          done
        elif [ -e /sys/kernel/boot_adsp/boot ]; then
          echo 0 > /sys/kernel/boot_adsp/boot
        fi
        echo "done"
        ;;
  restart)
        $0 stop
        $0 start
        ;;
  *)
        echo "Usage adsp.sh { start | stop | restart}" >&2
        exit 1
        ;;
esac

exit 0
