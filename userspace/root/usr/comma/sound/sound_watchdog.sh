#!/bin/sh
set -eu

TINYMIX="/usr/comma/sound/tinymix"
PLAYBACK_CTL="SEC_MI2S_RX Audio Mixer MultiMedia1"

fix_audio_permissions() {
  if ls /dev/snd/* >/dev/null 2>&1; then
    chgrp audio /dev/snd/* 2>/dev/null || true
    chmod 660 /dev/snd/* 2>/dev/null || true
  fi
}

ensure_playback_route() {
  cur="$($TINYMIX get "$PLAYBACK_CTL" 2>/dev/null || true)"
  if [ "$cur" != "On" ]; then
    echo "[WARN] $PLAYBACK_CTL was '$cur', forcing On"
    $TINYMIX set "$PLAYBACK_CTL" 1 >/dev/null 2>&1 || return 1
  fi
  return 0
}

soundcards_present() {
  ! grep -q '^--- no soundcards ---' /proc/asound/cards 2>/dev/null
}

prime_playback_route() {
  # Boot-time race: force route on a few times while audio stack settles.
  i=0
  while [ "$i" -lt 20 ]; do
    if ensure_playback_route; then
      cur="$($TINYMIX get "$PLAYBACK_CTL" 2>/dev/null || true)"
      [ "$cur" = "On" ] && return 0
    fi
    i=$((i + 1))
    sleep 0.25
  done
  return 1
}

echo "[INFO] Running sound initialization"
/usr/comma/sound/sound_init.sh
prime_playback_route || true

while :; do
  if soundcards_present; then
    ensure_playback_route || true
    fix_audio_permissions
  else
    echo "[WARN] No soundcards detected, re-running sound initialization"
    /usr/comma/sound/sound_init.sh || true
  fi
  sleep 2
done
