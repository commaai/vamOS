#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

if [ ! -f "$DIR/build/system_raw.img" ]; then
  echo "system_raw.img not found, running prepare step..."
  "$DIR/tools/vm/prepare.sh"
fi

BUILD_DIR="$DIR/build"
EXTRA_CMDLINE="${1:-}"
set -x
qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a57 \
  -smp 8 \
  -m 4G \
  -kernel ./kernel/linux/out/arch/arm64/boot/Image \
  -drive file=${BUILD_DIR}/system_raw.img,if=virtio,format=raw \
  -no-reboot \
  -append "root=/dev/vda console=ttyAMA0 loglevel=7 earlycon=pl011,0x9000000 panic=-1 ${EXTRA_CMDLINE}" \
  -nographic
