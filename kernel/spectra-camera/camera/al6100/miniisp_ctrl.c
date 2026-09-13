/*
 * File: miniisp_ctrl.c
 * Description: Mini ISP Ctrl sample codes
 *
 * (C)Copyright altek Corporation 2013
 *
 *  2013/10/14; Aaron Chuang; Initial version
 *  2013/12/05; Bruce Chung; 2nd version
 */

/******Include File******/
#include <linux/delay.h>

#include "include/miniisp_customer_define.h"
#include "include/ispctrl_if_master.h"
#include "include/miniisp_ctrl.h"
#include "include/altek_statefsm.h"
#include "include/error/altek_state_err.h"
#include "include/miniisp_chip_base_define.h"
/******Private Constant Definition******/
#define MINI_ISP_LOG_TAG	"[miniisp_ctrl]"

/******Private Function Prototype******/

static int load_code_task(void *data);
static u16 calibration_check_sum(u8 *input_buffer_addr, u16 input_buffer_size);

/******Private Type Declaration******/


/******Private Global Variable******/

static struct memmory_dump_hdr_info mem_dum_hdr_cfg = {0};
static struct common_log_hdr_info  com_log_hdr_cfg = {0};
static bool stop_to_log;

/*Command parameter buffer*/
static u8 cmd_param_buf[T_SPI_CMD_LENGTH];
static u8 rcv_cmd_param_buf[T_SPI_CMD_LENGTH];


static bool load_code_ready;

/******Public Global Variable******/


/******Public Function******/

/*************************************************************************/
/*operation cmd*/

/**
 *\brief Mini ISP open 0x4000
 *\param boot_code_file_name [In], Boot code filename
 *\param basic_code_file_name [In], Basic code filename
 *\param advanced_code_file_name [In], Advanced code filename
 *\param scenario_table_file_name [In], SC table filename
 *\param hdr_qmerge_data_file_name [In], HDR Qmerge data filename
 *\param irp0_qmerge_data_file_name [In], IRP0 Qmerge data filename
 *\param irp1_qmerge_data_file_name [In], IRP1 Qmerge data filename
 *\param pp_map_file_name [In], pp map filename
 *\return Error code
 */
errcode mini_isp_drv_open(char *boot_code_file_name,
				char *basic_code_file_name,
				char *advanced_code_file_name,
				char *scenario_table_file_name,
				char *hdr_qmerge_data_file_name,
				char *irp0_qmerge_data_file_name,
				char *irp1_qmerge_data_file_name,
				char *pp_map_file_name)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_MINIISPOPEN; /*0x4000*/
	u64 bootPath, basicPath, advPath, scenarioPath;
	u64 hdrPath, irp0Path, irp1Path, ppmapPath;

	bootPath  = (u64)boot_code_file_name;
	basicPath = (u64)basic_code_file_name;
	advPath   = (u64)advanced_code_file_name;
	scenarioPath  = (u64)scenario_table_file_name;
	hdrPath  = (u64)hdr_qmerge_data_file_name;
	irp0Path  = (u64)irp0_qmerge_data_file_name;
	irp1Path  = (u64)irp1_qmerge_data_file_name;
	ppmapPath  = (u64)pp_map_file_name;

	/* Command parameter buffer*/
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	misp_info("%s - start", __func__);
	/* Parameter 0 boot code filename*/
	memcpy(&cmd_param_buf[0], &bootPath, 8);
	/* Parameter 1 basic code filename*/
	memcpy(&cmd_param_buf[8], &basicPath, 8);
	/* Parameter 2 advanced code filename*/
	memcpy(&cmd_param_buf[16], &advPath, 8);
	/* Parameter 3 calibration filename(sc atable)*/
	memcpy(&cmd_param_buf[24], &scenarioPath, 8);
	/* Parameter 4 hdr qmerge data filename*/
	memcpy(&cmd_param_buf[32], &hdrPath, 8);
	/* Parameter 5 irp0 qmerge data filename*/
	memcpy(&cmd_param_buf[40], &irp0Path, 8);
	/* Parameter 6 irp1 qmerge data filename*/
	memcpy(&cmd_param_buf[48], &irp1Path, 8);
	/* Parameter 7 PP map filename*/
	memcpy(&cmd_param_buf[56], &ppmapPath, 8);


	/* mini ISP open*/
	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	if (err != ERR_SUCCESS)
		misp_err("%s open file failed. err: 0x%x", __func__, err);

	misp_info("%s - open files success", __func__);

	return err;

}

/*************************************************************************/
/*bulk cmd*/

/**
 *\brief Mini ISP write boot code 0x2008
 *\return Error code
 */
