import hashlib
import json
import lzma
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).with_name("package_update.py")


class TestPackageUpdate(unittest.TestCase):
  def test_images_round_trip_with_agnos_manifest(self):
    with tempfile.TemporaryDirectory() as tmp:
      root = Path(tmp)
      images = {"boot.img": b"ANDROID!" + bytes(range(256)) * 100,
                "tmp-system/system.img": b"raw ext4 image" * 8192}
      for name, data in images.items():
        (root / name).parent.mkdir(parents=True, exist_ok=True)
        (root / name).write_bytes(data)
      output = root / "release"
      url = "https://example.com/releases/liberation-day-7.2"
      subprocess.run([sys.executable, str(SCRIPT), "--build-dir", str(root), "--output-dir", str(output),
                      "--images-url", url], check=True)
      manifest = json.loads((output / "vamos.json").read_text())
      self.assertEqual([entry["name"] for entry in manifest], ["boot", "system"])
      for entry, (name, data) in zip(manifest, images.items(), strict=True):
        archive = Path(name).name + ".xz"
        self.assertEqual(lzma.decompress((output / archive).read_bytes()), data)
        self.assertEqual(entry["url"], url + "/" + archive)
        self.assertEqual(entry["hash"], hashlib.sha256(data).hexdigest())
        self.assertEqual(entry["hash_raw"], entry["hash"])
        self.assertEqual(entry["size"], len(data))
        self.assertFalse(entry["sparse"])
        self.assertTrue(entry["has_ab"])
        self.assertEqual(entry["full_check"], entry["name"] == "boot")

  def test_missing_image_fails_before_packaging(self):
    with tempfile.TemporaryDirectory() as tmp:
      root = Path(tmp)
      (root / "boot.img").write_bytes(b"boot")
      output = root / "release"
      result = subprocess.run([sys.executable, str(SCRIPT), "--build-dir", str(root), "--output-dir", str(output),
                               "--images-url", "https://example.com/release"], capture_output=True, text=True, check=False)
      self.assertNotEqual(result.returncode, 0)
      self.assertIn("tmp-system/system.img", result.stderr)
      self.assertFalse(output.exists())


if __name__ == "__main__":
  unittest.main()
