#!/bin/bash

set -eu

wait_for_sound_card() {
  local count=0
  local max_count=1200

  while grep -q '^--- no soundcards ---' /proc/asound/cards 2>/dev/null; do
    count=$((count + 1))
    if [ "$count" -ge "$max_count" ]; then
      echo "timed out waiting for ALSA sound cards" >&2
      return 1
    fi
    sleep 0.05
  done
}

wait_for_tinymix_control() {
  local count=0
  local max_count=1200

  while ! /usr/comma/sound/tinymix controls 2>/dev/null | grep -q "SEC_MI2S_RX Audio Mixer MultiMedia1"; do
    count=$((count + 1))
    if [ "$count" -ge "$max_count" ]; then
      echo "timed out waiting for tinymix controls" >&2
      return 1
    fi
    sleep 0.05
  done
}

/usr/comma/sound/adsp-start.sh

echo "waiting for sound card to come online"
wait_for_sound_card
echo "sound card online"

# Fix permissions for audio group
if ls /dev/snd/* >/dev/null 2>&1; then
  chgrp audio /dev/snd/*
  chmod 660 /dev/snd/*
fi

wait_for_tinymix_control
echo "tinymix controls ready"

/usr/comma/sound/tinymix set "SEC_MI2S_RX Audio Mixer MultiMedia1" 1
if grep -q mici /sys/firmware/devicetree/base/model; then
  /usr/comma/sound/tinymix set "MultiMedia1 Mixer SEC_MI2S_TX" 1
else
  /usr/comma/sound/tinymix set "MultiMedia1 Mixer TERT_MI2S_TX" 1
  /usr/comma/sound/tinymix set "TERT_MI2S_TX Channels" Two
fi

# setup the amplifier registers
/usr/local/venv/bin/python /usr/comma/sound/amplifier.py
