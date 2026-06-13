# vamOS Cameras — Task Tracker

Live work breakdown for the Spectra-in-mainline port. Architecture &amp; rationale:
[`DESIGN.md`](./DESIGN.md). Status legend: ☐ todo · ◐ in progress · ☑ done · ✖ blocked.

Implementation work-items are delegated to **Opus 4.8 subagents** (one bounded
item each) and reviewed before the patch is committed to `kernel/patches/`.

---

## Phase 0 — Baseline &amp; reconnaissance  ☑

- [x] Capture legacy working baseline (v4l node/subdev names, sensors, ABI) →
      `docs/legacy-index/CAMERAS.md`.
- [x] Confirm device on legacy 4.9.103, cameras stream (Spectra cam-* stack +
      `sensor_id:0x5304` probed in dmesg).
- [x] Check out legacy driver source (agnos-kernel-sdm845 submodule @ c368754).
- [x] Inventory 4.9→6.18 kernel-API gap by subsystem (memory/iommu/bus/sync/...).
- [x] Decide architecture: **in-tree port, no userspace** → DESIGN.md.

## Phase 1 — Land the tree &amp; make it build  ☐

Exit: `./vamos build kernel` produces a `boot.img` with `CONFIG_SPECTRA_CAMERA=y`,
booting on device (blocking subsystems may be stubbed; no camera function yet).

- [x] **1.1** Land driver as **out-of-tree source** `kernel/spectra-camera/`
      (revised from a 351-file import patch — see DESIGN.md §2). Driver tree
      (`camera/` 135 .c + slim `msm/{Kconfig,Makefile}`) + `uapi/media/cam_*.h`
      live as a normal versioned dir; `build_kernel.sh:install_spectra()` copies
      them into `drivers/media/platform/msm/` + `include/uapi/media/` after
      patch-apply. Only patch is the 24-line link patch
      `0012-driver-link-spectra-camera.patch` (adds `obj-y += msm/` +
      `source ".../msm/Kconfig"` to the 6.18 platform Makefile/Kconfig). Driver
      subdir Makefiles rewritten in-source from `-Idrivers/...` →
      `-I$(srctree)/drivers/...` (45 files) so includes resolve under `O=out`.
      Verified: patch applies, source links in, build reaches camera compilation.
- [x] **1.2** `CONFIG_SPECTRA_CAMERA=y` + deps added to `kernel/configs/vamos.config`
      (`MEDIA_SUPPORT_FILTER` disabled — `CONFIG_EXPERT=y` had turned it on, hiding
      camera support; `MEDIA_SUPPORT`/`MEDIA_CAMERA_SUPPORT`/`MEDIA_CONTROLLER`/
      `MEDIA_PLATFORM_SUPPORT`/`VIDEO_DEV`/`V4L_PLATFORM_DRIVERS`/
      `MEDIA_PLATFORM_DRIVERS`=y). Note: 6.18 dropped `VIDEO_V4L2` → use `VIDEO_DEV`.
      `olddefconfig` keeps `CONFIG_SPECTRA_CAMERA=y`.
- [x] **1.3** First compile pass against 6.18 done — full error surface captured
      below ("Phase 1 build error surface"). Patch applies; compilation reaches the
      camera tree and breaks on the 4.9→6.18 API gap as expected.
- [x] **1.4** Stub/adapt blocking subsystems just enough to **link**. ☑ 2026-06-12
      — `./vamos build kernel` now COMPILES + LINKS the whole Spectra tree
      (`CONFIG_SPECTRA_CAMERA=y`) into `vmlinux` and produces
      `build/boot.img` (16.8M). A (ion→dma-buf), B (iommu), C (msm-bus→icc),
      E (scm→stub), and all mechanical sig-drift (2.G/2.G-residual + the trailing
      `.remove`/i2c-probe/debugfs/CMA batch) are done. D (sync) needed no link-time
      change. See "Phase 2.C/E + build-links surface" below. Remaining stubs in the
      ledger are all OFF the mici non-secure data path (secure SCM, stage-2 ION,
      QPNP flash, SPI/OIS CMA, AHB icc no-op pending DTS).
- [ ] **1.5** Boots on device; `dmesg` shows the driver registering (even if HW
      blocks fail to probe). *(pending on-device flash/boot — build artifact ready)*

## Phase 2 — Blocking subsystem rewrites  ☐

(See DESIGN.md §3 table. Each is its own subagent task + patch.)

- [x] **2.A** Memory: `msm_ion`/`ion_*` → dma-buf / `dma_buf_vmap`
      (`cam_req_mgr/cam_mem_mgr.{c,h}` + 13 stray-include sites). ★★★★★
      ☑ 2026-06-12 — all ION/`msm_ion`/`slub_def` errors cleared; `cam_mem_mgr`,
      `cam_req_mgr_dev`, `cam_isp_dev`, `cam_cdm_*`, sensor/cci/csiphy headers
      advance. Leading edge moved entirely off the memory bucket onto C/E/F/G.
      KEY constraint discovered: the **in-kernel dma-heap consumer API
      (`dma_heap_find`/`dma_heap_buffer_alloc`) is NOT present in this 6.18
      baseline** (`dma_heap_buffer_alloc` is `static`, fd-only; no heap-by-name
      lookup for built-in drivers). So the kernel-allocation path uses a minimal
      **page-backed dma-buf exporter** built into `cam_mem_mgr.c` (functional
      equivalent of the legacy non-secure ION system heap: buddy pages → real
      `sg_table` → `dma_buf_export`). cam_smmu (2.B) consumes that dma_buf via its
      normal attach/map path — the 2.A/2.B seam is real sg_tables. Userspace
      buffers import via `dma_buf_get(fd)`; CPU maps via `dma_buf_vmap_unlocked`
      into a `struct iosys_map`. See "Phase 2.A surface" below.
- [x] **2.B** SMMU/IOMMU: `arm_iommu_*` + `msm_dma_iommu_mapping.h` → generic
      `iommu_paging_domain_alloc`/`iommu_attach_device`/`iommu_map[_sg]`
      (`cam_smmu/cam_smmu_api.{c,h}`). ★★★★★ ☑ 2026-06-12 — `cam_smmu_api.o`
      compiles clean against 6.18; cam_core + cam_utils/cam_packet_util
      transitively unblocked. Per-cb explicit `iommu_domain` replaces the removed
      ARM `dma_iommu_mapping`; IO-region IOVA now driver-owned via a `gen_pool`
      (replaces downstream `msm_dma_map_sg_lazy`). Secure/stage-2 ION path shimmed
      at the 2.A/2.E seam (see ledger). See "Phase 2.B surface" below.
- [x] **2.C** Bus BW: `msm_bus_scale_*` → interconnect `icc_*`
      (`cam_cpas/cam_cpas_hw.{c,h}`). ★★★★ ☑ 2026-06-12 — `cam_cpas_hw.c`
      compiles clean. `struct msm_bus_scale_pdata *pdata` + `client_id`/`src`/`dst`
      replaced by a single `struct icc_path *icc_path` per bus client;
      register=`of_icc_get(dev, name)`, vote=`icc_set_bw(path, ab, ib)`,
      unregister=`icc_put`. NULL-tolerant: if the DT does not (yet) wire
      `interconnects`/`interconnect-names`, `of_icc_get` returns NULL/ERR_PTR and
      votes degrade to a safe no-op (bw/perf, not probe-blocking). Phase 3 DTS adds
      the paths. (`cam_utils/cam_soc_util.c` had no msm-bus use — only an unrelated
      `enum msm_bus_perf_setting` in a sensor header, harmless.) See surface below.
- [ ] **2.D** Sync: modernize downstream `cam_sync` against 6.18
      `dma_fence`/`completion` (`cam_sync/cam_sync.c`). ★★★★
      (No link-time change needed — cam_sync already compiles/links with the 2.G
      `.remove`/debugfs fixes; real dma_fence modernization is a Phase 4 streaming
      concern, not a build blocker.)
- [x] **2.E** SCM/secure buffer: `soc/qcom/scm.h` → `qcom_scm_*`. ★★★ ☑ 2026-06-12
      — both `scm_call2` call sites ported off `<soc/qcom/scm.h>`:
      `cam_ife_hw_mgr.c` (IFE safe-LUT SMMU toggle) and `cam_csiphy_core.c` (CSIPHY
      hyp secure-mode protect). Neither has a clean mainline `qcom_scm_*`
      equivalent (camera-specific TZ/hyp ops) and both are SECURE-path only — the
      mici non-secure data path never calls them — so both are documented no-op
      stubs (benign success) per the bring-up plan. The cam_smmu stage-2 ION shim
      (`cam_smmu_compat.h`) is retained (see ledger): it is the secure stage-2
      path's sole remaining consumer, also off the mici data path, and a real
      `qcom_scm_assign_mem` impl needs DTS-backed secure heaps (Phase 3+).
- [ ] **2.F** V4L2/media: verify subdev + media-dev + cam_req_mgr char-dev
      registration; **preserve exact `/dev` + subdev names**. ★★★
- [x] **2.G** Misc mechanical: core-types / moved-header / renamed-helper fixes.
      ☑ first slice (2026-06-12, "Phase 2.G surface" below): `struct timeval`→
      `timespec64` (incomplete-type in cam_hw_mgr_intf.h, the high-leverage one) +
      `get_monotonic_boottime[64]`→`ktime_get_boottime_ts64`; `strlcpy`→`strscpy`
      (29 sites); `writel_relaxed_no_log`→`writel_relaxed`; kref `atomic_read(
      &refcount.refcount)`→`kref_read`; `linux/clk/qcom.h`+`soc/qcom/socinfo.h`→
      local compat shim (`cam_compat_qcom.h`, see ledger); `VFL_TYPE_GRABBER`→
      `VFL_TYPE_VIDEO`, `debugfs_create_bool` void-return, `.remove` void-sig
      (cam_sync + cam_req_mgr only).
      ☑ **2.G-residual** (2026-06-12, "Phase 2.G-residual surface" below): legacy
      integer-GPIO/`of_gpio` migration in `cam_soc_util.c` +
      `cam_res_mgr.c` + `cam_sensor_util.c` (legacy-integer bridge, NOT gpiod —
      sensor power-sequencer drives GPIOs by number; see surface note); the
      mechanical quick-wins `timer_setup`/`timer_delete_sync` (cam_req_mgr_timer),
      `cpu_latency_qos_*` (cam_req_mgr_dev), `kfree_sensitive` (hfi.c),
      `dma-contiguous.h` drop (2 sensor io headers), `cam_dt_match` typo +
      pinctrl includes. cam_soc_util/res_mgr/sensor_util compile clean; leading
      edge now pure C (msm-bus) + E (scm) + the remaining `.remove`/i2c-probe
      void-sig drift + a CMA-API drift in cam_sensor_spi.c. ★★

## Phase 3 — Hardware bring-up (DTS + probe, bottom-up)  ☐

Exit: every `cam-*` block probes; node/subdev names match legacy capture.

