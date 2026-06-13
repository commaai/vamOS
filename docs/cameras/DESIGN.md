# vamOS Cameras — Implementation Design

**Goal:** make openpilot's `camerad` stream all three comma four (mici) cameras on
the **mainline 6.18 vamOS kernel**, exactly as it does on the legacy 4.9.103 AGNOS
kernel — with **openpilot completely unmodified**.

This document is the architecture + rationale. The live work breakdown is in
[`TASKS.md`](./TASKS.md). The captured legacy baseline (what "working" looks like)
is in [`../legacy-index/CAMERAS.md`](../legacy-index/CAMERAS.md).

---

## 1. The constraint that dictates everything

openpilot's `camerad` (`system/camerad/cameras/spectra.cc`, `camera_qcom2.cc`) is
written **directly against the downstream Qualcomm "Spectra" CAMSS kernel ABI**:

- Video nodes: `platform-soc:qcom_cam-req-mgr-video-index0` (cam-req-mgr),
  `platform-cam_sync-video-index0` (cam_sync).
- 15 `cam-*` v4l-subdevs: `cam-cpas`, `cam-isp`, `cam-cci-driver`,
  `cam-csiphy-driver`×4, `cam-sensor-driver`×4, `cam-icp`, `cam-jpeg`, `cam-fd`,
  `cam-lrme`.
- Custom UAPI ioctls: `cam_req_mgr`, `cam_isp`, `cam_sensor`, `cam_sync`; IFE/BPS
  config blobs (`ife.h`, `bps_blobs.h`, `cdm.cc`).

Mainline Linux ships `qcom-camss`, but it is a **completely different interface**
(a `media-ctl` graph of `msm_csiphy`/`msm_csid`/`msm_vfe` + plain `/dev/videoN`
RDI/bayer capture). It has **no** cam-req-mgr, no cam_sync, no cam_isp/cam_sensor
UAPI, and **no on-device ISP pipeline** (debayer/AWB/tonemap) to produce the YUV
frames camerad's VisionIPC consumers expect.

**Therefore:** there is no mainline config or userspace shim that makes camerad
work unmodified. The downstream ABI must *exist* on the device. We bring the
Spectra stack itself to mainline.

## 2. Chosen approach — out-of-tree source, built-in, linked via copy (decided 2026-06-13)

Keep the downstream Spectra camera driver as a **normal versioned source tree** in
this repo at **`kernel/spectra-camera/`** (NOT encoded as a giant add-every-file
patch). At build time it is **copied into** the mainline 6.18 kernel tree at
`drivers/media/platform/msm/` and compiled **built-in** (`CONFIG_SPECTRA_CAMERA=y`)
into `boot.img`. **No userspace / system.img / `.ko` — the driver is in the kernel.**

How it links (see `tools/build/build_kernel.sh`):
- `kernel/spectra-camera/{camera/,Kconfig,Makefile}` → copied to
  `drivers/media/platform/msm/` by `install_spectra()` (runs after patch-apply,
  like `install_dts()`); `kernel/spectra-camera/uapi/media/cam_*.h` →
  `include/uapi/media/`. `clean_kernel_tree` removes them before the next build.
- A tiny **24-line** patch `kernel/patches/0012-driver-link-spectra-camera.patch`
  adds just two lines to mainline: `obj-y += msm/` (platform Makefile) and
  `source ".../msm/Kconfig"` (platform Kconfig). That's the entire kernel-tree
  footprint of the patch series.
- The driver subdir Makefiles' include paths were rewritten in-source from bare
  `-Idrivers/...` to `-I$(srctree)/drivers/...` so cross-subdir includes resolve
  under the `O=out` build (45 Makefiles; done once in `kernel/spectra-camera/`).

Why copy and not symlink: kbuild with `O=out` + ccache mis-handles symlinks that
point outside the source tree; a plain `cp -a` is robust and the source still
lives as an ordinary editable/versioned dir.

Considered and rejected:
- *Add-every-file in-tree patch (351 files)* — wasteful, unreviewable, churny.
- *External `.ko` in system.img or boot.img initramfs* — user wants it built into
  the kernel with no userspace/rootfs changes for now.
