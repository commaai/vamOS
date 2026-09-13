/*
 * File:  sys_managec_md.c
 * Description: System manage command
 *
 * (C)Copyright altek Corporation 2013
 *
 *  2013/10/14; Aaron Chuang; Initial version
 */
/* Standard C*/

/* Global Header Files*/
/*#include <osshell.h>*/

#include "include/isp_camera_cmd.h"
/* ISP Ctrl IF slave*/
#include "include/ispctrl_if_master.h"
/* ISP Ctrl IF slave error*/
#include "include/error/ispctrl_if_master_err.h"

/* Local Header Files*/
#include "include/ispctrl_if_master_local.h"



/******Include File******/



/******Private Constant Definition******/


#define MINI_ISP_LOG_TAG "[sys_manage_cmd]"


/******Private Type Declaration******/



/******Private Function Prototype******/

/******Private Global Variable******/



/******Public Global Variable******/

/******Public Function******/

/**
 *\brief Get Status of Last Executed Command
 *\param  devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_get_status_of_last_exec_command(
			void *devdata, u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Parameter size*/
	u32 para_size = sizeof(struct system_cmd_status_of_last_command);



	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode, param, 0);
	if (err  != ERR_SUCCESS)
		goto mast_sys_manage_cmd_get_status_of_last_exec_command_end;

	/* Get data from slave*/
	err = ispctrl_mast_recv_response_from_slave(devdata, param,
						para_size, true);
mast_sys_manage_cmd_get_status_of_last_exec_command_end:

	return err;


}

/**
 *\brief Get Error Code Command
 *\param  devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_get_error_code_command(void *devdata,
							u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Parameter size*/
	/*get last ten error code and error status*/
	u32 para_size = (sizeof(errcode))*10;

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode, param, 0);
	if (err  != ERR_SUCCESS)
		goto mast_sys_manage_cmd_get_error_code_command_end;

	/* Get data from slave*/
	err = ispctrl_mast_recv_response_from_slave(devdata, param,
							para_size, false);
	if (err  != ERR_SUCCESS)
		goto mast_sys_manage_cmd_get_error_code_command_end;

	misp_err("%s last error code %x %x %x %x", __func__, *(param),
		*(param+1), *(param+2), *(param+3));
mast_sys_manage_cmd_get_error_code_command_end:

	return err;
}

/**
 *\brief Set ISP register
 *\param devdata [In], misp_data
 *\param opCode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_set_isp_register(void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Parameter size*/
	u32 para_size = sizeof(struct system_cmd_isp_register);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
						param, para_size);
	return err;
}

/**
 *\brief Get ISP register
 *\param  devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_get_isp_register(void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	/* Parameter size*/
	u32 para_size = sizeof(struct system_cmd_isp_register);
	/* Response size*/
	u32 *reg_count = (u32 *)&param[4];



	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
						param, para_size);
	if (err  != ERR_SUCCESS)
		goto mast_sys_manage_cmd_get_isp_register_end;

	/* Update response size*/
	para_size = sizeof(struct system_cmd_isp_register) + *reg_count*4;

	/* Get data from slave*/
	err = ispctrl_if_mast_recv_isp_register_response_from_slave(
								devdata,
								param,
								&para_size,
								*reg_count);
mast_sys_manage_cmd_get_isp_register_end:

	return err;
}

/**
 *\brief Set common log level
 *\param  devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_set_comomn_log_level(void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size = sizeof(struct system_cmd_common_log_level);

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
					param, para_size);
	return err;
}

/**
 *\brief Get chip test report
 *\param  devdata [In], misp_data
 *\param opcode [In], Operation code
 *\param param [In], CMD param
 *\return Error code
 */
errcode mast_sys_manage_cmd_get_chip_test_report(void *devdata,
						u16 opcode, u8 *param)
{
	/* Error Code*/
	errcode err = ERR_SUCCESS;
	u32 para_size = 0;

	/* Send command to slave*/
	err = ispctrl_mast_send_cmd_to_slave(devdata, opcode,
						param, para_size);
	if (err  != ERR_SUCCESS)
		goto mast_sys_manage_cmd_get_chip_test_report_end;

	/* Update response size*/
	para_size = ReportRegCount;

	/* Get data from slave*/
	err = ispctrl_mast_recv_response_from_slave(devdata, param,
						para_size, true);
mast_sys_manage_cmd_get_chip_test_report_end:

	return err;
}

/******End Of File******/
