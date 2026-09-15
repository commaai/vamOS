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
match `uname -r` on the target. Preserve the complete kernel `Module.symvers`
when building individual in-tree codec targets: a partial module build can
replace it with a table that omits symbols needed by external Spectra.

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

The September 14 continuation fixes further module cleanup and failed-start
ownership issues. CAMCC revision 3 rounds the shared PLL parent request to its
achievable rate and resolves the BPS/IPE clock warnings. Do not hot-unload
CAMCC: this kernel's GDSC unregister path does not remove its PM-domain list
entry. Use a fresh boot when replacing the clock provider.

Spectra revision 26 passes two 135-second manager-driven onroad/offroad cycles
with all three cameras, driving and driver-monitoring inference, physical
sensors, UI rendering and four encoders. All 24 recorded files decode and
their 21,611 frames reconcile with camera and encode metadata. Stock 5.0 Venus
firmware is selected by the tizi device tree; patches 0023–0028 correct recovery,
reference ownership, timing, capture-buffer requirements and EOS memory.
The original 5.2 firmware still fails the matched comparison.

The intermittent zero BPS patch handles were reproduced without sensors:
dirty cached page zeroing overwrote command buffers exposed through WC aliases.
The allocator now prepares uncached pages with the DMA API before exporting
them. High-order pages also use `__GFP_COMP`, as required by their size/free
accounting. Public allocation tests reproduce both defects and pass after the
fixes, including 4,096 uncached 64 KiB buffers under cache pressure.

On the final module, gain/integration/automatic-return controls pass on all
three cameras. Eager and JIT camera-buffer arithmetic matches CPU data;
120 uninterrupted JIT frames per stream run at 20 Hz with reused DMA-BUFs
and no GPU faults. The real driving model consumes 60 synchronized DMA-BUF
pairs with a 29.5 ms median inference time. Spectra unloads with no camera
nodes, workqueues or clock debug directories left, then reloads and captures
again. These are stationary bench results, not Panda or road acceptance.
See the acceptance report and changelog for the current boot and exact tested
revisions. Keep host serial capture and test each lifecycle operation separately.

## Runtime bundle

`tools/runtime/load_camera_modules.py` loads an explicitly versioned bundle
from writable storage. The manifest lists modules in dependency order, their
SHA256 hashes and ELF build IDs, existing firmware hashes and the camera udev
rule hash. It verifies every artifact and any already loaded module before
changing runtime state. It never unloads a module, replaces firmware, flashes
a partition or changes a slot. A different loaded build requires a restart.

The loader installs rules only in `/run/udev/rules.d`, selects the existing
firmware search path and verifies the camera/codec nodes and DMA-heap access.
The `--check` option verifies the prepared runtime without changing it.
Do not run the loading mode while camera consumers are active.

The prepared `camera-runtime-01` bundle in the companion investigation folder
contains the exact tested 7.2 modules and existing-firmware identities. Its
loader is being integrated into startup; fresh-boot acceptance is still pending.
