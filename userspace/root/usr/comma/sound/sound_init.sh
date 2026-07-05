#!/bin/bash

# The ADSP is auto-booted by the q6v5-pas remoteproc; wait for the ASoC card
# to register. Only mici has a sound card wired up on mainline, so bail out
# cleanly if none appears (e.g. tizi) instead of spinning forever.
echo "waiting for sound card to come online"
for _ in $(seq 1 1000); do
  [ -d /proc/asound/commamici ] && break
  sleep 0.01
done
if [ ! -d /proc/asound/commamici ]; then
  echo "no sound card, skipping"
  exit 0
fi
echo "sound card online"

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
/usr/comma/sound/tinymix set "SEC_MI2S_RX Audio Mixer MultiMedia1" 1
/usr/comma/sound/tinymix set "MultiMedia2 Mixer SEC_MI2S_TX" 1
