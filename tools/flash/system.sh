#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

tools/bin/qdl flash system $DIR/build/system.img
tools/bin/qdl reset
