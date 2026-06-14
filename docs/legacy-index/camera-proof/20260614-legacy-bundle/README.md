# Legacy Camera Proof Bundle - 2026-06-14

Captured from mici on the legacy 4.9.103 AGNOS kernel (`build/boot-legacy.img`).
This is the current hardware-good baseline for the mainline Spectra port.

## Result

- `snapshot_standalone` exit code: `0`
- Snapshot summary: `=== captured 3/3 cameras ===`
- Captured files in the local raw bundle:
  - `snap/snap_wide.png` / `.nv12`
  - `snap/snap_road.png` / `.nv12`
  - `snap/snap_driver.png` / `.nv12`
- Image geometry: 1344x760 RGB PNGs plus matching NV12 dumps.

## Chip-ID Proof

Openpilot probes all three active cameras over CCI and reads the expected sensor
ID:

- cam0: slave `0x6c`, register `0x300a`, expected/read `0x5304`
- cam1: slave `0x20`, register `0x300a`, expected/read `0x5304`
- cam2: slave `0x6c`, register `0x300a`, expected/read `0x5304`

Legacy kernel dmesg also reports `Probe success` and `CAM_ACQUIRE_DEV Success`
for slots 0, 1, and 2 with the same addresses and `sensor_id:0x5304`.

## Local Artifacts

The full raw bundle was intentionally left untracked because it contains large
generated images, NV12 dumps, dmesg, debugfs, sysfs, and a raw `/proc/device-tree`
tarball. Local path:

`docs/legacy-index/camera-proof/20260614-legacy-bundle/legacy-cam-bundle-20260614-113436/`

Commit the raw artifacts only if we explicitly want the repository to carry the
binary camera proof data.
