/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
 *
 * cam_flash_compat.h - Mainline 6.18 compatibility shim for the downstream
 * linux/leds-qpnp-flash.h interface used by cam_flash_core.
 *
 * The Qualcomm QPNP flash-LED driver (CONFIG_LEDS_QPNP_FLASH*) and its header
 * linux/leds-qpnp-flash.h do not exist in mainline. The downstream header
 * already degraded qpnp_flash_led_prepare() to a -ENODEV stub when the QPNP
 * flash driver was not built; this header keeps exactly that behaviour.
 *
 * The mici comma four has no camera flash LED on the data path, so the flash
 * "prepare"/regulator/query-current ops are never exercised; the stub returning
 * -ENODEV is functionally identical to the downstream no-QPNP build.
 *
 * TECH DEBT: tracked in docs/cameras/TASKS.md "Stub / tech-debt ledger".
 */

#ifndef _CAM_FLASH_COMPAT_H_
#define _CAM_FLASH_COMPAT_H_

#include <linux/leds.h>
#include <linux/errno.h>
#include <linux/bits.h>

#define ENABLE_REGULATOR	BIT(0)
#define DISABLE_REGULATOR	BIT(1)
#define QUERY_MAX_CURRENT	BIT(2)

#define FLASH_LED_PREPARE_OPTIONS_MASK	GENMASK(3, 0)

static inline int qpnp_flash_led_prepare(struct led_trigger *trig, int options,
					int *max_current)
{
	return -ENODEV;
}

#endif /* _CAM_FLASH_COMPAT_H_ */
