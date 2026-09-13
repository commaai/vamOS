/*
 * File: miniisp_top.c
 * Description: Mini ISP sample codes
 *
 * (C)Copyright altek Corporation 2017
 *
 *  2017/04/11; LouisWang; Initial version
 */

 /******Include File******/
 /* Linux headers*/
#include <linux/delay.h>
#include <linux/buffer_head.h>

#include "include/miniisp.h"
#include "include/miniisp_ctrl.h"
#include "include/miniisp_customer_define.h"
#include "include/miniisp_chip_base_define.h"

#include "include/error/miniisp_err.h"

/****************************************************************************
*                        Private Constant Definition                        *
****************************************************************************/

#define MINI_ISP_LOG_TAG "[miniisp_top]"
static bool irqflag;
static u32 event = MINI_ISP_RCV_WAITING;
static u32 current_event = MINI_ISP_RCV_WAITING;
static DECLARE_WAIT_QUEUE_HEAD(WAITQ);
/**********************************************************************
*                         Public Function                             *
**********************************************************************/
int mini_isp_get_chip_id(u32 mini_isp_reg_addr, u8 *id_buf)
{
	int status = ERR_SUCCESS;
	u8 *send_buffer;
	u8 *recv_buffer;
	u8 ctrlbyte = CTRL_BYTE_REGRD;
	u32 address = mini_isp_reg_addr;
	struct misp_data *devdata;
	struct misp_global_variable *dev_global_variable;
	u8 send_buffer_value[64] = {0};
	u8 recv_buffer_value[64] = {0};
	u32 rx_dummy_len;

	misp_info("%s - enter", __func__);

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);

	send_buffer = send_buffer_value;
	recv_buffer = recv_buffer_value;
	rx_dummy_len = devdata->rx_dummy_len;

	memcpy(send_buffer, &ctrlbyte, 1);
	memcpy(send_buffer + 1, &address, 4);

	status = devdata->intf_fn->read(devdata,
					send_buffer_value, EMODE_TXCMD_LEN,
					recv_buffer_value, rx_dummy_len + 4); //4bytes chip id value

	if (status) {
		misp_err("%s - sync error: status=%d", __func__, status);
		goto mini_isp_get_chip_id_end;
	}

	if (rx_dummy_len > 0 &&
		mini_isp_check_rx_dummy(&recv_buffer, rx_dummy_len)) {
		misp_err("[miniISP]Can't get chip ID");
		goto mini_isp_get_chip_id_end;
	}

	misp_err("[miniISP]Get Chip ID %x %x %x %x",
				*recv_buffer, *(recv_buffer + 1),
				*(recv_buffer + 2), *(recv_buffer + 3));

mini_isp_get_chip_id_end:
	return status;
}


void mini_isp_register_write(u32 reg_addr, u32 reg_new_value)
{
	u8 *send_buffer;
	u8 ctrlbyte = CTRL_BYTE_REGWR;
	u32 address = reg_addr, value = reg_new_value;
	struct misp_data *devdata;
	//u8 send_buffer_value[64];
	struct misp_global_variable *dev_global_variable;
	misp_info("%s - enter, reg_addr[%#08x], write_value[%#08x]",
		__func__, reg_addr, reg_new_value);

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);

	send_buffer = devdata->tx_buf;

	memcpy(send_buffer, &ctrlbyte, 1);
	memcpy(send_buffer + 1, &address, 4);
	memcpy(send_buffer + 5, &value, 4);
	dev_global_variable->before_booting = 1;
	devdata->intf_fn->write(devdata, devdata->tx_buf, devdata->rx_buf, 9);
	dev_global_variable->before_booting = 0;
}

void mini_isp_register_write_one_bit(u32 reg_addr, u8 bit_offset, u8 bit_value)
{
	u8 *send_buffer;
	u32 reg_value;
	u8 ctrlbyte = CTRL_BYTE_REGWR;
	u32 address = reg_addr;
	struct misp_data *devdata;
	//u8 send_buffer_value[64];
	struct misp_global_variable *dev_global_variable;
	misp_info("%s - enter", __func__);

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);
	send_buffer = devdata->tx_buf;

	mini_isp_register_read(reg_addr, &reg_value);

	if (bit_value)
		reg_value |= 1UL << bit_offset;
	else
		reg_value &= ~(1UL << bit_offset);

	memcpy(send_buffer, &ctrlbyte, 1);
	memcpy(send_buffer + 1, &address, 4);
	memcpy(send_buffer + 5, &reg_value, 4);
	dev_global_variable->before_booting = 1;
	devdata->intf_fn->write(devdata, devdata->tx_buf, devdata->rx_buf, 9);
	dev_global_variable->before_booting = 0;
}


void mini_isp_register_write_bit_field(u32 reg_addr, u32 mask, u32 mask_value)
{
	u8 *send_buffer;
	u32 reg_value;
	u8 ctrlbyte = CTRL_BYTE_REGWR;
	u32 address = reg_addr, value;
	struct misp_data *devdata;
	//u8 send_buffer_value[64];
	struct misp_global_variable *dev_global_variable;
	misp_info("%s - enter", __func__);

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);
	send_buffer = devdata->tx_buf;

	mini_isp_register_read(reg_addr, &reg_value);
	value = (reg_value & ~mask) | mask_value;

	memcpy(send_buffer, &ctrlbyte, 1);
	memcpy(send_buffer + 1, &address, 4);
	memcpy(send_buffer + 5, &value, 4);
	dev_global_variable->before_booting = 1;
	devdata->intf_fn->write(devdata, devdata->tx_buf, devdata->rx_buf, 9);
	dev_global_variable->before_booting = 0;
}

void mini_isp_e_to_a(void)
{
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	misp_info("mini_isp_drv_setting(1) mini_isp_e_to_a");
	mini_isp_register_write(0xffef0240, 0x0);
	mdelay(1);
	dev_global_variable->altek_spi_mode = ALTEK_SPI_MODE_A;
}


void mini_isp_a_to_e(void)
{
	u8 *send_buffer;
	u8 ctrlbyte = CTRL_BYTE_A2E;
	struct misp_data *devdata;
	//u8 send_buffer_value[64];
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);
	send_buffer = devdata->tx_buf;

	memcpy(send_buffer, &ctrlbyte, 1);
	devdata->intf_fn->write(devdata, devdata->tx_buf, devdata->rx_buf, 1);
	dev_global_variable->altek_spi_mode = ALTEK_SPI_MODE_E;
	misp_info("mini_isp_drv_setting(2) mini_isp_a_to_e");
}

void mini_isp_chip_init(void)
{
	misp_info("%s - enter", __func__);
	mini_isp_register_write(0xffe40050, 0x1);
	mini_isp_register_write(0xffef00a4, 0xe);
	udelay(70);
	mini_isp_register_write(0xffef00a0, 0xe);
	mini_isp_register_write(0xffe81080, 0x8);
	mini_isp_register_write(0xffef0090, 0x30079241);
	mini_isp_register_write(0xffe800c4, 0x0);
	udelay(100);
	misp_info("%s - leave", __func__);
}

void mini_isp_cp_mode_suspend_flow(void)
{
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	misp_info("%s - enter", __func__);

	/*# AP disable OCRAM0*/
	mini_isp_register_write_one_bit(0xffe609f4, 0, 1);/* # ocr0_disable*/

	/*# 2.AP reset ARC5*/
	mini_isp_register_write_one_bit(0xffe800c4, 1, 1);/* # base_ck_in*/

	/*# 3.AP reset modules(including arbiter bus)*/
	mini_isp_register_write_one_bit(0xffe801a4, 1, 1);/* # standby_top*/
	mini_isp_register_write_one_bit(0xffe80104, 1, 1);/* # arb_bus_stdby_0*/

	/*# 4.AP stop clock of modules(including arbiter bus, OCRAM0) and ARC5*/
	mini_isp_register_write_one_bit(0xffe800c4, 0, 1);/* # base_ck_in*/
	mini_isp_register_write_one_bit(0xffe801a4, 0, 1);/* # standby_top*/
	mini_isp_register_write_one_bit(0xffe80104, 0, 1);/* # arb_bus_stdby_0*/
	mini_isp_register_write_one_bit(0xffe80104, 2, 1);/* # ocram_0*/

	/*# 5.AP isolate standby power domain*/
	mini_isp_register_write_one_bit(0xffef00a0, 0, 1);/* # iso_pd_standby*/

	/*# 6.AP power down standby power domain*/
	mini_isp_register_write_one_bit(0xffef00a4, 0, 1);/* # psw_pd_standby*/

	/*# AP issue global reset of standby power domain*/
	/* # pd_rstn_standby_top*/
	mini_isp_register_write_one_bit(0xffe81080, 3, 0);
	/*# 7.AP keep PLL factor then disable PLLs (Keep PLL_fix)
	 *# ClkGen have kept PLL factor so don't keep here
	 *#pll_var_ext_dat = []
	 *# address of PLL factor
	 *#pll_var_ext_addr = [0xffe81120, 0xffe81124, 0xffe81128,
	 *#                    0xffe8112c, 0xffe81130, 0xffe81134,
	 *#                    0xffe81140, 0xffe81144, 0xffe81148,
	 *#                    0xffe8114c, 0xffe81150, 0xffe81154 ]
	 *#for addr in pll_var_ext_addr:
	 *#    (retsts, retdat) = altek_get_register(interface,
	 *#                        handle, I2C_SLAVE_ID, addr)
	 *#    pll_var_ext_dat.append(retdat)
	 *# bit 11: reset_pll_ext,
	 *# bit 10: reset_pll_var,
	 *# bit 3: disable_pll_ext,
	 *# bit 2: disable_pll_var
	 */
	/*# 7.AP keep PLL factor then disable PLLs
	 *(disable PLL_fix, PLL_ext and PLL_var)
	 */
	mini_isp_register_write_bit_field(0xffe81004, 0x0e0e, 0x0e0e);
	/*# AP do something*/
	mdelay(10);
	misp_info("%s - leave", __func__);
}

