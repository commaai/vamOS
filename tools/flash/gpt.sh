#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

for lun in 0 1 2 3 4 5; do
  tools/bin/qdl repairgpt "$lun" "$DIR/firmware/gpt_main_$lun.img"
done
