#!/usr/bin/env bash
set -euox pipefail

sudo killall -9 qemu-system-aarch64 || true
