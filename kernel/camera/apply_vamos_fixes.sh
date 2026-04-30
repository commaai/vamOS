#!/usr/bin/env bash
# VAMOS: Apply all mainline kernel compatibility fixes to Spectra camera drivers
# All changes are tagged with "VAMOS:" comments for grepability
set -e
cd "$(dirname "$0")"
CAM=drivers/media/platform/msm/camera

echo "=== Applying VAMOS mainline compatibility fixes ==="

# --- Removed vendor headers ---
echo "Fixing removed vendor headers..."

# linux/ion.h — ION allocator removed from mainline in 5.10+
find . -name "*.c" -o -name "*.h" | xargs grep -l 'linux/ion.h' 2>/dev/null | while read f; do
  sed -i '' "s|#include <linux/ion.h>|/* VAMOS: removed <linux/ion.h> — ION removed from mainline 5.10+ */|" "$f"
done

# linux/msm_ion.h — MSM ION extensions
find . -name "*.c" -o -name "*.h" | xargs grep -l 'linux/msm_ion.h' 2>/dev/null | while read f; do
  sed -i '' "s|#include <linux/msm_ion.h>|/* VAMOS: removed <linux/msm_ion.h> — vendor ION extensions not in mainline */|" "$f"
done

# asm/dma-iommu.h — ARM DMA-IOMMU glue removed in mainline 6.x
find . -name "*.c" -o -name "*.h" | xargs grep -l 'asm/dma-iommu.h' 2>/dev/null | while read f; do
  sed -i '' "s|#include <asm/dma-iommu.h>|/* VAMOS: removed <asm/dma-iommu.h> — ARM DMA-IOMMU removed in mainline 6.x */|" "$f"
done

# linux/msm_dma_iommu_mapping.h
find . -name "*.c" -o -name "*.h" | xargs grep -l 'linux/msm_dma_iommu_mapping.h' 2>/dev/null | while read f; do
  sed -i '' "s|#include <linux/msm_dma_iommu_mapping.h>|/* VAMOS: removed <linux/msm_dma_iommu_mapping.h> — vendor DMA-IOMMU not in mainline */|" "$f"
done

# soc/qcom/scm.h — vendor SCM
find . -name "*.c" -o -name "*.h" | xargs grep -l 'soc/qcom/scm.h' 2>/dev/null | while read f; do
  sed -i '' "s|#include <soc/qcom/scm.h>|/* VAMOS: removed <soc/qcom/scm.h> — vendor SCM not in mainline */|" "$f"
done

# soc/qcom/secure_buffer.h
find . -name "*.c" -o -name "*.h" | xargs grep -l 'soc/qcom/secure_buffer.h' 2>/dev/null | while read f; do
  sed -i '' "s|#include <soc/qcom/secure_buffer.h>|/* VAMOS: removed <soc/qcom/secure_buffer.h> — vendor secure buffer not in mainline */|" "$f"
done

# soc/qcom/socinfo.h
find . -name "*.c" -o -name "*.h" | xargs grep -l 'soc/qcom/socinfo.h' 2>/dev/null | while read f; do
  sed -i '' "s|#include <soc/qcom/socinfo.h>|/* VAMOS: removed <soc/qcom/socinfo.h> — vendor socinfo not in mainline */|" "$f"
done

# linux/slub_def.h — not directly includable in mainline
sed -i '' 's|#include <linux/slub_def.h>|/* VAMOS: removed <linux/slub_def.h> — not includable in mainline, slab.h suffices */|' \
  $CAM/cam_req_mgr/cam_req_mgr_dev.c

# linux/msm-bus.h — vendor bus voting
sed -i '' 's|#include <linux/msm-bus.h>|/* VAMOS: removed <linux/msm-bus.h> — vendor bus voting not in mainline */|' \
  $CAM/cam_cpas/cam_cpas_hw.c

# linux/clk/qcom.h — vendor clock helpers
find . -name "*.c" -o -name "*.h" | xargs grep -l 'linux/clk/qcom.h' 2>/dev/null | while read f; do
  sed -i '' "s|#include <linux/clk/qcom.h>|/* VAMOS: removed <linux/clk/qcom.h> — vendor clock helpers not in mainline */|" "$f"