errcode mini_isp_drv_write_boot_code(void)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u16 opcode = ISPCMD_BULK_WRITE_BOOTCODE; /*0x2008*/

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);
	misp_info("%s write boot code state: %d", __func__, err);

	return err;
}

/**
 *\brief Mini ISP write basic code 0x2002
 *\return Error code
 */
errcode mini_isp_drv_write_basic_code(void)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u16 opcode = ISPCMD_BULK_WRITE_BASICCODE; /*0x2002*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	misp_info("%s write basic code state: %d", __func__, err);

	if (err == ERR_SUCCESS)
		dev_global_variable->now_state = 3;
	return err;
}


/**
 *\brief MiniISP Write Calibration Data   0x210B
 *\param info_id [In],		0   :  otp data
 *				1   :  packet data
 *				2   :  scenario table
 *				3   :  qmerge hdr
 *				4   :  qmerge irp0
 *				5   :  qmerge irp1
 *				6   :  PP map
 *				7   :  blending table
 *\param buf_addr [In], otp/packet data buffer start address
 *\param buf_len [In], otp/packet data buffer len
 *\return Error code
 */
errcode mini_isp_drv_write_calibration_data(u8 info_id, u8 *buf_addr,
					u32 buf_len)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u16 opcode = ISPCMD_BULK_WRITE_CALIBRATION_DATA; /*0x210B*/
	u16 chk_sum;
	u32 block_size = 384*1024;
	u8 *allocated_memmory = 0;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	/*
	 *misp_info("%s info_id %d  buf_addr %p buf_len %d",
	 *	__func__, info_id, buf_addr, buf_len);
	 */
	/* Parameter 0 Info ID*/
	cmd_param_buf[8] = info_id;
	if ((info_id >= 2) && (info_id < 7)) {
		err = ispctrl_if_mast_execute_cmd(opcode,
						cmd_param_buf);
	} else {
		/*  Request memory*/
		allocated_memmory = kzalloc(buf_len+T_SPI_CMD_LENGTH,
					GFP_KERNEL);
		if (!allocated_memmory) {
			err = ~ERR_SUCCESS;
			goto allocate_memory_fail;
		}
		memcpy(allocated_memmory + T_SPI_CMD_LENGTH, buf_addr,
			buf_len);
		memcpy(allocated_memmory, &buf_len, sizeof(u32));
		memcpy(allocated_memmory + 4, &block_size, sizeof(u32));
		memcpy(allocated_memmory + 8, &info_id, sizeof(u8));
		chk_sum = calibration_check_sum(
			allocated_memmory + T_SPI_CMD_LENGTH,
			buf_len);
		memcpy(allocated_memmory+9, &chk_sum, sizeof(u16));
		/*
		 *misp_info("%s Cal_param[0][1][2][3]:%02x %02x %02x %02x",
		 *		__func__, allocated_memmory[0],
		 *		allocated_memmory[1],
		 *		allocated_memmory[2],
		 *		allocated_memmory[3]);
		 *misp_info("%s Cal_param[4][5][6][7]:%02x %02x %02x %02x",
		 *		__func__, allocated_memmory[4],
		 *		allocated_memmory[5],
		 *		allocated_memmory[6],
		 *		allocated_memmory[7]);
		 *misp_info("%s Cal_param[8][9][10]:%02x %02x %02x",
		 *		__func__, allocated_memmory[8],
		 *		allocated_memmory[9],
		 *		allocated_memmory[10]);
		 */
		err = ispctrl_if_mast_execute_cmd(opcode,
					allocated_memmory);
		kfree(allocated_memmory);
	}
	misp_info("%s write calibration data state: %d", __func__, err);


	goto miniisp_drv_write_calibration_data_end;
allocate_memory_fail:
	misp_err("%s Allocate memory failed.", __func__);
	kfree(allocated_memmory);
miniisp_drv_write_calibration_data_end:

	return err;

}
EXPORT_SYMBOL(mini_isp_drv_write_calibration_data);

/**
 *\brief Read memory
 *\param start_addr [In]starting address
 *\param read_size [In]TotalReadSize
 *\return Error code
 */
errcode mini_isp_drv_read_memory(u32 start_addr, u32 read_size)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_BULK_READ_MEMORY;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	mem_dum_hdr_cfg.start_addr = start_addr;/*0x0;//0x9DC00;*/
	mem_dum_hdr_cfg.total_size = read_size;/*T_MEMSIZE;*/
	mem_dum_hdr_cfg.block_size = SPI_TX_BULK_SIZE;
	mem_dum_hdr_cfg.dump_mode = T_MEMDUMP_CPURUN;

	/*Copy it to transmission header*/
	memcpy(&cmd_param_buf[0], &mem_dum_hdr_cfg.start_addr,
		sizeof(struct memmory_dump_hdr_info));

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_read_memory);


