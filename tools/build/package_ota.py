#!/usr/bin/env python3
import json
import os
import hashlib
import shutil
import struct
from pathlib import Path
from collections import namedtuple

ROOT = Path(__file__).parent.parent.parent
OUTPUT_DIR = ROOT / "build"
FIRMWARE_DIR = ROOT / "firmware"
OTA_OUTPUT_DIR = OUTPUT_DIR / "ota"

SECTOR_SIZE = 4096

RELEASE_URL = os.environ.get("RELEASE_URL", "https://github.com/commaai/vamos/releases/download/untagged")

GPT = namedtuple('GPT', ['lun', 'name', 'path', 'start_sector', 'num_sectors', 'has_ab', 'full_check', 'sparse'])
GPTS = [
  GPT(0, 'gpt_main_0', FIRMWARE_DIR / 'gpt_main_0.img', 0, 6, False, True, False),
  GPT(1, 'gpt_main_1', FIRMWARE_DIR / 'gpt_main_1.img', 0, 6, False, True, False),
  GPT(2, 'gpt_main_2', FIRMWARE_DIR / 'gpt_main_2.img', 0, 6, False, True, False),
  GPT(3, 'gpt_main_3', FIRMWARE_DIR / 'gpt_main_3.img', 0, 6, False, True, False),
  GPT(4, 'gpt_main_4', FIRMWARE_DIR / 'gpt_main_4.img', 0, 6, False, True, False),
  GPT(5, 'gpt_main_5', FIRMWARE_DIR / 'gpt_main_5.img', 0, 6, False, True, False),
]

Partition = namedtuple('Partition', ['name', 'path', 'has_ab', 'full_check', 'sparse'])
PARTITIONS = [
  # Non-A/B firmware
  Partition('persist', FIRMWARE_DIR / 'persist.img', False, True, False),
  Partition('systemrw', FIRMWARE_DIR / 'systemrw.img', False, True, False),
  Partition('cache', FIRMWARE_DIR / 'cache.img', False, True, False),
  Partition('devinfo', FIRMWARE_DIR / 'devinfo.img', False, True, False),
  Partition('limits', FIRMWARE_DIR / 'limits.img', False, True, False),
  Partition('logfs', FIRMWARE_DIR / 'logfs.img', False, True, False),
  Partition('splash', FIRMWARE_DIR / 'splash.img', False, True, False),
  Partition('splash_cc', FIRMWARE_DIR / 'splash_cc.img', False, True, False),
  # A/B firmware
  Partition('xbl', FIRMWARE_DIR / 'xbl.img', True, True, False),
  Partition('xbl_config', FIRMWARE_DIR / 'xbl_config.img', True, True, False),
  Partition('abl', FIRMWARE_DIR / 'abl.img', True, True, False),
  Partition('aop', FIRMWARE_DIR / 'aop.img', True, True, False),
  Partition('bluetooth', FIRMWARE_DIR / 'bluetooth.img', True, True, False),
  Partition('cmnlib64', FIRMWARE_DIR / 'cmnlib64.img', True, True, False),
  Partition('cmnlib', FIRMWARE_DIR / 'cmnlib.img', True, True, False),
  Partition('devcfg', FIRMWARE_DIR / 'devcfg.img', True, True, False),
  Partition('dsp', FIRMWARE_DIR / 'dsp.img', True, True, False),
  Partition('hyp', FIRMWARE_DIR / 'hyp.img', True, True, False),
  Partition('keymaster', FIRMWARE_DIR / 'keymaster.img', True, True, False),
  Partition('modem', FIRMWARE_DIR / 'modem.img', True, True, False),
  Partition('qupfw', FIRMWARE_DIR / 'qupfw.img', True, True, False),
  Partition('storsec', FIRMWARE_DIR / 'storsec.img', True, True, False),
  Partition('tz', FIRMWARE_DIR / 'tz.img', True, True, False),
  # Built images
  Partition('boot', OUTPUT_DIR / 'boot.img', True, True, False),
  Partition('system', OUTPUT_DIR / 'system.erofs.img', True, False, False),
]


def file_checksum(fn):
  sha256 = hashlib.sha256()
  with open(fn, 'rb') as f:
    for chunk in iter(lambda: f.read(4096), b""):
      sha256.update(chunk)
  return sha256


def process_file(entry):
  size = entry.path.stat().st_size
  print(f"\n{entry.name} {size} bytes")

  sha256 = file_checksum(entry.path)
  hash = hash_raw = sha256.hexdigest()

  # Compute ondevice_hash: hash with zero-padding to sector boundary
  sha256.update(b'\x00' * ((SECTOR_SIZE - (size % SECTOR_SIZE)) % SECTOR_SIZE))
  ondevice_hash = sha256.hexdigest()

  # Copy to output directory
  out_fn = OTA_OUTPUT_DIR / f"{entry.name}-{hash_raw}.img"
  print(f"  copying to {out_fn.name}")
  shutil.copy(entry.path, out_fn)

  ret = {
    "name": entry.name,
    "url": f"{RELEASE_URL}/{out_fn.name}",
    "hash": hash,
    "hash_raw": hash_raw,
    "size": size,
    "sparse": entry.sparse,
    "full_check": entry.full_check,
    "has_ab": entry.has_ab,
    "ondevice_hash": ondevice_hash,
  }

  if isinstance(entry, GPT):
    ret["gpt"] = {
      "lun": entry.lun,
      "start_sector": entry.start_sector,
      "num_sectors": entry.num_sectors,
    }

  return ret


if __name__ == "__main__":
  OTA_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

  entries = []
  for entry in GPTS + PARTITIONS:
    entries.append(process_file(entry))

  with open(OTA_OUTPUT_DIR / "manifest.json", "w") as f:
    json.dump(entries, f, indent=2)

  print(f"\nWrote manifest with {len(entries)} entries to {OTA_OUTPUT_DIR / 'manifest.json'}")
