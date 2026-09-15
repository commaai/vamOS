#!/usr/bin/env python3
"""Load a matched comma tizi camera module bundle without changing the boot image."""
import argparse
import fcntl
import grp
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess


def require(condition, message):
  if not condition:
    raise RuntimeError(message)


def verify_loaded(module):
  note = Path('/sys/module', module['name'], 'notes/.note.gnu.build-id')
  digest = bytes.fromhex(module['build_id'])
  expected = struct.pack('<III', 4, len(digest), 3) + b'GNU\0' + digest
  require(note.read_bytes() == expected, f"loaded {module['name']} differs from the bundle; restart before replacing it")


def video_nodes():
  return {path.name: (path / 'name').read_text().strip() for path in Path('/sys/class/video4linux').glob('*')}


def run(bundle, check_only):
  manifest = json.loads((bundle / 'manifest.json').read_text())
  release = Path('/proc/sys/kernel/osrelease').read_text().strip()
  require(release == manifest['kernel'], f"kernel {release} does not match {manifest['kernel']}")
  compatible = Path('/sys/firmware/devicetree/base/compatible').read_bytes().split(b'\0')
  require(b'comma,tizi' in compatible, 'this bundle requires comma tizi')
  require(Path('/proc/sys/kernel/tainted').read_text().strip() in ('0', '4096'), 'kernel fault detected; restart before loading modules')

  # Check the entire bundle and existing modules before making any change.
  missing = []
  for module in manifest['modules']:
    path = bundle / 'modules' / module['file']
    require(hashlib.sha256(path.read_bytes()).hexdigest() == module['sha256'], f'module hash mismatch: {path}')
    require(module['vermagic'].split()[0] == release, f'module ABI mismatch: {path}')
    if Path('/sys/module', module['name']).exists():
      verify_loaded(module)
    else:
      missing.append(module)
  for path, expected in manifest['firmware'].items():
    require(hashlib.sha256(Path(path).read_bytes()).hexdigest() == expected, f'firmware hash mismatch: {path}')

  rule_source = bundle / '90-vamos-camera.rules'
  require(hashlib.sha256(rule_source.read_bytes()).hexdigest() == manifest['udev_rules_sha256'], 'udev rule hash mismatch')
  rule_target = Path('/run/udev/rules.d/90-vamos-camera.rules')
  request_manager = Path('/dev/v4l/by-path/platform-soc:qcom_cam-req-mgr-video-index0')
  firmware_path = Path('/sys/module/firmware_class/parameters/path')
  wanted_path = manifest['firmware_search_path']
  if check_only:
    require(not missing, f"modules not loaded: {[module['name'] for module in missing]}")
    require(rule_target.exists() and rule_target.read_bytes() == rule_source.read_bytes(), 'runtime udev rules are not installed')
    require(firmware_path.read_text().strip() == wanted_path, 'firmware search path differs from the bundle')
  else:
    require(os.geteuid() == 0, 'module loading requires root')
    for process in Path('/proc').glob('[0-9]*/exe'):
      try:
        name = process.resolve(strict=True).name
      except (FileNotFoundError, PermissionError, ProcessLookupError):
        continue
      require(name not in ('camerad', 'encoderd', 'loggerd'), f'stop camera consumers first: {process} ({name})')
    if rule_target.exists():
      require(rule_target.read_bytes() == rule_source.read_bytes(), 'unexpected runtime camera rules; preserve and review them first')
    rule_target.parent.mkdir(parents=True, exist_ok=True)
    rule_target.write_bytes(rule_source.read_bytes())
    subprocess.run(['udevadm', 'control', '--reload'], check=True)
    firmware_path.write_text(wanted_path)
    for module in missing:
      print(json.dumps({'loading': module['name'], 'sha256': module['sha256']}), flush=True)
      subprocess.run(['insmod', str(bundle / 'modules' / module['file'])], check=True)
      verify_loaded(module)
      require(Path('/proc/sys/kernel/tainted').read_text().strip() in ('0', '4096'), 'new kernel fault while loading modules')
    subprocess.run(['udevadm', 'trigger', '--action=change', '--subsystem-match=dma_heap'], check=True)
    # Generic V4L probing opens private Spectra controls outside camerad's lifecycle.
    if not request_manager.exists():
      subprocess.run(['udevadm', 'trigger', '--action=change', '--subsystem-match=video4linux',
                      '--attr-match=name=cam-req-mgr'], check=True)
    subprocess.run(['udevadm', 'settle', '--timeout=15'], check=True)

  nodes = video_nodes()
  require(sum(name.startswith(('cam-', 'cam_')) for name in nodes.values()) == 16, f'camera nodes missing: {nodes}')
  require('qcom-venus-encoder' in nodes.values() and 'qcom-venus-decoder' in nodes.values(), f'codec nodes missing: {nodes}')
  require(request_manager.exists(), 'request-manager alias missing')
  require(Path('/sys/firmware/devicetree/base/soc@0/video-codec@aa00000/firmware-name').read_bytes() == b'venus.mdt\0',
          'Venus device tree selects different firmware')
  heap = Path('/dev/dma_heap/system').stat()
  require(heap.st_mode & 0o777 == 0o660 and heap.st_gid == grp.getgrnam('gpu').gr_gid, 'camera DMA heap permissions differ')
  require(Path('/proc/sys/kernel/tainted').read_text().strip() == '4096', 'new kernel fault during camera setup')
  receipt = {'ready': True, 'boot_id': Path('/proc/sys/kernel/random/boot_id').read_text().strip(), 'kernel': release,
             'bundle': str(bundle), 'modules': {module['name']: module['build_id'] for module in manifest['modules']}, 'nodes': nodes}
  if not check_only:
    Path('/run/vamos-camera-modules.json').write_text(json.dumps(receipt, indent=2) + '\n')
  print(json.dumps(receipt, indent=2), flush=True)


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('bundle', type=Path)
  parser.add_argument('--check', action='store_true', help='verify the prepared runtime without changing it')
  args = parser.parse_args()
  if args.check:
    run(args.bundle.resolve(), True)
  else:
    with Path('/run/vamos-camera-modules.lock').open('w') as lock:
      fcntl.flock(lock, fcntl.LOCK_EX)
      run(args.bundle.resolve(), False)