/**
 *\brief Reading Common Log
 *\param stop [In], Stop to log flag
 *\return Error code
 */
errcode mini_isp_drv_read_com_log(bool stop)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_BULK_READ_COMLOG;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	/* Wait semaphore*/
	/*SEMAPHORE_LW_Wait( ISPCTRLIFMASTER_SEMAPHORE_LOGDUMP,*/
	/*	SEMAPHORE_WAITFOREVER );*/

	/* Force to stop log*/
	/*To inform isp to set log level as 0 for stoping log reight away*/
	if (stop)
		mini_isp_drv_set_com_log_level(0);

	if (!stop_to_log) {
		com_log_hdr_cfg.total_size = LEVEL_LOG_BUFFER_SIZE;
		com_log_hdr_cfg.block_size = SPI_TX_BULK_SIZE;

		/*Copy it to transmission header*/
		memcpy(&cmd_param_buf[0], &com_log_hdr_cfg.total_size,
				sizeof(struct common_log_hdr_info));

		err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

		/* Force to stop log*/
		if (stop)
			stop_to_log = true;
	}

	/* Post semaphore*/
	/*SEMAPHORE_LW_Post( ISPCTRLIFMASTER_SEMAPHORE_LOGDUMP );*/

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_read_com_log);

/*************************************************************************/
/*camera profile cmd*/

/**
 *\brief Set Sensor Mode	0x300A
 *\param sensor_on_off [In],sensor on/off
 *\param scenario_id[In], Scenario ID
 *\param mipi_tx_skew_enable[In],  mipi tx skew on(1)/off(0)
 *\param ae_weighting_table_index[In]
 *\return Error code
 */
errcode mini_isp_drv_set_sensor_mode(u8 sensor_on_off, u8 scenario_id,
		u8 mipi_tx_skew_enable, u8 ae_weighting_table_index,
		u8 merge_mode_enable)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	struct altek_statefsm *fsm;
	struct transferdata *param;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();

	fsm = get_mini_isp_fsm();

	param = kzalloc(sizeof(struct transferdata), GFP_KERNEL);

	param->opcode = ISPCMD_CAMERA_SET_SENSORMODE;
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	/* Parameter 0 sensor on/off*/
	cmd_param_buf[0] = sensor_on_off;
	/* Parameter 1 Scenario ID*/
	cmd_param_buf[1] = scenario_id;
	/* Parameter 2 mipi tx skew on/off*/
	cmd_param_buf[2] = mipi_tx_skew_enable;
	/* Parameter 3 ae weighting table index*/
	cmd_param_buf[3] = ae_weighting_table_index;
	/* Parameter 4 merge_mode_enable*/
	cmd_param_buf[4] = merge_mode_enable;
	/* Parameter 5 reserve*/
	cmd_param_buf[5] = 0;
	/* Parameter 6 reserve*/
	cmd_param_buf[6] = 0;
	param->data = cmd_param_buf;

	if (sensor_on_off == 0) {
		err = altek_statefsmispdrv_stop(fsm, param);
		if (err != 0) {
			misp_err("%s err, error code = %x",
				 __func__, err);
			kfree(param);
			return err;
		}
		dev_global_variable->now_state = 3;
	} else {
		err = altek_statefsmispdrv_scenario_chg(fsm, param);
		if (err != 0) {
			misp_err("%s err, error code = %x",
				 __func__, err);
			kfree(param);
			return err;
		}
		dev_global_variable->now_state = 4;
	}
	kfree(param);
	return err;

}
EXPORT_SYMBOL(mini_isp_drv_set_sensor_mode);

/*0x300B*/
errcode mini_isp_drv_get_sensor_mode(void)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_CAMERA_GET_SENSORMODE; /*0x300B*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;
	err = ispctrl_if_mast_execute_cmd(opcode, rcv_cmd_param_buf);

	return err;

}
EXPORT_SYMBOL(mini_isp_drv_get_sensor_mode);

/**
 *\brief Set Output format		0x300D
 *\param [In]depth_map_setting = resolution | opereation_mode
 *\resolution
 *\ 0: Disable depth function (Depth engine is disable)
 *\ 1: 180p
 *\ 2: 360p
 *\ 3: 720p
 *\ 4: 480p
 *\opereation_mode,
 *\ 0x00: DEPTH_BIT_DG_ONLY
 *\ 0x10: DEPTH_BIT_DP
 *\ 0x40: DEPTH_BIT_HIGH_DISTORTION_RATE
 *\
 *\param [In]depth_process_type: value 0x6 as reserve
 *\return Error code
 */
