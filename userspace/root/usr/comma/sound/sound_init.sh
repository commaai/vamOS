#!/bin/bash
set -euo pipefail

# The remoteproc driver may probe before the firmware partition is mounted.
test -s /firmware/image/adsp.mdt
echo -n /firmware/image > /sys/module/firmware_class/parameters/path

adsp=""
for remoteproc in /sys/class/remoteproc/remoteproc*; do
  if [ "$(cat "$remoteproc/name")" = adsp ]; then
    adsp="$remoteproc"
    break
  fi
done
if [ -z "$adsp" ]; then
  echo "ADSP remoteproc is unavailable" >&2
  exit 1
fi
if [ "$(cat "$adsp/state")" = offline ]; then
  echo start > "$adsp/state"
fi

model="$(tr -d '\0' < /sys/firmware/devicetree/base/model)"
case "$model" in
  "comma tizi") card=commatizi; capture=TERT_MI2S_TX ;;
  "comma mici") card=commamici; capture=SEC_MI2S_TX ;;
  *) echo "Unsupported sound card: $model" >&2; exit 1 ;;
esac

for _ in $(seq 1 100); do
  [ -d "/proc/asound/$card" ] && break
  sleep 0.1
done
if [ ! -d "/proc/asound/$card" ] || [ "$(cat "$adsp/state")" != running ]; then
  echo "Sound card $card did not become ready" >&2
  exit 1
fi

# Q6 routing needs a separate frontend for each direction. Amplifier power and
# register configuration belong to openpilot's hardware initialization/wake path.
amixer -c "$card" cset 'name=SEC_MI2S_RX Audio Mixer MultiMedia1' on > /dev/null
amixer -c "$card" cset "name=MultiMedia2 Mixer $capture" on > /dev/null
echo "Sound card ready: $card"
