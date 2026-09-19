#!/usr/bin/env python3
"""Package boot and raw ext4 system images for openpilot's A/B OS updater."""
import argparse
import hashlib
import json
import lzma
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
IMAGES = (("boot", "boot.img"), ("system", "tmp-system/system.img"))


def package_update(build_dir, output_dir, images_url):
  for _, filename in IMAGES:
    if (build_dir / filename).stat().st_size == 0:
      raise ValueError(f"Empty image: {filename}")
  output_dir.mkdir(parents=True, exist_ok=True)
  manifest = []
  for name, filename in IMAGES:
    digest = hashlib.sha256()
    size = 0
    archive = Path(filename).name + ".xz"
    with (build_dir / filename).open("rb") as source, lzma.open(output_dir / archive, "wb") as destination:
      while data := source.read(1024 * 1024):
        digest.update(data)
        size += len(data)
        destination.write(data)
    manifest.append({
      "name": name,
      "url": f"{images_url.rstrip('/')}/{archive}",
      "hash": digest.hexdigest(),
      "hash_raw": digest.hexdigest(),
      "size": size,
      "sparse": False,
      "full_check": name == "boot",
      "has_ab": True,
    })
    print(f"{archive}: {size} uncompressed bytes, SHA256 {digest.hexdigest()}")
  (output_dir / "vamos.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
  parser.add_argument("--output-dir", type=Path, default=ROOT / "build" / "release")
  parser.add_argument("--images-url", required=True, help="Published release asset base URL")
  args = parser.parse_args()
  package_update(args.build_dir, args.output_dir, args.images_url)
