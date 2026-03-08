#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd "$DIR"

echo "Checking active slot..."
QDL="github:commaai/qdl.js#553c8ae2def761c8ec82e0c6ce63333e4c0b4a9a"
ACTIVE_SLOT=$(bunx "$QDL" getactiveslot)

if [[ "$ACTIVE_SLOT" != "a" && "$ACTIVE_SLOT" != "b" ]]; then
  echo "Invalid active slot: '$ACTIVE_SLOT'"
  exit 1
fi

echo "Active slot: $ACTIVE_SLOT"
echo "Flashing boot_$ACTIVE_SLOT..."
bunx "$QDL" flash "boot_$ACTIVE_SLOT" "$DIR/output/boot.img"

echo "Flashed boot_$ACTIVE_SLOT!"
