/*
 * File: miniisp_ctrl.h
 * Description: The structure and API definition mini ISP Ctrl
 * It is a header file that define structure and API for mini ISP Ctrl
 * (C)Copyright altek Corporation 2013
 *
 *  2013/10/14; Aaron Chuang; Initial version
 *  2013/12/05; Bruce Chung; 2nd version
 */


#ifndef _MINIISP_CTRL_H_
#define _MINIISP_CTRL_H_

/**
 *@addtogroup MiniISPCtrl
 *@{
 */

/******Include File******/

#include "mtype.h"
#include "isp_camera_cmd.h"
#include "error.h"
#include "moduleid_pj.h"

/******Global define******/
#define DEPTH_NONE                  0x0  /*No depth*/
#define DEPTH_BIT_RES_180           0x1  /*180p*/
#define DEPTH_BIT_RES_360           0x2  /*360p*/
#define DEPTH_BIT_RES_720           0x3  /*720p*/
#define DEPTH_BIT_RES_480           0x4  /*480p*/

#define DEPTH_BIT_DG_ONLY           0x00
#define DEPTH_BIT_DP_SPARSE_1       0x10
#define DEPTH_BIT_DP_SPARSE_2       0x20
#define DEPTH_BIT_DP_SPARSE_3       0x30
#define DEPTH_BIT_DP_DENSE          0x40
#define DEPTH_BIT_MASK_TYPE         0x70
#define DEPTH_BIT_RESV              0x80

/******Public Function Prototype******/
extern struct altek_statefsm *get_mini_isp_fsm(void);

/**
 *\brief Mini ISP open
 *\param boot_code_file_name		[In], Boot code filename
 *\param basic_code_file_name	   [In], Basic code filename
 *\param advanced_code_file_name	[In], Advanced code filename
 *\param scenario_table_file_name	 [In], Sc table filename
 *\param hdr_qmerge_file_name	 [In], hdr qmerge filename
 *\param irp0_qmerge_file_name	 [In], irp0 qmerge filename
 *\param irp1_qmerge_file_name	 [In], irp1 qmerge filename
  *\param pp_map_file_name	 [In], PP map filename
 *\return Error code
 *\image html BootSeq.jpg
 */
extern errcode mini_isp_drv_open(char *boot_code_file_name,
				char *basic_code_file_name,
				char *advanced_code_file_name,
				char *scenario_table_file_name,
				char *hdr_qmerge_file_name,
				char *irp0_qmerge_file_name,
				char *irp1_qmerge_file_name,
				char *pp_map_file_name
				);

/**
 *\brief Mini ISP write boot code
 *\param n/a
 *\return Error code
 */
extern errcode mini_isp_drv_write_boot_code(void);

/**
 *\brief Mini ISP write basic code
 *\param n/a
 *\return Error code
 */
extern errcode mini_isp_drv_write_basic_code(void);

/**
 *\brief Mini ISP Write Calibration Data 0x210B
 *\param info_id [In],		0   :  otp data
 *				1   :  packet data
 *				2   :  scenario table
 *				3   :  hdr
 *				4   :  irp0
 *				5   :  irp1
 *				6   :  PP map
 *				7   :  blending table
 *\param buf_addr [In], otp/packet data buffer start address
 *\param buf_len [In], otp/packet data buffer len
 *\return Error code
 */
extern errcode mini_isp_drv_write_calibration_data(u8 info_id,
					u8 *buf_addr, u32 buf_len);


/**
 *\brief Set Sensor Mode	0x300A
 *\param sensor_on_off [In],sensor on/off
 *\param scenario_id[In], Scenario ID
 *\param mipi_tx_skew_enable[In],  mipi tx skew on(1)/off(0)
 *\param ae_weighting_table_index[In]
 *\param merge_mode_enable[In]
 *\return Error code
 */
extern errcode mini_isp_drv_set_sensor_mode(u8 sensor_on_off,
					u8 scenario_id,
					u8 mipi_tx_skew_enable,
					u8 ae_weighting_table_index,
					u8 merge_mode_enable);

/**
 *\brief Set Output Format	0x300D
 *\param depth_map_setting [In]
 *\param depth_process_type[In] value 0x6 as reserve
 *\return Error code
 */
extern errcode mini_isp_drv_set_output_format(u8 depth_map_setting, u8 depth_process_type);

/**
 *\brief Set CP Mode	0x300E
 *\return Error code
 */
extern errcode mini_isp_drv_set_cp_mode(void);

/**
 *\brief Leave CP Mode
 *\using set sensor mode opcode :0x300A
 *\param sensor_on_off [In],sensor on/off
 *\param scenario_id[In], Scenario ID
 *\param mipi_tx_skew_enable[In],  mipi tx skew on(1)/off(0)
 *\param ae_weighting_table_index[In]
 *\param merge_mode_enable[In]
 *\return Error code
 */
extern errcode mini_isp_drv_leave_cp_mode(u8 sensor_on_off, u8 scenario_id,
		u8 mipi_tx_skew_enable, u8 ae_weighting_table_index, u8 merge_mode_enable);

/**
 *\brief Set AE statistics		0x300F
 *\param gr_channel_weight [In],
 *\param gb_channel_weight [In],
 *\param r_channel_weight [In],
 *\param b_channel_weight [In],
 *\param shift_bits [In],
 *\return Error code
 */
extern errcode mini_isp_drv_set_ae_statistics(
	struct isp_cmd_ae_statistics *ae_statistics);