errcode mini_isp_drv_set_output_format(u8 depth_map_setting,
	u8 depth_process_type)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_CAMERA_SET_OUTPUTFORMAT; /*0x300D*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	/* Parameter 0*/
	cmd_param_buf[0] = depth_map_setting;
	/* Parameter 1*/
	cmd_param_buf[1] = depth_process_type;
	/* Parameter 2 reserve*/
	cmd_param_buf[2] = 0;

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;

}
EXPORT_SYMBOL(mini_isp_drv_set_output_format);

/**
 *\brief Set CP mode		0x300E
 *\return Error code
 */
errcode mini_isp_drv_set_cp_mode(void)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	struct transferdata *param;
	struct altek_statefsm *fsm;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	fsm = get_mini_isp_fsm();
	param = kzalloc(sizeof(struct transferdata), GFP_KERNEL);
	param->opcode = ISPCMD_CAMERA_SET_CP_MODE;
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	param->data = cmd_param_buf;

	err = altek_statefsmispdrv_enter_cp(fsm, param);
	kfree(param);
	if (err != 0) {
		misp_err("%s err, error code = %x",
				 __func__, err);
		return err;
	}
	dev_global_variable->now_state = 5;
	return err;

}
EXPORT_SYMBOL(mini_isp_drv_set_cp_mode);

/**
 *\brief Set AE statistics		0x300F
 *\param ae_statistics [In], ae statistics
 *\return Error code
 */
errcode mini_isp_drv_set_ae_statistics(
	struct isp_cmd_ae_statistics *ae_statistics)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	struct transferdata *param;
	struct altek_statefsm *fsm;

	fsm = get_mini_isp_fsm();
	param = kzalloc(sizeof(struct transferdata), GFP_KERNEL);
	param->opcode = ISPCMD_CAMERA_SET_AE_STATISTICS;
	param->data = (void *)ae_statistics;

	err = altek_statefsmispdrv_set_qp(fsm, param);
	kfree(param);
	if (err != 0)
		misp_err("%s err, error code = %x", __func__, err);

	return err;

}
EXPORT_SYMBOL(mini_isp_drv_set_ae_statistics);

/**
 *\brief Preview stream on/off		0x3010
 *\param tx0_stream_on_off [In], Tx0 stream on/off
 *\param tx1_stream_on_off [In], Tx1 stream on/off
 *\return Error code
 */
errcode mini_isp_drv_preview_stream_on_off(u8 tx0_stream_on_off,
				u8 tx1_stream_on_off)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	struct transferdata *param;
	struct altek_statefsm *fsm;

	fsm = get_mini_isp_fsm();
	param = kzalloc(sizeof(struct transferdata), GFP_KERNEL);
	param->opcode = ISPCMD_CAMERA_PREVIEWSTREAMONOFF;
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	/* Parameter 0 Tx0 stream on/off*/
	cmd_param_buf[0] = tx0_stream_on_off;
	/* Parameter 1 Tx1 stream on/off*/
	cmd_param_buf[1] = tx1_stream_on_off;
	param->data = cmd_param_buf;
	err = altek_statefsmispdrv_strm_on_off(fsm, param);
	kfree(param);
	if (err != 0)
		misp_err("%s err, error code = %x", __func__, err);

	return err;


}
EXPORT_SYMBOL(mini_isp_drv_preview_stream_on_off);

/**
 *\brief Dual PD Y Calcualtion Weighting		0x3011
 *\param isp_cmd_dual_pd_y_calculation_weightings [In],
   dual PD Y calculation weightings
 *\return Error code
 */
errcode mini_isp_drv_dual_pd_y_calculation_weighting(
	struct isp_cmd_dual_pd_y_calculation_weightings *calculation_weighting)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_CAMERA_DUALPDYCALCULATIONWEIGHT; /*0x3010*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if (dev_global_variable->now_state != 4)
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	memcpy(cmd_param_buf, calculation_weighting,
		sizeof(struct isp_cmd_dual_pd_y_calculation_weightings));

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_dual_pd_y_calculation_weighting);


/**
 *\brief LED power control		0x3012
 *\param projector_control_param [In],
 *\return Error code
 */
errcode mini_isp_drv_led_power_control(
	struct isp_cmd_led_power_control *projector_control_param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_LED_POWERCONTROL; /*0x3012*/

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	memcpy(cmd_param_buf, projector_control_param,
		sizeof(struct isp_cmd_led_power_control));

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_led_power_control);

/**
 *\brief Active AE		0x3013
 *\param active_ae_param [In],
 *\return Error code
 */
errcode mini_isp_drv_active_ae(
	struct isp_cmd_active_ae *active_ae_param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_CAMERA_ACTIVE_AE; /*0x3013*/

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	memcpy(cmd_param_buf, active_ae_param,
		sizeof(struct isp_cmd_active_ae));

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_active_ae);


