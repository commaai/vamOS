#!/bin/bash

# The q6v5-pas driver tries to auto-boot the ADSP at probe (~0.5s), before the
# rootfs firmware is available, so that load fails and the remoteproc stays
# offline. Start it here, once /lib/firmware is up. Bail out cleanly if no adsp
# remoteproc is present.
adsp=""
for rproc in /sys/class/remoteproc/remoteproc*; do
  [ "$(cat "$rproc/name" 2>/dev/null)" = adsp ] && adsp="$rproc" && break
done
if [ -z "$adsp" ]; then
  echo "no adsp remoteproc, skipping"
  exit 0
fi
if [ "$(cat "$adsp/state")" != running ]; then
  echo "starting adsp remoteproc"
  echo start > "$adsp/state"
fi

echo "waiting for sound card to come online"
card=""
for _ in $(seq 1 1000); do
  for c in /proc/asound/comma*; do
    [ -d "$c" ] && card="$(basename "$c")"
  done
  [ -n "$card" ] && break
  sleep 0.01
done
if [ -z "$card" ]; then
  echo "no sound card, skipping"
  exit 0
fi
echo "sound card online: $card"

# Fix permissions for audio group (no udev rule fires for /dev/snd)
chgrp audio /dev/snd/*
chmod 660 /dev/snd/*

for _ in $(seq 1 1000); do
  /usr/comma/sound/tinymix controls | grep -q "SEC_MI2S_RX Audio Mixer MultiMedia1" && break
  sleep 0.01
done
echo "tinymix controls ready"

# MultiMedia1 = playback (hw:0,0), MultiMedia2 = capture (hw:0,1); q6routing
# keys one port per frontend, so playback and capture need distinct frontends.
# tizi captures off Tertiary MI2S TX (2ch); mici off Secondary MI2S TX.
/usr/comma/sound/tinymix set "SEC_MI2S_RX Audio Mixer MultiMedia1" 1
if [ "$card" = commamici ]; then
  /usr/comma/sound/tinymix set "MultiMedia2 Mixer SEC_MI2S_TX" 1
else
  /usr/comma/sound/tinymix set "MultiMedia2 Mixer TERT_MI2S_TX" 1
  /usr/comma/sound/tinymix set "TERT_MI2S_TX Channels" Two
fi
