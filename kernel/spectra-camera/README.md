# SDM845 Spectra module

This driver retains the camera request-manager, sync, sensor, ISP and ICP ABI
used by openpilot's Spectra camerad. The source was imported unchanged from
`cameras-spectra-mainline-andi` at
`391dedc767d96d7c36d1ade81f9d6d2230b6b407`, including Trey Moen's mainline port
and the subsequent pinctrl ownership fix. The donor subtree is
`30f4024a9f37811e6c361dbd2451170f071c80c2`.

The external `Kbuild` combines the donor objects into `spectra_camera.ko`.
`spectra_module.c` registers components in dependency order, publishes subdevice
nodes after registration, and unregisters components in reverse order.

## Build

Use the complete kernel build output for the running image, including its
configuration, generated headers and `Module.symvers`. For the retained ARM64
bring-up container, `/linux/out` is Linux 7.2 with the camera device tree and
shared-GPIO configuration from vamOS `4ec4055e8b2ba709f6e528d982ea8b62539ea0b5`:

```sh
make -C /linux O=out ARCH=arm64 LOCALVERSION=-vamos-4ec4055 \
  M=/repo/kernel/spectra-camera MO=/repo/build/spectra-module -j8 modules
```

The output is `/repo/build/spectra-module/spectra_camera.ko`. The release must
match `uname -r` on the target. This external module is built manually and is
not installed or loaded by the normal boot process.

Runtime dependencies are the matching `mc`, `videodev` and `camcc-sdm845`
modules, the tizi Spectra device tree, and `CAMERA_ICP.elf`. Firmware uses the
existing no-map camera memory reservation. The camera udev rule in
`userspace/root/etc/udev/rules.d/90-camera.rules` creates camerad's stock request
manager path without changing the platform device's parent during probe.

## Bring-up status

Revision 18, built from the driver source at `941b0c8`, passes stationary
three-camera capture on comma tizi with `7.2.0-vamos-4ec4055`. All three streams
deliver hardware-processed NV12 at 20 Hz. Separate module unload and reload
after capture succeed, followed by another successful three-camera session.

The reload hang was traced to CPAS device-tree reference ownership. The current
driver also balances optional clock and debugfs cleanup and unwinds failed
sensor GPIO acquisition. The kernel configuration disables automatic shared
GPIO ownership because Spectra manages sharing of its physical GPIOs itself.

Real camera DMA-BUFs pass tinygrad eager/JIT byte checks and driving-model
inference. The tested MSM allocation lookup fix reduces live model inference
from a median 82.7 ms to 29.5 ms. Evidence and reproduction commands are retained
in `/Volumes/Stuff/openpilot-extra/vamos-camera-bringup-2026-09-13/ACCEPTANCE.md`.

BPS/IPE clock RCG warnings still occur during camera startup, and legacy
teardown diagnostics remain. Successful bench capture does not establish
long-duration reliability or public-road safety. Preserve kernel logs on the
host and record each module operation separately when continuing.
