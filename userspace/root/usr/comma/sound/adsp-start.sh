#!/bin/sh
set -e

firmware_available() {
  [ -s /firmware/image/adsp.mdt ] && return 0
  [ -s /lib/firmware/updates/adsp.mdt ] && return 0
  [ -s /lib/firmware/adsp.mdt ] && return 0
  [ -s /lib/firmware/qcom/sdm845/adsp.mdt ] && return 0
  return 1
}

ensure_firmware_ready() {
  N=0
  while :; do
    firmware_available && return 0

    # If modem firmware partition is not mounted yet, try mounting it.
    if ! mountpoint -q /firmware 2>/dev/null; then
      if [ -b /dev/disk/by-partlabel/modem_a ]; then
        mount -t vfat -o ro /dev/disk/by-partlabel/modem_a /firmware 2>/dev/null || true
      elif [ -b /dev/sde4 ]; then
        mount -t vfat -o ro /dev/sde4 /firmware 2>/dev/null || true
      fi
    fi

    N=$((N + 1))
    [ "$N" -ge 600 ] && return 1
    sleep 0.1
  done
}

find_adsp_remoteproc() {
  for d in /sys/class/remoteproc/remoteproc*; do
    [ -d "$d" ] || continue
    if [ "$(cat "$d/name" 2>/dev/null)" = "adsp" ]; then
      echo "$d"
      return 0
    fi
  done
  return 1
}

start_adsp_remoteproc() {
  ADSP_RP=""
  ADSP_STATE=""
  TRY_N=0
  ADSP_N=0
  while :; do
    ADSP_RP="$(find_adsp_remoteproc || true)"
    [ -n "$ADSP_RP" ] && break
    ADSP_N=$((ADSP_N + 1))
    [ "$ADSP_N" -ge 200 ] && return 1
    sleep 0.1
  done

  ADSP_STATE="$(cat "$ADSP_RP/state" 2>/dev/null || true)"
  if [ "$ADSP_STATE" = "running" ]; then
    return 0
  fi

  ensure_firmware_ready

  # Firmware can become available slightly after remoteproc shows up, so retry start.
  TRY_N=0
  while :; do
    echo "adsp.mdt" > "$ADSP_RP/firmware"
    echo "start" > "$ADSP_RP/state" 2>/dev/null || true
    ADSP_STATE="$(cat "$ADSP_RP/state" 2>/dev/null || true)"
    [ "$ADSP_STATE" = "running" ] && return 0
    TRY_N=$((TRY_N + 1))
    [ "$TRY_N" -ge 120 ] && return 1
    sleep 0.1
  done
}

start_adsp_legacy() {
  ADSP_SUBSYS_NAME=""
  ADSP_COUNT=0
  ADSP_STATE=""

  echo -n "/firmware/image" > /sys/module/firmware_class/parameters/path

  while [ ! -s /firmware/image/adsp.mdt ]; do
    sleep 1
    ADSP_COUNT=$((ADSP_COUNT + 1))
    [ "$ADSP_COUNT" -ge 100 ] && return 1
  done

  for subsys in /sys/bus/msm_subsys/devices/*; do
    [ -d "$subsys" ] || continue
    if [ "$(cat "$subsys/name" 2>/dev/null)" = "adsp" ]; then
      ADSP_SUBSYS_NAME="${subsys##*/}"
      break
    fi
  done

  [ -n "$ADSP_SUBSYS_NAME" ] || return 1

  if [ -e /sys/module/subsystem_restart/parameters/enable_debug ]; then
    echo 1 > /sys/module/subsystem_restart/parameters/enable_debug
  fi

  echo 1 > /sys/kernel/boot_adsp/boot

  ADSP_COUNT=0
  ADSP_STATE="$(cat "/sys/bus/msm_subsys/devices/${ADSP_SUBSYS_NAME}/state" 2>/dev/null || true)"
  while [ "$ADSP_STATE" != "ONLINE" ]; do
    ADSP_COUNT=$((ADSP_COUNT + 1))
    [ "$ADSP_COUNT" -ge 200 ] && return 1
    sleep 0.1
    ADSP_STATE="$(cat "/sys/bus/msm_subsys/devices/${ADSP_SUBSYS_NAME}/state" 2>/dev/null || true)"
  done
}

if [ -d /sys/class/remoteproc ]; then
  echo "[INFO] Starting ADSP via remoteproc"
  start_adsp_remoteproc
else
  echo "[INFO] Starting ADSP via legacy msm_subsys"
  start_adsp_legacy
fi