void mini_isp_cp_mode_resume_flow(void)
{
	u32 magic_number;
	struct misp_global_variable *dev_global_variable;
	errcode status = ERR_SUCCESS;

	misp_info("%s - enter", __func__);
	dev_global_variable = get_mini_isp_global_variable();
	/*# 1.AP check magic number*/
	mini_isp_register_read(0xffef0008, &magic_number);/* # magic_number*/

	/*
	 *# 2.AP check if magic number is equal to RESUME (0x19961224)
	 *then jump to step 4
	 */
	if (magic_number != 0x19961224) {
		/*
		 *# 3.Else exit resume flow
		 *(Note: AP can decide what to do.
		 *Ex: Toggle system reset of SK1 to reboot SK1)
		 */
		misp_err("%s - resume fail!, magic number not equal!", __func__);
		return;
	}

	/*# 4.AP enable/configure PLLs*/
	/*
	 *# bit 11: reset_pll_ext,
	 *bit 10: reset_pll_var,
	 *bit 3: disable_pll_ext,
	 *bit 2: disable_pll_var
	 */
	/*# ClkGen have kept PLL factor so don't set here*/
	/*#for idx, addr in enumerate(pll_var_ext_addr):*/
	/*
	 *#altek_set_register(interface, handle,
	 *I2C_SLAVE_ID, addr, pll_var_ext_dat[idx])
	 */
	/* # 5us, TO-DO: depend on IP spec*/
	/*
	 *# bit 11: reset_pll_ext,
	 *bit 10: reset_pll_var,
	 *bit 3: disable_pll_ext,
	 *bit 2: disable_pll_var
	 */

	// Fix CP mode resume bug >>>
	//mini_isp_register_write_bit_field(0xffe81004, 0x0e0e, 0x0e0c);
	//udelay(10);/* # 10us, TO-DO: depend on IP spec*/
	// Fix CP mode resume bug <<

	//mini_isp_register_write_bit_field(0xffe81004, 0x0e0e, 0x0c0c);
	//udelay(750);/* # 750us, TO-DO: depend on IP spec*/

	/*# 5.AP power up standby power domain*/
	mini_isp_register_write_one_bit(0xffef00a4, 0, 0);/* # psw_pd_standby*/
	udelay(70);/* # 70us, TO-DO: depend on backend spec*/

	/*# 6.AP release isolation of standby power domain*/
	mini_isp_register_write_one_bit(0xffef00a0, 0, 0);/* # iso_pd_standby*/

	/*# AP release global reset of standby power domain*/
	/*# pd_rstn_standby_top*/
	mini_isp_register_write_one_bit(0xffe81080, 3, 1);

	/*# AP power up SRAM of ARC5*/
	mini_isp_register_write_one_bit(0xffef0090, 22, 0);/*# srampd_base_arc*/

	/*
	 *# 7.AP enable clock of modules(including arbiter bus, OCRAM0)
	 *and ARC5
	 */
	mini_isp_register_write_one_bit(0xffe800c4, 0, 0);/* # base_ck_in*/
	mini_isp_register_write_one_bit(0xffe801a4, 0, 0);/* # standby_top*/
	mini_isp_register_write_one_bit(0xffe80104, 0, 0);/* # arb_bus_stdby_0*/
	mini_isp_register_write_one_bit(0xffe80104, 2, 0);/* # ocram_0*/

	/*# 8.AP release reset of modules(including arbiter bus)*/
	mini_isp_register_write_one_bit(0xffe801a4, 1, 0);/* # standby_top*/
	mini_isp_register_write_one_bit(0xffe80104, 1, 0);/* # arb_bus_stdby_0*/

	/*# AP restore OCRAM0 setting*/
	mini_isp_register_write(0xffe609f8, 0xdbfc0);
	mini_isp_register_write(0xffe609f4, 0);
	mini_isp_register_write(0xffe60800, 0);
	mini_isp_register_write(0xffe60900, 0);

	/*# 9. AP release reset of ARC5*/
	mini_isp_register_write_one_bit(0xffe800c4, 1, 0);/* # base_ck_in*/

	/*# 10. AP wait interrupt then clean interrupt status*/
	status = mini_isp_wait_for_event(MINI_ISP_RCV_CPCHANGE);
	misp_info("%s - leave", __func__);
}

void mini_isp_check_and_leave_bypass_mode(void)
{
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	if (dev_global_variable->be_set_to_bypass) {
		misp_info("AL6100 is in bypass mode");
		/*Add code here*/
		mini_isp_register_write(0xffe80b04, 0x2a2a);/*mipidphy exc*/
		mini_isp_register_write(0xffe80944, 0x2);/*mipi tx phy 1*/
		mini_isp_register_write(0xffe80884, 0x2);/*mipi tx phy 0*/
		mini_isp_register_write(0xffe804e4, 0x2);/*ppibridge 1 exc*/
		mini_isp_register_write(0xffe804c4, 0x2);/*ppibridge 1*/
		mini_isp_register_write(0xffe804a4, 0x2);/*ppibridge 0 exc*/
		mini_isp_register_write(0xffe80484, 0x2);/*ppibridge 0*/
		mini_isp_register_write(0xffe80444, 0xa);/*mipi rx phy 1*/
		mini_isp_register_write(0xffe80404, 0xa);/*mipi rx phy 0*/

		dev_global_variable->be_set_to_bypass = 0;
	} else {
		/*do nothing*/
		misp_info("not running bypass mode yet");
	}
}