- [x] **3.1** Port camera DTS into this repo's
      `kernel/dts/sdm845-comma-{common.dtsi,mici.dts}`: CPAS/CDM, CCI, 4×CSIPHY,
      CSID/VFE, ICP/A5, JPEG, FD, 4× cam-sensor, regulator + mclk pinctrl mapping
      (DESIGN.md §4). ☑ 2026-06-12 — full Spectra camera node block + 4 sensors +
      camera pinctrl states + GDSC/LDO regulator stand-ins added to
      `sdm845-comma-common.dtsi`; board mclk drive-strength override in
      `sdm845-comma-mici.dts`; upstream `&camss`/`&cci` disabled (reg-overlap).
      `./vamos build kernel` builds the mici .dtb clean (only benign
      `shared-gpios` dtc false-positives) → `build/boot.img` 16.8M. See
      "Phase 3.1 notes" below. (On-device probe = Phase 3.2/3.3, parent's flash.)
- [x] **3.2** `cam-cpas` + `cam_smmu` probe clean on device. ☑ 2026-06-13 — see
      "Phase 3 on-device bring-up log" below. cam-cpas binds after the AHB-level-
      vote + string-index fixes.
- [◐] **3.3** `cam-cci-driver` + 4× `cam-csiphy-driver` + 4× `cam-sensor-driver`
      probe; sensor chip-id read over CCI succeeds (expect 0x5304 family).
      ◐ 2026-06-13 — cam-cci-driver + 4× cam-sensor-driver + 3× cam-csiphy-driver
      now register (subdevs present). camerad (session 4) now reaches per-sensor
      bringup but each sensor fails `VIDIOC_CAM_CONTROL op_code 266 -> -ENODEV`
      (`sensor N FAILED bringup`) — sensor power-up / chip-id read over CCI is the
      live blocker. Pending: 4th csiphy; sensor power sequence + CCI i2c chip-id.
- [◐] **3.4** `cam-isp` (CSID/IFE) + `cam-req-mgr` + `cam_sync` register; by-path
      video nodes appear with correct names. ◐ 2026-06-13 — cam-req-mgr video node
      `platform-soc@0:qcom_cam-req-mgr-video-index0` present; cam-isp still blocked
      on VFE soc-enable -EBUSY (multi-domain GDSC / clock).
- [ ] **3.5** Verify `/dev/v4l/by-path/` + `/dev/v4l-subdev*` names == legacy
      capture exactly.

## Phase 4 — Streaming &amp; openpilot  ☐

Exit: camerad unmodified streams all 3 cameras; real snapshot JPEG.

- [ ] **4.1** Single RDI/IFE stream produces frames (memory path A end-to-end).
- [ ] **4.2** Full ISP→YUV pipeline (ICP/BPS) — the format camerad's VisionIPC
      consumers expect.
- [ ] **4.3** Run **unmodified** camerad: 3 cameras, incrementing hw-timestamped
      frame_ids.
- [ ] **4.4** `snapshot.py` → real YUV→RGB JPEG matching legacy proof images.
- [ ] **4.5** `test_onroad` camera checks pass; `dmesg` clean.

## Phase 5 — Land  ☐

- [ ] **5.1** Squash/order patches per naming convention; update README TODO
      (`cameras (OS04C10): kernel wiring / ISP / openpilot`).
- [ ] **5.2** Final review; commit to branch; update this tracker + memory.

---

## Stub / tech-debt ledger
(Stubs introduced in Phase 1 to reach a build; each must be retired in Phase 2.)

| Stub | File | Introduced | Retired by | Status |
|------|------|-----------|-----------|--------|
| `clk_set_flags()` no-op + `CLKFLAG_*` enum (downstream `linux/clk/qcom.h`; mainline CCF has no per-consumer flag setter — flags are idle RAM-retention power opt, safe to no-op) | `cam_utils/cam_compat_qcom.h` (replaces `#include <linux/clk/qcom.h>` in `cam_soc_util.h`) | 2026-06-12 (2.G) | Phase 3 HW bring-up (FD clk setup) | active |
| `socinfo_get_id()` / `socinfo_get_version()` → return 0 (downstream `soc/qcom/socinfo.h` not in mainline; only used for SoC-revision quirk decisions) | `cam_utils/cam_compat_qcom.h` (replaces `#include <soc/qcom/socinfo.h>` in `cam_soc_util.c` + `cam_icp/hfi.c`) | 2026-06-12 (2.G) | Phase 3 (wire real soc-rev lookup via qcom_socinfo/nvmem) | active |
| ION secure/stage-2 types + helpers: `ion_phys_addr_t`/`struct ion_client`/`struct ion_handle` aliases + `ion_import_dma_buf_fd()`/`ion_phys()`/`ion_free()` stubs (return `-ENODEV`/`NULL`). Keeps cam_smmu's PUBLIC `cam_smmu_map_stage2_iova()` signature byte-identical AND lets the secure path compile. Non-secure (mici) data path does NOT use these. **2.A update (2026-06-12):** cam_mem_mgr is now fully off ION and confirmed to never call these on the non-secure path (it passes `NULL` for the `struct ion_client *` arg of `cam_smmu_map_stage2_iova`, used only under `CAM_MEM_FLAG_PROTECTED_MODE`, which mici does not exercise). The shim is now referenced **only** by cam_smmu_api.c's stage-2 helpers — sole remaining owner is **2.E**. **2.E update (2026-06-12):** RETAINED. The secure stage-2 path has no clean mainline `qcom_scm_*` mapping without DTS-backed secure dma-heaps, and it is off the mici non-secure data path (never exercised), so the shim stays as a documented stub (returns `-ENODEV`/`NULL`). A real `qcom_scm_assign_mem`-based stage-2 impl is deferred to Phase 3+ (needs secure-heap DTS). | `cam_smmu/cam_smmu_compat.h` (replaces `#include <linux/msm_ion.h>` in `cam_smmu_api.h`; the stage-2 funcs in `cam_smmu_api.c` resolve against it) | 2026-06-12 (2.B) | Phase 3+ (`qcom_scm_assign_mem` + secure dma-heap DTS) | active |
| Secure SCM call sites → documented no-op stubs (benign success). Downstream `scm_call2`/`scm_desc` ops with no clean mainline `qcom_scm_*` equivalent: **(a)** IFE safe-LUT SMMU toggle (`TZ_SVC_SMMU_PROGRAM`/`TZ_SAFE_SYSCALL_ID`) in `cam_ife_notify_safe_lut_scm()`; **(b)** CSIPHY hyp secure-mode protect (`SCM_SVC_CAMERASS`/`SECURE_SYSCALL_ID_2`) in `cam_csiphy_notify_secure_mode()`. Both are SECURE-camera-path only; the mici non-secure data path never calls them. `<soc/qcom/scm.h>` include removed from both files. | `cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c`, `cam_sensor_module/cam_csiphy/cam_csiphy_core.c` | 2026-06-12 (2.E) | Phase 3+ (only if a secure camera path is ever needed on mici — likely never) | active |
| Camera-bus `icc_set_bw` votes degrade to a **safe no-op** when `of_icc_get` returns NULL/ERR_PTR (DT does not wire `interconnects`/`interconnect-names`). The msm-bus→interconnect port (2.C) is otherwise a real port. Bus bandwidth is a perf/power vote, not probe-blocking, so a missing-path no-op is safe for bring-up; AHB level votes also collapse to a single on/off bw (icc has no per-level usecase table). **3.1 update (2026-06-12): NOT retired — interconnects deliberately DEFERRED.** `cam_cpas_hw.c` requests the icc path with `of_icc_get(&pdev->dev, name)` where `name` comes from each axi-port child's `qcom,axi-port-name` (falling back to the inner `qcom,axi-port-mnoc`/`-camnoc` node *name*, which is the same string for all three ports), so a clean 1:1 map to distinct mainline icc paths (MASTER_CAMNOC_HF0/HF1/SF→EBI via `&mmss_noc`/`&mem_noc`) is not expressible via `interconnect-names` without driver changes (out of scope for DTS-only Phase 3.1). The legacy `qcom,axi-port-list` structure IS kept verbatim in the cam-cpas node (cam_cpas_hw.c hard-requires it for probe). Bandwidth voting stays a safe no-op; mainline camss proves the camera memory path works on this SoC without explicit camera interconnects. Retire in a later phase if streaming shows bandwidth starvation (would need a small driver tweak to read distinct `interconnect-names`). | `cam_cpas/cam_cpas_hw.{c,h}` (`cam_cpas_util_*_bus_client*`) | 2026-06-12 (2.C) | deferred (streaming-phase, needs driver tweak — see 3.1 notes) | active |
| QPNP camera-flash `qcom_flash_led_prepare()` → `-ENODEV` stub + `ENABLE/DISABLE_REGULATOR`/`QUERY_MAX_CURRENT` macros (downstream `linux/leds-qpnp-flash.h` + `CONFIG_LEDS_QPNP_FLASH` absent in mainline; downstream itself stubbed to `-ENODEV` when QPNP not built). mici has no camera flash LED — never exercised. | `cam_sensor_module/cam_flash/cam_flash_compat.h` (new; replaces `#include <linux/leds-qpnp-flash.h>` in `cam_flash_core.h`) | 2026-06-12 (mech) | Phase 3+ (only if a flash LED is wired — not on mici) | active |
| SPI/OIS bounce-buffer CMA (`dev_get_cma_area`+`cma_alloc`/`cma_release`, all removed/changed in 6.18) → plain contiguous `alloc_pages()`/`__free_pages()`. Functional superset of the old per-device CMA area. SPI sensor + OIS FW-download paths are off the mici I2C/CCI data path. | `cam_sensor_module/cam_sensor_io/cam_sensor_spi.{c,h}`, `cam_sensor_module/cam_ois/cam_ois_core.{c,h}` | 2026-06-12 (mech) | Phase 4+ (only if an SPI sensor / OIS is used — not on mici) | active |
| al6100 (Altek companion mini-ISP) subdir **excluded** from the build (`# obj-...al6100/` in `camera/Makefile`). Self-contained, unreferenced by any `cam_*` subdir, NOT on the mici Spectra (IFE/ICP) data path, and its `isp_camera_cmd.h` emits unbalanced `#pragma pack(1)` that leaks into `linux/fs.h` and trips the `struct filename` alignment `static_assert`. | `cam_sensor_module/.../camera/Makefile` | 2026-06-12 (mech) | never (not used on mici) | active |
| PROTECTED_MODE (secure) allocation in cam_mem_mgr falls back to the **non-secure** page exporter (no `qcom_scm_assign_mem` stage-2 hand-off). Allocation/import paths are otherwise real. The mici data path is non-secure and never sets `CAM_MEM_FLAG_PROTECTED_MODE`, so this is never hit; if a secure buffer is requested it will map as ordinary memory and the stage-2 smmu path (still shimmed, above) returns `-ENODEV`. | `cam_req_mgr/cam_mem_mgr.c` (`cam_mem_util_buffer_alloc`, secure branch of `cam_mem_util_map_hw_va`) | 2026-06-12 (2.A) | 2.E (qcom_scm secure assignment + a CMA/secure dma-heap) | active |
| `iommu_domain_set_attr()` non-fatal-fault / iova-guard tuning hints **dropped** (no generic mainline equivalent; `DOMAIN_ATTR_*` removed). They were power/debug tuning, not correctness. `non_fatal_fault`/`enable_iova_guard` struct fields now unused. | `cam_smmu/cam_smmu_api.c` (`cam_smmu_setup_cb`) | 2026-06-12 (2.B) | Phase 3 (re-add via mainline mechanism if a fault storm needs the non-fatal hint) | active |

## Phase 1 build error surface (first wave)

Captured from `build/spectra-build-1.log` (`./vamos build kernel`). The build
stops each subdir at its first failures, so this is the **leading edge**, not the
exhaustive list — more surfaces as each bucket is fixed. Maps to Phase 2 buckets:

- **A (memory/ion)** — `fatal: linux/ion.h: No such file` (cam_cdm_soc.c,
  cam_mem_mgr). ION removed in mainline → dma-buf heaps.
- **B (iommu)** — `fatal: asm/dma-iommu.h: No such file` (cam_smmu_api.{c,h},
  pulled in widely via cam_mem_mgr.h); `iommu.h: No such file` (3×). ARM-IOMMU
  API gone → generic `iommu_*`.
- **C/E (qcom soc glue)** — `fatal: linux/clk/qcom.h` (5×, cam_soc_util etc.),
  `soc/qcom/socinfo.h` (cam_soc_util.c). Map to 6.18 qcom clk/soc APIs.
- **F/G (core types & helpers)** — `field 'timestamp' has incomplete type`
  (cam_hw_mgr_intf.h:78 — a `struct timeval`/ktime-ish header issue blocking
  cam_core + cam_req_mgr broadly); `atomic_read` from incompatible pointer
  (kref/atomic type drift); `strlcpy` implicit (removed in 6.18 → `strscpy`);
  `writel_relaxed_no_log` implicit (downstream I/O macro → `writel_relaxed`).

33 `make Error` lines so far across cam_core, cam_req_mgr, cam_utils, cam_smmu.
The `field 'timestamp' has incomplete type` in cam_hw_mgr_intf.h is high-leverage
— it blocks a header included almost everywhere, so fixing it first will unmask
the real per-subsystem surface.

## Phase 2.G surface (after core-types / mechanical slice — 2026-06-12)

Captured from `build/spectra-build-phase2g.log`. The 2.G mechanical slice cleared
**all** the F/G core-types errors (timestamp incomplete-type, strlcpy, no_log I/O,
kref atomic_read, clk/qcom + socinfo headers, VFL_TYPE_GRABBER, void debugfs_create_bool,
`.remove` void-sig). `cam_core` + `cam_req_mgr` no longer have any errors of their
own — they now fail **only** because they transitively `#include cam_smmu_api.h`
(→ iommu) and `cam_mem_mgr.h` (→ ion). The leading edge is now the big buckets:

- **B (iommu)** — `fatal: asm/dma-iommu.h: No such file` (`cam_smmu/cam_smmu_api.{c,h}`).
  `cam_smmu_api.h` is pulled in widely, so this transitively blocks `cam_core`
  (`cam_context_utils.o`) and `cam_utils/cam_packet_util.o`. **This is now the
  single highest-leverage blocker** — same role timestamp had in wave 1.
- **A (memory/ion)** — `fatal: linux/msm_ion.h: No such file` (`cam_req_mgr/cam_mem_mgr.c`;
  note: legacy include is `linux/msm_ion.h`, not the `linux/ion.h` seen in wave 1).
  Transitively blocks `cam_req_mgr/cam_req_mgr_dev.o` via `cam_mem_mgr.h`.
- **G-residual (gpio, in `cam_utils/cam_soc_util.c`)** — legacy integer-GPIO API
  removed in 6.18: `struct gpio` incomplete, `of_gpio_count`/`of_get_gpio`
  implicit (from removed `linux/of_gpio.h`), `gpio_free_array` implicit. Needs the
  gpiod-descriptor migration (`devm_gpiod_get_array`/`gpiod_*`) — semantic, best
  validated on device, so handed to Phase 2.F/3 rather than blind-ported here.
  (The sibling pinctrl errors there were a pure missing-include and are fixed:
  `linux/pinctrl/consumer.h` added.)

18 `make Error` lines, now concentrated in cam_smmu (B), cam_req_mgr (A), and
cam_utils (gpio + the same A/B via cam_packet_util). Next subagent: start with
**2.B (iommu, `cam_smmu`)** — it unblocks cam_core + cam_utils transitively — then
**2.A (ion → dma-buf, `cam_mem_mgr`)**.

## Phase 2.B surface (after IOMMU/SMMU port — 2026-06-12)

Captured from `build/spectra-build-2b.log`. The 2.B port cleared **all** cam_smmu
errors — `cam_smmu_api.o` now compiles clean against 6.18 (`CC ... cam_smmu_api.o`,
zero errors). That transitively unblocked `cam_core` (no errors of its own now) and
`cam_utils/cam_packet_util`. The leading edge advanced PAST cam_smmu into the
remaining buckets. 68 `error:` lines, mapped:

- **A (ion/memory)** — `fatal: linux/ion.h` / `linux/msm_ion.h` / `linux/slub_def.h`
  No such file in: `cam_req_mgr/cam_mem_mgr.c`, `cam_req_mgr/cam_req_mgr_dev.c`,
  `cam_cdm/cam_cdm_soc.c`, `cam_isp/cam_isp_dev.c`,
  `cam_sensor_module/cam_cci/cam_cci_dev.h`. **Now the single highest-leverage
  blocker** (same role iommu had): cam_mem_mgr produces the sg_tables cam_smmu
  consumes. Next agent: **2.A**. NOTE seam: cam_smmu's secure stage-2 ION shim
  (`cam_smmu_compat.h`) should be retired here — `cam_smmu_map_stage2_iova()` will
  want real dma-buf-heap-backed secure buffers.
- **C (bus/interconnect)** — `fatal: linux/msm-bus.h` (`cam_cpas/cam_cpas_hw.c`).
  → interconnect `icc_*`.
- **E (scm/secure)** — `fatal: soc/qcom/scm.h`
  (`cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c`). → `qcom_scm_*`.
- **G-residual (gpio/pinctrl)** — 29 `struct gpio` incomplete + `of_gpio_count`/
  `of_get_gpio`/`gpio_free_array`/`devm_pinctrl_get`/`pinctrl_lookup_state`
  implicit, in `cam_utils/cam_soc_util.c` AND now also
  `cam_sensor_module/cam_res_mgr/cam_res_mgr.c` (+ `cam_res_mgr_api.h`). Same
  legacy integer-GPIO→gpiod migration noted in 2.G; still semantic / on-device.
- **F/G (.remove void-sig drift)** — 6 `incompatible pointer type` from
  `int (*)(struct platform_device *)` `.remove` assignments now that
  `platform_device.h` resolves: `cam_cpas/cam_cpas_intf.c`,
  `cam_isp/.../cam_ife_csid170.c`, `cam_ife_csid_lite170.c`, `cam_vfe170.c`,
  `cam_vfe_lite170.c`, `cam_sensor_module/cam_res_mgr/cam_res_mgr.c`. Mechanical
  (2.G already did this for cam_sync/cam_req_mgr/cam_smmu; these are the
  remaining drivers). Trivial `.remove` → void-returning fix.

Files changed in 2.B (all under `kernel/spectra-camera/camera/cam_smmu/`):
- `cam_smmu_api.c` — ARM-IOMMU mapping API → generic `iommu_domain`; IO-region
  gen_pool IOVA allocator replaces `msm_dma_map_sg_lazy`/`msm_dma_unmap_sg`; added
  `gfp` arg to `iommu_map`/`iommu_map_sg`; dropped `iommu_domain_set_attr`; added
  `linux/platform_device.h` include (lost when dead `asm/dma-iommu.h` removed);
  `.remove` → void.
- `cam_smmu_api.h` — dropped `asm/dma-iommu.h` + `linux/msm_ion.h`; include local
  `cam_smmu_compat.h`.
- `cam_smmu_compat.h` (new) — minimal ION type aliases + secure-path stubs for the
  2.A/2.E seam.

## Phase 2.A surface (after ion → dma-buf port — 2026-06-12)

Captured from `build/spectra-build-2a.log`. The 2.A port cleared **all** ION /
`msm_ion` / `slub_def` errors (0 remaining) plus the `struct kmem_cache`
incomplete-type it unmasked. The memory bucket is **gone from the leading edge**;
78 `error:` lines remain, none in the memory bucket. Mapped:

- **G-residual (gpio/pinctrl)** — ~45 errors, the dominant remaining mass:
  `invalid use of undefined type 'struct gpio'` (29), `sizeof` on `struct gpio`
  (6), `of_gpio_count`/`of_get_gpio`/`gpio_free_array`, `devm_pinctrl_get`/
  `devm_pinctrl_put`/`pinctrl_lookup_state`/`pinctrl_select_state` implicit, and
  `int`→pointer assigns for `struct pinctrl*`/`struct pinctrl_state*`/`struct gpio*`
  in `cam_utils/cam_soc_util.c` + `cam_sensor_module/cam_res_mgr/cam_res_mgr.c`
  (`cam_res_mgr_gpio_free_arry` conflicting type). Same legacy integer-GPIO →
  gpiod-descriptor migration flagged in 2.G/2.B; semantic, on-device validated.
- **G-mechanical (newly unmasked, small)** — `fatal: linux/dma-contiguous.h`
  (`cam_sensor_module/cam_sensor_io/cam_sensor_spi.h`, `cam_ois/cam_ois_core.h`;
  6.18 folded it into `linux/dma-map-ops.h` / it's gone — just drop or swap the
  include, CMA is pulled via dma-mapping); `setup_timer`/`del_timer_sync` removed
  (`cam_req_mgr/cam_req_mgr_timer.c` → `timer_setup()` + `timer_delete_sync()`);
  `pm_qos_add/remove/update_request` + `PM_QOS_CPU_DMA_LATENCY` renamed
  (`cam_req_mgr_dev.c` → `cpu_latency_qos_*`); `kzfree`→`kvfree`/`kfree_sensitive`;
  `cam_dt_match` undeclared at `MODULE_DEVICE_TABLE` (an `#ifdef CONFIG_OF`
  ordering issue in `cam_req_mgr_dev.c`, unmasked now the file compiles through).
- **F (.remove void-sig drift)** — 5 `int (*)(struct platform_device *)` `.remove`
  assignments: `cam_vfe170.c`, `cam_vfe_lite170.c`, `cam_csiphy/cam_csiphy_dev.c`,
  + the cam_cpas/csid ones from the 2.B list. Trivial → void-returning (same fix
  2.G/2.B applied elsewhere).
- **C (bus/interconnect)** — `fatal: linux/msm-bus.h` (`cam_cpas/cam_cpas_hw.c`).
  → interconnect `icc_*`.
- **E (scm/secure)** — `fatal: soc/qcom/scm.h` (`cam_csiphy/cam_csiphy_core.c`,
  `cam_isp/.../cam_ife_hw_mgr.c`). → `qcom_scm_*`; also owns the now-sole-referenced
  ION stage-2 shim in `cam_smmu_compat.h`.

Next agent: **2.G/2.F gpio+pinctrl gpiod migration** is now the single largest
mass and unblocks cam_utils + cam_sensor_module broadly; the small G-mechanical
items (dma-contiguous, timer, pm_qos) are quick wins; then **C** and **E**.

Files changed in 2.A:
- `cam_req_mgr/cam_mem_mgr.c` — full ION→dma-buf rewrite: added a page-backed
  `dma_buf` exporter (`cam_mem_mgr_buf_ops` + `cam_mem_util_buffer_alloc`) as the
  kernel-allocation backend (the in-kernel `dma_heap_*` consumer API is absent in
  this 6.18 baseline); `ion_map_kernel`→`dma_buf_vmap_unlocked`+`iosys_map`;
  `ion_import_dma_buf_fd`→`dma_buf_get(fd)`; `ion_share_dma_buf_fd`→
  `dma_buf_fd`; `ion_handle_get_size`→`dma_buf->size`; `msm_ion_do_cache_op`→
  `dma_buf_begin/end_cpu_access`; dropped `tbl.client`/ion_client. Public API
  (`cam_mem_get_cpu_buf`/`cam_mem_mgr_alloc_and_map`/`cam_mem_get_io_buf`/
  request/reserve/release + handle-table semantics) **unchanged**.
- `cam_req_mgr/cam_mem_mgr.h` — `struct ion_handle *i_hdl`→canonical `struct
  dma_buf *dma_buf`; added `struct iosys_map kmap` for vmap teardown; dropped
  `struct ion_client *client` from `cam_mem_table`; `+#include <linux/iosys-map.h>`.
- `cam_req_mgr/cam_req_mgr_dev.c` — dropped `linux/slub_def.h` (internal SLUB
  header, gone; was for unused `ksize`); fixed `struct kmem_cache ->name` private
  deref it unmasked (opaque in mainline) → literal string.
- Removed stray (unused) `#include <linux/ion.h>` from 12 files: `cam_cdm/cam_cdm_soc.c`,
  `cam_cdm/cam_cdm_intf.c`, `cam_cdm/cam_cdm_core_common.c`, `cam_cdm/cam_cdm_hw_core.c`,
  `cam_cdm/cam_cdm_virtual_core.c`, `cam_isp/cam_isp_dev.c`, `cam_jpeg/cam_jpeg_dev.c`,
  `cam_sensor_module/cam_cci/cam_cci_dev.h`, `cam_sensor_module/cam_sensor/cam_sensor_dev.h`,
  `cam_sensor_module/cam_actuator/cam_actuator_dev.h`,
  `cam_sensor_module/cam_csiphy/cam_csiphy_dev.h`,
  `cam_sensor_module/cam_csiphy/cam_csiphy_soc.h`.

## Phase 2.G-residual surface (after gpio + mechanical quick-wins — 2026-06-12)

Captured from `build/spectra-build-2gr.log`. The 2.G-residual slice cleared **all**
the GPIO (`struct gpio`/`of_gpio_count`/`of_get_gpio`/`gpio_free_array`), pinctrl,
timer, pm_qos, kzfree, dma-contiguous, and `cam_dt_match` errors — `cam_soc_util.c`,
`cam_res_mgr.c`, `cam_sensor_util.c`, `cam_req_mgr_timer.c`, `cam_req_mgr_dev.c`,
`hfi.c` and the 2 sensor-io headers no longer have any errors of their own. The
gpio bucket (the dominant ~45-error mass in the 2.A surface) is **gone from the
leading edge**. 26 `error:`/`fatal` lines remain, all in C/E/F:

- **E (scm/secure)** — `fatal: soc/qcom/scm.h` (`cam_isp/.../cam_ife_hw_mgr.c`,
  `cam_csiphy/cam_csiphy_core.c`). → `qcom_scm_*`; also owns the ION stage-2 shim.
- **C (bus/interconnect)** — `fatal: linux/msm-bus.h` (`cam_cpas/cam_cpas_hw.c`).
  → interconnect `icc_*`.
- **F/G (.remove + i2c-probe void-sig drift)** — ~16 `incompatible pointer type`:
  `int (*)(struct platform_device *)` `.remove` in cam_cdm_intf, cam_cpas_intf,
  cam_isp_dev, cam_ife_csid170, cam_ife_csid_lite170, cam_vfe170, cam_vfe_lite170,
  cam_cci_dev, cam_actuator_dev, cam_csiphy_dev, cam_sensor_dev; PLUS the I2C
  driver `.probe`/`.remove` sig change (6.18 dropped the `i2c_device_id *` arg from
  `.probe` and made `.remove` void) in cam_actuator_dev + cam_sensor_dev. Trivial
  mechanical — same fix 2.G/2.B applied to cam_sync/cam_req_mgr/cam_smmu/cam_res_mgr,
  now the remaining drivers.
- **C-residual (CMA API drift, newly unmasked in `cam_sensor_io/cam_sensor_spi.c`)**
  — dropping `dma-contiguous.h` unmasked real use of `dev_get_cma_area()` (removed)
  + `cma_alloc()`/`cma_release()` whose signatures changed (`cma_alloc` now takes 4
  args incl. gfp; both take `struct cma *` not the old area). This is the **SPI
  sensor** bounce-buffer path; mici sensors are I2C/CCI so it is **not on the data
  path** — can be stubbed/ported lazily. (Note: the `debugfs_create_bool` void-return
  in cam_icp_hw_mgr.c — same fix 2.G did for cam_sync — will resurface once C/E clear
  the subdir-build past cam_isp; it's a latent mechanical item, not yet reached.)

GPIO approach (decision): used the **legacy single-integer GPIO bridge**, not the
gpiod-descriptor API. Rationale: the Spectra sensor power-sequencer
(`cam_sensor_util.c` `cam_sensor_core_power_*`) references GPIOs **by number**,
indexing into DT-parsed integer tables (`cam_gpio_common_tbl[idx].gpio`) and driving
reset/standby/avdd lines via `cam_res_mgr_gpio_set_value(gpio_number, value)` and
`gpio_request_one(gpio_number, ...)`. 6.18 keeps the full single-integer API
(`gpio_request[_one]`/`gpio_free`/`gpio_set_value[_cansleep]`/`gpio_direction_*`) and
`of_get_named_gpio()`; only `struct gpio`, `gpio_request_array`/`gpio_free_array`,
`of_gpio_count`/`of_get_gpio` were removed. So: (1) `struct gpio` → a local
`struct cam_gpio` with the identical `{gpio, flags, label}` fields (in cam_soc_util.h),
keeping the table layout and every `.gpio/.flags/.label` accessor byte-identical;
(2) `of_gpio_count` → `of_count_phandle_with_args(np,"gpios","#gpio-cells")` and
`of_get_gpio(np,i)` → `of_get_named_gpio(np,"gpios",i)` (exactly what the removed
wrappers expanded to — same numbers, same order); (3) `gpio_free_array(tbl,n)` →
per-entry `gpio_free()` loop (mirrors the existing per-entry `gpio_request_one`
request side). The request/free lifecycle and power on/off ordering are unchanged —
no semantic change to the power-sequencer, which is exactly what Phase 3 sensor probe
needs. Switching to gpiod descriptors would have required rewriting the by-number
sequencer tables and the shared-gpio refcount logic in cam_res_mgr (which keys its
shared-gpio list on the integer number from `shared-gpios = <...>`), a much larger
and riskier change for zero benefit here.

Files changed in 2.G-residual (all under `kernel/spectra-camera/camera/`):
- `cam_utils/cam_soc_util.h` — new `struct cam_gpio` (replaces removed `struct gpio`);
  `cam_soc_gpio_data` tables retyped to it.
- `cam_utils/cam_soc_util.c` — `of_gpio_count`→`of_count_phandle_with_args`,
  `of_get_gpio`→`of_get_named_gpio`, `sizeof(struct gpio)`→`sizeof(struct cam_gpio)`,
  `gpio_free_array`→per-entry `gpio_free` loop.
- `cam_sensor_module/cam_res_mgr/cam_res_mgr_api.h` — `cam_res_mgr_gpio_free_arry`
  param `const struct gpio *`→`const struct cam_gpio *` + forward-decl.
- `cam_sensor_module/cam_res_mgr/cam_res_mgr.c` — matching def retype; `.remove`→void;
  explicit `<linux/pinctrl/consumer.h>`.
- `cam_sensor_module/cam_sensor_utils/cam_sensor_util.c` — `struct gpio *gpio_tbl`→
  `struct cam_gpio *`; explicit `<linux/gpio.h>` + `<linux/pinctrl/consumer.h>`.
- `cam_req_mgr/cam_req_mgr_timer.c` — `setup_timer`→`timer_setup` via a trampoline
  (`crm_timer_trampoline` + `timer_container_of`) preserving the stored
  `timer_cb(unsigned long)` ABI for all callers; `del_timer_sync`→`timer_delete_sync`.
- `cam_req_mgr/cam_req_mgr_dev.c` — `pm_qos_*_request`+`PM_QOS_CPU_DMA_LATENCY`→
  `cpu_latency_qos_*`; `MODULE_DEVICE_TABLE(of, cam_dt_match)` typo→`cam_req_mgr_dt_match`.
- `cam_icp/hfi.c` — `kzfree`→`kfree_sensitive`.
- `cam_sensor_module/cam_ois/cam_ois_core.h`,
  `cam_sensor_module/cam_sensor_io/cam_sensor_spi.h` — dropped removed
  `<linux/dma-contiguous.h>` (unused; CMA folded into dma-map-ops).

## Phase 2.C/E + build-links surface (the build now LINKS — 2026-06-12)

Captured from `build/spectra-build-2ce.log`. This slice took the tree from the
last leading edge (C + E + trailing mechanical) all the way to a **clean compile
+ link**: `./vamos build kernel` produces
`build/boot.img` (16.8M) with `CONFIG_SPECTRA_CAMERA=y` built into `vmlinux`
(`AR drivers/media/platform/msm/camera/built-in.a` → linked; `LD vmlinux`,
`OBJCOPY arch/arm64/boot/Image`, `-- Done! boot.img --`). Order errors fell as:

1. **C (interconnect)** — `cam_cpas_hw.{c,h}` ported msm-bus→icc (real port,
   NULL-tolerant safe-degrade; see 2.C + ledger).
2. **E (scm)** — `cam_ife_hw_mgr.c` + `cam_csiphy_core.c` scm_call2 → documented
   secure-path no-op stubs; `<soc/qcom/scm.h>` dropped (see 2.E + ledger).
3. **Trailing mechanical** (subagent + this slice): ~24 platform_driver `.remove`
   int→void; i2c `.probe` drop-id + `.remove` void (cam_actuator/cam_sensor/
   cam_eeprom/cam_flash/cam_ois + al6100); spi `.remove` void; `debugfs_create_u32`
   /`debugfs_create_bool` void-return (`cam_ife_hw_mgr.c`, `cam_lrme_hw_mgr.c`,
   `cam_icp_hw_mgr.c`); SPI/OIS CMA→`alloc_pages` (ledger); QPNP-flash compat
   header (ledger).
4. **Newly unmasked once the tree compiled through:**
   - `linux/leds-qpnp-flash.h` missing (`cam_flash_core.h`) → `cam_flash_compat.h`
     stub (ledger).
   - `vfree`/`vzalloc` implicit (`cam_eeprom_core.c`, `cam_eeprom_soc.c`) → add
     `<linux/vmalloc.h>`.
   - al6100 `#pragma pack(1)` leaking into `linux/fs.h` `struct filename`
     static_assert → al6100 subdir excluded from build (ledger).
   - **modpost link error**: `EXPORT_SYMBOL` on `static` (TU-local) symbols
     `cam_ipe_hw_info`/`cam_bps_hw_info`/`cam_jpeg_dma_hw_info` — 6.18 modpost
     hard-errors on exporting a local symbol; driver is built-in so the exports
     were spurious → dropped. This was the **final** blocker before the link.

Files changed in 2.C/E + build-links (all under `kernel/spectra-camera/`):
- `cam_cpas/cam_cpas_hw.h` — `struct cam_cpas_bus_client`: msm-bus pdata/client_id/
  src/dst → single `struct icc_path *icc_path`; `+#include <linux/interconnect.h>`.
- `cam_cpas/cam_cpas_hw.c` — register/vote/unregister bus-client funcs ported to
  `of_icc_get`/`icc_set_bw`/`icc_put`; dropped pdata usecase toggling; fixed
  CAM_DBG references to removed fields; `<linux/msm-bus.h>`→`<linux/interconnect.h>`.
- `cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c` — `cam_ife_notify_safe_lut_scm` scm_call2
  → no-op stub; dropped `<soc/qcom/scm.h>`; `debugfs_create_u32` void fix.
- `cam_sensor_module/cam_csiphy/cam_csiphy_core.c` — `cam_csiphy_notify_secure_mode`
  scm_call2 → no-op stub; dropped `<soc/qcom/scm.h>`.
- `cam_sensor_module/cam_sensor_io/cam_sensor_spi.{c,h}`,
  `cam_sensor_module/cam_ois/cam_ois_core.{c,h}` — CMA→`alloc_pages`.
- `cam_sensor_module/cam_flash/cam_flash_compat.h` (new) +
  `cam_sensor_module/cam_flash/cam_flash_core.h` — QPNP-flash stub.
- `cam_sensor_module/cam_eeprom/cam_eeprom_{core,soc}.c` — `+<linux/vmalloc.h>`.
- `cam_lrme/lrme_hw_mgr/cam_lrme_hw_mgr.c`,
  `cam_icp/icp_hw/icp_hw_mgr/cam_icp_hw_mgr.c` — void-return debugfs fixes.
- `cam_icp/icp_hw/ipe_hw/ipe_dev.c`, `cam_icp/icp_hw/bps_hw/bps_dev.c`,
  `cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_dev.c` — dropped spurious EXPORT_SYMBOL.
- `camera/Makefile` — al6100 subdir commented out.
- ~24 `.remove` int→void + i2c/spi probe/remove sig fixes across cam_lrme,
  cam_jpeg, cam_isp (cam_ife_csid_dev/cam_vfe_dev), cam_cpas_intf, cam_icp_subdev,
  cam_cdm, cam_fd, cam_sensor_module (cci/csiphy/actuator/sensor/eeprom/flash/ois),
  al6100 (intf_i2c/intf_spi).

## Phase 3.1 notes (camera DTS port — 2026-06-12)

Captured from `build/spectra-build-dts.log`. The full Spectra camera device-tree
landed in this repo's DTS; `./vamos build kernel` builds the mici .dtb with no dtc
errors and produces `build/boot.img` (16.8M). Confirmed via `strings` on the
compiled `sdm845-comma-mici.dtb`: `qcom,cam-cpas@ac40000`, 8× `msm-cam-smmu-cb`,
`qcom,cci@ac4a000`, 4× `csiphy@ac65/66/67/68000`, `csid0/csid1` (`csid170`),
`csid-lite170`, `vfe170`×2 + `vfe-lite170`, `cam-a5@ac00000` + ipe/bps, jpeg-enc,
`fd@ac5a000`, `cam-res-mgr`, and all 4 `qcom,cam-sensor@0..3` present; upstream
`camss@acb3000` + `cci@ac4a000` remain in the dtb as `status="disabled"`.

**DTS chunks added (all in `kernel/dts/`):**
- `common.dtsi` root `/`: 6 GDSC fixed-regulator stand-ins (`titan_top_gdsc`,
  `ife_0_gdsc`, `ife_1_gdsc`, `ipe_0_gdsc`, `ipe_1_gdsc`, `bps_gdsc`,
  always-on/boot-on) + 4 per-sensor LDOs (`camera_rear_ldo`/`camera_ldo` on
  pm8998 gpio 12/9, `camera_vana_ldo`/`camera_vdig_ldo`). *Reason:* Spectra
  `cam_soc_util.c` acquires power via `regulator_get()` (not genpd); the legacy
  gdsc/ldo regulator labels must exist as regulators.
- `common.dtsi` `&camss { status="disabled"; }` + `&cci { status="disabled"; }`.
  *Reason:* mainline qcom-camss + cci claim the SAME reg regions as the Spectra
  cam-isp/CSID/CSIPHY/VFE/CCI nodes (proof inline in the DTS comment): csid0
  0xacb3000, csid1 0xacba000, csid2/lite 0xacc8000, csiphy0-3 0xac65-68000, vfe0
  0xacaf000, vfe1 0xacb6000, vfe_lite 0xacc4000, cci 0xac4a000 — all duplicated.
  Mainline has no separate ispif/cdm/cpas/icp/jpeg/fd nodes (they live only inside
  camss@acb3000), so disabling camss clears every overlap; no other reg-overlap
  node (no `ispif`) exists.
- `common.dtsi` `&tlmm`: 20 camera pinctrl states (cci0/1 active+suspend,
  cam_sensor_mclk0-3 active+suspend, sensor reset/vana rear/front/iris
  active+suspend, cam_res_mgr active+suspend) in the **mainline flat -state
  binding** (legacy used nested mux{}/config{}). `cam_mclk`/`cci_i2c`/`gpio` pin
  functions all exist in pinctrl-sdm845.c for these pins.
- `common.dtsi` `&soc`: the full Spectra HW node block (cam-req-mgr, 4×csiphy,
  cci, cam_smmu w/ 7 cbs, cam-cpas, cam-cdm-intf, cpas-cdm0, cam-isp, csid0/1 +
  vfe0/1 + csid-lite/vfe-lite, cam-icp + a5/ipe0/ipe1/bps, cam-jpeg + enc/dma,
  cam-fd) — ported from legacy `sdm845-camera.dtsi`.
- `common.dtsi` `&cam_cci`: cam-res-mgr + 4× `qcom,cam-sensor@0..3`
  (phy/cci-master (0,0)(1,0)(2,1)(3,1); roll 180/180/180/270 — matches legacy
  exactly) — ported from legacy `sdm845-camera-sensor-mtp.dtsi`.
- `mici.dts`: board mclk0/1/2 active drive-strength → 2 mA (legacy comma_mici.dts
  override; mclk3 already 2 mA in common).

**Transforms applied (legacy 4.9 → mainline 6.18 DTS):**
- `&clock_gcc` → `&gcc` (mainline GCC label).
- `&clock_camcc` kept — the mainline camcc label IS `clock_camcc`
  (`clock_controller@ad00000`, `qcom,sdm845-camcc`, `#clock-cells=1`, named
  CAM_CC_* via `dt-bindings/clock/qcom,camcc-sdm845.h`). The legacy
  `clocks=<&clock_camcc CAM_CC_*>` phandle lists are **drop-in**.
- `interrupts = <0 N 0>` → `<GIC_SPI N IRQ_TYPE_EDGE_RISING>` (matches the
  mainline camss encoding for the identical SPIs, e.g. csid0 SPI 464).
- All `reg`/multi-cell addrs widened to the soc@0 4-cell form `<0 0xADDR 0 0xLEN>`
  (mainline soc is `#address-cells=2 #size-cells=2`; legacy soc was 1/1).
- `mipi-csi-vdd-supply = <&pm8998_l1>` → `<&vreg_l1a_0p875>`.
- `memory-region = <&pil_camera_mem>` → `<&camera_mem>` (mainline 5MB `no-map`
  region @0x8bf00000 — exact size match for the ICP firmware region).
- jpeg-dma node name `@0xac52000` → `@ac52000` (dtc: unit-addr no `0x`).
- CPAS `vdd-corners`/`vdd-corner-ahb-mapping` **dropped** (the
  `RPMH_REGULATOR_LEVEL_*` corner macros are downstream-only, absent in mainline
  `qcom,rpmh-regulator.h`; the property is optional in `cam_cpas_soc.c`, parsed
  only when count>0 — a no-op AHB perf hint on the fixed-regulator gdsc).
- `qcom,msm-bus,*` per-port AXI vectors dropped (2.C uses icc now); the
  `qcom,axi-port-list` *structure* (3 ports × mnoc/camnoc children) is kept
  verbatim — cam_cpas_hw.c requires it for probe.

**camcc clock decision — CCF mapping (option b), NO syscon.** Evidence from
`cam_utils/cam_soc_util.c`: the driver acquires every clock via
`clk_get(dev, clock-name)` (cam_soc_util_request_platform_resource), sets rates
via `clk_set_rate()`/`clk_round_rate()` (cam_soc_util_set_clk_rate), and enables
via `clk_prepare_enable()` — pure standard Linux clk framework. **Zero** syscon /
regmap / camcc-register ioremap anywhere (the only ioremap is the device's own
reg-names blocks). The `clock-rates`/`clock-cntl-level` DT props are driver-side
OPP/perf tables, not register maps. So the mainline CCF `qcom,sdm845-camcc`
exposing named CAM_CC_* clocks via `<&clock_camcc CAM_CC_*>` is drop-in — the
legacy clocks lines port unchanged. No syscon view of camcc @ad00000 is needed.

**Regulator mapping table:**

| Legacy supply (role) | Legacy phandle | vamOS mapping |
|---|---|---|
| cam_vio (all sensors) | `&pm8998_lvs1` | `&vreg_lvs1a_1p8` |
| cam_vana (sensors 0-2) | `&pmi8998_bob` | `&vreg_bob` |
| cam_vana (sensor 3) | `&camera_vana_ldo` | new fixed `camera_vana_ldo` (2.85V) |
| cam_vdig (sensors 0-2) | `&camera_rear_ldo` | new fixed `camera_rear_ldo` (pm8998 gpio12, 1.05V) |
| cam_vdig (sensor 3) | `&camera_ldo` | new fixed `camera_ldo` (pm8998 gpio9, 1.05V) |
| cam_clk / gdscr / camss(-vdd) | `&titan_top_gdsc` | new fixed `titan_top_gdsc` (always-on) |
| ife0 | `&ife_0_gdsc` | new fixed `ife_0_gdsc` (always-on) |
| ife1 | `&ife_1_gdsc` | new fixed `ife_1_gdsc` (always-on) |
| ipe0-vdd | `&ipe_0_gdsc` | new fixed `ipe_0_gdsc` (always-on) |
| ipe1-vdd | `&ipe_1_gdsc` | new fixed `ipe_1_gdsc` (always-on) |
| bps-vdd | `&bps_gdsc` | new fixed `bps_gdsc` (always-on) |
| mipi-csi-vdd (csiphy) | `&pm8998_l1` | `&vreg_l1a_0p875` |

**GDSC handling (the riskiest seam) — fixed-regulator stand-ins + one genpd
attach.** In mainline the camera GDSC registers (0xad06004..0xad0b134) are owned
by `&clock_camcc`, which exposes them as genpd power-domains
(`#power-domain-cells=1`, TITAN_TOP_GDSC/IFE_0_GDSC/…). The Spectra driver has no
genpd/pm_runtime path — it `regulator_get()`s the gdsc names. So each gdsc name is
backed by an **always-on fixed regulator** to satisfy `regulator_get()`. To make
the Titan top rail genuinely power on (not just satisfy the API), the cam-cpas
node ALSO gets `power-domains = <&clock_camcc TITAN_TOP_GDSC>` — the driver-core
single-domain auto-attach (`dev_pm_domain_attach`) powers it at probe.
**Known gap to watch on-device:** genpd auto-attach handles only ONE domain per
device, so the multi-domain nodes (csid/vfe need titan+ife, ipe/bps need their own
gdsc) get only their fixed-regulator stand-in, not a genpd attach — they rely on
TITAN_TOP being on + the camcc clock branches the driver enables bringing the
sub-domains up through CCF. If a sub-block (IFE/IPE/BPS) reads back zeros / times
out at probe, the fix is to add `power-domains = <&clock_camcc IFE_0_GDSC>` etc.
to that node (single-domain) or teach the driver to runtime-get multiple domains.
This is the one item that can only be fully validated on-device.

**interconnects: DEFERRED (not added).** See the updated icc ledger row — the
driver's `of_icc_get` lookup name (per-axi-port, falling back to a shared inner
node name) doesn't map 1:1 to distinct mainline icc paths without a driver tweak;
the `qcom,axi-port-list` structure is kept (probe needs it) and bw votes stay a
safe no-op. The 2.C icc no-op is therefore NOT retired by 3.1.

**What to watch in dmesg on first flash (expected probe order, bottom-up):**
1. `cam_smmu` / `cam-cpas` (`qcom,msm-cam-smmu`, `qcom,cam-cpas`) — the foundation;
   cpas reads `qcom,cpas-hw-ver = 0x170100` (Titan v170). Watch for the TITAN_TOP
   genpd attach + camnoc reg reads (no `-ENXIO`).
2. `cam-cci-driver` (`qcom,cci@ac4a000`) — CCI controller + the two i2c masters.
3. 4× `cam-csiphy-driver` (`csiphy@ac65/66/67/68000`).
4. 4× `cam-sensor-driver` (`qcom,cam-sensor@0..3`) — then the sensor **chip-id read
   over CCI** (expect `0x5304` family). This is the key bring-up signal.
5. `cam-isp` (csid0/1 + vfe0/1 + lite), `cam-icp`/a5 (firmware
   `CAMERA_ICP.elf` from `camera_mem`), jpeg, fd register their subdevs.
Red flags: `regulator ... get failed` (a gdsc/ldo name typo), `-EPROBE_DEFER`
storms (clock/genpd ordering — camcc must probe before camera), reg read-back
zeros on IFE/IPE/BPS (the multi-domain genpd gap above), or CCI i2c NAK on the
sensor chip-id read (sensor power rail / mclk / reset GPIO).

## Phase 3 on-device bring-up log (2026-06-13)

Flashed boot.img to mici (QDL) and iterated on dmesg. Ordered fixes that took the
subdev count 0 → 10 of the legacy 15 (all committed):

1. **IRQ** — `platform_get_resource_byname(IORESOURCE_IRQ)` → `platform_get_irq_byname`
   (6.18 drops IORESOURCE_IRQ for DT); `irq_line` `struct resource*` → `int`.
   Cleared every "no irq resource" probe abort.
2. **Deferred probe** — `cam_register_subdev` returns `-EPROBE_DEFER` (not -ENODEV)
   when the camera root device isn't up, so HW subdevs retry.
3. **device_caps** — `cam_video_device_setup` set `vdev->device_caps`
   (mandatory since ~5.0). Was the keystone: without it `cam_req_mgr_probe`
   faulted at `video_register_device`, `g_dev.state` never went true, everything
   deferred forever. Found via `faddr2line` on the probe oops.
4. **camcc** — `CONFIG_SDM_CAMCC_845=m` → `=y` (no module loading in bring-up;
   camcc must be built-in or all camera clocks/sensors block on
   "ad00000.clock-controller not ready").
5. **subdev UAF** — csiphy + cci freed their `cam_subdev` on the CPAS-not-ready
   error path WITHOUT `cam_unregister_subdev`, leaving a freed node on
   `v4l2_dev->subdevs` → panic in `__v4l2_device_register_subdev_nodes`.
   Unregister before free.
6. **string-index** — `cam_common_util_get_string_index` `strnstr` substring match
   failed on exact tokens → CPAS CAMNOC regbase lookup -EINVAL. → `strcmp`.
7. **AHB level vote** — `cam_cpas_util_vote_bus_client_level` kept the legacy
   `level < num_usecases` gate; the icc port has no usecase table, so
   CAM_SVS_VOTE failed -EINVAL and aborted CPAS probe. Dropped the gate.

State after #7: cam-req-mgr video node + 10 subdevs (cam-icp, cam-cpas,
cam-cci-driver, 3× cam-csiphy-driver, 4× cam-sensor-driver). **Remaining: 4th
csiphy; cam-isp blocked on `cam_vfe_enable_soc_resources` -EBUSY (-16) — VFE
soc/clock/GDSC enable (suspect multi-domain genpd, DESIGN §4 GDSC gap);
jpeg/fd/lrme HW-manager init -19 (depend on child HW).**

