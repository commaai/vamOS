/*
 * File: miniisp_utility.c
 * Description: Mini ISP Utility sample codes
 *
 * (C)Copyright altek Corporation 2017
 *
 *  2017/03/29; Louis Wang; Initial version
 */
 /******Include File******/
#include "linux/init.h"
#include "linux/module.h"

#include "include/miniisp.h"
#include "include/error.h"
#include "include/miniisp_chip_base_define.h"


 #define MINI_ISP_LOG_TAG	"[miniisp_utility]"

 /******Private Global Variable******/


 /******Public Function******/
/**
 *\brief Read memory in E mode for bypass mode
 *\return Error code
*/
errcode mini_isp_utility_read_reg_e_mode_for_bypass_use(void)
{
	errcode err = ERR_SUCCESS;

	err = mini_isp_chip_base_dump_bypass_mode_register();
	return err;
}
EXPORT_SYMBOL(mini_isp_utility_read_reg_e_mode_for_bypass_use);


/**
 *\brief Read memory in E mode
 *\return Error code
*/
errcode mini_isp_utility_read_reg_e_mode(void)
{
	errcode err = ERR_SUCCESS;

	err = mini_isp_chip_base_dump_normal_mode_register();
	return err;
}
EXPORT_SYMBOL(mini_isp_utility_read_reg_e_mode);

/**
 *\brief Read irp and depth image based information
 *\return Error code
*/
errcode mini_isp_utility_get_irp_and_depth_information(
	struct irp_and_depth_information *info)
{
	errcode err = ERR_SUCCESS;
	u32 rg_img_in_size;
	u32 rg_depth_in_size;
	u32 crop_src;
	u8 fov_mode;

	mini_isp_register_read(0xfff8401c, &rg_img_in_size);
	info->irp_width = ((rg_img_in_size & 0x00001fff)+1);
	info->irp_height = (((rg_img_in_size & 0x1fff0000)>>16)+1);

	mini_isp_register_read(0xfffa7020, &crop_src);

	if (crop_src)
		info->irp_format = 1;
	else
		info->irp_format = 0;

	mini_isp_register_read(0xfff5601c, &rg_depth_in_size);
	info->depth_width = ((rg_depth_in_size & 0x00001fff)+1);
	info->depth_height = (((rg_depth_in_size & 0x1fff0000)>>16)+1);

	info->depth_image_address = 0x20715400;

	mini_isp_memory_read(0x24, &fov_mode, 1);

	info->fov_mode = fov_mode;
	if (fov_mode == 1)
		info->irp_image_address = 0x20500000 + 288000 -
			((rg_img_in_size & 0x00001fff) + 1) *
			(((rg_img_in_size & 0x1fff0000)>>16) + 1);
	else
		info->irp_image_address = 0x20500000;
	misp_info("%s:depth_image_address:%u, depth_width:%u, depth_height:%u, irp_format:%u, irp_image_address:%u, irp_width:%u, irp_height:%u, fov_mode:%u\n", __func__,
		info->depth_image_address, info->depth_width, info->depth_height,
		info->irp_format, info->irp_image_address, info->irp_width, info->irp_height, info->fov_mode);
	return err;
}
EXPORT_SYMBOL(mini_isp_utility_get_irp_and_depth_information);
