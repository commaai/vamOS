/*
 * File: isp_camera_cmd.h
 * Description: The structure and API definition ISP camera command
 *  It is a header file that define structure and API for ISP camera command
 * (C)Copyright Altek Digital Inc. 2013
 *
 * History
 *   2013/09/18; Aaron Chuang; Initial version
 *   2013/12/05; Bruce Chung; 2nd version
 */


#ifndef _ISPCAMERA_CMD_H_
#define _ISPCAMERA_CMD_H_

/*
 *@addtogroup ISPCameraCmd
 *@{
 */

/******Include File******/


#include "mtype.h"


/******Public Constant Definition******/

#define T_MEMSIZE (936*1024)
#define T_SPI_CMD_LENGTH 64

#define ISPCMD_DUMMYBYTES 4
#define FWVER_INFOSIZE_MAX 34

#define ISPCMD_LENFLDBYTES 2
#define ISPCMD_OPCODEBYTES 2
#define ReportRegCount 27
#define ISPCMD_CKSUMBYTES  2

/*length field size = 2, opcode field size = 2 */
#define ISPCMD_CMDSIZE ((ISPCMD_LENFLDBYTES) + (ISPCMD_OPCODEBYTES))

/*length field size = 2, opcode field size = 2, dummy bytes = 4*/
#define ISPCMD_CMDSIZEWDUMMY (ISPCMD_LENFLDBYTES+\
				ISPCMD_OPCODEBYTES+\
				ISPCMD_DUMMYBYTES)

#define ISPCMD_FILENAME_SIZE 15

#define ISPCMD_EXEBIN_ADDRBYTES 4
#define ISPCMD_EXEBIN_TOTALSIZEBYTES 4
#define ISPCMD_EXEBIN_BLOCKSIZEBYTES 4
#define ISPCMD_EXEBIN_CKSUMBYTES 4
#define ISPCMD_EXEBIN_INFOBYTES (ISPCMD_EXEBIN_ADDRBYTES+\
				ISPCMD_EXEBIN_TOTALSIZEBYTES+\
				ISPCMD_EXEBIN_BLOCKSIZEBYTES+\
				ISPCMD_EXEBIN_CKSUMBYTES)

/* Definition for Error code array number*/
#define MAX_RECERRORCODE_NUM 10

/*log buffer size*/
#define	LEVEL_LOG_BUFFER_SIZE (1024*4)



/*calibration profile*/
#define ISPCMD_CAMERA_SET_SENSORMODE 0x300A
#define ISPCMD_CAMERA_GET_SENSORMODE 0x300B
#define ISPCMD_CAMERA_SET_OUTPUTFORMAT 0x300D
#define ISPCMD_CAMERA_SET_CP_MODE 0x300E
#define ISPCMD_CAMERA_SET_AE_STATISTICS 0x300F
#define ISPCMD_CAMERA_PREVIEWSTREAMONOFF 0x3010
#define ISPCMD_CAMERA_DUALPDYCALCULATIONWEIGHT 0x3011
#define ISPCMD_LED_POWERCONTROL 0x3012
#define ISPCMD_CAMERA_ACTIVE_AE 0x3013
#define ISPCMD_ISP_AECONTROLONOFF 0x3014
#define ISPCMD_CAMERA_SET_FRAMERATELIMITS 0x3015
#define ISPCMD_CAMERA_SET_PERIODDROPFRAME 0x3016
#define ISPCMD_CAMERA_SET_MAX_EXPOSURE 0x3017
#define ISPCMD_CAMERA_SET_AE_TARGET_MEAN 0x3018

/* Bulk Data*/
#define ISPCMD_BULK_WRITE_BASICCODE 0x2002
#define ISPCMD_BULK_WRITE_BOOTCODE 0x2008
#define ISPCMD_BULK_READ_MEMORY 0x2101
#define ISPCMD_BULK_READ_COMLOG 0x2102
#define ISPCMD_BULK_WRITE_CALIBRATION_DATA 0x210B

