/*
 * File: ispctrl_if_master_local.h
 * Description: The structure and API definition ISP Ctrl IF Master Local
 *It,s a header file that define structure and API for ISP Ctrl IF Master Local
 * (C)Copyright altek Corporation 2013
 *
 *  2013/10/14; Aaron Chuang; Initial version
 *  2013/12/05; Bruce Chung; 2nd version
 */


/**
 * \file	 ispctrl_if_master_local.h
 * \brief	ISP Ctrl IF Master Local and API define
 * \version  0.01
 * \author   Aaron Chuang
 * \date	 2013/09/23
 * \see	  ispctrl_if_master_local.h
 */

#ifndef _ISPCTRLIF_MASTER_LOCAL_H_
#define _ISPCTRLIF_MASTER_LOCAL_H_

/**
 *@addtogroup ispctrl_if_master_local
 *@{
 */


/******Include File******/

#include "mtype.h"
#include "miniisp.h"


/******Public Constant Definition******/
#define ISP_REGISTER_PARA_SIZE  8
#define ISP_REGISTER_VALUE  4


/******Public Function Prototype******/

/*********************** System manage command ***********************/
/**
 *\brief Get status of last executed command
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_get_status_of_last_exec_command(
						void *devdata,
						u16 opcode, u8 *param);

/**
 *\brief Get error code CMD
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_get_error_code_command(void *devdata,
								u16 opcode,
								u8 *param);

/**
 *\brief Set ISP register
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_set_isp_register(void *devdata,
								u16 opcode,
								u8 *param);

/**
 *\brief Get ISP register
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_get_isp_register(void *devdata,
								u16 opcode,
								u8 *param);

/**
 *\brief Set common log level
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param

 *\return Error code
 */
errcode mast_sys_manage_cmd_set_comomn_log_level(void *devdata,
								u16 opcode,
								u8 *param);

/**
 *\brief Get chip test report
 *\param  devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_get_chip_test_report(void *devdata,
								u16 opcode,
								u8 *param);

void isp_mast_camera_profile_para_init(void);

/*********************** Camera profile command ***********************/
/**
 *\brief Set Sensor Mode
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_sensor_mode(void *devdata,
								u16 opcode,
								u8 *param);

/**
 *\brief Get Sensor Mode
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_get_sensor_mode(void *devdata,
								u16 opcode,
								u8 *param);

/**
 *\brief Set Output Format
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_output_format(void *devdata,
								u16 opcode,
								u8 *param);

/*
 *\brief Set CP Mode
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_cp_mode(void *devdata,
						u16 opcode, u8 *param);

/*
 *\brief Set AE statistics
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_basic_setting_cmd_set_ae_statistics(void *devdata,
							u16 opcode, u8 *param);

/**
 *\brief Preview stream on
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_preview_stream_on_off(
								void *devdata,
								u16 opcode,
								u8 *param);

/*
 *\brief dual PD Y calculation weightings
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_dual_pd_y_cauculation_weightings(
						void *devdata,
						u16 opcode, u8 *param);


/*
 *\brief led power control
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_led_power_control(
						void *devdata,
						u16 opcode, u8 *param);


/*
 *\brief Active AE
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_active_ae(
						void *devdata,
						u16 opcode, u8 *param);

/*
 *\brief ISP AE control on off
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_isp_ae_control_on_off(
						void *devdata,
						u16 opcode, u8 *param);

/*
 *\brief Set Frame Rate Limits
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_frame_rate_limits(
						void *devdata,
						u16 opcode, u8 *param);

/*
 *\brief Set Period Drop Frame
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_period_drop_frame(
						void *devdata,
						u16 opcode, u8 *param);

/*
 *\brief Set max exposure
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_max_exposure(
						void *devdata,
						u16 opcode, u8 *param);

/*
 *\brief Set target mean
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_camera_profile_cmd_set_target_mean(
						void *devdata,
						u16 opcode, u8 *param);

/* Bulk data command*/

/**
 *\brief Write Boot Code
 *\param devdata [In], misp device data
 *\param param [In], CMD param
 *\param filp [In], boot file
 *\return Error code
 */
errcode mast_bulk_data_cmd_write_boot_code(void *devdata,
								u8 *param,
							struct file *filp);


/**
 *\brief Write Basic Code
 *\param devdata [In], misp device data
 *\param param [In], CMD param
 *\param filp [In], basic file
 *\return Error code
 */
errcode mast_bulk_data_cmd_write_basic_code(void *devdata,
								u8 *param,
							struct file *filp);

/**
 *\brief Write Calibration Data
 *\param devdata [In], misp device data
 *\param param [In], CMD param
 *\param filp [In], sc table file
 *\return Error code
 */
errcode mast_bulk_data_cmd_write_calibration_data(void *devdata,
								u8 *param,
							struct file *filp);

/**
 *\brief Write Qmerge Data
 *\param devdata [In], misp device data
 *\param param [In], CMD param
 *\param filp [In], sc table file
 *\return Error code
 */
errcode mast_bulk_data_cmd_write_qmerge_data(void *devdata,
								u8 *param,
							struct file *filp);

/**
 *\brief Read Calibration Data
 *\param devdata [In], misp device data
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_bulk_data_cmd_read_memory_data(void *devdata,
							u8 *param);

/**
 *\brief Read common log data
 *\param devdata [In], misp device data
 *\param param [In], CMD param
 *\return Error code
 */
errcode bulk_data_cmd_read_common_log(void *devdata,
							u8 *param);

/*********************** Basic setting command ************************/
/**
 *\brief Set Depth 3A Info
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_basic_setting_cmd_set_depth_3a_info(void *devdata,
						u16 opcode,
						u8 *param);


/*
 *\brief Set Depth auto interleave mode
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_basic_setting_cmd_set_depth_auto_interleave_mode(
	void *devdata, u16 opcode, u8 *param);


/*
 *\brief Set projector interleave mode with depth type
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_basic_setting_cmd_set_interleave_mode_depth_type(
	void *devdata, u16 opcode, u8 *param);

/*
 *\brief Set depth polish level
 *\param devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_basic_setting_cmd_set_depth_polish_level(
	void *devdata, u16 opcode, u8 *param);
/*********************** operation command ***********************/
/**
 *\brief Mini ISP open
 *\param devdata [In], misp device data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_operation_cmd_miniisp_open(void *devdata,
						u16 opcode, u8 *param);


/****************************** End of File *********************************/

/**
 *@}
 */

#endif /* _ISPCTRLIF_MASTER_LOCAL_H_*/
