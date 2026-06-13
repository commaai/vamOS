/*
 * File: miniisp_ctrl_intf.c
 * Description: mini ISP control cmd interface. use for handling the control cmds instead of debug cmds
 *
 * (C)Copyright altek Corporation 2018
 *
 *  2018/08/28; PhenixChen; Initial version
 */

/******Include File******/
//#include <miniISP/miniISP_ioctl.h>
#include <miniISP/miniISP_ioctl32.h>
#include <asm-generic/uaccess.h> // copy_*_user()
#include "include/miniisp.h"
#include "include/miniisp_ctrl.h" // mini_isp_drv_setting()
#include "include/isp_camera_cmd.h" // MINI_ISP_MODE_E2A, MINI_ISP_MODE_NORMAL
#include "include/ispctrl_if_master.h" // ispctrl_if_mast_execute_cmd()
#include "include/miniisp_customer_define.h"
#include "include/miniisp_ctrl_intf.h"

/******Private Constant Definition******/
#define MINI_ISP_LOG_TAG	"[miniisp_ctrl_intf]"

/******Private Function Prototype******/

/******Public Function Prototype******/

/******Private Global Variable******/
enum miniisp_mode_state {
	ISP_POWERED_OFF = 0,
	ISP_POWERED_ON,
	ISP_PURE_BYPASS,
	ISP_CP_MODE,
};

static int g_isMiniISP_Powered = ISP_POWERED_OFF;
static int g_isMiniISP_FWLoaded;

// ALTEK_AL6100_CHI >>>
static int g_isMiniISP_RunAll = 0;
struct file *internal_file[OTHER_MAX];
// ALTEK_AL6100_CHI >>>


int handle_ControlFlowCmd_II(u16 miniisp_op_code, u8 *param)
{
	int retval = 0;
	misp_info("%s - enter", __func__);

	switch (miniisp_op_code) {
	case ISPCMD_LOAD_FW:
		//misp_info("%s - opcode[%u], size[%u], [%u] [%u] [%u] [%u]", __func__, config.opcode, config.size, param[0], param[1], param[2], param[3]);

		misp_info("%s - ISPCMD_LOAD_FW", __func__);
		//open boot and FW file then write boot code and FW code

		if (g_isMiniISP_Powered == ISP_POWERED_OFF) { //For AL6100 state machine, open state can't be opened again.
			mini_isp_poweron();
			g_isMiniISP_Powered = ISP_POWERED_ON;
		}
		if (g_isMiniISP_FWLoaded == 0) { // FW  can only be loaded just once if AL6100 didn't be reset.
			mini_isp_drv_setting(MINI_ISP_MODE_GET_CHIP_ID);

#ifndef AL6100_SPI_NOR
			mini_isp_drv_setting(MINI_ISP_MODE_CHIP_INIT); //if boot form SPI NOR, do not call this
#endif
			mini_isp_drv_setting(MINI_ISP_MODE_E2A);
			mini_isp_drv_setting(MINI_ISP_MODE_NORMAL);
			g_isMiniISP_FWLoaded = 1;
		}
		break;
	case ISPCMD_PURE_BYPASS:
		misp_info("%s - ISPCMD_PURE_BYPASS", __func__);

		if (g_isMiniISP_Powered != ISP_POWERED_OFF) {
			misp_info("%s - miniisp is not in poweroff state", __func__);
			break;
		}

		g_isMiniISP_Powered = ISP_PURE_BYPASS;
		mini_isp_poweron();
		mini_isp_drv_setting(MINI_ISP_MODE_GET_CHIP_ID);
		mini_isp_drv_set_bypass_mode(1);

		break;
	case ISPCMD_POWER_OFF:
		misp_info("%s - ISPCMD_POWER_OFF", __func__);

		if(g_isMiniISP_Powered == ISP_POWERED_ON || g_isMiniISP_Powered == ISP_PURE_BYPASS){
			mini_isp_poweroff();
			g_isMiniISP_Powered = ISP_POWERED_OFF;
		} else {
			misp_info("%s - miniisp is not in poweron state", __func__);
		}

		break;
	default:
		retval = ispctrl_if_mast_execute_cmd(miniisp_op_code, param);
		break;
	}
	misp_info("%s - leave", __func__);
	return retval;
}