/**
 *\brief  ISP AE control mode on off		0x3014
 *\param isp_ae_control_mode_on_off [In], 0:off 1:on
 *\return Error code
 */
errcode mini_isp_drv_isp_ae_control_mode_on_off(u8 isp_ae_control_mode_on_off)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_ISP_AECONTROLONOFF; /*0x3014*/

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	/* Parameter 0 isp_ae_control_mode_on_off*/
	cmd_param_buf[0] = isp_ae_control_mode_on_off;

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_isp_ae_control_mode_on_off);

/**
 *\brief  Set Frame Rate Limite		0x3015
 *\param set_frame_rate_param [In],
 *\return Error code
 */
errcode mini_isp_drv_set_frame_rate_limits(
	struct isp_cmd_frame_rate_limits *set_frame_rate_param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_CAMERA_SET_FRAMERATELIMITS; /*0x3015*/

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	memcpy(cmd_param_buf, set_frame_rate_param,
		sizeof(struct isp_cmd_frame_rate_limits));

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_frame_rate_limits);

/**
 *\brief  Set period drop frame		0x3016
 *\param set_period_drop_fram_param [In],
 *\return Error code
 */
errcode mini_isp_drv_set_period_drop_frame(
	struct isp_cmd_period_drop_frame *set_period_drop_fram_param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_CAMERA_SET_PERIODDROPFRAME; /*0x3016*/

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	memcpy(cmd_param_buf, set_period_drop_fram_param,
		sizeof(struct isp_cmd_period_drop_frame));

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_period_drop_frame);

/**
 *\brief Leave CP Mode
 *\using set sensor mode opcode :0x300A
 *\param sensor_on_off [In],sensor on/off
 *\param scenario_id[In], Scenario ID
 *\param mipi_tx_skew_enable[In],  mipi tx skew on(1)/off(0)
 *\param ae_weighting_table_index[In]
 *\return Error code
 */
errcode mini_isp_drv_leave_cp_mode(u8 sensor_on_off, u8 scenario_id,
		u8 mipi_tx_skew_enable, u8 ae_weighting_table_index,
		u8 merge_mode_enable)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	struct misp_global_variable *dev_global_variable;
	struct transferdata *param;
	struct altek_statefsm *fsm;

	fsm = get_mini_isp_fsm();
	dev_global_variable = get_mini_isp_global_variable();
	param = kzalloc(sizeof(struct transferdata), GFP_KERNEL);
	param->opcode = ISPCMD_CAMERA_SET_SENSORMODE;
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	/* Parameter 0 sensor on/off*/
	cmd_param_buf[0] = sensor_on_off;
	/* Parameter 1 Scenario ID*/
	cmd_param_buf[1] = scenario_id;
		/* Parameter 2 mipi tx skew on/off*/
	cmd_param_buf[2] = mipi_tx_skew_enable;
	/* Parameter 3 ae_weighting_table_index*/
	cmd_param_buf[3] = ae_weighting_table_index;
	/* Parameter 4 merge_mode_enable*/
	cmd_param_buf[4] = merge_mode_enable;
	/* Parameter 5 reserve*/
	cmd_param_buf[5] = 0;
	/* Parameter 6 reserve*/
	cmd_param_buf[6] = 0;
	param->data = cmd_param_buf;

	if (sensor_on_off) {
		err = altek_statefsmispdrv_leave_cp(fsm, param);
		if (err != 0) {
			misp_err("mini_isp_drv_leave_cp_mode err, errcode = %x",
				err);
			kfree(param);
			return err;
		}
		dev_global_variable->now_state = 4;
	} else {
		err = altek_statefsmispdrv_leave_cp_standy(fsm, param);
		if (err != 0) {
			misp_err("mini_isp_drv_leave_cp_mode err, errcode = %x",
				err);
			kfree(param);
			return err;
		}
		dev_global_variable->now_state = 3;
	}

	kfree(param);
	return err;

}
EXPORT_SYMBOL(mini_isp_drv_leave_cp_mode);

/*************************************************************************/
/*system cmd*/

/**
 *\brief Set ISP register	0x0100
 *\param a_udStartAddr [In], Reg start addr
 *\param reg_value [In], Reg value
 *\return Error code
 */
errcode mini_isp_drv_set_isp_register(u32 reg_start_addr, u32 reg_value)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_SYSTEM_SET_ISPREGISTER; /*0x0100*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	/* Reg start addr*/
	memcpy(&cmd_param_buf[0], &reg_start_addr, 4);
	/* Reg count*/
	memcpy(&cmd_param_buf[4], &reg_value, 4);

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_isp_register);


/**
 *\brief Set ISP register	0x0101
 *\param a_udStartAddr [In], Reg start addr
 *\param reg_count [In], Reg count
 *\return Error code
 */
