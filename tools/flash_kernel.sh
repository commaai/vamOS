#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." >/dev/null && pwd)"
cd "$DIR"

LEGACY_KERNEL_URL="https://commadist.azureedge.net/agnosupdate/boot-d726315cf98a43e1090e5b49297404cf3d084cfbd42ad8bb7d8afb68136b9f51.img.xz"

if [ "${1:-}" = "--legacy" ]; then
  echo "Downloading legacy kernel image..."
  LEGACY_IMG="$DIR/output/boot-legacy.img"
  curl -fSL "$LEGACY_KERNEL_URL" | xz -d > "$LEGACY_IMG"
  tools/qdl flash boot "$LEGACY_IMG"
else
  tools/qdl flash boot "$DIR/output/boot.img"
fi