Debug aids (REMOVED 2026-06-13): `debug_mdl` default back to 0 in
`cam_debug_util.c` (runtime-tunable via sysfs); `log_buf_len=8M` dropped from the
`build_kernel.sh` cmdline. No `vamos-dbg` instrumentation prints remain.

## Phase 4 camerad integration log (2026-06-13)

Ran the **unmodified** legacy-rootfs `camerad` against the mainline kernel
(legacy AGNOS system.img + openpilot still on /data; only boot.img is mainline).
Iterated past each `SpectraMaster::init()` assertion:

- **video0 (`assert video0_fd>=0`)** — camerad opens by-path
  `platform-soc:qcom_cam-req-mgr-video-index0`; mainline names the bus node
  `soc@0`. Fixed with `device_rename(&pdev->dev, "soc:qcom,cam-req-mgr")` in
  cam_req_mgr probe (openpilot untouched).
- **cam_sync (`assert cam_sync_fd>=0`)** — cam_sync_probe faulted at
  video_register_device (device_caps unset). Set device_caps.
- **cam-isp** — now a subdev (deferred-probe + rc-preservation fixes, Phase 3
  log). 12th subdev.
- **cam-icp (`assert icp_fd>=0`)** — cam-icp HW mgr left a5_dev_intf NULL because
  a5/ipe/bps weren't probed yet; deferred cam-icp until CPAS + all children ready.
  camerad now gets PAST icp.

