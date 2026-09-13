/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 *
 * cam_compat_qcom.h - Mainline 6.18 compatibility shim for downstream-only
 * Qualcomm platform APIs that the Spectra camera driver depends on.
 *
 * The legacy (kernel 4.9) driver used two downstream-only interfaces that do
 * not exist in mainline 6.18:
 *
 *   - linux/clk/qcom.h : the per-consumer clk branch memory/periphery retention
 *     flags (CLKFLAG_*) plus clk_set_flags(). Mainline's common clock framework
 *     has no equivalent per-clk flag setter; these flags are a power
 *     optimisation (RAM retention on idle clk branches) and are safe to no-op
 *     for bring-up.
 *
 *   - soc/qcom/socinfo.h : socinfo_get_id()/socinfo_get_version(). Mainline
 *     does not export these; they are only consumed for SoC-revision quirk
 *     decisions. Returning 0 is safe until Phase 3 HW bring-up wires up a real
 *     soc-revision lookup (e.g. via the qcom_socinfo / nvmem path).
 *
 * TECH DEBT: tracked in docs/cameras/TASKS.md "Stub / tech-debt ledger".
 * Retire during Phase 3 (HW bring-up) once the real clk-flag / soc-revision
 * handling is established.
 */

#ifndef _CAM_COMPAT_QCOM_H_
#define _CAM_COMPAT_QCOM_H_

#include <linux/clk.h>
#include <linux/types.h>

/* Downstream clk branch memory/periphery retention flags (linux/clk/qcom.h). */
enum branch_mem_flags {
	CLKFLAG_RETAIN_PERIPH,
	CLKFLAG_NORETAIN_PERIPH,
	CLKFLAG_RETAIN_MEM,
	CLKFLAG_NORETAIN_MEM,
	CLKFLAG_PERIPH_OFF_SET,
	CLKFLAG_PERIPH_OFF_CLEAR,
};

/*
 * Downstream clk_set_flags() has no mainline equivalent. The flags only control
 * idle-time RAM retention on the clk branch (a power optimisation); no-op is
 * functionally safe.
 */
static inline int clk_set_flags(struct clk *clk, unsigned long flags)
{
	return 0;
}

/*
 * Downstream socinfo helpers. Real SoC-revision lookup is deferred to HW
 * bring-up; 0 is a safe placeholder that disables revision-specific quirks.
 */
static inline uint32_t socinfo_get_id(void)
{
	return 0;
}

static inline uint32_t socinfo_get_version(void)
{
	return 0;
}

#endif /* _CAM_COMPAT_QCOM_H_ */