- *Userspace Spectra-ABI shim over mainline qcom-camss* — impossible: mainline
  camss has no ISP, so the YUV pipeline simply doesn't exist to shim.
- *Modify camerad to target qcom-camss* — violates "openpilot unmodified".

### Source of truth
Legacy driver tree (the thing we are porting):
`~/claudes/agnos-builder/agnos-kernel-sdm845` @ `c368754` (kernel 4.9.103)
- Driver: `drivers/media/platform/msm/camera/` — 135 `.c` + 168 `.h`, 14 subdirs:
  `cam_req_mgr cam_utils cam_core cam_sync cam_smmu cam_cpas cam_cdm cam_isp
  cam_sensor_module cam_icp cam_jpeg cam_fd cam_lrme al6100`.
- Kconfig/Makefile glue: `drivers/media/platform/msm/{Kconfig,Makefile}`.
- DTS: `arch/arm64/boot/dts/qcom/{sdm845-camera.dtsi,
  sdm845-camera-sensor-mtp.dtsi, comma_common.dtsi, comma_mici.dts}`.
- UAPI headers (camerad already builds against these, must match byte-for-byte):
  `include/uapi/media/cam_*.h`.

### Build integration (how it lands in vamOS)
Per `tools/build/build_kernel.sh`:
- Driver source lives at `kernel/spectra-camera/` and is copied into the kernel
  tree by `install_spectra()` (see §2 above); the only patch is the 24-line link
  patch `0012-driver-link-spectra-camera.patch`.
- Enables → `kernel/configs/vamos.config`: `CONFIG_SPECTRA_CAMERA=y` plus
  `# CONFIG_MEDIA_SUPPORT_FILTER is not set` and
  `MEDIA_SUPPORT/MEDIA_CAMERA_SUPPORT/MEDIA_CONTROLLER/MEDIA_PLATFORM_SUPPORT/
  VIDEO_DEV/V4L_PLATFORM_DRIVERS/MEDIA_PLATFORM_DRIVERS=y`. (6.18 dropped
  `VIDEO_V4L2` → use `VIDEO_DEV`; `EXPERT=y` turns on `MEDIA_SUPPORT_FILTER`
  which otherwise hides camera support.)