State: camerad initializes the **entire Spectra stack** (no init asserts) and
reaches **sensor acquisition**; ICP/BPS CPAS streamon seen. **Current blocker:**
`cam_sensor_subdev_ioctl: Invalid ioctl cmd: -2140645888` — camerad's control
ioctl to the sensor subdev isn't accepted (suspect 32/64-bit ioctl/compat or a
cam_control opcode mismatch); an RCU stall appears ~24s, likely camerad spinning
on it. Next: sensor (and csiphy) subdev ioctl handling → sensor chip-id 0x5304
read → frames.

15 subdevs target; currently 12 (+cam-jpeg/fd/lrme still pending, secondary).

## Phase 4 camerad integration log — session 2 (2026-06-13)

**Correction to session-1 "blocker":** the `cam_sensor_subdev_ioctl: Invalid ioctl
cmd: -2140645888` is **NOT camerad** and **not** the blocker. `-2140645888` =
`0x80685600` = `VIDIOC_QUERYCAP` (`_IOR('V',0,struct v4l2_capability[104])`), issued
by **udev's `v4l_id`** coldplug-probing every `/dev/v4l-subdev*`. camerad uses the
correct `VIDIOC_CAM_CONTROL` (`0xc06856c0`). Harmless noise; the subdevs just don't
implement QUERYCAP. The real camerad path stops much later, in ICP.

