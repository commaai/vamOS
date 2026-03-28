#!/usr/bin/env python3
import subprocess


def run(cmd, *, check=True, input_text=None):
  return subprocess.run(
    cmd,
    shell=True,
    check=check,
    input=input_text,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
  ).stdout


def expected_bt_addr():
  return run("/usr/comma/get-bt-address.sh").strip()


def test_bluetooth():
  expected_addr = expected_bt_addr().upper()

  devices = run("bluetoothctl list")
  assert "Controller" in devices, devices
  assert expected_addr in devices.upper(), devices

  power_on = run("sudo bluetoothctl", input_text="power on\nshow\nquit\n")
  assert "Changing power on succeeded" in power_on or "Powered: yes" in power_on, power_on
  assert "Powered: yes" in power_on, power_on
  assert expected_addr in power_on.upper(), power_on

  show = run("sudo btmgmt info")
  assert "hci0" in show, show
  assert "current settings:" in show, show
  assert "powered" in show.lower(), show
  assert expected_addr in show.upper(), show

  scan = run("sudo bluetoothctl", input_text="scan on\nshow\nscan off\nquit\n")
  assert "Discovery started" in scan or "Discovering: yes" in scan, scan
  assert "Discovering: yes" in scan, scan

  logs = run("journalctl -b", check=False)
  bt_logs = "\n".join(
    line for line in logs.splitlines()
    if any(token in line.lower() for token in ("bluetooth", "qca", "hci", "wcn399"))
  )
  for needle in (
    "Failed to download patch",
    "Failed to download NVM",
    "Direct firmware load for qca/",
  ):
    assert needle not in bt_logs, bt_logs
