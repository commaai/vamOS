import hashlib
import json
import lzma
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).with_name("upload_update.py")


class TestUploadUpdate(unittest.TestCase):
  def setUp(self):
    self.temp = tempfile.TemporaryDirectory()
    self.addCleanup(self.temp.cleanup)
    self.root = Path(self.temp.name)
    self.assets = self.root / "assets"
    self.assets.mkdir()
    self.manifest = self.root / "agnos.json"
    self.record = self.root / "upload.json"
    self.entries = []
    for name, data in (("boot", b"ANDROID!boot"), ("system", b"raw ext4 filesystem")):
      (self.assets / f"{name}.img.xz").write_bytes(lzma.compress(data))
      self.entries.append({"name": name,
                           "url": f"https://github.com/commaai/vamOS/releases/download/test-release/{name}.img.xz",
                           "hash": hashlib.sha256(data).hexdigest(), "hash_raw": hashlib.sha256(data).hexdigest(),
                           "size": len(data), "sparse": False, "has_ab": True, "full_check": name == "boot"})
    self.write_manifest()
    fake = self.root / "gh"
    fake.write_text(f"#!{sys.executable}\n" + '''import json, os, sys
from pathlib import Path
files = {Path(arg).name: Path(arg).read_bytes().hex() for arg in sys.argv[1:] if Path(arg).is_file()}
Path(os.environ['UPLOAD_RECORD']).write_text(json.dumps({'args': sys.argv[1:], 'files': files}))
''')
    fake.chmod(0o755)

  def write_manifest(self):
    self.manifest.write_text("\n" + json.dumps(self.entries, indent=3) + "\n\n")

  def run_cli(self, *extra):
    return subprocess.run([sys.executable, str(SCRIPT), "--tag", "test-release", "--manifest", str(self.manifest),
                           "--assets-dir", str(self.assets), *extra], capture_output=True, text=True, check=False,
                          env={**os.environ, "PATH": str(self.root) + os.pathsep + os.environ["PATH"],
                               "UPLOAD_RECORD": str(self.record)})

  def test_upload_preserves_exact_manifest_bytes_and_does_not_clobber(self):
    original = self.manifest.read_bytes()
    result = self.run_cli()
    self.assertEqual(result.returncode, 0, result.stderr)
    record = json.loads(self.record.read_text())
    self.assertEqual(record["args"][:5], ["release", "upload", "test-release", "--repo", "commaai/vamOS"])
    self.assertNotIn("--clobber", record["args"])
    self.assertEqual(set(record["files"]), {"boot.img.xz", "system.img.xz", "vamos.json", "SHA256SUMS"})
    self.assertEqual(bytes.fromhex(record["files"]["vamos.json"]), original)
    self.assertEqual(self.manifest.read_bytes(), original)
    uploaded = {name: bytes.fromhex(data) for name, data in record["files"].items()}
    for name in ("boot.img.xz", "system.img.xz"):
      self.assertEqual(uploaded[name], (self.assets / name).read_bytes())
    checksums = "".join(f"{hashlib.sha256(uploaded[name]).hexdigest()}  {name}\n"
                        for name in ("boot.img.xz", "system.img.xz", "vamos.json"))
    self.assertEqual(uploaded["SHA256SUMS"], checksums.encode())

  def test_persist_is_rejected_before_upload(self):
    self.entries.append({"name": "persist"})
    self.write_manifest()
    result = self.run_cli()
    self.assertNotEqual(result.returncode, 0)
    self.assertIn("persist", result.stderr)
    self.assertFalse(self.record.exists())

  def test_only_matching_raw_ab_boot_and_system_manifests_are_accepted(self):
    original = json.loads(json.dumps(self.entries))
    for field, value, message in (("name", "recovery", "boot and system"),
                                   ("sparse", True, "raw A/B"),
                                   ("has_ab", False, "raw A/B"),
                                   ("url", "https://example.com/stale.img.xz", "URL")):
      with self.subTest(field=field):
        self.entries = json.loads(json.dumps(original))
        self.entries[0][field] = value
        self.write_manifest()
        result = self.run_cli()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(message, result.stderr)
        self.assertFalse(self.record.exists())

  def test_stale_or_missing_images_fail_before_gh(self):
    archive = self.assets / "system.img.xz"
    original = archive.read_bytes()
    original_entries = json.loads(json.dumps(self.entries))
    for problem in ("missing", "invalid xz", "truncated xz", "hash", "hash_raw", "size"):
      with self.subTest(problem=problem):
        self.entries = json.loads(json.dumps(original_entries))
        archive.write_bytes(original)
        if problem == "missing":
          archive.unlink()
        elif problem == "invalid xz":
          archive.write_bytes(b"not an xz archive")
        elif problem == "truncated xz":
          archive.write_bytes(original[:-8])
        else:
          self.entries[1][problem] = 1 if problem == "size" else "0" * 64
        self.write_manifest()
        result = self.run_cli()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("system", result.stderr)
        self.assertFalse(self.record.exists())

  def test_clobber_is_explicit_and_gh_failure_is_returned(self):
    fake = self.root / "gh"
    fake.write_text(fake.read_text() + "\nsys.exit(7)\n")
    result = self.run_cli("--clobber")
    self.assertEqual(result.returncode, 7)
    record = json.loads(self.record.read_text())
    self.assertIn("--clobber", record["args"])
    self.assertEqual(bytes.fromhex(record["files"]["vamos.json"]), self.manifest.read_bytes())

  def test_duplicate_missing_and_malformed_entries_fail_before_gh(self):
    for entries in ([self.entries[0]], [self.entries[0], self.entries[0]], {"images": self.entries}, ["boot", "system"]):
      with self.subTest(entries=entries):
        self.manifest.write_text(json.dumps(entries))
        result = self.run_cli()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("boot and system", result.stderr)
        self.assertFalse(self.record.exists())


if __name__ == "__main__":
  unittest.main()