int mini_isp_pure_bypass(u16 mini_isp_mode)
{
	struct misp_global_variable *dev_global_variable;
	errcode err = ERR_SUCCESS;
	mm_segment_t oldfs;
	struct file *file_filp;
	off_t currpos;
	loff_t offset;
	char  *allocated_memmory;
	u8  *keep_allocated_memmory;
	char  allocated_memmory_buf[64];
	u32 reg_addr;
	u32 reg_new_value;
	u32 file_total_size;
	u8 byass_setting_file_location[80];
	u8 buf[8];
	int i;

	dev_global_variable = get_mini_isp_global_variable();

	snprintf(byass_setting_file_location, 64,
		"%saltek_bypass_setting_%d.log",
		MINIISP_BYPASS_SETTING_FILE_PATH, mini_isp_mode);

	misp_info("altek bypass mode %d", mini_isp_mode);
	misp_info("%s setting filepath : %s", __func__,
		byass_setting_file_location);

	allocated_memmory = allocated_memmory_buf;
	keep_allocated_memmory = allocated_memmory;
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	file_filp = filp_open(byass_setting_file_location,
		O_RDONLY, 0644);

	if (IS_ERR(file_filp)) {
		err = PTR_ERR(file_filp);
		misp_err("%s open file failed. err: %d", __func__, err);
		set_fs(oldfs);
		return err;
	}
	misp_info("%s open file success!", __func__);

#ifndef AL6100_SPI_NOR
	mini_isp_chip_init();
#endif
	/*get bin filie size*/
	currpos = vfs_llseek(file_filp, 0L, SEEK_END);
	file_total_size = currpos;
	currpos = vfs_llseek(file_filp, 0L, SEEK_SET);

	misp_info("%s  file_total_size = %d", __func__, file_total_size);
	offset = file_filp->f_pos;
	while (file_total_size > 0) {
		//		err = file_filp->f_op->read(file_filp,
		//			allocated_memmory_buf, 1, &offset);

		err = vfs_read(file_filp, allocated_memmory_buf,
								1, &offset);
		if (!err)
			misp_info("err: Read fail!, %s: %d", __func__, __LINE__);

		file_filp->f_pos = offset;
		file_total_size--;
		if (allocated_memmory_buf[0] == '0') {
			vfs_read(file_filp, (char *)allocated_memmory,
				1, &offset);
			file_filp->f_pos = offset;
			file_total_size--;
			if (allocated_memmory_buf[0] == 'x') {
				vfs_read(file_filp, (char *)allocated_memmory,
					8, &offset);
				file_filp->f_pos = offset;
				file_total_size = file_total_size - 8;

				for (i = 0; i < 4; i++)
					err = hex2bin(buf+3-i,
						allocated_memmory+2*i, 1);

				while (1) {
					vfs_read(file_filp,
						(char *)allocated_memmory,
						1, &offset);
					file_filp->f_pos = offset;
					file_total_size = file_total_size - 1;

					if (allocated_memmory[0] == '0')
						break;
				}

				if (file_total_size < 0)
					break;

				vfs_read(file_filp, (char *)allocated_memmory,
							1, &offset);
				file_filp->f_pos = offset;
				file_total_size = file_total_size - 1;
				if ((allocated_memmory[0] == 'x')) {
					vfs_read(file_filp,
						(char *)allocated_memmory,
						8, &offset);
					file_filp->f_pos = offset;
					file_total_size = file_total_size - 8;

					for (i = 0; i < 4; i++)
						err = hex2bin(buf+4+3-i,
							allocated_memmory+2*i,
							1);

					memcpy(&reg_addr, buf, 4);
					memcpy(&reg_new_value, buf+4, 4);

					mini_isp_register_write(
						reg_addr,
						reg_new_value);

				}
			}
		} else if (allocated_memmory_buf[0] == 's') {
			while (1) {
				vfs_read(file_filp, (char *)allocated_memmory,
							1, &offset);
				file_filp->f_pos = offset;
				file_total_size = file_total_size - 1;

				if (allocated_memmory[0] == 13) {
					udelay(350);
					break;
				}
			}
		}
	}
	/*Restore segment descriptor*/
	misp_info("miniisp bypass setting send finish");

	dev_global_variable->be_set_to_bypass = 1;
	set_fs(oldfs);
	filp_close(file_filp, NULL);

	return err;
}

