# Existing-device-tree Venus enablement

This module enables the existing disabled comma tizi Venus node and sets its
firmware name to `venus.mdt`. It is for the installed device tree in
`7.2.0-vamos-4ec4055`; new tizi device trees in this branch already enable
the node and select that firmware, so they do not need this overlay.

The module refuses other boards, an already enabled node or a node with an
existing platform device. It balances device-tree/platform references and
removes its own overlay at module exit. Camera and codec consumers must be
stopped before any module lifecycle operation.

Build against the complete output for the running kernel:

```sh
make -C /linux O=out ARCH=arm64 LOCALVERSION=-vamos-4ec4055 \
  M=/repo/kernel/venus-enable MO=/repo/build/venus-enable modules
```

Use the matching patched Venus core, encoder, decoder and V4L2 dependencies.
On the tested installed image, the firmware search path must select the
existing stock 5.0 files under `/firmware/image`; `/lib/firmware/updates`
contains a different 5.2 build that fails the matched drain/rotation test.
No firmware bytes are changed by this module.

The checked runtime bundle loader records and verifies all module and firmware
identities. Its production module set passes repeated camera/encoder start,
normal recording rotation and clean stop. The parent-reference correction in
kernel patch 0025 also passes twenty separately checked core unload/load pairs.
Do not unload the camera clock provider when testing Venus.
