/*
 * File: camera_profile_cmd.c
 * Description: Mini ISP sample codes
 *
 * (C)Copyright altek Corporation 2013
 *
 *  2013/10/14; Bruce Chung; Initial version
 *  2013/12/05; Bruce Chung; 2nd version
 */

/******Include File******/

#include <linux/string.h>

#include "include/mtype.h"
#include "include/error.h"
#include "include/miniisp.h"
#include "include/isp_camera_cmd.h"
#include "include/ispctrl_if_master.h"
#include "include/ispctrl_if_master_local.h"

/******Private Constant Definition******/
#define MINI_ISP_LOG_TAG "[[miniisp]camera_profile_cmd]"

/******Private Type Declaration******/

/******Private Function Prototype******/

/******Private Global Variable******/

static struct isp_cmd_get_sensor_mode mast_sensors_info;

/******Public Function******/

/*
 *\brief Camera profile parameters init
 *\return None
 */
void isp_mast_camera_profile_para_init(void)
{
	/*Reset Camera profile parameters*/
	memset(&mast_sensors_info, 0x0,
		sizeof(struct isp_cmd_get_sensor_mode));
}

/*
 *\brief Set Sensor Mode
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_sensor_mode(void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;

	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_set_sensor_mode);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
				param, para_size);


	if ((((struct isp_cmd_set_sensor_mode *)param)->sensor_on_off)
			&& (err == ERR_SUCCESS))
		err = mini_isp_wait_for_event(MINI_ISP_RCV_SETSENSORMODE);

	return err;
}

/*
 *\brief Get Sensor Mode
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_get_sensor_mode(void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;

	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_get_sensor_mode);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode, NULL, 0);
	if (err  != ERR_SUCCESS)
		goto mast_camera_profile_cmd_get_sensor_mode_end;

	/* Get data from slave*/
	err = ispctrl_mast_recv_response_from_slave(devdata,
			(u8 *)&mast_sensors_info, para_size, true);
	if (err  != ERR_SUCCESS)
		goto mast_camera_profile_cmd_get_sensor_mode_end;

	/* copy to usr defined target addr*/
	memcpy(param, &mast_sensors_info, para_size);

mast_camera_profile_cmd_get_sensor_mode_end:
	return err;
}

/*
 *\brief Set Output Format
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_output_format(void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;

	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_set_output_format);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode, param,
						para_size);

	return err;
}

/*
 *\brief Set CP Mode
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_cp_mode(void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;

	misp_info("%s - enter", __func__);

	para_size = 0;

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
				param, para_size);
	if (err != ERR_SUCCESS)
		goto mast_camera_profile_cmd_set_cp_mode_end;

	err = mini_isp_wait_for_event(MINI_ISP_RCV_CPCHANGE);

mast_camera_profile_cmd_set_cp_mode_end:
	return err;
}

/*
 *\brief Set AE statistics
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_basic_setting_cmd_set_ae_statistics(void *devdata,
							u16 opcode, u8 *param)
{
	/*Error Code */
	errcode err = ERR_SUCCESS;
	u32 para_size = sizeof(struct isp_cmd_ae_statistics);
	misp_info("%s - enter", __func__);

	/*Send command to slave */
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
					param, para_size);
	return err;
}

/*
 *\brief Preview stream on off
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_preview_stream_on_off(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_preview_stream_on_off);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/*
 *\brief dual PD Y calculation weightings
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_dual_pd_y_cauculation_weightings(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_dual_pd_y_calculation_weightings);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/*
 *\brief LED power control
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_led_power_control(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_led_power_control);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/*
 *\brief Active AE
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_active_ae(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_active_ae);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/*
 *\brief Control AE on/off
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_isp_ae_control_on_off(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_isp_ae_control_on_off);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/*
 *\brief Set Frame Rate Limits
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_frame_rate_limits(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_frame_rate_limits);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/*
 *\brief Set period drop frame
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_period_drop_frame(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_period_drop_frame);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/*
 *\brief Set Max exposure
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_max_exposure(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_max_exposure);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/*
 *\brief Set target mean
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_target_mean(
						void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size;
	misp_info("%s - enter", __func__);

	para_size = sizeof(struct isp_cmd_target_mean);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
			param, para_size);
	return err;
}

/************************** End Of File *******************************/