done

# --- API renames / removals ---
echo "Fixing API renames..."

# writel_relaxed_no_log -> writel_relaxed (Qualcomm vendor extension)
sed -i '' 's|writel_relaxed_no_log|writel_relaxed /* VAMOS: was writel_relaxed_no_log, vendor extension */|g' \
  $CAM/cam_utils/cam_io_util.c

# strlcpy -> strscpy (removed in 6.x)
find . -name "*.c" | xargs grep -l 'strlcpy' 2>/dev/null | while read f; do
  sed -i '' 's|strlcpy|strscpy /* VAMOS: was strlcpy */|g' "$f"
done

# VFL_TYPE_GRABBER -> VFL_TYPE_VIDEO_CAPTURE (renamed in mainline)
find . -name "*.c" | xargs grep -l 'VFL_TYPE_GRABBER' 2>/dev/null | while read f; do
  sed -i '' 's|VFL_TYPE_GRABBER|VFL_TYPE_VIDEO_CAPTURE /* VAMOS: was VFL_TYPE_GRABBER */|g' "$f"
done

# struct timeval -> struct timespec64 (timeval removed from kernel 5.x)
sed -i '' 's|struct timeval |struct timespec64 /* VAMOS: was timeval */ |g' \
  $CAM/cam_isp/isp_hw_mgr/isp_hw/include/cam_isp_hw.h \
  $CAM/cam_core/cam_hw_mgr_intf.h

# atomic_read(&(ctx->refcount.refcount)) -> refcount_read(&(ctx->refcount))
# refcount_t internals opaque in mainline
sed -i '' 's|atomic_read(&(ctx->refcount\.refcount))|refcount_read(\&(ctx->refcount)) /* VAMOS: refcount_t opaque */|g' \
  $CAM/cam_core/cam_node.c \
  $CAM/cam_core/cam_context.c

# setup_timer -> timer_setup (changed in 4.15+, different arg order)
# This needs manual attention for the callback signature change — mark for now
find . -name "*.c" | xargs grep -l 'setup_timer' 2>/dev/null | while read f; do
  sed -i '' 's|setup_timer|timer_setup /* VAMOS: was setup_timer, NOTE: arg order changed */|g' "$f"
done

# del_timer_sync -> timer_delete_sync (renamed in 6.x)
find . -name "*.c" | xargs grep -l 'del_timer_sync' 2>/dev/null | while read f; do
  sed -i '' 's|del_timer_sync|timer_delete_sync /* VAMOS: was del_timer_sync */|g' "$f"
done

# --- $(srctree)/ prefix for -I paths (required for O=out builds in mainline kbuild) ---
echo "Fixing include paths for O=out builds..."
find . -name Makefile | while read f; do
  if grep -q 'ccflags-y += -Idrivers/' "$f"; then
    sed -i '' 's|ccflags-y += -Idrivers/|ccflags-y += -I$(srctree)/drivers/|g' "$f"
  fi
done

# --- Kconfig / Makefile ---
echo "Writing Kconfig and Makefiles..."
cat > $CAM/Kconfig << 'KEOF'
# VAMOS: depends on VIDEO_DEV instead of VIDEO_V4L2 — VIDEO_V4L2 merged into VIDEO_DEV in mainline
config SPECTRA_CAMERA
	bool "Qualcomm Technologies, Inc. Spectra camera support"
	depends on ARCH_QCOM && VIDEO_DEV && I2C
	help
	  Qualcomm Spectra camera driver stack including sensor, IFE, and
	  camera request manager. Ported from comma-kernel (4.14) to mainline.
KEOF

cat > $CAM/Makefile << 'MEOF'
obj-$(CONFIG_SPECTRA_CAMERA) += cam_utils/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_core/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_sync/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_smmu/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_cpas/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_cdm/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_req_mgr/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_isp/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_sensor_module/
MEOF

echo "=== Done applying VAMOS fixes ==="