camerad's actual init wall: `SpectraMaster::init()` does
`icp_fd = open("cam-icp"); assert(icp_fd>=0)` — **opening cam-icp triggers
`cam_icp_subdev_open` → ICP A5 firmware download**. So ICP FW bring-up is on the
**critical path for ALL cameras**, even the 2 IFE-processed ones (road/wide) — camerad
opens+QUERYCAPs cam-icp unconditionally at init before any per-camera config.

**Four fixes landed this session (all on the camerad critical path):**
1. **BPS/IPE GDSC `-EBUSY`** — `cam_bps/ipe enable_soc_resources` failed
   `*_ahb_clk enable rc(-16)` because the multi-domain genpd gap (DESIGN §4) left
   BPS/IPE_0/IPE_1 GDSCs OFF (only a fixed-regulator stand-in, no genpd attach).
   Added `power-domains = <&clock_camcc BPS_GDSC/IPE_0_GDSC/IPE_1_GDSC>` to the
   `cam_bps`/`cam_ipe0`/`cam_ipe1` DTS nodes (single-domain auto-attach). Cleared.
2. **cam-req-mgr by-path symlink** (the real `video0_fd>=0` fix, replacing the
   session-1 `device_rename` which was racy). The DT cam-req-mgr device was nested
   under `soc@0`, so udev's `path_id` emitted an EMPTY `ID_PATH` → no by-path link →
   camerad open fails. Fix in `cam_req_mgr_probe`:
   `device_move(&pdev->dev, &platform_bus, DPM_ORDER_NONE)` to flatten to
   `/devices/platform/`, then `device_rename(..., "soc:qcom_cam-req-mgr")` —
   **UNDERSCORE not comma** (udev SYMLINK uses `$env{ID_PATH}` verbatim; comma would
   yield the wrong link string). Verified `platform-soc:qcom_cam-req-mgr-video-index0`
   is created deterministically; `udevadm test-builtin path_id` confirms the ID_PATH.
   (cam_sync's link always worked because it's a static early `platform_device`.)
   NOTE: `pkill -9 camerad` leaves `g_dev.cam_lock`/`open_cnt` held → next video0
   open hangs uninterruptibly; **reboot between dirty runs** (not a real bug).
