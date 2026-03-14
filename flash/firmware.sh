#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." >/dev/null && pwd)"
cd "$DIR"

# A/B partitions
AB_PARTS="abl aop bluetooth cmnlib cmnlib64 devcfg dsp hyp keymaster modem qupfw storsec tz xbl xbl_config"
for part in $AB_PARTS; do
  tools/bin/qdl flash "${part}_a" "$DIR/firmware/$part.img"
  tools/bin/qdl flash "${part}_b" "$DIR/firmware/$part.img"
done

# Non-A/B partitions
for part in cache devinfo limits logfs splash systemrw; do
  tools/bin/qdl flash "$part" "$DIR/firmware/$part.img"
done