- Camera DTS nodes → into `kernel/dts/sdm845-comma-{common.dtsi,mici.dts}`
  (this repo's DTS, not the legacy one) and built into the mici dtb.

## 3. The hard part — 4.9 → 6.18 API gap

The driver is 9 years behind mainline. The blocking subsystem rewrites
(from the dependency scan) — each is a TASKS.md work item:

| # | Subsystem | Legacy API (4.9) | Mainline 6.18 target | Risk |
|---|-----------|------------------|----------------------|------|
| A | Memory alloc | `msm_ion`, `ion_alloc/_map_kernel`, `ion_handle` | dma-buf heaps (`dma_heap_*`) / `dma_buf_vmap` | ★★★★★ |
| B | SMMU/IOMMU | `arm_iommu_*`, `asm/dma-iommu.h`, `msm_dma_iommu_mapping.h`, `struct dma_iommu_mapping` | generic `iommu_domain_alloc`/`iommu_attach_device`/`iommu_map_sg` | ★★★★★ |
| C | Bus/BW | `msm-bus.h`, `msm_bus_scale_*` | interconnect (`icc_get`/`icc_set_bw`/`icc_put`) | ★★★★ |
| D | Sync/fence | downstream `cam_sync` + `sync.h` | keep custom cam_sync; modernize against 6.18 `dma_fence`/`completion` | ★★★★ |
| E | SCM/secure | `soc/qcom/scm.h`, `secure_buffer.h` | `qcom_scm_*` (mainline qcom firmware API) | ★★★ |
| F | V4L2/media | `v4l2_device`, custom `cam_subdev`, private cam_req_mgr UAPI char dev | verify subdev-node + media-dev registration, `/dev` naming preserved | ★★★ |
| G | misc headers | `linux/sched.h` field moves, `asm/cacheflush.h`, `get_user_pages` sig, pinctrl | mechanical header/signature fixes | ★★ |

clk / regulator / pinctrl / `of_*` are largely stable (★).

### Naming is load-bearing
camerad finds nodes by **exact** names: video-node by-path strings and subdev
`name` fields (`cam-cpas`, `cam-isp`, `cam-cci-driver`, …). The port must preserve
every `cam_subdev` name and the platform-device names that produce the by-path
symlinks (`soc:qcom_cam-req-mgr`, `cam_sync`). Verified target list:
[`../legacy-index/CAMERAS.md`](../legacy-index/CAMERAS.md) §"How openpilot talks".

## 4. Hardware wiring (mici)

- 4 CSIPHY @ `ac65000/ac66000/ac67000/ac68000`; CCI @ `ac4a000`;
  CPAS @ `ac40000`; CDM @ `ac48000`; CSID0/1/lite @ `acb3000/acba000/acc8000`;
  VFE0/1/lite @ `acaf000/acb6000/acc4000`; ICP/A5 @ `ac00000`;
  JPEG @ `ac4e000`; FD @ `ac5a000`.
- 4 `qcom,cam-sensor@0..3`; phy/cci-master = (0,0)(1,0)(2,1)(3,1); sensor
  positions roll 180/180/180/270. 3 stream on mici (road/wide/driver).
- Sensors **OS04C10** + **OX03C10**; DT does **not** name the part — camerad
  probes chip-id over CCI at runtime (`0x5304` family seen on device).
- Power supplies per sensor: `cam_vio`/`cam_vana`/`cam_vdig`/`cam_clk` — must map
  the legacy `pm8998_lvs1`/`pmi8998_bob`/`camera_*_ldo` regulators onto the
  regulator nodes already in this repo's `sdm845-comma-common.dtsi`.
- mclk pinctrl overrides live in legacy `comma_mici.dts`
  (`&cam_sensor_mclk{0,1,2}_active`).

## 5. Strategy / sequencing

Bring-up is bottom-up so each layer can be validated before the next:

1. **Land the tree, make it build** (stubs allowed) — get `CONFIG_SPECTRA_CAMERA=y`
   compiling against 6.18 with the four blocking subsystems (A/B/C/D) stubbed or
   minimally adapted. Goal: a `boot.img` that boots.
2. **Platform/CPAS + SMMU (B)** — `cam-cpas` + `cam_smmu` probe cleanly; this is
   the foundation every other HW block sits on.
3. **CCI + CSIPHY + sensor** — `cam-cci-driver`, 4× `cam-csiphy-driver`, 4×
   `cam-sensor-driver` probe; sensor chip-id read over CCI succeeds.
4. **Memory (A) + ISP (IFE/CSID) + req-mgr** — `cam-isp`, `cam-req-mgr`,
   `cam_sync`, real buffers; a single RDI/IFE stream produces frames.
5. **Full pipeline + camerad** — ICP/BPS YUV path; run camerad unmodified, get
   synced frame_ids on all 3 cameras and a real `snapshot.py` JPEG.

Each step's exit criterion + status is tracked in `TASKS.md`.

## 6. Validation (definition of done)

Reproduce the legacy baseline on mainline:
- `/dev/v4l/by-path/` + `/dev/v4l-subdev*` names match the legacy capture exactly.
- `dmesg` shows all cam blocks probing without `-ENXIO`/`-EPROBE_DEFER` storms.
- camerad (unmodified) streams all 3 cameras with incrementing, hw-timestamped
  frame_ids.
- `system/camerad/snapshot.py` produces real YUV→RGB JPEGs matching
  `legacy-{back,front}.jpg`.
- `test_onroad` camera checks pass; `dmesg` clean.

## 7. Process

- Implementation tasks are delegated to **Opus 4.8 subagents**, one bounded
  work-item at a time, and **reviewed** before integration.
- Build is hermetic: `./vamos build kernel` (Docker `vamos-builder`).
- Flash + on-device test via the **mici skill** (EDL/QDL) and
  `ssh comma@10.0.0.22`. Legacy fallback image: `build/boot-legacy.img`
  (`./vamos flash kernel --legacy`).