//TODO: Need to solve the kernel panic >>>

long handle_ControlFlowCmd(unsigned int cmd, unsigned long arg)
{
	long retval = 0;
	//struct miniISP_cmd_config32 config = {};

	void    *mbuf = NULL;
	void	*parg = (void *)arg;
	unsigned int n = _IOC_SIZE(cmd);
	//u8 param[4];

	misp_info("%s - enter", __func__);

	if (!access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd))) {
        retval = -EFAULT;
        goto done;
    }

	misp_info("%s - VERIFY_READ OK!", __func__);

	if (!access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd))) {
        retval = -EFAULT;
        goto done;
    }

	misp_info("%s - VERIFY_WRITE OK!", __func__);

	mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
	if (NULL == mbuf)
		return -ENOMEM;

	parg = mbuf;

	if (copy_from_user(parg, (void __user *)arg, n)) {
	    retval = -EFAULT;
	    goto done;
	 }

	misp_info("%s - n = %u, [%x] [%x] [%x]", __func__, n, *((u8 *)mbuf), *(((u8 *)mbuf) + 1), *(((u8 *)mbuf) + 2));

/*
	misp_info("%s - before copy from user opcode[%u], size[%u]", __func__, config.opcode, config.size);
	if (copy_from_user(&config, (void __user *)arg, sizeof(struct miniISP_cmd_config32))) {
		misp_info("%s - arg copy_from_user fail", __func__);
		retval = -EFAULT;
		goto done;
	}
	memcpy(&config2, &config,sizeof(struct miniISP_cmd_config32));
*/
/*
	if (copy_from_user(param, (void __user *)(unsigned long)(config.param), config.size)) {
		misp_info("%s - param copy_from_user fail", __func__);
		retval = -EFAULT;
		goto done;
	}
*/
	switch (cmd) {
	case IOCTL_ISP_LOAD_FW32:
		//misp_info("%s - opcode[%u], size[%u], [%u] [%u] [%u] [%u]", __func__, config.opcode, config.size, param[0], param[1], param[2], param[3]);

		misp_info("%s - IOCTL_ISP_LOAD_FW", __func__);
		//open boot and FW file then write boot code and FW code

		if (g_isMiniISP_Powered == ISP_POWERED_OFF) { //For AL6100 state machine, open state can't be opened again.
			mini_isp_poweron();
			g_isMiniISP_Powered = ISP_POWERED_ON;
		}
		if (g_isMiniISP_FWLoaded == 0) { // FW  can only be loaded just once if AL6100 didn't be reset.
			mini_isp_drv_setting(MINI_ISP_MODE_GET_CHIP_ID);

#ifndef AL6100_SPI_NOR
			mini_isp_drv_setting(MINI_ISP_MODE_CHIP_INIT); //if boot form SPI NOR, do not call this
#endif
			mini_isp_drv_setting(MINI_ISP_MODE_E2A);
			mini_isp_drv_setting(MINI_ISP_MODE_NORMAL);
			g_isMiniISP_FWLoaded = 1;
		}
		break;
	case IOCTL_ISP_PURE_BYPASS32:
		misp_info("%s - IOCTL_ISP_PURE_BYPASS", __func__);

		if (g_isMiniISP_Powered != ISP_POWERED_OFF) {
			misp_info("%s - miniisp is not in poweroff state", __func__);
			break;
		}

		g_isMiniISP_Powered = ISP_PURE_BYPASS;
		mini_isp_poweron();
		mini_isp_drv_setting(MINI_ISP_MODE_GET_CHIP_ID);
		mini_isp_drv_set_bypass_mode(1);

		break;
	case IOCTL_ISP_POWER_OFF32:
		misp_info("%s - IOCTL_ISP_POWER_OFF32", __func__);

		if(g_isMiniISP_Powered == ISP_POWERED_ON || g_isMiniISP_Powered == ISP_PURE_BYPASS){ //Powered on
			mini_isp_poweroff();
			g_isMiniISP_Powered = ISP_POWERED_OFF; //Powered off
		} else {
			misp_info("%s - miniisp is not in poweron state", __func__);
		}

		break;
	default:
		misp_info("%s - UNKNOWN CMD[%x], while IOCTL_ISP_LOAD_FW = [%lx], pointer size[%lx]", __func__, cmd, IOCTL_ISP_LOAD_FW, sizeof(void *));
		retval = -ENOTTY;
		break;
	}

