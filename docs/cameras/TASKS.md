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
- [ ] **1.4** Stub/adapt blocking subsystems (A memory, B iommu, C bus, D sync)
      just enough to **link**. Track each stub as a debt item below.
- [ ] **1.5** Boots on device; `dmesg` shows the driver registering (even if HW
      blocks fail to probe).

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
- [ ] **2.C** Bus BW: `msm_bus_scale_*` → interconnect `icc_*`
      (`cam_utils/cam_soc_util.c`, `cam_cpas/cam_cpas_hw.c`). ★★★★
- [ ] **2.D** Sync: modernize downstream `cam_sync` against 6.18
      `dma_fence`/`completion` (`cam_sync/cam_sync.c`). ★★★★
- [ ] **2.E** SCM/secure buffer: `soc/qcom/scm.h` → `qcom_scm_*`. ★★★
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

- [ ] **3.1** Port camera DTS into this repo's
      `kernel/dts/sdm845-comma-{common.dtsi,mici.dts}`: CPAS/CDM, CCI, 4×CSIPHY,
      CSID/VFE, ICP/A5, JPEG, FD, 4× cam-sensor, regulator + mclk pinctrl mapping
      (DESIGN.md §4).
- [ ] **3.2** `cam-cpas` + `cam_smmu` probe clean on device.
- [ ] **3.3** `cam-cci-driver` + 4× `cam-csiphy-driver` + 4× `cam-sensor-driver`
      probe; sensor chip-id read over CCI succeeds (expect 0x5304 family).
- [ ] **3.4** `cam-isp` (CSID/IFE) + `cam-req-mgr` + `cam_sync` register; by-path
      video nodes appear with correct names.
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
| ION secure/stage-2 types + helpers: `ion_phys_addr_t`/`struct ion_client`/`struct ion_handle` aliases + `ion_import_dma_buf_fd()`/`ion_phys()`/`ion_free()` stubs (return `-ENODEV`/`NULL`). Keeps cam_smmu's PUBLIC `cam_smmu_map_stage2_iova()` signature byte-identical AND lets the secure path compile. Non-secure (mici) data path does NOT use these. **2.A update (2026-06-12):** cam_mem_mgr is now fully off ION and confirmed to never call these on the non-secure path (it passes `NULL` for the `struct ion_client *` arg of `cam_smmu_map_stage2_iova`, used only under `CAM_MEM_FLAG_PROTECTED_MODE`, which mici does not exercise). The shim is now referenced **only** by cam_smmu_api.c's stage-2 helpers — sole remaining owner is **2.E**. | `cam_smmu/cam_smmu_compat.h` (replaces `#include <linux/msm_ion.h>` in `cam_smmu_api.h`; the stage-2 funcs in `cam_smmu_api.c` resolve against it) | 2026-06-12 (2.B) | 2.E (`qcom_scm_assign_mem` secure assign) — real stage-2 impl | active |
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

## Decisions / notes log
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
