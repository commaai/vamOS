/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2014-2019, The Linux Foundation. All rights reserved.
 *
 * cam_smmu_compat.h - Mainline 6.18 compatibility shim for the downstream-only
 * Qualcomm secure-buffer / ION interfaces that the cam_smmu public API
 * (cam_smmu_api.h) and its stage-2 (secure) code path still reference.
 *
 * Phase 2.B (this task) ported cam_smmu off the removed ARM-IOMMU mapping API
 * (asm/dma-iommu.h, arm_iommu_*, struct dma_iommu_mapping) and the downstream
 * msm_dma_iommu_mapping helpers onto the generic mainline IOMMU API
 * (iommu_paging_domain_alloc / iommu_attach_device / iommu_map[_sg] /
 * iommu_unmap). That covered the NON-secure data path that the camera actually
 * uses on mici.
 *
 * The SECURE (stage-2) path still uses ION client/handle types and the
 * Qualcomm secure-buffer SCM calls. Those belong to:
 *   - bucket 2.A (ion -> dma-buf), which owns the real memory-allocation rework
 *     in cam_req_mgr/cam_mem_mgr.c, and
 *   - bucket 2.E (SCM/secure-buffer -> qcom_scm_*).
 * Neither is in scope for 2.B, but cam_smmu_api.h's PUBLIC contract names
 * `struct ion_client *` and `ion_phys_addr_t` in cam_smmu_map_stage2_iova(),
 * and cam_smmu_api.c's stage-2 helpers call ion_import_dma_buf_fd()/ion_phys()/
 * ion_free(). To keep the cam_smmu public API byte-compatible AND let the file
 * compile against 6.18, this header provides MINIMAL forward types + stub
 * inlines for exactly those ION symbols. mici streams via the non-secure cb, so
 * the secure path is never exercised; the stubs return -ENODEV/NULL.
 *
 * TECH DEBT: tracked in docs/cameras/TASKS.md "Stub / tech-debt ledger".
 * Retire at the 2.A / 2.E seam (real dma-buf-heap secure allocation + qcom_scm
 * assign_mem), at which point the stage-2 path gets a proper implementation.
 */

#ifndef _CAM_SMMU_COMPAT_H_
#define _CAM_SMMU_COMPAT_H_

#include <linux/types.h>
#include <linux/dma-buf.h>
#include <linux/err.h>

/*
 * Downstream msm_ion.h exported ion_phys_addr_t and the opaque ion_client /
 * ion_handle handle types. Mainline removed ION entirely (replaced by dma-buf
 * heaps). Provide the minimal type aliases so the cam_smmu public API and the
 * secure path keep their original signatures.
 */
#ifndef _CAM_SMMU_ION_COMPAT_TYPES
#define _CAM_SMMU_ION_COMPAT_TYPES
typedef phys_addr_t ion_phys_addr_t;
struct ion_client;
struct ion_handle;
#endif /* _CAM_SMMU_ION_COMPAT_TYPES */

/*
 * Secure-path ION helper stubs. The non-secure camera data path on mici does
 * not use these; they exist only so the stage-2 (secure) code in
 * cam_smmu_api.c compiles. Real implementations land with bucket 2.A/2.E.
 */
static inline struct ion_handle *ion_import_dma_buf_fd(
	struct ion_client *client, int fd)
{
	return ERR_PTR(-ENODEV);
}

static inline int ion_phys(struct ion_client *client, struct ion_handle *handle,
	ion_phys_addr_t *addr, size_t *len)
{
	return -ENODEV;
}

static inline void ion_free(struct ion_client *client,
	struct ion_handle *handle)
{
}

#endif /* _CAM_SMMU_COMPAT_H_ */