3. **HFI `cam_hfi_disable_cpu` NULL-deref crash** — on the ICP FW-init error-unwind
   (`hfi_init_failed:`), `cam_hfi_disable_cpu` ran before `cam_hfi_init` allocated
   `g_hfi`, so `g_hfi->csr_base` was a NULL deref that **oopsed the kernel and took
   the whole device down** on every ICP failure. `g_hfi->csr_base` is always
   `icp_base` anyway → use the passed-in `icp_base`. Now the device SURVIVES ICP
   failures (essential for any further debug).
4. **BPS/IPE `regulator_set_mode` on fixed-regulator** — `cam_bps/ipe_get/transfer_
   gdsc_control` call `regulator_set_mode(rgltr, NORMAL/FAST)` on the `bps-vdd`/
   `ipe*-vdd` gdsc stand-ins; fixed-regulator has no `.set_mode` → `-EINVAL` →
   `Regulator set mode failed` → BPS/IPE reset aborted (`BPS CDM/top rst failed
   status 0x0`). Made the 4 set_mode sites treat `-EINVAL/-ENOSYS/-EOPNOTSUPP` as
   benign (mode is an RPMh perf hint; genpd does the real power-gating). BPS/IPE now
   power + reset cleanly.

**Current blocker (next):** **`watch dog interrupt from A5`** → `hfi not set up yet`
→ `FW download failed`. `CAMERA_ICP.elf` (1.1M) loads with NO `request_firmware`
error, and BPS/IPE are now up — but the A5 ICP processor boots the FW and
**watchdogs without completing the HFI handshake** (suspect FW memory-region IOVA /
HFI shared-queue SMMU mapping wrong on mainline, or A5 reset/clock). SECONDARY: a
`secheap` double-release (`Trying to release secheap twice` → `dma_buf_release`
`BUG()` oops in the 2.A page-exporter `.release` on camerad-exit fput) — only hit on
the FW-download-failed cleanup path; fixing A5 FW boot likely avoids it. Both are
tracked. On-device debug recipe (low printk + persistent `/data` synced logs +
flood-filter) saved to memory `vamos-camerad-ondevice-debug`.

State: device survives ICP failure; camerad reliably reaches ICP FW download and
fails the A5 handshake. ICP A5 bring-up is the remaining gate to frames/snapshot.

## Phase 4 camerad integration log — session 3 (2026-06-13): ICP A5 FW BOOTS