void mini_isp_pure_bypass_debug(u16 mini_isp_mode)
{
	mini_isp_chip_init();
	misp_info("mini_isp_pure_bypass_debug(%d) set bypass mode",
		mini_isp_mode);
	switch (mini_isp_mode) {
	case 1:
		mini_isp_register_write(0xffe40000, 0x00000008);
		mini_isp_register_write(0xffe40004, 0x00006621);
		mini_isp_register_write(0xffe40008, 0x00006621);
		mini_isp_register_write(0xffe4000c, 0x00006621);
		mini_isp_register_write(0xffe40010, 0x00006621);
		mini_isp_register_write(0xffe40050, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000200);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81100, 0x00000000);
		mini_isp_register_write(0xffe81104, 0x00000000);
		mini_isp_register_write(0xffe81108, 0x000000dc);
		mini_isp_register_write(0xffe8110c, 0x00000000);
		mini_isp_register_write(0xffe81110, 0x00000001);
		mini_isp_register_write(0xffe81114, 0x00000000);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe800c0, 0x0000000a);
		mini_isp_register_write(0xffe800e0, 0x0000000a);
		mini_isp_register_write(0xffe80100, 0x0000000a);
		mini_isp_register_write(0xffe80120, 0x0000000a);
		mini_isp_register_write(0xffe81004, 0x00000800);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81120, 0x00000000);
		mini_isp_register_write(0xffe81124, 0x00000000);
		mini_isp_register_write(0xffe81128, 0x0000017a);
		mini_isp_register_write(0xffe8112c, 0x00000000);
		mini_isp_register_write(0xffe81130, 0x00000001);
		mini_isp_register_write(0xffe81134, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe81004, 0x00000400);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81140, 0x00000000);
		mini_isp_register_write(0xffe81144, 0x00000000);
		mini_isp_register_write(0xffe81148, 0x0000017a);
		mini_isp_register_write(0xffe8114c, 0x00000000);
		mini_isp_register_write(0xffe81150, 0x00000001);
		mini_isp_register_write(0xffe81154, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe80b00, 0x00000819);
		mini_isp_register_write(0xffe80880, 0x00000400);
		mini_isp_register_write(0xffe80380, 0x00000004);
		mini_isp_register_write(0xffe80400, 0x00000802);
		mini_isp_register_write(0xffed1008, 0x0000aab0);
		mini_isp_register_write(0xffed100c, 0x00000306);
		mini_isp_register_write(0xffed1010, 0x00000147);
		mini_isp_register_write(0xffed1014, 0x0000aa73);
		mini_isp_register_write(0xffed1018, 0x00000eaa);
		mini_isp_register_write(0xffed101c, 0x00008e1a);
		mini_isp_register_write(0xffed1044, 0x000000b8);
		mini_isp_register_write(0xffed1044, 0x00000098);
		udelay(0x00000028);
		mini_isp_register_write(0xffed1044, 0x00000088);
		udelay(0x00000028);
		mini_isp_register_write(0xffed1030, 0x00080000);
		mini_isp_register_write(0xffed1034, 0x00080000);
		mini_isp_register_write(0xffed1038, 0x00080000);
		mini_isp_register_write(0xffed103c, 0x00080000);
		mini_isp_register_write(0xffed1040, 0x00080000);
		udelay(0x00000006);
		mini_isp_register_write(0xffed1030, 0x00080002);
		mini_isp_register_write(0xffed1034, 0x00080002);
		mini_isp_register_write(0xffed1038, 0x00080002);
		mini_isp_register_write(0xffed103c, 0x00080002);
		mini_isp_register_write(0xffed1040, 0x00080002);
		mini_isp_register_write(0xffed1000, 0x00000000);
		mini_isp_register_write(0xfff97000, 0x00000001);
		mini_isp_register_write(0xfff97004, 0x00003210);
		mini_isp_register_write(0xfff97008, 0x00003210);
		mini_isp_register_write(0xfff9700c, 0x145000b4);
		mini_isp_register_write(0xfff97010, 0x00000000);
		mini_isp_register_write(0xfff97014, 0x00000000);
		mini_isp_register_write(0xfff97018, 0x00000000);
		mini_isp_register_write(0xfff9701c, 0x00000000);
		mini_isp_register_write(0xfff97020, 0x00000000);
		mini_isp_register_write(0xfff97024, 0x00000010);
		mini_isp_register_write(0xfff97028, 0x0000001e);
		mini_isp_register_write(0xfff9702c, 0x0000000b);
		mini_isp_register_write(0xfff97030, 0x0f0f0f0f);
		mini_isp_register_write(0xfff97000, 0x00000000);
		mini_isp_register_write(0xfff91000, 0x1000000b);
		mini_isp_register_write(0xfff91024, 0x0000000f);
		mini_isp_register_write(0xfff91028, 0x00001010);
		mini_isp_register_write(0xfff9106c, 0x00000c0c);
		mini_isp_register_write(0xfff91040, 0x00003c02);
		udelay(0x00000028);
		mini_isp_register_write(0xfff91004, 0x00000000);
		mini_isp_register_write(0xfff91008, 0x00003033);
		mini_isp_register_write(0xfff91010, 0x00003c02);
		mini_isp_register_write(0xfff91014, 0x00003c02);
		mini_isp_register_write(0xfff9103c, 0x00000000);
		mini_isp_register_write(0xfff91098, 0x00444404);
		mini_isp_register_write(0xfff9104c, 0x000d0011);
		mini_isp_register_write(0xfff91000, 0x1000000b);
		mini_isp_register_write(0xfff91024, 0x0000000f);
		mini_isp_register_write(0xfff91028, 0x0000013f);
		mini_isp_register_write(0xfff9106c, 0x00000e0e);
		mini_isp_register_write(0xfff9104c, 0x000d0011);
		mini_isp_register_write(0xfff91070, 0x01000005);
		mini_isp_register_write(0xfff910a8, 0x00000000);
		mini_isp_register_write(0xfff91094, 0x00001021);
		mini_isp_register_write(0xfff91000, 0x1000000a);
		break;
	case 2:
		mini_isp_register_write(0xffe40000, 0x00000008);
		mini_isp_register_write(0xffe40004, 0x00006621);
		mini_isp_register_write(0xffe40008, 0x00006621);
		mini_isp_register_write(0xffe4000c, 0x00006621);
		mini_isp_register_write(0xffe40010, 0x00006621);
		mini_isp_register_write(0xffe40050, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000200);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81100, 0x00000000);
		mini_isp_register_write(0xffe81104, 0x00000000);
		mini_isp_register_write(0xffe81108, 0x000000dc);
		mini_isp_register_write(0xffe8110c, 0x00000000);
		mini_isp_register_write(0xffe81110, 0x00000001);
		mini_isp_register_write(0xffe81114, 0x00000000);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe800c0, 0x0000000a);
		mini_isp_register_write(0xffe800e0, 0x0000000a);
		mini_isp_register_write(0xffe80100, 0x0000000a);
		mini_isp_register_write(0xffe80120, 0x0000000a);
		mini_isp_register_write(0xffe81004, 0x00000800);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81120, 0x00000000);
		mini_isp_register_write(0xffe81124, 0x00000000);
		mini_isp_register_write(0xffe81128, 0x0000017a);
		mini_isp_register_write(0xffe8112c, 0x00000000);
		mini_isp_register_write(0xffe81130, 0x00000001);
		mini_isp_register_write(0xffe81134, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe81004, 0x00000400);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81140, 0x00000000);
		mini_isp_register_write(0xffe81144, 0x00000000);
		mini_isp_register_write(0xffe81148, 0x0000017a);
		mini_isp_register_write(0xffe8114c, 0x00000000);
		mini_isp_register_write(0xffe81150, 0x00000001);
		mini_isp_register_write(0xffe81154, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe80b00, 0x00000819);
		mini_isp_register_write(0xffe80940, 0x00000800);
		mini_isp_register_write(0xffe80440, 0x00000004);
		mini_isp_register_write(0xffe80460, 0x00000802);
		mini_isp_register_write(0xffed6008, 0x0000aab0);
		mini_isp_register_write(0xffed600c, 0x00000306);
		mini_isp_register_write(0xffed6010, 0x00000147);
		mini_isp_register_write(0xffed6014, 0x0000aa73);
		mini_isp_register_write(0xffed6018, 0x00000eaa);
		mini_isp_register_write(0xffed601c, 0x00008e1a);
		mini_isp_register_write(0xffed6044, 0x000000b8);
		mini_isp_register_write(0xffed6044, 0x00000098);
		udelay(0x00000028);
		mini_isp_register_write(0xffed6044, 0x00000088);
		udelay(0x00000028);
		mini_isp_register_write(0xffed6030, 0x00080000);
		mini_isp_register_write(0xffed6034, 0x00080000);
		mini_isp_register_write(0xffed6038, 0x00080000);
		mini_isp_register_write(0xffed603c, 0x00080000);
		mini_isp_register_write(0xffed6040, 0x00080000);
		udelay(0x00000006);
		mini_isp_register_write(0xffed6030, 0x00080002);
		mini_isp_register_write(0xffed6034, 0x00080002);
		mini_isp_register_write(0xffed6038, 0x00080002);
		mini_isp_register_write(0xffed603c, 0x00080002);
		mini_isp_register_write(0xffed6040, 0x00080002);
		mini_isp_register_write(0xffed6000, 0x00000000);
		mini_isp_register_write(0xfff98000, 0x00000001);
		mini_isp_register_write(0xfff98004, 0x00003210);
		mini_isp_register_write(0xfff98008, 0x00003210);
		mini_isp_register_write(0xfff9800c, 0x14500344);
		mini_isp_register_write(0xfff98010, 0x00000000);
		mini_isp_register_write(0xfff98014, 0x00000000);
		mini_isp_register_write(0xfff98018, 0x00000000);
		mini_isp_register_write(0xfff9801c, 0x00000000);
		mini_isp_register_write(0xfff98020, 0x00000000);
		mini_isp_register_write(0xfff98024, 0x000000ec);
		mini_isp_register_write(0xfff98028, 0x0000001e);
		mini_isp_register_write(0xfff9802c, 0x000000c3);
		mini_isp_register_write(0xfff98030, 0x56565656);
		mini_isp_register_write(0xfff98000, 0x00000000);
		mini_isp_register_write(0xfff94000, 0x1000000b);
		mini_isp_register_write(0xfff94024, 0x0000000f);
		mini_isp_register_write(0xfff94028, 0x00001010);
		mini_isp_register_write(0xfff9406c, 0x00000c0c);
		mini_isp_register_write(0xfff94040, 0x00003c02);
		udelay(0x00000028);
		mini_isp_register_write(0xfff94004, 0x00000000);
		mini_isp_register_write(0xfff94008, 0x00003033);
		mini_isp_register_write(0xfff94010, 0x00003c02);
		mini_isp_register_write(0xfff94014, 0x00003c02);
		mini_isp_register_write(0xfff9403c, 0x00000000);
		mini_isp_register_write(0xfff94098, 0x00444404);
		mini_isp_register_write(0xfff9404c, 0x000d0011);
		mini_isp_register_write(0xfff94000, 0x1000000b);
		mini_isp_register_write(0xfff94024, 0x0000000f);
		mini_isp_register_write(0xfff94028, 0x00003f01);
		mini_isp_register_write(0xfff9406c, 0x00000e0e);
		mini_isp_register_write(0xfff9404c, 0x000d0011);
		mini_isp_register_write(0xfff94070, 0x01000005);
		mini_isp_register_write(0xfff940a8, 0x00000000);
		mini_isp_register_write(0xfff94094, 0x00001021);
		mini_isp_register_write(0xfff94000, 0x1000000a);
		break;
	case 3:
		mini_isp_register_write(0xffe40000, 0x00000008);
		mini_isp_register_write(0xffe40004, 0x00006621);
		mini_isp_register_write(0xffe40008, 0x00006621);
		mini_isp_register_write(0xffe4000c, 0x00006621);
		mini_isp_register_write(0xffe40010, 0x00006621);
		mini_isp_register_write(0xffe40050, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000200);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81100, 0x00000000);
		mini_isp_register_write(0xffe81104, 0x00000000);
		mini_isp_register_write(0xffe81108, 0x000000dc);
		mini_isp_register_write(0xffe8110c, 0x00000000);
		mini_isp_register_write(0xffe81110, 0x00000001);
		mini_isp_register_write(0xffe81114, 0x00000000);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe800c0, 0x0000000a);
		mini_isp_register_write(0xffe800e0, 0x0000000a);
		mini_isp_register_write(0xffe80100, 0x0000000a);
		mini_isp_register_write(0xffe80120, 0x0000000a);
		mini_isp_register_write(0xffe81004, 0x00000800);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81120, 0x00000000);
		mini_isp_register_write(0xffe81124, 0x00000000);
		mini_isp_register_write(0xffe81128, 0x0000017a);
		mini_isp_register_write(0xffe8112c, 0x00000000);
		mini_isp_register_write(0xffe81130, 0x00000001);
		mini_isp_register_write(0xffe81134, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe81004, 0x00000400);
		udelay(0x00000005);
		mini_isp_register_write(0xffe81140, 0x00000000);
		mini_isp_register_write(0xffe81144, 0x00000000);
		mini_isp_register_write(0xffe81148, 0x0000017a);
		mini_isp_register_write(0xffe8114c, 0x00000000);
		mini_isp_register_write(0xffe81150, 0x00000001);
		mini_isp_register_write(0xffe81154, 0x00000001);
		mini_isp_register_write(0xffe81004, 0x00000000);
		udelay(0x0000015e);
		mini_isp_register_write(0xffe80b00, 0x00000819);
		mini_isp_register_write(0xffe80880, 0x00000400);
		mini_isp_register_write(0xffe80380, 0x00000004);
		mini_isp_register_write(0xffe80400, 0x00000802);
		mini_isp_register_write(0xffe80940, 0x00000800);
		mini_isp_register_write(0xffe80440, 0x00000004);
		mini_isp_register_write(0xffe80460, 0x00000802);
		mini_isp_register_write(0xffed1008, 0x0000aab0);
		mini_isp_register_write(0xffed100c, 0x00000306);
		mini_isp_register_write(0xffed1010, 0x00000147);
		mini_isp_register_write(0xffed1014, 0x0000aa73);
		mini_isp_register_write(0xffed1018, 0x00000eaa);
		mini_isp_register_write(0xffed101c, 0x00008e1a);
		mini_isp_register_write(0xffed1044, 0x000000b8);
		mini_isp_register_write(0xffed1044, 0x00000098);
		udelay(0x00000028);
		mini_isp_register_write(0xffed1044, 0x00000088);
		udelay(0x00000028);
		mini_isp_register_write(0xffed1030, 0x00080000);
		mini_isp_register_write(0xffed1034, 0x00080000);
		mini_isp_register_write(0xffed1038, 0x00080000);
		mini_isp_register_write(0xffed103c, 0x00080000);
		mini_isp_register_write(0xffed1040, 0x00080000);
		udelay(0x00000006);
		mini_isp_register_write(0xffed1030, 0x00080002);
		mini_isp_register_write(0xffed1034, 0x00080002);
		mini_isp_register_write(0xffed1038, 0x00080002);
		mini_isp_register_write(0xffed103c, 0x00080002);
		mini_isp_register_write(0xffed1040, 0x00080002);
		mini_isp_register_write(0xffed1000, 0x00000000);
		mini_isp_register_write(0xfff97000, 0x00000001);
		mini_isp_register_write(0xfff97004, 0x00003210);
		mini_isp_register_write(0xfff97008, 0x00003210);
		mini_isp_register_write(0xfff9700c, 0x145000b4);
		mini_isp_register_write(0xfff97010, 0x00000000);
		mini_isp_register_write(0xfff97014, 0x00000000);
		mini_isp_register_write(0xfff97018, 0x00000000);
		mini_isp_register_write(0xfff9701c, 0x00000000);
		mini_isp_register_write(0xfff97020, 0x00000000);
		mini_isp_register_write(0xfff97024, 0x00000010);
		mini_isp_register_write(0xfff97028, 0x0000001e);
		mini_isp_register_write(0xfff9702c, 0x0000000b);
		mini_isp_register_write(0xfff97030, 0x0f0f0f0f);
		mini_isp_register_write(0xfff97000, 0x00000000);
		mini_isp_register_write(0xfff91000, 0x1000000b);
		mini_isp_register_write(0xfff91024, 0x0000000f);
		mini_isp_register_write(0xfff91028, 0x00001010);
		mini_isp_register_write(0xfff9106c, 0x00000c0c);
		mini_isp_register_write(0xfff91040, 0x00003c02);
		udelay(0x00000028);
		mini_isp_register_write(0xfff91004, 0x00000000);
		mini_isp_register_write(0xfff91008, 0x00003033);
		mini_isp_register_write(0xfff91010, 0x00003c02);
		mini_isp_register_write(0xfff91014, 0x00003c02);
		mini_isp_register_write(0xfff9103c, 0x00000000);
		mini_isp_register_write(0xfff91098, 0x00444404);
		mini_isp_register_write(0xfff9104c, 0x000d0011);
		mini_isp_register_write(0xfff91000, 0x1000000b);
		mini_isp_register_write(0xfff91024, 0x0000000f);
		mini_isp_register_write(0xfff91028, 0x0000013f);
		mini_isp_register_write(0xfff9106c, 0x00000e0e);
		mini_isp_register_write(0xfff9104c, 0x000d0011);
		mini_isp_register_write(0xfff91070, 0x01000005);
		mini_isp_register_write(0xfff910a8, 0x00000000);
		mini_isp_register_write(0xfff91094, 0x00001021);
		mini_isp_register_write(0xfff91000, 0x1000000a);
		mini_isp_register_write(0xffed6008, 0x0000aab0);
		mini_isp_register_write(0xffed600c, 0x00000306);
		mini_isp_register_write(0xffed6010, 0x00000147);
		mini_isp_register_write(0xffed6014, 0x0000aa73);
		mini_isp_register_write(0xffed6018, 0x00000eaa);
		mini_isp_register_write(0xffed601c, 0x00008e1a);
		mini_isp_register_write(0xffed6044, 0x000000b8);
		mini_isp_register_write(0xffed6044, 0x00000098);
		udelay(0x00000028);
		mini_isp_register_write(0xffed6044, 0x00000088);
		udelay(0x00000028);
		mini_isp_register_write(0xffed6030, 0x00080000);
		mini_isp_register_write(0xffed6034, 0x00080000);
		mini_isp_register_write(0xffed6038, 0x00080000);
		mini_isp_register_write(0xffed603c, 0x00080000);
		mini_isp_register_write(0xffed6040, 0x00080000);
		udelay(0x00000006);
		mini_isp_register_write(0xffed6030, 0x00080002);
		mini_isp_register_write(0xffed6034, 0x00080002);
		mini_isp_register_write(0xffed6038, 0x00080002);
		mini_isp_register_write(0xffed603c, 0x00080002);
		mini_isp_register_write(0xffed6040, 0x00080002);
		mini_isp_register_write(0xffed6000, 0x00000000);
		mini_isp_register_write(0xfff98000, 0x00000001);
		mini_isp_register_write(0xfff98004, 0x00003210);
		mini_isp_register_write(0xfff98008, 0x00003210);
		mini_isp_register_write(0xfff9800c, 0x14500344);
		mini_isp_register_write(0xfff98010, 0x00000000);
		mini_isp_register_write(0xfff98014, 0x00000000);
		mini_isp_register_write(0xfff98018, 0x00000000);
		mini_isp_register_write(0xfff9801c, 0x00000000);
		mini_isp_register_write(0xfff98020, 0x00000000);
		mini_isp_register_write(0xfff98024, 0x000000ec);
		mini_isp_register_write(0xfff98028, 0x0000001e);
		mini_isp_register_write(0xfff9802c, 0x000000c3);
		mini_isp_register_write(0xfff98030, 0x56565656);
		mini_isp_register_write(0xfff98000, 0x00000000);
		mini_isp_register_write(0xfff94000, 0x1000000b);
		mini_isp_register_write(0xfff94024, 0x0000000f);
		mini_isp_register_write(0xfff94028, 0x00001010);
		mini_isp_register_write(0xfff9406c, 0x00000c0c);
		mini_isp_register_write(0xfff94040, 0x00003c02);
		udelay(0x00000028);
		mini_isp_register_write(0xfff94004, 0x00000000);
		mini_isp_register_write(0xfff94008, 0x00003033);
		mini_isp_register_write(0xfff94010, 0x00003c02);
		mini_isp_register_write(0xfff94014, 0x00003c02);
		mini_isp_register_write(0xfff9403c, 0x00000000);
		mini_isp_register_write(0xfff94098, 0x00444404);
		mini_isp_register_write(0xfff9404c, 0x000d0011);
		mini_isp_register_write(0xfff94000, 0x1000000b);
		mini_isp_register_write(0xfff94024, 0x0000000f);
		mini_isp_register_write(0xfff94028, 0x00003f01);
		mini_isp_register_write(0xfff9406c, 0x00000e0e);
		mini_isp_register_write(0xfff9404c, 0x000d0011);
		mini_isp_register_write(0xfff94070, 0x01000005);
		mini_isp_register_write(0xfff940a8, 0x00000000);
		mini_isp_register_write(0xfff94094, 0x00001021);
		mini_isp_register_write(0xfff94000, 0x1000000a);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(mini_isp_pure_bypass_debug);


