#!/usr/bin/env python3

import os
from pathlib import Path
import signal
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).with_name("reboot_edl.c")
POWER_OFF = SOURCE.parents[1] / "root/usr/local/sbin/poweroff"


class RebootEdlTest(unittest.TestCase):
  def build_helper(self, executable):
    subprocess.run([
      os.environ.get("CC", "cc"), "-Wall", "-Wextra", "-Werror", "-O2",
      str(SOURCE), "-o", str(executable),
    ], check=True)

  def test_rejects_arguments(self):
    with tempfile.TemporaryDirectory() as tmp:
      executable = Path(tmp) / "reboot-edl"
      self.build_helper(executable)
      result = subprocess.run([executable, "unexpected"], text=True, capture_output=True)

    self.assertNotEqual(result.returncode, 0)
    self.assertIn("Usage:", result.stderr)

  @unittest.skipUnless(os.geteuid() == 0, "requires an isolated root container")
  def test_reboot_edl_routes_to_helper(self):
    helper = Path("/usr/local/libexec/reboot-edl")
    self.assertFalse(helper.exists())
    helper.parent.mkdir(parents=True, exist_ok=True)
    self.build_helper(helper)

    try:
      with tempfile.TemporaryDirectory() as tmp:
        reboot = Path(tmp) / "reboot"
        reboot.symlink_to(POWER_OFF)
        process = subprocess.Popen(
          ["/bin/sh", reboot, "edl"], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
          start_new_session=True,
        )
        try:
          _, stderr = process.communicate(timeout=3)
        except subprocess.TimeoutExpired:
          os.killpg(process.pid, signal.SIGKILL)
          process.communicate()
          self.fail("reboot edl did not route to the helper")
    finally:
      helper.unlink()

    self.assertNotEqual(process.returncode, 0)
    self.assertIn("reboot edl: Operation not permitted", stderr)
    self.assertIn("EDL reboot failed with status 1", stderr)


if __name__ == "__main__":
  unittest.main()