**The ICP A5 firmware now boots fully and cleanly** (`status=1` ICP_INIT_RESP_SUCCESS,
`FW download done successfully`, no watchdog). The session-2 `watch dog interrupt
from A5` blocker is RESOLVED. Three root causes, all in the 2.A/2.B memory seam,
found by reading the full pre-watchdog trace (log_buf_len=16M on the cmdline +
debug_mdl=0x3FFFFFF at runtime + the persistent-/data flood-filtered recipe). All
committed.

1. **ICP FW carveout had no coherent backing** (`FW memory alloc failed`). Legacy
   4.9 used `compatible="removed-dma-pool"` on pil_camera_mem; mainline 6.18
   dropped that driver, so `dma_alloc_coherent(fw_dev)` fell back to the default
   pool and the A5 booted wrong pages. Fix: override upstream `camera_mem`
   (sdm845.dtsi `camera-mem@8bf00000`) to `compatible="shared-dma-pool"` (no-map)
   so kernel/dma/coherent.c binds it as a per-device **write-combine** coherent
   pool with **PA-identity** handles — the removed-dma-pool equivalent. cam_smmu
   fw-dev probe now calls `of_reserved_mem_device_init()` explicitly (mainline
   auto-attaches only `restricted-dma-pool`, not `shared-dma-pool`). **Resized
   5 MiB → 4 MiB**: the per-device coherent allocator (__dma_alloc_from_coherent
   → bitmap_find_free_region) rounds to 2^get_order(size) PAGES, so a 5 MiB
   (1280-page) request needed order-11 = 2048 pages = 8 MiB and FAILED against a
   5 MiB pool; 4 MiB is exactly order-10 (1024 pages) and exceeds the 1.1 MiB
   CAMERA_ICP.elf. Firmware IOVA region (`iova-mem-region-firmware`) shrunk to
   0x400000 to match. Verified: `DMA alloc returned fw=...,hdl=0x8bf00000`,
   `iova:0, len:4194304`.

2. **HFI queues mapped only 64 KiB of each 1 MiB buffer** (the watchdog cause).
   cam_smmu mapped qtbl/cmd_q/msg_q/dbg_q (+secheap) with `iommu_map_sg(...,
   table->nents, ...)`. But `cam_mem_buf_map()` (the 2.A exporter `.map_dma_buf`)
   runs `dma_map_sgtable()`, which **coalesces** the list and overwrites
   `table->nents` with the DMA-segment count (often 1), while `iommu_map_sg()`
   walks `sg_phys()/sg->length` over the **physical** entries. So only the first
   chunk got mapped; the A5 walked unmapped HFI pages → watchdog. Fix: map by
   **`table->orig_nents`** in all three sites (SHARED, IO, secheap) + add the
   missing `size < *len_ptr` check on the SHARED path. Verified: `iommu_map_sg
   returned 1048576` for every queue.

3. **HFI queues were cached but the SMMU is non-coherent** (the decisive A5 fix).
   With 1+2 fixed, the A5 booted and read valid HW[10000000]/FW[1000100] versions
   but `HOST_INIT_RESPONSE` stayed **3** (never SUCCESS=1) and it watchdogged on
   the first FW_INIT. Root cause: the camera SMMU context banks are **non-coherent**
   (`arm-smmu: non-coherent table walk`), so the A5 reading the qtbl through its
   context bank does NOT snoop the CPU D-cache; the page exporter always vmap'd
   `PAGE_KERNEL` (**cached**), so the host's qtbl-header writes sat in cache and
   the A5 read stale descriptors. Legacy allocated these uncached (ION, no
   CAM_MEM_FLAG_CACHE). Fix: in `cam_mem_buf_do_vmap()`, map buffers WITHOUT
   `CAM_MEM_FLAG_CACHE` as **`pgprot_writecombine(PAGE_KERNEL)`** (store `flags`
   in `struct cam_mem_buffer`). **Verified: status=1, FW download done
   successfully, no watchdog.**

Also fixed: **secheap dma_buf double-put** — `cam_smmu_release_sec_heap()`
`dma_buf_put()`'d a buf it only borrowed (reserve attaches/maps without
`dma_buf_get`; the owning ref is the mem-mgr slot, freed by the same teardown).
Dropped the put. Cleared `release secheap twice` / `failed to unreserve sec heap`.