u32 mini_isp_register_read_then_write_file(u32 start_reg_addr, u32 end_reg_addr,
							char *module_name)
{
	struct misp_data *devdata;
	struct misp_global_variable *dev_global_variable;
	u32 retval = ERR_SUCCESS;
	u32 count;
	u32 rx_dummy_len;
	u8 *send_buffer;
	u8 *recv_buffer;
	u8 *io_buffer = NULL;
	u32 io_size;
	u8 *dump_memory = NULL;
	u8 *keep_dump_memory = NULL;
	u32 ouput_size;
	u8 ctrlbyte;
	u8 filename[80];
	struct file *f;
	mm_segment_t fs;

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);

	count = ((end_reg_addr - start_reg_addr) / 4) + 1;  //how many registers(4 bytes) do you want to read?
	ouput_size = (count + 2) * 4; // read 4 bytes register value

	rx_dummy_len = devdata->rx_dummy_len;
	io_size = EMODE_TXCMD_LEN + rx_dummy_len + 4; // read 4 bytes register value at a time

	io_buffer = kzalloc(io_size, GFP_KERNEL);
	if (!io_buffer) {
		misp_err("%s Allocate memory failed.", __func__);
		retval = -ENOMEM;
		goto allocate_memory_fail;
	}

	dump_memory = kzalloc(ouput_size, GFP_KERNEL);
	if (!dump_memory){
		misp_err("%s Allocate memory failed.", __func__);
		retval = -ENOMEM;
		goto allocate_memory_fail;
	}
	keep_dump_memory = dump_memory;

	send_buffer = io_buffer;
	recv_buffer = io_buffer + EMODE_TXCMD_LEN;

	memcpy(dump_memory, &start_reg_addr, 4);
	dump_memory = dump_memory + 4;
	memcpy(dump_memory, &count, 4);
	dump_memory = dump_memory + 4;

	ctrlbyte = CTRL_BYTE_REGRD;
	while (start_reg_addr <= end_reg_addr) {

		memset(io_buffer, 0, io_size);
		memcpy(send_buffer, &ctrlbyte, 1);
		memcpy(send_buffer + 1, &start_reg_addr, 4);

		retval = devdata->intf_fn->read((void *)devdata,
					send_buffer, EMODE_TXCMD_LEN,
					recv_buffer, rx_dummy_len + 4);

		if (retval) {
			misp_err("%s read failed.", __func__);
			goto mini_isp_register_read_get_fail;
		}

		if (rx_dummy_len > 0 &&
			mini_isp_check_rx_dummy(&recv_buffer, rx_dummy_len)) {
			misp_err("%s read failed.", __func__);
			retval = -EIO;
			goto mini_isp_register_read_get_fail;
		}

		memcpy(dump_memory, recv_buffer, 4);
		start_reg_addr = start_reg_addr + 4;
		dump_memory = dump_memory + 4;
	}

	snprintf(filename, 64, "%s/%s_%x.regx",
		MINIISP_INFO_DUMPLOCATION, module_name, start_reg_addr);
	f = filp_open(filename, O_APPEND | O_CREAT | O_RDWR, 0777);
	/*Get current segment descriptor*/
	fs = get_fs();
	/*Set segment descriptor associated*/
	set_fs(get_ds());
	/*write the file*/
	f->f_op->write(f, (char *)keep_dump_memory, ouput_size, &f->f_pos);
	/*Restore segment descriptor*/
	set_fs(fs);
	filp_close(f, NULL);