done:
	misp_info("%s - leave", __func__);
	return retval;
}
//TODO: Need to solve the kernel panic <<<

// ALTEK_AL6100_CHI >>>
void mini_isp_other_drv_open(char *file_name, u8 type) {

	/* Error Code*/
	errcode err = ERR_SUCCESS;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	misp_info("%s filepath : %s", __func__, file_name);

	internal_file[type] = filp_open(file_name, O_RDONLY, 0644);
	set_fs(oldfs);

	if (IS_ERR(internal_file[type])) {
		err = PTR_ERR(internal_file[type]);
		misp_err("%s open file failed. err: %x", __func__, err);
	} else {
		misp_info("%s open file success!", __func__);
	}
}

void mini_isp_other_drv_read(struct file *filp, u8 type) {
	static u8 *calibration_data_buf_addr;
	errcode err = ERR_SUCCESS;
	u32 filesize;
	off_t currpos;
	mm_segment_t oldfs;
	loff_t offset;

	if (filp == NULL) {
		misp_err("%s - file didn't exist.", __func__);
		err = ~ERR_SUCCESS;
		goto read_calibration_data_end;
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	/*get the file size*/
	currpos = vfs_llseek(filp, 0L, SEEK_END);
	if (currpos == -1) {
		set_fs(oldfs);
		misp_err("%s  llseek end failed", __func__);
		err = ~ERR_SUCCESS;
		goto read_calibration_data_end;
	}

	filesize = (u32)currpos;
	/*misp_info("%s  filesize : %u", __func__, filesize);*/

	currpos = vfs_llseek(filp, 0L, SEEK_SET);
	if (currpos == -1) {
		set_fs(oldfs);
		misp_err("%s  llseek set failed", __func__);
		err = ~ERR_SUCCESS;
		goto read_calibration_data_end;
	}

	/*Request memory*/
	calibration_data_buf_addr = kzalloc(filesize, GFP_KERNEL);
	if (!calibration_data_buf_addr) {
		err = ~ERR_SUCCESS;
		kfree(calibration_data_buf_addr);
		goto read_calibration_data_end;
	}

	/*read the header info (first 16 bytes in the data)*/
	offset = filp->f_pos;
	err = vfs_read(filp, calibration_data_buf_addr, filesize,
		&offset);
	set_fs(oldfs);
	if (err == -1) {
		misp_err("%s Read file failed.", __func__);
		/*close the file*/
		filp_close(filp, NULL);
		kfree(calibration_data_buf_addr);
		goto read_calibration_data_end;
	}
	filp->f_pos = offset;
	vfs_llseek(filp, 0L, SEEK_SET);

	// write_calibration_data
	err = mini_isp_drv_write_calibration_data(type, calibration_data_buf_addr, filesize);
	if(err == 0)
		goto done;
	else
		goto write_calibration_data_end;

read_calibration_data_end:
	misp_err("%s read_calibration_data_fail", __func__);
write_calibration_data_end:
	misp_err("%s write_calibration_data_fail", __func__);
done:
	misp_info("%s read and write calibration_data success!! ", __func__);
}


long handle_ControlFlowCmd_CHI(unsigned int cmd, unsigned long arg) {

	long retval = 0;
	struct isp_cmd_active_ae active_ae;
	struct isp_cmd_led_power_control control_param;
	memset(&active_ae, 0, sizeof(struct isp_cmd_active_ae));
	memset(&control_param, 0, sizeof(struct isp_cmd_led_power_control));

	misp_info("%s - enter", __func__);

	if (!access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd))) {
		retval = -EFAULT;
		goto done;
    }

	misp_info("%s - VERIFY_READ OK!", __func__);

	if (!access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd))) {
		retval = -EFAULT;
		goto done;
    }

	misp_info("%s - VERIFY_WRITE OK!", __func__);
	switch (cmd) {
		case IOCTL_ISP_RUN_TASK_START:
			misp_info("%s - IOCTL_ISP_RUN_TASK_START", __func__);
			if(g_isMiniISP_RunAll == 0) {
				g_isMiniISP_RunAll = 1;
				// open auto depth mode

				mini_isp_poweron();

				if(0 != mini_isp_drv_setting(MINI_ISP_MODE_GET_CHIP_ID)){
					misp_err("get chip id failed \n");
				}
				if(0 != mini_isp_drv_setting(MINI_ISP_MODE_CHIP_INIT)){ //if boot form SPI NOR, do not call this
					misp_err("chip init failed \n");
				}
				if(0 != mini_isp_drv_setting(MINI_ISP_MODE_E2A)){
					misp_err("change MINI_ISP_MODE_E2A failed failed \n");
				}
				if(0 != mini_isp_drv_setting(MINI_ISP_MODE_NORMAL)){
					misp_err("misp_load_fw failed \n");
				}


				mini_isp_other_drv_open(IQCALIBRATIONDATA_FILE_LOCATION, IQ_CODE);
				mini_isp_other_drv_read(internal_file[IQ_CODE], IQ_CODE);

				mini_isp_other_drv_open(DEPTHPACKDATA_FILE_LOCATION, DEPTH_CODE);
				mini_isp_other_drv_read(internal_file[DEPTH_CODE], DEPTH_CODE); // Depth calibration data

				mini_isp_drv_write_calibration_data(2, NULL, 0); // Scenario table
				mini_isp_drv_write_calibration_data(3, NULL, 0); // HDR Qmerge
				mini_isp_drv_write_calibration_data(4, NULL, 0); // IRP0 Qmerge
				mini_isp_drv_write_calibration_data(5, NULL, 0); // IRP1 Qmerge
				// mini_isp_drv_write_calibration_data(7, NULL, 0); // Blending table for ground depth
				// mini_isp_drv_write_calibration_data(8, NULL, 0); // Depth Qmerge

				// set depth output resolution
				//mini_isp_drv_set_output_format(18, 6);// set depth output resolution
                                mini_isp_drv_set_output_format(19, 7);// set depth output resolution

				active_ae.active_ae = 1;
				active_ae.f_number_x1000 = 2000;
				mini_isp_drv_active_ae(&active_ae);

				mini_isp_drv_set_sensor_mode(1,4,0,0,0);  // Set sensor mode
				//projector control
				control_param.led_on_off = 3;
				control_param.led_power_level = 255;
				control_param.control_projector_id = 0;
				mini_isp_drv_led_power_control(&control_param);
				mini_isp_drv_preview_stream_on_off(1,1); // open preview
				mini_isp_drv_isp_ae_control_mode_on_off(1);

			}
			break;
		case IOCTL_ISP_RUN_TASK_STOP:
			misp_info("%s - IOCTL_ISP_RUN_TASK_STOP", __func__);
			if(g_isMiniISP_RunAll == 1) {
				g_isMiniISP_RunAll = 0;
				// close auto depth mode
				mini_isp_drv_preview_stream_on_off(0,0); // close preview
				mini_isp_drv_isp_ae_control_mode_on_off(0);
				mini_isp_drv_led_power_control(&control_param);
				mini_isp_drv_set_sensor_mode(0,0,0,0,0);  // Set sensor mode
				mini_isp_drv_active_ae(&active_ae);
				mini_isp_drv_set_output_format(0,0); // set depth output resolution:
                                                      // 0: Disable depth function (Depth engine is disable)
				mini_isp_poweroff();				
			}
		break;
		default:
			misp_info("%s - UNKNOWN CMD[%x]", __func__, cmd);
			retval = -ENOTTY;
		break;
	}
done:
	misp_info("%s - leave", __func__);
	return retval;
}
// ALTEK_AL6100_CHI >>>
