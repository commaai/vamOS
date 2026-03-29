/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _CAM_FLASH_CORE_H_
#define _CAM_FLASH_CORE_H_

#include <linux/errno.h>
#include <media/cam_sensor.h>

/* VAMOS: vendor header <linux/leds-qpnp-flash.h> not in mainline;
 * stub the constants and function it provided */
#define ENABLE_REGULATOR   1
#define DISABLE_REGULATOR  2
#define QUERY_MAX_CURRENT  3

struct led_trigger;
static inline int qpnp_flash_led_prepare(struct led_trigger *trig,
	int options, int *max_current)
{
	return -ENOSYS;
}
#include "cam_flash_dev.h"

int cam_flash_publish_dev_info(struct cam_req_mgr_device_info *info);
int cam_flash_establish_link(struct cam_req_mgr_core_dev_link_setup *link);
int cam_flash_apply_request(struct cam_req_mgr_apply_request *apply);
int cam_flash_process_evt(struct cam_req_mgr_link_evt_data *event_data);
int cam_flash_flush_request(struct cam_req_mgr_flush_request *flush);


#endif /*_CAM_FLASH_CORE_H_*/