**Current blocker (next): dma_buf `vmapping_counter` BUG on teardown.** After
`FW download done successfully`, ICP does its normal post-download power-collapse
(`cam_icp_mgr_icp_power_collapse`, the clk_disable storm) and camerad exits; on
the process fput the kernel BUGs at **`dma_buf_release+0x94` = `BUG_ON(dmabuf->
vmapping_counter)`** (`__fput`→`__dentry_kill`→`dma_buf_release`). A kernel
`dma_buf_vmap` (KMD_ACCESS buffers, `cam_mem_util_map_cpu_va`) is not balanced by
a `dma_buf_vunmap` before the buffer's last ref is dropped. The device SURVIVES
the oops ([#1], one CPU). This is a **teardown refcount-balance bug, NOT a
bring-up gate** — the cameras initialise before it. Next: trace which buffer
leaks its vmap (likely an fd-exported KMD_ACCESS buffer whose slot-release vunmap
path is missed), balance the vmap/vunmap, then confirm camerad stays up →
sensor acquire (chip-id 0x5304 over CCI) → IFE/RDI frames → snapshot.py JPEG.
Secondary, probably benign: an IPE shared-RCG `clk-rcg2.c:136 update_config`
WARN in `cam_ipe_enable_soc_resources` (non-fatal WARN, IPE CPAS streamon still
succeeds).

State: **ICP A5 firmware boots successfully — the central blocker for ALL cameras
is cleared.** Remaining to frames: the teardown dma_buf vmap BUG, then sensor/IFE
bring-up.

## Phase 4 camerad integration log — session 4 (2026-06-13): teardown crashes cleared, camerad reaches SENSOR BRINGUP

Three memory-seam bugs fixed; camerad now runs cleanly past ICP teardown AND
buffer allocation, all the way to **per-sensor bringup**. The device no longer
oopses at any point in camerad init. All committed (2 commits).

1. **dma_buf `vmapping_counter` BUG (session-3 blocker) — FIXED.**
   `cam_mem_mgr_request_mem()` vmaps the HFI buffers (qtbl/cmd_q/msg_q/dbg_q/sfr)
   **unconditionally**, but `cam_mem_util_unmap()`/`cam_mem_mgr_unmap_active_buf()`
   only vunmapped when `CAM_MEM_FLAG_KMD_ACCESS` was set — and those buffers carry
   `HW_SHARED_ACCESS`, not KMD_ACCESS. The leaked vmap left
   `dmabuf->vmapping_counter != 0` at the last put → `BUG_ON` in
   `dma_buf_release()`. Fix: key the vunmap on `kmdvaddr` (set by both the
   request_mem and the lazy KMD paths), not the flag.
2. **kernel-IOVA dma_buf refcount underflow — FIXED (unmasked by #1).** Once #1
   let teardown proceed, the kernel BUG moved to `__file_ref_put_badval` at
   `dma_buf_put` in `cam_mem_util_unmap` ← `cam_icp_free_hfi_mem`. Root cause in the
   2.B IOMMU port: `cam_smmu_map_kernel_buffer_and_add_to_list()` (kernel path)
   took **no** dma_buf ref, but the shared teardown
   `cam_smmu_unmap_buf_and_remove_from_list()` unconditionally `dma_buf_put`s it →
   file-refcount underflow. (The **user** path balances this with
   `dma_buf_get(ion_fd)`.) Diagnosed by logging `file_count()`/`vmapping_counter`
   at the slot put: the HFI buffers showed `fcount=0` going into the put. Fix:
   `get_dma_buf(buf)` in the kernel map path (validate's err_put balances the
   failure case). Commit `d804018`.
3. **dma_buf mmap MAP_FAILED — FIXED.** camerad's `alloc_w_mmu_hdl()`
   (`spectra.cc:111`) `mmap(MAP_SHARED)`s the alloc-and-map fd and asserted on
   `MAP_FAILED`. The page-backed exporter set `exp_info.size = raw len`; the
   dma-buf core mmap bounds check is
   `vma->vm_pgoff + vma_pages(vma) > dmabuf->size >> PAGE_SHIFT`, and `vma_pages()`
   rounds the mmap length UP — so an unaligned exported size truncated the RHS and
   a full-length mmap was rejected `-EINVAL`. Fix: export `PAGE_ALIGN(len)` (backing
   memory is already page-aligned). Also mapped uncached buffers **write-combine**
   in `cam_mem_buf_mmap()` (non-coherent SMMU, matches the vmap path).

**Current blocker (next): sensor power-up / chip-id read fails.** camerad reaches
per-sensor bringup and each sensor fails:
`VIDIOC_CAM_CONTROL error: op_code 266 - errno 19` (`-ENODEV`) →
`** sensor 0/1/2 FAILED bringup, disabling`. op_code 266 = `0x10A` is the sensor
power-up / probe control; `-ENODEV` points at the CSIPHY/CCI/sensor-power path
(reset/standby GPIO, mclk, LDO rails, or the chip-id read over CCI not landing).
This is Phase 3.3's pending item (sensor chip-id 0x5304 over CCI). Device survives
cleanly — no kernel oops anywhere in camerad init now.

**Diagnosis so far (op_code 266 / -ENODEV):** the kernel-side failure is a CCI i2c
read returning no data:
```
CAM-CCI: cam_cci_read: read_words = 0, exp words = 1
CAM-CCI: cam_cci_read_bytes: failed to read rc:-22   (-EINVAL)
CAM-SENSOR: cam_cci_i2c_read: rc = -22
CAM-SENSOR: cam_sensor_match_id: chip id 0 does not match 5304   (alt sensor: 5803)
```
All 4 sensors on BOTH cci masters fail identically (chip id reads 0) — a
**common-mode** failure, not per-sensor. Key facts established on-device (sampled
clk_summary / sysfs-regulator DURING an active probe):
- **CCI controller IS clocked** — `cam_cc_cci_clk` enable_cnt reaches 1 during the
  read (rate 37.5 MHz). Rules out the CCI-clock hypothesis.
- **The probing sensor's mclk IS enabled** — summed `cam_cc_mclk[0-3]` enable_cnt
  reaches exactly 1 at a time (one mclk per sensor, 24 MHz). Rules out no-mclk.
- **`bob` (cam_vana 0-2) + `lvs1` (cam_vio, all) are enabled.**
- The CCI transaction **completes** (no `wait_for_completion_timeout` / no CCI
  TIMEOUT logged) but the read FIFO buf-level is 0 → the slave is **NOT ACKing**.
A clocked controller + on mclk + vana/vio on, yet a NACK, points at the remaining
sensor-power/reset items NOT yet verified:
  1. **cam_vdig** (`camera_rear_ldo`/`camera_ldo`, 1.05 V) actually enabling during
     probe — at-rest they read `disabled` (expected), but a live during-probe
     sample was inconclusive (the on-device sysfs-walk-in-a-loop over serial was
     too slow / garbled to confirm; do it with a lighter probe or a driver log).
  2. **Sensor reset GPIO** being deasserted (a held-in-reset sensor is clocked but
     won't ACK) and **mclk pin actually muxed** (CCF clock on != pin driving — verify
     the `cam_sensor_mclk*_active` pinctrl state is selected).
Next step (offline-reviewable): read the driver power-up sequence
(`cam_sensor_util.c` `cam_sensor_core_power_up`) + the DTS sensor power-setting /
gpio tables (`qcom,cam-sensor@0..3` in `sdm845-comma-common.dtsi`) to verify
cam_vdig regulator mapping + reset-gpio wiring + mclk pinctrl, then targeted
on-device confirmation. (On-device sampling note: prefer the mici-skill `bash
--timeout N` inline form with a SHORT script; long sysfs loops over serial garble
the frame.)

### Session 5 (2026-06-13): ROOT CAUSE = cam_vdig pm8998 GPIO enable pin not driving

Cross-referenced against the **legacy 4.9 kernel** (flashed `boot-legacy.img`,
ran the same `camerad`): legacy reads `sensor_id:0x5304` at slave 0x6c/0x20,
`CAM_ACQUIRE_DEV Success`, all 3 cameras sync, and `snapshot.py` writes real
JPEGs — the definitive working baseline. The single difference found:

**cam_vdig (1.05 V) never physically powers on mainline.** `camera_rear_ldo`/
`camera_ldo` are GPIO-gated fixed regulators on **pm8998 gpio12 / gpio9**. On
mainline the pmic enable pin stays **`out low func0 2mA pull down`** even when the
regulator's software `state` reads `enabled` — i.e. the SPMI-GPIO output buffer is
never actually driven high, so the digital rail stays off and the sensor i2c block
NACKs (`read_words=0`). Legacy drives it (cam_vdig enabled while streaming).

**The stubborn part:** getting the pm8998 gpio pinconf (`function=normal`,
`drive-push-pull`, `STRENGTH_HIGH`) APPLIED. Tried, none worked so far:
- regulator-fixed `gpio = <&pm8998_gpios 12>` + `pinctrl-0 = <&...dvdd_en>` (the
  exact mainline **db845c `cam0_dvdd_1v2`** pattern) — pin stays func0/2mA/low;
  **the regulator gets NO pinctrl handle** (`/sys/kernel/debug/pinctrl/*/pinctrl-handles`
  empty for it), so reg-fixed-voltage isn't applying its pinctrl-0 here.
- `regulator-always-on` + the pinctrl — state flips `enabled` but pin still
  func0/low.
- **gpio-hog** `output-high` on `&pm8998_gpios` — sets direction `out` but value
  stays **low** and func0/2mA (a hog sets the gpiolib value, NOT the pinconf
  push-pull/function, and the SPMI output buffer is weak/open-drain by default so
  the value reads back low).
- manual sysfs `export` of the global gpio (pmic chip `gpiochip512` base=512 →
  gpio12=523, gpio9=520) failed to create the node (line already claimed).

Confirmed NOT the cause (all ruled in during an active probe): CCI clock on, the
probing sensor's mclk on, cam_vana (`bob`) + cam_vio (`lvs1`) on, CCI transaction
completes (no timeout). It is specifically the cam_vdig GPIO-enable drive.

**Mechanism understood (mainline `pinctrl-spmi-gpio.c`):** `pmic_gpio_config_set`
sets pin mode = `DIGITAL_OUTPUT` only if `output_enabled` is true, which is set
ONLY by `PIN_CONFIG_OUTPUT_ENABLE`/`PIN_CONFIG_LEVEL` (DT `output-high`/`output-low`).
`direction_output(val)` → `config_set(PIN_CONFIG_LEVEL,val)` does the same. So
driving the pin needs `output-high` AND push-pull (`PMIC_GPIO_OUT_BUF_CMOS`); the
default buffer is read from HW at probe (`pmic_gpio_populate`) and the pin reads
`func0 2mA pull-down` = the reset/INPUT default.

**The real blocker: the pinconf NEVER gets applied.** Verified via debugfs:
- regulator-fixed `pinctrl-0` → `pinctrl-handles` has NO entry for the regulator
  (reg-fixed-voltage's pinctrl not bound on this kernel).
- controller-node `&pm8998_gpios { pinctrl-0 = <&...> }` → the props ARE in the
  live dtb (`/proc/device-tree/.../gpio@c000/pinctrl-0` present) but
  `pinctrl-maps` has NO `dvdd` entry → **the spmi-gpio PROVIDER does not apply its
  own default pinctrl** (provider-probes-before-its-own-state ordering).
- gpio-hog `output-high` → sets direction `out` but value reads **low**; suspect
  the `pmic_gpio_of_xlate` `- PMIC_GPIO_PHYSICAL_OFFSET` means `gpios = <12>` in a
  hog may not address the intended physical pad, OR the hog value path leaves
  buffer open-drain. Hogs can't carry pinconf (push-pull/strength) anyway.

**RESOLVED the pin-drive, but it was NOT the root cause.** The working config:
`regulator-fixed` + `gpio = <&pm8998_gpios N GPIO_ACTIVE_HIGH>` + `enable-active-high`
+ `regulator-always-on/boot-on` + `pinctrl-0 = <&..._dvdd_en_default>` (state =
function normal, drive-push-pull, STRENGTH_HIGH, **output-high**). With this, the
**pmic@0 (gpiochip0) gpio9 reads `out high push-pull high`** — cam_vdig for sensor 3
is genuinely powered. (Earlier `func0 2mA pull-down` reads were the WRONG chip —
pmic@2/gpiochip1; the real `&pm8998_gpios` is **pmic@0 / gpiochip0, base 0**, and the
debugfs is 1-indexed while of_xlate subtracts PMIC_GPIO_PHYSICAL_OFFSET. Always read
the `c440000.spmi:pmic@0:gpio@c000` chip section specifically.)

**DECISIVE RESULT: even with cam_vdig powered (gpio9 driven high), sensor 3 STILL
NACKs (chip id 0).** So cam_vdig-not-powering was a real bug but **NOT the cause of
the chip-id failure** — the NACK is common-mode across all 4 sensors and persists
with vdig on. Two loose ends on the vdig fix: (i) gpio12 (camera_rear_ldo, sensors
0-2) is NOT being requested/driven while gpio9 (camera_ldo) is, despite identical
DTS — a per-pin gpiod request quirk to resolve; (ii) it doesn't matter for the NACK.

**The REAL common-mode blocker is upstream of vdig — almost certainly the sensor
RESET GPIO (TLMM) not deasserting, or the MCLK pin not actually muxed/driving.**
Both are common to all sensors; a held-in-reset or unclocked sensor NACKs regardless
of rails. Next session MUST verify the **TLMM** pins during probe (NOT the pmic
chip): sensor reset = `<&tlmm 9>`/`<&tlmm 7>`, mclk0 = `<&tlmm 13>` — confirm the
`cam_sensor_mclk*_active` + `cam_sensor_*_active` (reset) pinctrl states actually
select and drive. Read the SPECIFIC tlmm gpiochip section (`3400000.pinctrl`), since
multiple gpiochips share gpioN labels and cross-chip greps are misleading. Likely
fix area: the sensor-node `pinctrl-0` (mclk+reset active) not applying, or the
cam_res_mgr/cam_soc_util gpio request path on 6.18 (the legacy-integer GPIO bridge
from 2.G-residual) not driving reset. The cam_vdig DTS work is committed as a
checkpoint (gpio9 proven; gpio12 quirk + the fact vdig is insufficient both noted).

State: **camerad initialises the entire Spectra stack with ZERO kernel oopses and
reaches real sensor bringup.** The remaining gate to frames is sensor power-up /
chip-id read.

## Decisions / notes log
- 2026-06-12 — **Phase 3.1: camera DTS ported; mici .dtb builds clean.** Full
  Spectra camera node block + 4 sensors + 20 pinctrl states + 6 gdsc + 4 ldo
  regulator stand-ins in `sdm845-comma-common.dtsi`; board mclk drive-strength in
  `sdm845-comma-mici.dts`; upstream `&camss`/`&cci` disabled (reg-overlap proof
  inline). camcc = CCF mapping (no syscon — driver uses clk_get/clk_set_rate).
  GDSCs = always-on fixed-regulator stand-ins + TITAN_TOP genpd attach on cam-cpas
  (multi-domain gap noted for on-device). interconnects deferred (icc no-op kept).
  `./vamos build kernel` → `build/boot.img` 16.8M, no dtc errors (only benign
  `shared-gpios` phandle-heuristic false-positives — cam_res_mgr reads it as a u32
  array). Verified node presence by `strings` on the compiled dtb. Next: Phase
  1.5/3.2 (flash + on-device probe via mici skill). Log: `build/spectra-build-dts.log`.
- 2026-06-12 — **Phase 2.C/E + Phase 1.4: the Spectra tree COMPILES AND LINKS.**
  `./vamos build kernel` → `build/boot.img` 16.8M, `CONFIG_SPECTRA_CAMERA=y`
  built into vmlinux. 2.C (msm-bus→interconnect, real NULL-tolerant port) and 2.E
  (scm→documented secure-path no-op stubs) done; all trailing mechanical sig-drift
  cleared; al6100 excluded; spurious static EXPORT_SYMBOLs dropped (final modpost
  blocker). Remaining stubs are all OFF the mici non-secure data path (secure SCM,
  stage-2 ION, QPNP flash, SPI/OIS CMA, AHB icc no-op pending DTS) — see ledger.
  Next: Phase 1.5 (flash + boot on device) and Phase 3 (DTS incl. CPAS/AXI
  `interconnects` props to retire the icc no-op). Log: `build/spectra-build-2ce.log`.
- 2026-06-12 — Phase 2.G-residual: gpio (legacy-integer bridge) + mechanical
  quick-wins done (see "Phase 2.G-residual surface"). 10 files changed. GPIO
  bucket gone from leading edge; no new stubs. Leading edge now pure C (msm-bus)
  + E (scm) + remaining `.remove`/i2c-probe void-sig drift + a CMA-API drift in
  the SPI sensor path. Log: `build/spectra-build-2gr.log`.
- 2026-06-12 — Phase 2.A: memory bucket (ion → dma-buf) done (see "Phase 2.A
  surface"). 15 files changed under `kernel/spectra-camera/`. Key finding: the
  in-kernel **dma-heap consumer API is absent in this 6.18 baseline**, so the
  kernel-allocation path got a minimal built-in page-backed dma-buf exporter
  (legacy non-secure ION system-heap equivalent) rather than `dma_heap_find` +
  `dma_heap_buffer_alloc`. cam_smmu (2.B) consumes the resulting real sg_tables.
  Secure/PROTECTED_MODE backing left to 2.E (qcom_scm). Leading edge now C/E/F/G
  only. Log: `build/spectra-build-2a.log`.
- 2026-06-13 — Architecture: **out-of-tree source** `kernel/spectra-camera/`,
  copied into the kernel tree at build time, built-in (=y); openpilot &amp;
  userspace/system.img untouched; only a 24-line link patch (user). Revised from
  the initial in-tree 351-file-patch idea.
- 2026-06-13 — Legacy baseline captured &amp; device confirmed streaming on 4.9.
- 2026-06-13 — Phase 1 milestone: out-of-tree mechanism verified end-to-end
  (patch applies, source links, `CONFIG_SPECTRA_CAMERA=y`, build reaches camera
  compilation, fails on the expected 4.9→6.18 API gap).
- 2026-06-12 — Phase 2.G first slice: core-types / mechanical port done (see
  "Phase 2.G surface"). 11 driver files changed under `kernel/spectra-camera/`;
  `struct timeval`→`timespec64`, `strlcpy`→`strscpy`, `*_no_log` I/O macros,
  `kref_read`, ktime boottime, clk/qcom+socinfo compat shim, VFL/debugfs/.remove
  mechanical V4L2 fixes. cam_core/cam_req_mgr cleared of own errors; leading edge
  advanced to A (ion) + B (iommu) + residual gpiod migration. Log:
  `build/spectra-build-phase2g.log`.