mini_isp_register_read_get_fail:

allocate_memory_fail:
	kfree(io_buffer);
	kfree(keep_dump_memory);

	return retval;
}

u32 mini_isp_register_read(u32 reg_addr, u32 *reg_value)
{
	int status = ERR_SUCCESS;
	u8 *send_buffer;
	u8 *recv_buffer;
	u8 ctrlbyte = CTRL_BYTE_REGRD;
	u32 address = reg_addr;
	struct misp_data *devdata;
	struct misp_global_variable *dev_global_variable;

	u8 send_buffer_value[64] = {0};
	u8 recv_buffer_value[64] = {0};
	u32 rx_dummy_len;;

	dev_global_variable = get_mini_isp_global_variable();

	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);
	send_buffer = send_buffer_value;
	recv_buffer = recv_buffer_value;
	rx_dummy_len = devdata->rx_dummy_len;

	memcpy(send_buffer, &ctrlbyte, 1);
	memcpy(send_buffer + 1, &address, 4);


	status = devdata->intf_fn->read(devdata,
						send_buffer_value, EMODE_TXCMD_LEN,
						recv_buffer_value, rx_dummy_len + 4); //read 4 bytes register value

	if (status) {
		misp_err("%s - sync error: status = %d", __func__, status);
		goto mini_isp_register_read_end;
	}

	if (rx_dummy_len > 0 &&
		mini_isp_check_rx_dummy(&recv_buffer, rx_dummy_len)) {
		misp_err("[miniisp]Can't get reg");
		goto mini_isp_register_read_end;
	}
	memcpy(reg_value, recv_buffer, 4);

mini_isp_register_read_end:

	return status;
}

void mini_isp_memory_write(u32 memory_addr, u8 *write_buffer, u32 write_len)
{
	u8 *send_buffer;
	u8 ctrlbyte = CTRL_BYTE_MEMWR;
	u32 address = memory_addr;
	struct misp_data *devdata;
	u8 send_buffer_value[EMODE_TXCMD_LEN + write_len];
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);
	send_buffer = send_buffer_value;

	memcpy(send_buffer, &ctrlbyte, 1);
	memcpy(send_buffer + 1, &address, 4);
	memcpy(send_buffer + 5, write_buffer, write_len);
	dev_global_variable->before_booting = 1;
	devdata->intf_fn->write(devdata, send_buffer, NULL, EMODE_TXCMD_LEN + write_len);
	dev_global_variable->before_booting = 0;
}

u32 mini_isp_memory_read_then_write_file(u32 start_addr, u32 len,
	char *file_name)
{
	struct misp_data *devdata;
	struct misp_global_variable *dev_global_variable;
	u32 retval = ERR_SUCCESS;
	u8 *io_buffer = NULL;
	u8 *send_buffer;
	u8 *recv_buffer;
	u8 *dump_memory   = NULL;
	u8 *keep_dump_memory = NULL;
	u32 dump_addr = start_addr;
	u32 ouput_size;
	u32 io_size, remain_size, one_size;
	u32 rx_dummy_len;
	u8 ctrlbyte;
	u8 filename[80];
	struct file *f;
	mm_segment_t fs;

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);

	rx_dummy_len = devdata->rx_dummy_len;
	io_size = EMODE_TXCMD_LEN + rx_dummy_len + 60000; // read 60000 bytes at a time;
	io_buffer = kzalloc(io_size, GFP_KERNEL);
	if (!io_buffer) {
		misp_err("%s Allocate memory failed.", __func__);
		goto allocate_memory_fail;
	}

	dump_memory = kzalloc(len, GFP_KERNEL);
	if (!dump_memory) {
		misp_err("%s Allocate memory failed.", __func__);
		goto allocate_memory_fail;
	}
	keep_dump_memory = dump_memory;
	ouput_size = len;
	ctrlbyte = CTRL_BYTE_MEMRD; //memory read

	for (remain_size = ouput_size; remain_size > 0;
		remain_size -= one_size) {
		one_size = (remain_size > 60000) ?
			60000 : remain_size;

		memset(io_buffer, 0, io_size);
		send_buffer = io_buffer;
		recv_buffer = io_buffer + EMODE_TXCMD_LEN;

		memcpy(send_buffer, &ctrlbyte, 1);
		memcpy(send_buffer + 1, &dump_addr, 4);

		retval = devdata->intf_fn->read((void *)devdata,
							send_buffer,
							EMODE_TXCMD_LEN,
							recv_buffer,
							one_size + rx_dummy_len);
		if (retval) {
			misp_err("%s get failed.", __func__);
			goto mini_isp_memory_read_get_fail;
		}

		if (rx_dummy_len > 0 &&
			mini_isp_check_rx_dummy(&recv_buffer, rx_dummy_len)) {
			misp_err("%s get failed.", __func__);
			retval = -EIO;
			goto mini_isp_memory_read_get_fail;
		}

		memcpy(dump_memory, recv_buffer, one_size);
		misp_info("%s dump_addr = 0x%x one_size = %d",
			__func__, dump_addr, one_size);
		dump_memory += one_size;
		dump_addr += one_size;
	} //for-loop

	misp_info("%s read_finish", __func__);

	snprintf(filename, 128, "%s/%s.raw",
		MINIISP_INFO_DUMPLOCATION, file_name);
	f = filp_open(filename, O_APPEND | O_CREAT | O_RDWR, 0777);
	/*Get current segment descriptor*/
	fs = get_fs();
	/*Set segment descriptor associated*/
	set_fs(get_ds());
	/*write the file*/
	f->f_op->write(f, (char *)keep_dump_memory, ouput_size, &f->f_pos);
	/*Restore segment descriptor*/
	set_fs(fs);
	filp_close(f, NULL);

mini_isp_memory_read_get_fail:
allocate_memory_fail:
	kfree(keep_dump_memory);
	kfree(io_buffer);

	return retval;
}
EXPORT_SYMBOL(mini_isp_memory_read_then_write_file);