errcode mini_isp_drv_get_isp_register(u32 reg_start_addr, u32 reg_count)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_SYSTEM_GET_ISPREGISTER; /*0x0101*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	/* Reg start addr*/
	memcpy(&cmd_param_buf[0], &reg_start_addr, 4);
	/* Reg count*/
	memcpy(&cmd_param_buf[4], &reg_count, 4);

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_get_isp_register);


errcode mini_isp_drv_set_com_log_level(u32 log_level)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_SYSTEM_SET_COMLOGLEVEL;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	memcpy(cmd_param_buf, &log_level, sizeof(u32));


	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_com_log_level);

/*0x0015*/
errcode mini_isp_drv_get_last_exec_cmd(void)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_SYSTEM_GET_STATUSOFLASTEXECUTEDCOMMAND; /*0x0015*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;
	err = ispctrl_if_mast_execute_cmd(opcode, rcv_cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_get_last_exec_cmd);

/*0x0016*/
errcode mini_isp_drv_get_err_code_cmd(void)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_SYSTEM_GET_ERRORCODE; /*0x0016*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if ((dev_global_variable->now_state != 3) &&
		(dev_global_variable->now_state != 4))
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;
	err = ispctrl_if_mast_execute_cmd(opcode, rcv_cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_get_err_code_cmd);

/*0x0016*/
errcode mini_isp_drv_get_err_code_cmd_in_irq(void)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	void *devdata = (void *)get_mini_isp_intf(MINIISP_I2C_TOP);
	u8 param[64];
	/* Parameter size*/
	/*get last ten error code and error status*/
	u32 para_size = (sizeof(errcode))*10;

	misp_err("%s - enter + + + + +", __func__);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, ISPCMD_SYSTEM_GET_ERRORCODE, param, 0);
	if (err != ERR_SUCCESS)
		goto mast_sys_manage_cmd_get_error_code_command_end;

	/* Get data from slave*/
	err = ispctrl_mast_recv_response_from_slave(devdata, param,
							para_size, false);
	if (err != ERR_SUCCESS)
		goto mast_sys_manage_cmd_get_error_code_command_end;

	misp_err("%s last error code %#02x %#02x %#02x %#02x", __func__,
							*(param), *(param+1), *(param+2), *(param+3));
mast_sys_manage_cmd_get_error_code_command_end:

	misp_err("%s - leave - - - - -", __func__);

	return err;
}

EXPORT_SYMBOL(mini_isp_drv_get_err_code_cmd_in_irq);


/**
 *\brief Get Chip test Report	0x010A
 *\return Error code
 */
errcode mini_isp_drv_get_chip_test_report(void)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_SYSTEM_GET_CHIPTESTREPORT; /*0x010A*/
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if (dev_global_variable->now_state != 4)
		return ERR_MINIISP_STATE_ERROR_SEQUENCE;
	err = ispctrl_if_mast_execute_cmd(opcode, rcv_cmd_param_buf);

	misp_info("%s chip test report: %x %x %x %x", __func__,
		rcv_cmd_param_buf[0], rcv_cmd_param_buf[1],
		rcv_cmd_param_buf[2], rcv_cmd_param_buf[3]);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_get_chip_test_report);

/*************************************************************************/
/*basic cmd*/

/**
 *\brief Set Depth 3A Information	0x10B9
 *\param depth_3a_info [In], ISP Depth 3A parameter
 *\return Error code
 */
errcode mini_isp_drv_set_depth_3a_info(
			struct isp_cmd_depth_3a_info *depth_3a_info)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	struct transferdata *param;
	struct altek_statefsm *fsm;

	fsm = get_mini_isp_fsm();
	param = kzalloc(sizeof(struct transferdata), GFP_KERNEL);
	param->opcode = ISPCMD_BASIC_SET_DEPTH_3A_INFO;
	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	/* Copy ISP Depth 3A Info*/
	memcpy(cmd_param_buf, depth_3a_info,
		sizeof(struct isp_cmd_depth_3a_info));
	param->data = cmd_param_buf;
	err = altek_statefsmispdrv_set_qp(fsm, param);
	kfree(param);
	if (err != 0)
		misp_err("%s err, error code = %x", __func__, err);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_depth_3a_info);


/**
 *\brief Set Depth auto interleave mode	0x10BC
 *\param depth_auto_interleave_param [In], ISP Depth auto interleave parameter
 *\return Error code
 */
errcode mini_isp_drv_set_depth_auto_interleave_mode(
	struct isp_cmd_depth_auto_interleave_param *depth_auto_interleave_param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_BASIC_SET_DEPTH_AUTO_INTERLEAVE_MODE; /*0x10BC*/

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);
	memcpy(cmd_param_buf, depth_auto_interleave_param,
		sizeof(struct isp_cmd_depth_auto_interleave_param));

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_depth_auto_interleave_mode);

