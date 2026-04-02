#!/usr/bin/env python3
import json
import os
import hashlib
import shutil
from pathlib import Path
from collections import namedtuple

ROOT = Path(__file__).parent.parent.parent
OUTPUT_DIR = ROOT / "build"
FIRMWARE_DIR = ROOT / "firmware"
OTA_OUTPUT_DIR = OUTPUT_DIR / "ota"

SECTOR_SIZE = 4096
CHUNK_SIZE = 52_428_800  # 50 MB - must be under raw.githubusercontent.com's 100 MB limit

VERSION = open(ROOT / "userspace" / "root" / "VERSION").read().strip()
IMAGES_URL = os.environ.get("IMAGES_URL", f"https://github.com/commaai/vamos-images/raw/v{VERSION}")

GPT = namedtuple('GPT', ['lun', 'name', 'path', 'start_sector', 'num_sectors', 'has_ab', 'full_check'])
GPTS = [
  GPT(0, 'gpt_main_0', FIRMWARE_DIR / 'gpt_main_0.img', 0, 6, False, True),
  GPT(1, 'gpt_main_1', FIRMWARE_DIR / 'gpt_main_1.img', 0, 6, False, True),
  GPT(2, 'gpt_main_2', FIRMWARE_DIR / 'gpt_main_2.img', 0, 6, False, True),
  GPT(3, 'gpt_main_3', FIRMWARE_DIR / 'gpt_main_3.img', 0, 6, False, True),
  GPT(4, 'gpt_main_4', FIRMWARE_DIR / 'gpt_main_4.img', 0, 6, False, True),
  GPT(5, 'gpt_main_5', FIRMWARE_DIR / 'gpt_main_5.img', 0, 6, False, True),
]

Partition = namedtuple('Partition', ['name', 'path', 'has_ab', 'full_check'])
PARTITIONS = [
  Partition('persist', FIRMWARE_DIR / 'persist.img', False, True),
  Partition('systemrw', FIRMWARE_DIR / 'systemrw.img', False, True),
  Partition('cache', FIRMWARE_DIR / 'cache.img', False, True),
  Partition('xbl', FIRMWARE_DIR / 'xbl.img', True, True),
  Partition('xbl_config', FIRMWARE_DIR / 'xbl_config.img', True, True),
  Partition('abl', FIRMWARE_DIR / 'abl.img', True, True),
  Partition('aop', FIRMWARE_DIR / 'aop.img', True, True),
  Partition('bluetooth', FIRMWARE_DIR / 'bluetooth.img', True, True),
  Partition('cmnlib64', FIRMWARE_DIR / 'cmnlib64.img', True, True),
  Partition('cmnlib', FIRMWARE_DIR / 'cmnlib.img', True, True),
  Partition('devcfg', FIRMWARE_DIR / 'devcfg.img', True, True),
  Partition('devinfo', FIRMWARE_DIR / 'devinfo.img', False, True),
  Partition('dsp', FIRMWARE_DIR / 'dsp.img', True, True),
  Partition('hyp', FIRMWARE_DIR / 'hyp.img', True, True),
  Partition('keymaster', FIRMWARE_DIR / 'keymaster.img', True, True),
  Partition('limits', FIRMWARE_DIR / 'limits.img', False, True),
  Partition('logfs', FIRMWARE_DIR / 'logfs.img', False, True),
  Partition('modem', FIRMWARE_DIR / 'modem.img', True, True),
  Partition('qupfw', FIRMWARE_DIR / 'qupfw.img', True, True),
  Partition('splash', FIRMWARE_DIR / 'splash.img', False, True),
  Partition('storsec', FIRMWARE_DIR / 'storsec.img', True, True),
  Partition('tz', FIRMWARE_DIR / 'tz.img', True, True),
  Partition('boot', OUTPUT_DIR / 'boot.img', True, True),
  Partition('system', OUTPUT_DIR / 'system.erofs.img', True, False),
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

  # ondevice_hash: hash with zero-padding to sector boundary
  sha256.update(b'\x00' * ((SECTOR_SIZE - (size % SECTOR_SIZE)) % SECTOR_SIZE))
  ondevice_hash = sha256.hexdigest()

  base_name = f"{entry.name}-{hash_raw}.img"

  # Write file(s) to output directory, splitting into chunks if needed
  chunks = None
  if size > CHUNK_SIZE:
    chunks = []
    chunk_idx = 0
    with open(entry.path, 'rb') as f:
      while True:
        data = f.read(CHUNK_SIZE)
        if not data:
          break
        chunk_name = f"{base_name}.{chunk_idx:02d}"
        (OTA_OUTPUT_DIR / chunk_name).write_bytes(data)
        chunks.append({"url": f"{IMAGES_URL}/{chunk_name}", "size": len(data)})
        print(f"  chunk {chunk_idx}: {chunk_name} ({len(data)} bytes)")
        chunk_idx += 1
  else:
    print(f"  copying to {base_name}")
    shutil.copy(entry.path, OTA_OUTPUT_DIR / base_name)

  ret = {
    "name": entry.name,
    "url": f"{IMAGES_URL}/{base_name}",
    "hash": hash,
    "hash_raw": hash_raw,
    "size": size,
    "sparse": False,
    "full_check": entry.full_check,
    "has_ab": entry.has_ab,
    "ondevice_hash": ondevice_hash,
  }

  if chunks:
    ret["url"] = ""
    ret["chunks"] = chunks

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
