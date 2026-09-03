/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#ifndef _CAM_EEPROM_CORE_H_
#define _CAM_EEPROM_CORE_H_

#include "cam_eeprom_dev.h"
/*for tof camera Begin*/
#include "tl_dev_eeprom.h"
/*for tof camera End*/

int32_t cam_eeprom_driver_cmd(struct cam_eeprom_ctrl_t *e_ctrl, void *arg);
int32_t cam_eeprom_parse_read_memory_map(struct device_node *of_node,
	struct cam_eeprom_ctrl_t *e_ctrl);
/**
 * @e_ctrl: EEPROM ctrl structure
 *
 * This API handles the shutdown ioctl/close
 */
void cam_eeprom_shutdown(struct cam_eeprom_ctrl_t *e_ctrl);

/*for tof camera Begin*/
/**
 * @initial_settings: EEPROM data list_head
 * @type: operation type
 * @mode: sensor mode
 *
 * This API get EEPROM data list;
 */
int transmit_sensor_reg_setting_get(struct list_head *initial_settings,enum EEPROM_DATA_OP_T type);
/**
 * This API return EEPROM data list
 */
int transmit_sensor_reg_setting_ret(struct list_head *initial_settings,enum EEPROM_DATA_OP_T type);
/**
 * This API kfree list_head structure
 */
int cam_eeprom_free_list_head(enum cam_eeprom_free cmd);
/*for tof camera End*/
#endif
/* _CAM_EEPROM_CORE_H_ */