/**
 *\brief Set Projector Interleave Mode with Depth Type	0x10BD
 *\param projector_interleave_mode_with_depth_type [In],
 *\      0: depth active, 1: depth passive
 *\return Error code
 */
errcode mini_isp_drv_projector_interleave_mode_depth_type(
	u8 projector_interleave_mode_with_depth_type)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_BASIC_SET_INTERLEAVE_MODE_DEPTH_TYPE; /*0x10BD*/

	cmd_param_buf[0] = projector_interleave_mode_with_depth_type;

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_projector_interleave_mode_depth_type);

/**
 *\brief Set Depth Polish LEVEL	0x10BE
 *\param depth_polish_level [In], 0~100
 *\return Error code
 */
errcode mini_isp_drv_set_depth_polish_level(u8 depth_polish_level)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_BASIC_SET_DEPTH_POLISH_LEVEL; /*0x10BE*/

	cmd_param_buf[0] = depth_polish_level;

	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_depth_polish_level);

/*************************************************************************/

/*currently not use
u16 mini_isp_drv_read_spi_status(void)
{
	return ispctrl_if_mast_read_spi_status();
}
EXPORT_SYMBOL(mini_isp_drv_read_spi_status);
*/

/**
 *\brief Write boot code and basic code
 *\param None
 *\return None
 */
int mini_isp_drv_boot_mini_isp(void)
{
	errcode err = ERR_SUCCESS;


	/* Write boot code*/
	err = mini_isp_drv_write_boot_code();
	if (err != ERR_SUCCESS)
		goto mini_isp_drv_boot_mini_isp_end;

	udelay(500);

	/* Write basic code*/
	err = mini_isp_drv_write_basic_code();

mini_isp_drv_boot_mini_isp_end:

	return err;

}
EXPORT_SYMBOL(mini_isp_drv_boot_mini_isp);

/**
 *\brief Open boot and FW file then write boot code and FW code
 *\param None
 *\return Error code
 */
errcode mini_isp_drv_load_fw(void)
{
	errcode err = ERR_SUCCESS;

	misp_info("mini_isp_drv_setting(0) mini_isp_drv_load_fw start");
	/* Clear load code ready flag;*/
	load_code_ready = false;
	/*spi isr task*/
	/*g_ptload_code_task = kthread_run(load_code_task, NULL, */
	/*		"miniISP_loadcode_thread");*/

	load_code_task(NULL);

	misp_info("mini_isp_drv_setting(0) mini_isp_drv_load_fw X");
	return err;

}
EXPORT_SYMBOL(mini_isp_drv_load_fw);

/**
 *\brief  Wait miniISP event
 *\param  e [In], MINI_ISP_EVENT
 *\return Errorcode
 */
int mini_isp_drv_wait_for_event(u16 e)
{
	return mini_isp_wait_for_event(e);
}
EXPORT_SYMBOL(mini_isp_drv_wait_for_event);


/**
 *\brief Set mode to miniISP
 *\param  mini_isp_mode [In], Select ISP MODE,
 *0:(isp already in state A)normal case load FW directly,
 *1 :(isp state inital in state E)set state E to state A
 *2 :(isp already in state A)set state A to state E for debug ,
 *3 :leave HW bypass
 *4 :Get Chip ID
 *else : None Support
 *\return Errorcode
 */
errcode mini_isp_drv_setting(u16 mini_isp_mode)
{
	errcode err = ERR_SUCCESS;
	struct misp_global_variable *dev_global_variable;
	struct altek_statefsm *fsm;

	fsm = get_mini_isp_fsm();
	dev_global_variable = get_mini_isp_global_variable();

	if (mini_isp_mode == MINI_ISP_MODE_NORMAL) {
		err = altek_statefsmispdrv_coldboot(fsm, 0);
		if (err != 0)
			goto mini_isp_drv_setting_err;
	} else if (mini_isp_mode == MINI_ISP_MODE_E2A) {
		/*isp, inital in E,*/
		mini_isp_e_to_a();
	} else if (mini_isp_mode == MINI_ISP_MODE_A2E) {
		mini_isp_a_to_e();
	} else if (mini_isp_mode == MINI_ISP_MODE_LEAVE_BYPASS) {
		err = altek_statefsmispdrv_leave_hwpt(fsm, 0);
		if (err != 0)
			goto mini_isp_drv_setting_err;
	} else if (mini_isp_mode == MINI_ISP_MODE_GET_CHIP_ID) {
		u8 buff_id[4];
		mini_isp_get_chip_id(CHIP_ID_ADDR, buff_id);
	} else if (mini_isp_mode == MINI_ISP_MODE_CHIP_INIT) {
		/*set some reg value to let it know should chang to A*/
		// For SPI_Nor, do not call this
		mini_isp_chip_init();
	} else {
		misp_err("%s err, none support setting", __func__);
	}
	return err;
mini_isp_drv_setting_err:
	misp_err("%s err, mini_isp_mode = %d, error code = %x", __func__,
			mini_isp_mode, err);
	return err;
}
EXPORT_SYMBOL(mini_isp_drv_setting);