u32 mini_isp_memory_read(u32 start_addr, u8 *read_buffer, u32 len)
{
	struct misp_data *devdata;
	struct misp_global_variable *dev_global_variable;
	u32 retval = ERR_SUCCESS;
	u8 *io_buffer = NULL;
	u8 *send_buffer;
	u8 *recv_buffer;
	u8 *dump_memory   = NULL;
	u8 *keep_dump_memory = NULL;
	u32 dump_addr = start_addr;
	u32 ouput_size;
	u32 io_size, remain_size, one_size;
	u32 rx_dummy_len;
	u8 ctrlbyte;

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);

	rx_dummy_len = devdata->rx_dummy_len;
	io_size = EMODE_TXCMD_LEN + rx_dummy_len + 60000; // read 60000 bytes at a time;
	io_buffer = kzalloc(io_size, GFP_KERNEL);
	if (!io_buffer) {
		misp_err("%s Allocate memory failed.", __func__);
		goto allocate_memory_fail;
	}

	dump_memory = kzalloc(len, GFP_KERNEL);
	if (!dump_memory) {
		misp_err("%s Allocate memory failed.", __func__);
		goto allocate_memory_fail;
	}
	keep_dump_memory = dump_memory;
	ouput_size = len;
	ctrlbyte = CTRL_BYTE_MEMRD; //memory read

	for (remain_size = ouput_size; remain_size > 0;
		remain_size -= one_size) {
		one_size = (remain_size > 60000) ?
			60000 : remain_size;

		memset(io_buffer, 0, io_size);
		send_buffer = io_buffer;
		recv_buffer = io_buffer + EMODE_TXCMD_LEN;

		memcpy(send_buffer, &ctrlbyte, 1);
		memcpy(send_buffer + 1, &dump_addr, 4);

		retval = devdata->intf_fn->read((void *)devdata,
							send_buffer,
							EMODE_TXCMD_LEN,
							recv_buffer,
							one_size + rx_dummy_len);
		if (retval) {
			misp_err("%s get failed.", __func__);
			goto mini_isp_memory_read_get_fail;
		}

		if (rx_dummy_len > 0 &&
			mini_isp_check_rx_dummy(&recv_buffer, rx_dummy_len)) {
			misp_err("%s get failed.", __func__);
			retval = -EIO;
			goto mini_isp_memory_read_get_fail;
		 }

		memcpy(dump_memory, recv_buffer, one_size);
		misp_info("%s dump_addr = 0x%x one_size = %d",
			__func__, dump_addr, one_size);
		dump_memory += one_size;
		dump_addr += one_size;
	} //for-loop

	misp_info("%s read_finish", __func__);

	memcpy(read_buffer, keep_dump_memory, ouput_size);

mini_isp_memory_read_get_fail:
allocate_memory_fail:
	kfree(keep_dump_memory);
	kfree(io_buffer);

	return retval;
}
EXPORT_SYMBOL(mini_isp_memory_read);


int mini_isp_get_bulk(struct misp_data *devdata, u8 *response_buf,
		u32 total_size, u32 block_size)
{
	int status = ERR_SUCCESS, count = 0;
	int remain_size, one_size;
	u8 io_buffer[3] = {0}; // 1byte ctrlbyte, 2bytes recv
	u8 *send_buffer;
	u8 *recv_buffer;
	u8 ctrlbyte = USPICTRL_MS_CB_DIS;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();

	send_buffer = io_buffer;
	send_buffer[0] = ctrlbyte;
	recv_buffer = io_buffer + 1;

	misp_info("%s started.", __func__);

	status = devdata->intf_fn->read(devdata, send_buffer, 1, recv_buffer, 2);
	if (status) {
		misp_err("mini_isp_send_bulk send ctrl byte failed. status:%d", status);
		status = -EINVAL;
		goto G_EXIT;
	}

	for (remain_size = total_size; remain_size > 0; remain_size -= one_size) {
		one_size = (remain_size > block_size) ? block_size : remain_size;
		/*get the data*/
		misp_info("%s dump start", __func__);
		status = devdata->intf_fn->read((void *)devdata, response_buf, 0, response_buf, one_size);
		if (status != 0) {
			misp_err("%s failed!! block:%d status: %d", __func__, count, status);
			break;
		}

		response_buf += one_size;
		count++;
	}

G_EXIT:

	if (status != ERR_SUCCESS)
		misp_info("%s - error: %d", __func__, status);
	else
		misp_info("%s - success.", __func__);

	return status;
}
EXPORT_SYMBOL_GPL(mini_isp_get_bulk);


int mini_isp_debug_dump_img(void)
{
	int errcode = ERR_SUCCESS;
	u8 write_buffer[4];
	u32 rg_img_in_size;
	u32 crop_src;
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();
	/*
	if (dev_global_variable->now_state != 4) {
		misp_err("%s Error dump state, no dump", __func__);
		return ~0;
	}
	*/
	if ((dev_global_variable->intf_status & INTF_SPI_READY) &&
		(dev_global_variable->altek_spi_mode == ALTEK_SPI_MODE_A)) {
		misp_info("%s a_to_e", __func__);
		mini_isp_a_to_e();
	}
	mini_isp_register_read(0xfff8401c, &rg_img_in_size);
	mini_isp_register_read(0xfffa7020, &crop_src);
	write_buffer[0] = 1;
	mini_isp_memory_write(0x10, write_buffer, 1);

	errcode = mini_isp_wait_for_event(MINI_ISP_RCV_CMD_READY);
	if (errcode) {
		misp_info("%s wait first interrupt fail", __func__);
		return errcode;
	}
	misp_info("%s - crop_src %x, width = %d, height = %d",
			__func__, crop_src, ((rg_img_in_size & 0x00001fff)+1),
			(((rg_img_in_size & 0x1fff0000)>>16)+1));
	if (crop_src)
		mini_isp_memory_read_then_write_file(0x20500000,
			((rg_img_in_size & 0x00001fff)+1)*
			(((rg_img_in_size & 0x1fff0000)>>16)+1)*10/8,
			"irp");
	else
		mini_isp_memory_read_then_write_file(0x20500000,
			((rg_img_in_size & 0x00001fff)+1)*
			(((rg_img_in_size & 0x1fff0000)>>16)+1),
			"irp");
	mini_isp_register_read(0xfff5601c, &rg_img_in_size);
	write_buffer[0] = 2;
	mini_isp_memory_write(0x10, write_buffer, 1);
	errcode = mini_isp_wait_for_event(MINI_ISP_RCV_CMD_READY);
	if (errcode) {
		misp_info("%s wait second interrupt fail", __func__);
		return errcode;
	}
	mini_isp_memory_read_then_write_file(0x20715400,
		((rg_img_in_size & 0x00001fff)+1)*
		(((rg_img_in_size & 0x1fff0000)>>16)+1)*10/8,
		"depth");
	if (dev_global_variable->intf_status & INTF_SPI_READY) {
		misp_info("%s e_to_a", __func__);
		mini_isp_e_to_a();
	}
	return errcode;
}
EXPORT_SYMBOL(mini_isp_debug_dump_img);