/**
 *\brief Preview stream on/off		0x3010
 *\param tx0_stream_on_off [In], Tx0 stream on/off
 *\param tx1_stream_on_off [In], Tx1 stream on/off
 *\return Error code
 */
errcode mini_isp_drv_preview_stream_on_off(u8 tx0_stream_on_off,
				u8 tx1_stream_on_off);

/**
 *\brief Dual PD Y Calcualtion Weighting		0x3011
 *\param isp_cmd_dual_pd_y_calculation_weightings [In],
   dual PD Y calculation weightings
 *\return Error code
 */
errcode mini_isp_drv_dual_pd_y_calculation_weighting(
	struct isp_cmd_dual_pd_y_calculation_weightings *calculation_weighting);

/**
 *\brief LED power control		0x3012
 *\param projector_control_param,
 *\return Error code
 */
errcode mini_isp_drv_led_power_control(
	struct isp_cmd_led_power_control *projector_control_param);

/**
 *\brief Active AE		0x3013
 *\param active_ae [In],
 *\return Error code
 */
extern errcode mini_isp_drv_active_ae(
	struct isp_cmd_active_ae *active_ae_param);

/**
 *\brief  ISP AE control mode on off		0x3014
 *\param isp_ae_control_mode_on_off [In], 0:off 1:on
 *\return Error code
 */
extern errcode mini_isp_drv_isp_ae_control_mode_on_off(
	u8 isp_ae_control_mode_on_off);

/**
 *\brief  Set Frame Rate Limite		0x3015
 *\param set_frame_rate_param [In],
 *\return Error code
 */
extern errcode mini_isp_drv_set_frame_rate_limits(
	struct isp_cmd_frame_rate_limits *set_frame_rate_param);

extern errcode mini_isp_drv_get_sensor_mode(void);

extern errcode mini_isp_drv_get_last_exec_cmd(void);

extern errcode mini_isp_drv_get_err_code_cmd(void);

extern errcode mini_isp_drv_get_err_code_cmd_in_irq(void);

extern u16 mini_isp_drv_read_spi_status(void);


/**
 *\brief Set ISP register	0x0100
 *\param reg_start_addr [In], Reg start addr
 *\param reg_value [In], Reg value
 *\return Error code
 */
extern errcode mini_isp_drv_set_isp_register(u32 reg_start_addr,
					u32 reg_value);


/**
 *\brief Get ISP register
 *\param reg_start_addr [In], Reg start addr
 *\param reg_count  [In], Reg count
 *\return Error code
 */
extern errcode mini_isp_drv_get_isp_register(u32 reg_start_addr,
					u32 reg_count);

/**
 *\brief Get Chip test Report	0x010A
 *\return Error code
 */
extern errcode mini_isp_drv_get_chip_test_report(void);

/**
 *\brief Set Depth 3A Information	0x10B9
 *\param depth_3a_info [In], Depth 3A parameter
 *\return Error code
 */
extern errcode mini_isp_drv_set_depth_3a_info(
	struct isp_cmd_depth_3a_info *depth_3a_info);

/**
 *\brief Set Depth auto interleave mode	0x10BC
 *\param depth_auto_interleave_param [In], ISP Depth auto interleave parameter
 *\return Error code
 */
extern errcode mini_isp_drv_set_depth_auto_interleave_mode(
	struct isp_cmd_depth_auto_interleave_param
	*depth_auto_interleave_param);

/**
 *\brief Set Projector Interleave Mode with Depth Type	0x10BD
 *\param mini_isp_drv_projector_interleave_mode_depth_type [In],
 *\      0: depth active, 1: depth passive
 *\return Error code
 */
extern errcode mini_isp_drv_projector_interleave_mode_depth_type(
	u8 projector_interleave_mode_with_depth_type);

/**
 *\brief Set Depth Polish LEVEL	0x10BE
 *\param depth_polish_level [In], 0~100
 *\return Error code
 */
extern errcode mini_isp_drv_set_depth_polish_level(
	u8 depth_polish_level);
/**
 *\brief Reading Common Log
 *\param stop [In], Stop to log flag
 *\return Error code
 */
extern errcode mini_isp_drv_read_com_log(bool stop);

/**
 *\brief Read memory
 *\param start_addr [In]starting address
 *\param read_size [In]TotalReadSize
 *\return Error code
 */
extern errcode mini_isp_drv_read_memory(u32 start_addr, u32 read_size);

extern errcode mini_isp_drv_set_com_log_level(u32 log_level);

/**
 *\brief  Master boot miniISP
 *\param  e [In], MINI_ISP_EVENT
 *\return Errorcode
 */
extern int mini_isp_drv_wait_for_event(u16 e);

extern errcode mini_isp_drv_setting(u16 mini_isp_mode);

extern errcode mini_isp_drv_set_bypass_mode(u16 bypass_mode);

extern errcode mini_isp_drv_set_max_exposure(u32 max_exposure);

extern errcode mini_isp_drv_set_target_mean(u16 target_mean);

extern void mini_isp_drv_send_use_i2c(u8 mini_isp_i2c_use);

extern void mini_isp_drv_altek_i2c_mode_change(void);

extern errcode mini_isp_drv_load_fw(void);

extern void mini_isp_reset(void);


/** \brief  Master boot miniISP
 *\param  None
 *\return None
 */
extern int mini_isp_drv_boot_mini_isp(void);

/******End of File******/

/**
 *@}
 */

#endif /* _MINIISP_CTRL_H_*/