/**
 *\brief Set miniISP using i2c sned signals
 *\param  mini_isp_i2c_use [In], use i2c send or not,
 *0:use spi
 *1:use i2c
 */
void mini_isp_drv_send_use_i2c(u8 mini_isp_i2c_use)
{
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();

	//dev_global_variable->i2c_enable = mini_isp_i2c_use;
}
EXPORT_SYMBOL(mini_isp_drv_send_use_i2c);

void mini_isp_drv_altek_i2c_mode_change(void)
{
	mini_isp_register_write(0xffea0100, 0x3201);
	mini_isp_register_write(0xffea0104, 0x3201);
}
EXPORT_SYMBOL(mini_isp_drv_altek_i2c_mode_change);

/**
 *\brief set bypass mode
 *\param  bypass_mode [In], Select bypass MODE,
 *\return Errorcode
 */
errcode mini_isp_drv_set_bypass_mode(u16 bypass_mode)
{
	errcode err = ERR_SUCCESS;
	struct altek_statefsm *fsm;

	fsm = get_mini_isp_fsm();

	if (!fsm) {
		misp_err("%s ERROR fsm == NULL", __func__);
		return err;
	}
	err = altek_statefsmispdrv_enter_hwpt(fsm, &bypass_mode);
	if (err != 0)
		goto mini_isp_drv_setting_err;

	return err;
mini_isp_drv_setting_err:
	misp_err("%s err, bypass_mode = %d, error code = %x", __func__,
			bypass_mode, err);
	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_bypass_mode);

/**
 *\brief set Max exposure
 *\param  paramlength [In], Select bypass MODE,
 *\return Errorcode
 */
errcode mini_isp_drv_set_max_exposure(u32 max_exposure)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_CAMERA_SET_MAX_EXPOSURE;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	memcpy(cmd_param_buf, &max_exposure, sizeof(u32));


	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_max_exposure);

/**
 *\brief set target mean
 *\param  paramlength [In], Select bypass MODE,
 *\return Errorcode
 */
errcode mini_isp_drv_set_target_mean(u16 target_mean)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Op code*/
	u16 opcode = ISPCMD_CAMERA_SET_AE_TARGET_MEAN;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();

	memset(cmd_param_buf, 0, T_SPI_CMD_LENGTH);

	memcpy(cmd_param_buf, &target_mean, sizeof(u16));


	err = ispctrl_if_mast_execute_cmd(opcode, cmd_param_buf);

	return err;
}
EXPORT_SYMBOL(mini_isp_drv_set_target_mean);

/******Private Function******/

static int load_code_task(void *data)
{
	/* Error code*/
	errcode err = ERR_SUCCESS;

	misp_info("misp_load_fw start");

	/* Reset mini-isp low for at least 200us, release to high for 20ms*/
	/*mini_isp_reset();*/

	/* Open boot file and FW file*/
	err = mini_isp_drv_open(BOOT_FILE_LOCATION,
				BASIC_FILE_LOCATION,
				ADVANCED_FILE_LOCATION,
				SCENARIO_TABLE_FILE_LOCATION,
				HDR_QMERGE_DATA_FILE_LOCATION,
				IRP0_QMERGE_DATA_FILE_LOCATION,
				IRP1_QMERGE_DATA_FILE_LOCATION,
				PP_MAP_FILE_LOCATION);
	if (err != ERR_SUCCESS)
		goto load_code_task_end;



	/* Write boot code and basic code*/
	err = mini_isp_drv_boot_mini_isp();
	if (err != ERR_SUCCESS)
		goto load_code_task_end;

	/* Set load code ready flag*/
	load_code_ready = true;

load_code_task_end:

	return (int)err;
}


static u16 calibration_check_sum(u8 *input_buffer_addr, u16 input_buffer_size)
{
	u16 i;
	u32 sum = 0;
	u16 sumvalue;

	/* calculating unit is 2 bytes*/
	for (i = 0; i < input_buffer_size; i++) {
		if (0 == (i % 2))
			sum += input_buffer_addr[i];
		else
			sum += (input_buffer_addr[i] << 8);
	}

	/* Do 2's complement*/
	sumvalue = (u16)(65536 - (sum & 0x0000FFFF));  /*get 2's complement*/

	return sumvalue;
}






/******End Of File******/