/*basic setting*/
#define ISPCMD_BASIC_SET_DEPTH_3A_INFO 0x10B9
#define ISPCMD_BASIC_SET_DEPTH_AUTO_INTERLEAVE_MODE 0x10BC
#define ISPCMD_BASIC_SET_INTERLEAVE_MODE_DEPTH_TYPE 0x10BD
#define ISPCMD_BASIC_SET_DEPTH_POLISH_LEVEL 0x10BE
/*system cmd*/
#define ISPCMD_SYSTEM_GET_STATUSOFLASTEXECUTEDCOMMAND 0x0015
#define ISPCMD_SYSTEM_GET_ERRORCODE 0x0016
#define ISPCMD_SYSTEM_SET_ISPREGISTER 0x0100
#define ISPCMD_SYSTEM_GET_ISPREGISTER 0x0101
/*#define ISPCMD_SYSTEM_SET_DEBUGCMD 0x0104*/
#define ISPCMD_SYSTEM_SET_COMLOGLEVEL 0x0109
#define ISPCMD_SYSTEM_GET_CHIPTESTREPORT 0x010A

/*operarion code*/
#define ISPCMD_MINIISPOPEN 0x4000

/* constants for memory dump mode*/
#define T_MEMDUMP_CPURUN 0
#define T_MEMDUMP_CPUHALT 1

/******Public Type Declaration******/
/*mode id*/
/*define for ISP decide mode*/
enum mini_isp_mode {
	MINI_ISP_MODE_NORMAL = 0x0000,
	MINI_ISP_MODE_E2A = 0x0001,
	MINI_ISP_MODE_A2E = 0x0002,
	MINI_ISP_MODE_LEAVE_BYPASS = 0x0003,
	MINI_ISP_MODE_GET_CHIP_ID = 0x0004,
	MINI_ISP_MODE_SET_CP_MODE = 0x0005,
	MINI_ISP_MODE_LEAVE_CP_MODE = 0x0006,
	MINI_ISP_MODE_CHIP_INIT = 0x0007,
	MINI_ISP_MODE_BYPASS = 0x1000,
	MINI_ISP_MODE_QUARTER_BYPASS = 0x1001,
};

#pragma pack(1)
struct transferdata {
	u16 opcode;
	void *data;
};

/*camera profile cmd use structure*/
/**
 *@struct isp_cmd_set_sensor_mode(opcode:300A)
 *@brief ISP master cmd for set sensor mode
 */
#pragma pack(1)
struct isp_cmd_set_sensor_mode {
	u8 sensor_on_off;
	u8 scenario_id;
	u8 mipi_tx_skew;
	u8 ae_weighting_table_index;
	u8 merge_mode_enable;        //0: disable,1: enable
	u8 reserve[2];
};

/**
 *@struct isp_cmd_get_sensor_mode(opcode:300B)
 *@brief ISP master cmd for get sensor mode
 */
#pragma pack(1)
struct isp_cmd_get_sensor_mode {
	bool on; /* On/off flag*/
	u8 scenario_id; /* scenario mode*/
	u8 reserve[4];
};

/**
 *@struct isp_cmd_set_output_format(opcode:300D)
 *@brief ISP master cmd for set depth output format
 */
#pragma pack(1)
struct isp_cmd_set_output_format {
	u8 depth_size;
	u8 reserve[2];
};

/**
 *@struct isp_cmd_ae_statistics(opcode:300F)
 *@brief ae statistics
 */
#pragma pack(1)
struct isp_cmd_ae_statistics {
	u16 gr_channel_weight;
	u16 gb_channel_weight;
	u16 r_channel_weight;
	u16 b_channel_weight;
	u8 shift_bits;
};

/**
 *@struct isp_cmd_preview_stream_on_off(opcode:3010)
 *@briefISP master cmd for control tx stream on or off
 */
