import io
import runpy
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

ABCTL = Path(__file__).resolve().parents[2] / 'userspace/root/usr/bin/abctl'


class TestAbctl(unittest.TestCase):
  def test_slot_changes_escalate_before_accessing_devices(self):
    class ExecRequested(Exception):
      pass

    for args in (['--set_active', '1'], ['--set_unbootable', '0'], ['--set_success']):
      with self.subTest(args=args), patch.object(sys, 'argv', ['abctl', *args]), \
           patch('os.geteuid', return_value=1000), patch('os.execv', side_effect=ExecRequested) as execute, \
           patch('builtins.open', side_effect=AssertionError('device access before privilege escalation')):
        with self.assertRaises(ExecRequested):
          runpy.run_path(str(ABCTL), run_name='__main__')
        execute.assert_called_once_with('/usr/bin/sudo', ['sudo', '-n', '--', '/usr/bin/abctl', *args])

  def test_boot_slot_query_does_not_require_sudo(self):
    with patch.object(sys, 'argv', ['abctl', '--boot_slot']), patch('os.geteuid', return_value=1000), \
         patch('os.execv') as execute, patch('builtins.open', return_value=io.StringIO('androidboot.slot_suffix=_b')), \
         patch('sys.stdout', new_callable=io.StringIO) as output:
      runpy.run_path(str(ABCTL), run_name='__main__')
      self.assertEqual(output.getvalue(), '_b\n')
      execute.assert_not_called()

  def test_invalid_slot_is_rejected_before_sudo(self):
    for command in ('--set_active', '--set_unbootable'):
      with self.subTest(command=command), patch.object(sys, 'argv', ['abctl', command, '2']), \
           patch('os.geteuid', return_value=1000), patch('os.execv') as execute, \
           patch('sys.stderr', new_callable=io.StringIO) as error:
        with self.assertRaises(SystemExit) as result:
          runpy.run_path(str(ABCTL), run_name='__main__')
        self.assertEqual(result.exception.code, 1)
        self.assertIn('expected 0 or 1', error.getvalue())
        execute.assert_not_called()

  def test_root_mutation_proceeds_without_reexecuting(self):
    class DeviceRequested(Exception):
      pass

    with patch.object(sys, 'argv', ['abctl', '--set_active', '1']), patch('os.geteuid', return_value=0), \
         patch('os.execv') as execute, patch('builtins.open', side_effect=DeviceRequested):
      with self.assertRaises(DeviceRequested):
        runpy.run_path(str(ABCTL), run_name='__main__')
      execute.assert_not_called()

  def test_failed_escalation_does_not_access_devices(self):
    with patch.object(sys, 'argv', ['abctl', '--set_active', '1']), patch('os.geteuid', return_value=1000), \
         patch('os.execv', side_effect=PermissionError('sudo unavailable')), patch('builtins.open') as device:
      with self.assertRaisesRegex(PermissionError, 'sudo unavailable'):
        runpy.run_path(str(ABCTL), run_name='__main__')
      device.assert_not_called()


if __name__ == '__main__':
  unittest.main()
