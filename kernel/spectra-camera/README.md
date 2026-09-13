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
bring-up container, `/linux/out` is Linux 7.2 with the camera device tree from
vamOS `b6f2735b1470a35d1bd89ea42a760d122a5746cc`:

```sh
make -C /linux O=out ARCH=arm64 LOCALVERSION=-vamos-b6f2735 \
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

The Linux 7.2 module builds. Earlier revisions loaded the camera device nodes,
but load/unload testing exposed lifetime faults. Revision 14 also corrects CDM
lock/refcount cleanup and the request-manager runtime-PM parent mismatch. The
CDM control-flow regression and udev syntax checks pass locally.

Revision 14 has not been loaded on hardware. Device connectivity was lost
during the previous revision's combined load/unload/reload command; the exact
failing step is unknown. All three camera streams, DMA-BUF frame consumption by
tinygrad, and stable restarts remain acceptance requirements. Preserve kernel
logs on the host and record each module operation separately when continuing.