#pragma pack(1)
struct isp_cmd_preview_stream_on_off {
	u8 tx0_stream_on_off;
	u8 tx1_stream_on_off;
	u8 reserve;
};

/**
 *@struct isp_cmd_dual_pd_y_calculation_weightings(opcode:3011)
 *@briefISP master cmd for control dual pd y calculation weightings
 */
#pragma pack(1)
struct isp_cmd_dual_pd_y_calculation_weightings {
	u8 y_weight_gr_short;
	u8 y_weight_gr_long;
	u8 y_weight_r_short;
	u8 y_weight_r_long;
	u8 y_weight_b_short;
	u8 y_weight_b_long;
	u8 y_weight_gb_short;
	u8 y_weight_gb_long;
	u8 y_sum_right_shift;
};

/**
 *@struct isp_cmd_led_power_control(opcode:3012)
 *@briefISP master cmd for control led power
 */
#pragma pack(1)
struct isp_cmd_led_power_control {
	u8 led_on_off;                   //0: off   1: always on   2: AP control pulse mode  3: AHCC control pulse mode
	u8 led_power_level;              //0~255
	u8 control_projector_id;         //0: projector   1:illuminator
	u32 delay_after_sof;             //when led_on_off = 2, use this param to set delay time between SOF and start pulse time
	u32 pulse_time;                  //when led_on_off = 2, use this param to set pulse time
	u8  control_mode;                //when led_on_off = 2, use this param to decide if pulse time met next SOF need triggler imediately or not
	u8  pulse_mode_skip_frame;       //when led_on_off = 3, use this param to control if one frame pulse, how many followed frames need skeep pulse
	u8 rolling_shutter;              //when led_on_off = 3, use this param to let projector/illuminator konw hoe exposure deal
};

/**
 *@struct isp_cmd_active_ae(opcode:3013)
 *@briefISP master cmd for avtive AE
 */
#pragma pack(1)
struct isp_cmd_active_ae {
	u8 active_ae;
	u16 f_number_x1000;
};

/**
 *@struct isp_cmd_isp_ae_control_on_off(opcode:3014)
 *@briefISP master cmd for isp AE control on off
 */
#pragma pack(1)
struct isp_cmd_isp_ae_control_on_off {
	u8 isp_ae_control_mode_on_off;
};


/**
 *@struct isp_cmd_frame_rate_limits(opcode:3015)
 *@brief set frame rate limits
 */
#pragma pack(1)
struct isp_cmd_frame_rate_limits {
	u16 main_min_framerate_x100;
	u16 main_max_framerate_x100;
	u16 sub_min_framerate_x100;
	u16 sub_max_framerate_x100;
};

/**
 *@struct isp_cmd_period_drop_frame(opcode:3016)
 *@brief set period drop frame
 */
#pragma pack(1)
struct isp_cmd_period_drop_frame {
	u8 period_drop_type;//0: no drop, 1: drop active, 2; drop passive
};

/**
 *@struct isp_cmd_max_exporsure(opcode:3017)
 *@brief set max exporsure
 */
#pragma pack(1)
struct isp_cmd_max_exposure {
	u32 max_exporsure;//us
};

/**
 *@struct isp_cmd_target_mean(opcode:3018)
 *@brief set target mean
 */
#pragma pack(1)
struct isp_cmd_target_mean {
	u16 target_mean;//0~255
};


/*basic cmd use structure*/

/**
 *@struct isp_cmd_depth_3a_info(opcode:10B9)
 *@brief depth 3A information
 */