/*
 *param dump_item[In],8 bit value
 *bit 0 stand for dump detail mode
 *bit 1 stand for dump Main_RectReg
 *bit 2 stand for dump Sub RectReg, DGCa/ DGMcc Reg, DG output image
 *bit 3 stand for dump DP Reg, DP output image
 *bit 4 stand for dump InvRect Reg, InvRect output image
 *bit 5 stand for dump detail mode
*/
int mini_isp_debug_depth_dump(u8 dump_item)
{
	int errcode = ERR_SUCCESS;
	u8 write_buffer[4];
	u32 rg_img_in_size;
	u32 crop_src;
	u8 fov_mode;
	u8 dump_step;
	u8 count = 0;
	u8 filename[80];
	struct misp_global_variable *dev_global_variable;

	dev_global_variable = get_mini_isp_global_variable();

	if (dev_global_variable->now_state != 4) {
		misp_err("%s Error dump state, no dump", __func__);
		return ~0;
	}

	if ((dev_global_variable->intf_status & INTF_SPI_READY) &&
		(dev_global_variable->altek_spi_mode == ALTEK_SPI_MODE_A)) {
		misp_info("%s a_to_e", __func__);
		mini_isp_a_to_e();
	}

	write_buffer[0] = 1;
	mini_isp_memory_write(0x10, write_buffer, 1);

	errcode = mini_isp_wait_for_event(MINI_ISP_RCV_CMD_READY);
	if (errcode) {
		misp_info("%s wait first interrupt fail", __func__);
		return errcode;
	}

	mini_isp_memory_read(0x24, &fov_mode, 1);

	mini_isp_register_read(0xfff8401c, &rg_img_in_size);
	mini_isp_register_read(0xfffa7020, &crop_src);
	misp_info("%s - crop_src %x, width = %d, height = %d",
		__func__, crop_src, ((rg_img_in_size & 0x00001fff)+1),
		(((rg_img_in_size & 0x1fff0000)>>16)+1));

	if (fov_mode) {
		/*sub image*/
		mini_isp_memory_read_then_write_file(
			0x20500000+288000-((rg_img_in_size & 0x00001fff)+1)*
			(((rg_img_in_size & 0x1fff0000)>>16)+1)/2,
			((rg_img_in_size & 0x00001fff)+1)*
			(((rg_img_in_size & 0x1fff0000)>>16)+1)/2, "sub_y");
		/*main image*/
		mini_isp_memory_read_then_write_file(
			0x20500000+288000-((rg_img_in_size & 0x00001fff)+1)*
			(((rg_img_in_size & 0x1fff0000)>>16)+1),
			((rg_img_in_size & 0x00001fff)+1)*
			(((rg_img_in_size & 0x1fff0000)>>16)+1)/2, "main_y");
	} else {
		if (crop_src)
			mini_isp_memory_read_then_write_file(0x20500000,
				((rg_img_in_size & 0x00001fff)+1)*
				(((rg_img_in_size & 0x1fff0000)>>16)+1)*10/8,
				"irp");
		else
			mini_isp_memory_read_then_write_file(0x20500000,
				((rg_img_in_size & 0x00001fff)+1)*
				(((rg_img_in_size & 0x1fff0000)>>16)+1), "irp");
	}

	if (dump_item&0x1) {
		dump_step = dump_item >> 1;
		while (!(dump_step & 0x1) && (count < 6)) {
			dump_step = dump_step >> 1;
			count++;
			}
		write_buffer[0] = count;
		mini_isp_memory_write(0x20, write_buffer, 1);
		dump_step = dump_step >> 1;
		count++;
	}

	mini_isp_register_read(0xfff5601c, &rg_img_in_size);
	write_buffer[0] = 2;
	mini_isp_memory_write(0x10, write_buffer, 1);
	errcode = mini_isp_wait_for_event(MINI_ISP_RCV_CMD_READY);
	if (errcode) {
		misp_info("%s wait second interrupt fail", __func__);
		return errcode;
	}

	if ((dump_item & 0x1) && (count < 6)) {
		snprintf(filename, 128, "%d.dump", count);
		mini_isp_memory_read_then_write_file(0x20715400,
		((rg_img_in_size & 0x00001fff)+1)*
		(((rg_img_in_size & 0x1fff0000)>>16)+1)*10/8, filename);
		while (count < 6) {
			dump_step = dump_step >> 1;
			count++;
			if (dump_step & 0x1)
				count++;
			else
				break;
		}

	}

	mini_isp_memory_read_then_write_file(0x20715400,
		((rg_img_in_size & 0x00001fff)+1)*
		(((rg_img_in_size & 0x1fff0000)>>16)+1)*10/8, "depth");
	if (dev_global_variable->intf_status & INTF_SPI_READY) {
		misp_info("%s e_to_a", __func__);
		mini_isp_e_to_a();
	}
	return errcode;
}
EXPORT_SYMBOL(mini_isp_debug_depth_dump);


int mini_isp_get_altek_status(void *devdata, u32 *altek_status)
{
	int status = ERR_SUCCESS;
	u8 *send_buffer;
	u8 *recv_buffer;
	u8 send_buffer_value[64] = {0};
	u8 recv_buffer_value[64] = {0};
	u32 write_register_addr = INTERRUPT_STATUS_REGISTER_ADDR;
	u32 rx_dummy_len = ((struct misp_data *)devdata)->rx_dummy_len;

	struct misp_global_variable *dev_global_variable;
	dev_global_variable = get_mini_isp_global_variable();

	misp_info("%s - entering", __func__);

	recv_buffer = recv_buffer_value;
	send_buffer = send_buffer_value;

	send_buffer_value[0] = CTRL_BYTE_REGRD;
	memcpy(send_buffer + 1, &write_register_addr, 4);

	/*get altek status*/
	status = ((struct misp_data *)devdata)->intf_fn->read(
					devdata, send_buffer, EMODE_TXCMD_LEN,
					recv_buffer, rx_dummy_len + 4);

	if (status) {
		misp_err("%s - sync error: status = %d", __func__, status);
		goto mini_isp_get_altek_status;
	}

	if (rx_dummy_len > 0 &&
		mini_isp_check_rx_dummy(&recv_buffer, rx_dummy_len)) {
		misp_err("%s - polling fail", __func__);
		status = -EAGAIN;
		goto mini_isp_get_altek_status;
	}

	memcpy(altek_status, recv_buffer, 4);
	misp_info("%s - altek_status = %#x", __func__, *altek_status);

mini_isp_get_altek_status:
	return status;
}

/*interrupt handler function */
extern irqreturn_t mini_isp_irq(int irq, void *handle)
{
	struct misp_data *devdata = NULL;
	struct misp_global_variable *dev_global_variable;
	int errcode = ERR_SUCCESS;
	int original_altek_spi_mode;
	u32 altek_event_state = 0;

	misp_info("%s - enter", __func__);

	irqflag = true;

	dev_global_variable = get_mini_isp_global_variable();
	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);
	if (!devdata || !dev_global_variable)
		return -IRQ_NONE;

	original_altek_spi_mode = dev_global_variable->altek_spi_mode;
	if ((dev_global_variable->intf_status & INTF_SPI_READY) &&
	  (dev_global_variable->altek_spi_mode == ALTEK_SPI_MODE_A)) {
			mini_isp_a_to_e();
	}
	errcode = mini_isp_get_altek_status(devdata,
		&altek_event_state);
	misp_info("%s - read spi register: %#x",
			__func__, altek_event_state);

	event = MINI_ISP_RCV_WAITING;

	if (errcode == ERR_SUCCESS) {
		/*error event*/
		if (altek_event_state & SYSTEM_ERROR_LEVEL1) {
			event = event | MINI_ISP_RCV_ERROR;
			if (dev_global_variable->intf_status & INTF_SPI_READY)
				mini_isp_e_to_a();
			//mini_isp_drv_get_err_code_cmd(); // need to port out of this ISR
			mini_isp_drv_get_err_code_cmd_in_irq();
			if (dev_global_variable->intf_status & INTF_SPI_READY)
				mini_isp_a_to_e();
		}
		if (altek_event_state & SYSTEM_ERROR_LEVEL2) {
			event = event | MINI_ISP_RCV_ERROR2;
			mini_isp_utility_read_reg_e_mode();
		}
		/*set sensor mode event*/
		if (altek_event_state & SET_SENSOR_MODE_READY)
			event = event | MINI_ISP_RCV_SETSENSORMODE;
		/*change cp mode event*/
		if (altek_event_state & CP_STATUS_CHANGE_DONE)
			event = event | MINI_ISP_RCV_CPCHANGE;
		/*ready event*/
		/*CMD*/
		if (altek_event_state & COMMAND_COMPLETE)
			event = event | MINI_ISP_RCV_CMD_READY;
		/*Bulk Data*/
		if (altek_event_state & BULK_DATA_COMPLETE)
			event = event | MINI_ISP_RCV_BULKDATA;
		/* streamoff event*/
		if (altek_event_state & STRMOFF_READY)
			event = event | MINI_ISP_RCV_STRMOFF;

		mini_isp_register_write(
				INTERRUPT_STATUS_REGISTER_ADDR,
				altek_event_state);
		} else {
			misp_err("%s - err: %d", __func__, errcode);
		}

		if ((dev_global_variable->intf_status & INTF_SPI_READY) &&
			original_altek_spi_mode !=
			dev_global_variable->altek_spi_mode)
			mini_isp_e_to_a();

	wake_up_interruptible(&WAITQ);

	return IRQ_HANDLED;
}

int mini_isp_wait_for_event(u32 e)
{
	int state  = 0;

	misp_info("%s - entering. event: %#x", __func__, e);

	state = wait_event_interruptible(WAITQ, event & e);

	current_event = event;

	event = (~e) & event;/*MINI_ISP_RCV_WAITING;*/

	if (state)
		misp_err("%s - irq error. err: %d", __func__, state);

	misp_info("%s - leaving. event: %#x", __func__, e);

	return state;
}

u32 mini_isp_get_currentevent(void)
{
	return current_event;
}

u32 mini_isp_check_rx_dummy(u8 **recv_buffer, u32 rx_dummy_len)
{
	u32 ret = 0;
	u32 get_count = 0;
	while (**recv_buffer == 0x00 && get_count < rx_dummy_len) {
		(*recv_buffer)++;
		get_count++;
	}
	if (**recv_buffer == 0xa5) {
		(*recv_buffer)++;
	} else {
		ret = -EIO;
	}
	return ret;
}



