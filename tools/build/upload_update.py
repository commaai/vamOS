#!/usr/bin/env python3
"""Upload boot/system archives and the exact updater manifest to an existing release."""
import argparse
import hashlib
import json
import lzma
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def upload_update(assets_dir, manifest_path, repo, tag, clobber=False):
  manifest_bytes = manifest_path.read_bytes()
  entries = json.loads(manifest_bytes)
  if not isinstance(entries, list) or not all(isinstance(entry, dict) for entry in entries):
    raise ValueError("Manifest must contain boot and system image entries")
  names = [entry.get("name") for entry in entries]
  if "persist" in names:
    raise ValueError("persist updates are forbidden")
  if len(names) != 2 or names.count("boot") != 1 or names.count("system") != 1:
    raise ValueError("Manifest must contain exactly boot and system images")
  for entry in entries:
    if entry.get("sparse") is not False or entry.get("has_ab") is not True:
      raise ValueError(f"{entry['name']}: only raw A/B images are supported")
    expected_url = f"https://github.com/{repo}/releases/download/{tag}/{entry['name']}.img.xz"
    if entry.get("url") != expected_url:
      raise ValueError(f"{entry['name']}: URL must be {expected_url}")
    archive = assets_dir / f"{entry['name']}.img.xz"
    expected_size = entry.get("size")
    if type(expected_size) is not int or expected_size <= 0:
      raise ValueError(f"{archive.name}: manifest size must be a positive integer")
    digest = hashlib.sha256()
    size = 0
    try:
      with lzma.open(archive, format=lzma.FORMAT_XZ) as image:
        while data := image.read(1024 * 1024):
          digest.update(data)
          size += len(data)
          if size > expected_size:
            raise ValueError(f"{archive.name}: raw size exceeds manifest size")
    except (OSError, EOFError, lzma.LZMAError) as error:
      raise ValueError(f"{archive.name}: {error}") from error
    if size != expected_size:
      raise ValueError(f"{archive.name}: raw size does not match manifest")
    if entry.get("hash") != digest.hexdigest() or entry.get("hash_raw") != digest.hexdigest():
      raise ValueError(f"{archive.name}: raw SHA256 does not match manifest")
  with tempfile.TemporaryDirectory() as tmp:
    manifest = Path(tmp) / "vamos.json"
    manifest.write_bytes(manifest_bytes)
    files = [assets_dir / "boot.img.xz", assets_dir / "system.img.xz", manifest]
    checksums = Path(tmp) / "SHA256SUMS"
    with checksums.open("w") as output:
      for file in files:
        with file.open("rb") as image:
          digest = hashlib.file_digest(image, "sha256").hexdigest()
        output.write(f"{digest}  {file.name}\n")
    command = ["gh", "release", "upload", tag, "--repo", repo, *(str(file) for file in files), str(checksums)]
    if clobber:
      command.append("--clobber")
    subprocess.run(command, check=True)


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--repo", default="commaai/vamOS")
  parser.add_argument("--tag", required=True)
  parser.add_argument("--manifest", type=Path, required=True)
  parser.add_argument("--assets-dir", type=Path, default=ROOT / "build" / "release")
  parser.add_argument("--clobber", action="store_true", help="Explicitly replace existing release assets")
  args = parser.parse_args()
  try:
    upload_update(args.assets_dir, args.manifest, args.repo, args.tag, args.clobber)
  except (OSError, ValueError) as error:
    parser.error(str(error))
  except subprocess.CalledProcessError as error:
    parser.exit(error.returncode)