#pragma pack(1)
struct isp_cmd_depth_3a_info {
	u16 hdr_ratio;
	u32 main_cam_exp_time;
	u16 main_cam_exp_gain;
	u16 main_cam_amb_r_gain;
	u16 main_cam_amb_g_gain;
	u16 main_cam_amb_b_gain;
	u16 main_cam_iso;
	u16 main_cam_bv;
	s16 main_cam_vcm_position;
	u8  main_cam_vcm_status;
	u32 sub_cam_exp_time;
	u16 sub_cam_exp_gain;
	u16 sub_cam_amb_r_gain;
	u16 sub_cam_amb_g_gain;
	u16 sub_cam_amb_b_gain;
	u16 sub_cam_iso;
	u16 sub_cam_bv;
	s16 sub_cam_vcm_position;
	u8  sub_cam_vcm_status;
	u16 main_cam_isp_d_gain;
	u16 sub_cam_isp_d_gain;
	s16 hdr_long_exp_ev_x1000;
	s16 hdr_short_exp_ev_x1000;
	u16 ghost_prevent_low;
	u16 ghost_prevent_high;
	u8 depth_dp_active;
};

/**
 *@struct isp_cmd_depth_auto_interleave_param(opcode:10BC)
 *@brief depth Interleave mode param
 */
#pragma pack(1)
struct isp_cmd_depth_auto_interleave_param {
	u8 depth_interleave_mode_on_off;/*1: on, 0: off*/
	u8 skip_frame_num_after_illuminator_pulse;
	u8 projector_power_level;/*0~255*/
	u8 illuminator_power_level;/*0~255*/
};

/**
 *@struct isp_cmd_interleave_mode_depth_type(opcode:10BD)
 *@brief interleave mode projector with depth type
 */
#pragma pack(1)
struct isp_cmd_interleave_mode_depth_type {
	u8 projector_interleave_mode_with_depth_type;/*1: passive, 0: active, default active*/
};

/**
 *@struct isp_cmd_depth_polish_level(opcode:10BE)
 *@brief set depth polish level
 */
#pragma pack(1)
struct isp_cmd_depth_polish_level {
	u8 depth_polish_level;/*0~100*/
};
/*system cmd use structure*/

/* ISP operation mode*/
enum ispctrl_operation_mode {
	ISPCTRL_TEST_MODE,
	ISPCTRL_STILLLV_MODE,
	ISPCTRL_VIDEOLV_MODE,
	ISPCTRL_CONCURRENT_MODE,
	ISPCTRL_BYPASS_MODE,
	ISPCTRL_POWERDOWN_MODE
};

/**
 *@struct system_cmd_isp_mode(opcode:0010 and 0011)
 *@brief depth 3A information
 */
#pragma pack(1)
struct system_cmd_isp_mode {
	u8 isp_mode;  /*ispctrl_operation_mode*/
};

/**
 *@struct system_cmd_status_of_last_command(opcode:0015)
 *@brief last execution command status
 */
#pragma pack(1)
struct system_cmd_status_of_last_command {
	u16 opcode;
	u32 execution_status;
};

/**
 *@struct system_cmd_isp_register(opcode:0100 and 0101)
 *@brief use on set/get isp register
 */
#pragma pack(1)
struct system_cmd_isp_register {
	u32 address;
	u32 value;
};

/**
 *@struct system_cmd_debug_mode(opcode:0104)
 *@brief use on get irq status
 */
#pragma pack(1)
struct system_cmd_debug_mode {
	u8 on;
};

/**
 *@struct system_cmd_common_log_level(opcode:0109)
 *@brief use on set common log level
 */
#pragma pack(1)
struct system_cmd_common_log_level {
	u32 log_level;
};



/*bulk cmd use structure*/

/**
 *@struct memmory_dump_hdr_info
 *@brief use on isp memory read
 */
#pragma pack(1)
struct memmory_dump_hdr_info {
	u32 start_addr;
	u32 total_size;
	u32 block_size;
	u32 dump_mode;
};


/**
 *@struct common_log_hdr_info
 *@brief Bulk data for memory dump header
 */
#pragma pack(1)
struct common_log_hdr_info {
	u32 total_size;
	u32 block_size;
};




/******Public Function Prototype******/


/******End of File******/

/**
 *@}
 */

#endif /* _ISPCAMERA_CMD_H_*/
