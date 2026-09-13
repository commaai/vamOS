/*
 * File: miniisp.h
 * Description: Mini ISP sample codes
 *
 * (C)Copyright altek Corporation 2013
 *
 *  2013/11/01; Bruce Chung; Initial version
 *  2017/03/30; LouisWang; 2rd version
 */

#ifndef _MINI_ISP_H_
#define _MINI_ISP_H_

/******Include File******/
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>

#include "mtype.h"
/******Public Constant Definition******/

#define MINIISP_DRIVER_VERSION "v1.11"
//#define AL6100_SPI_NOR

//SPI_E_AND_I2C_SLAVE_RELATED
#define CTRL_BYTE_REGRD 0x19
#define CTRL_BYTE_REGWR 0x09
#define CTRL_BYTE_MEMRD 0x15
#define CTRL_BYTE_MEMWR 0x05
#define EMODE_TXCMD_LEN (5)  //1byte ctrl byte + 4 bytes address

//SPI_A_MODE_RELATED
#define CTRL_BYTE_A2E 0x20
#define SPI_TX_BULK_SIZE (64 * 1024)
#define SPI_TX_BULK_SIZE_BOOT (8 * 1024)
#define SPI_RX_DUMMY_LEN (16) //for spi waiting 0xa5

//I2C_TOP_RELATED
//#define I2C_BULK_SIZE (4 * 1024) // AQ360 can work
#define I2C_TX_BULK_SIZE (1024)  //AL6100 can work
#define I2C_RX_DUMMY_LEN (0)

#define TX_BUF_SIZE	64
#define RX_BUF_SIZE	64

#define MINIISP_I2C_SLAVE 0
#define MINIISP_I2C_TOP 1

#define INTF_SPI_READY (1 << 0)
#define INTF_I2C_READY (1 << 1)
#define INTF_CCI_READY (1 << 2)

#define DEBUG_ALERT 1
#define DEBUG_TURNON 1

/*#define ALTEK_TEST*/

#ifdef DEBUG_TURNON

	#ifdef DEBUG_ALERT
	#define misp_info(fmt, ...) \
			pr_err(MINI_ISP_LOG_TAG ": " fmt "\n",\
				##__VA_ARGS__)

	#define misp_warn(fmt, ...) \
			pr_warn(MINI_ISP_LOG_TAG ": " fmt "\n",\
				##__VA_ARGS__)

	#define misp_err(fmt, ...) \
			pr_err(MINI_ISP_LOG_TAG ": " fmt "\n",\
				##__VA_ARGS__)
	#else
	#define misp_info(fmt, ...) \
			pr_err(MINI_ISP_LOG_TAG ": " fmt "\n",\
				##__VA_ARGS__)

	#define misp_warn(fmt, ...) \
			pr_debug(MINI_ISP_LOG_TAG ": " fmt "\n",\
				##__VA_ARGS__)

	#define misp_err(fmt, ...) \
			pr_debug(MINI_ISP_LOG_TAG ": " fmt "\n",\
				##__VA_ARGS__)
	#endif

#else
	#define misp_info(fmt, ...)
	#define misp_err(fmt, ...)
	#define misp_warn(fmt, ...)

#endif

/*ALTEK SPI MODE*/
/*define Altek SPI mode*/
enum ALTEK_SPI_MODE {
	ALTEK_SPI_MODE_E = 0,
	ALTEK_SPI_MODE_A,
};

/*
 *@typedef USPICTRL_MS_CB_ID
 *@brief USPI control byte definition (for master control)
 */
enum {
	/*!< Ctrl-Byte, original command */
	USPICTRL_MS_CB_ORG				  = (0x00<<6),
	/*!< Ctrl-Byte, polling status */
	USPICTRL_MS_CB_STS				  = (0x01<<6),
	/*!< Ctrl-Byte, get response */
	USPICTRL_MS_CB_RSP				  = (0x02<<6),
	/*!< Ctrl-Byte, disable Ctrl-Byte mode */
	USPICTRL_MS_CB_DIS				  = (0x03<<6),
};

/*Event Bit define*/
/*define ISP control Master event Bit*/
enum MINI_ISP_EVENT {
	MINI_ISP_RCV_WAITING  = 0, /* 0x00000000*/
	MINI_ISP_RCV_CMD_READY = (1L << 0), /* 0x00000001*/
	MINI_ISP_RCV_BULKDATA = (1L << 1), /* 0x00000002*/
	MINI_ISP_RCV_CPCHANGE  = (1L << 2), /* 0x00000004*/
	MINI_ISP_RCV_SETSENSORMODE = (1L << 3),/* 0x00000008*/
	MINI_ISP_RCV_ERROR = (1L << 4),/* 0x00000010*/
	MINI_ISP_RCV_ERROR2 = (1L << 5),/* 0x00000020*/
	MINI_ISP_RCV_STRMOFF = (1L << 6),/* 0x00000040*/
};

/* Definition for IRQ status*/
#define COMMAND_COMPLETE 0x0001/*cmd deal complete by altek chip*/
#define BULK_DATA_COMPLETE 0x0002/*bulk data deal complete by altek chip*/
#define CP_STATUS_CHANGE_DONE 0x0004/*code persistence mode change finish*/
#define SET_SENSOR_MODE_READY 0x0008
#define SYSTEM_ERROR_LEVEL1 0x0010/*(Get Report)*/
#define SYSTEM_ERROR_LEVEL2 0x0020/*(Dump Register)*/
#define STRMOFF_READY 0x0040/*default not use*/

/******Public Type Declaration******/
struct misp_intf_fn_t {
	int (*send)(void *devdata, u32 len);
	int (*recv)(void *devdata, u32 len, bool waitINT);
	int (*read)(void *devdata, u8 *tx_buf, u32 tx_len, u8 *rx_buf, u32 rx_len);
	int (*write)(void *devdata, u8 *tx_buf, u8 *rx_buf, u32 len);
	int (*send_bulk)(void *devdata, struct file *filp, u32 total_size, u32 block_size, bool is_raw, u8 *Sendbulkbuffer);
};

struct misp_data {
	union {
		struct spi_device *spi;
		struct i2c_client *i2c;
		struct msm_camera_i2c_client *cci;
	} __attribute__((packed)) cfg;

	struct misp_intf_fn_t *intf_fn;
	int bulk_cmd_blocksize;
	u32 rx_dummy_len;
	u8 tx_buf[TX_BUF_SIZE];
	u8 rx_buf[RX_BUF_SIZE];
};

struct misp_global_variable {
	int vcc1_gpio;
	int vcc2_gpio;
	int vcc3_gpio;
	int irq_gpio;
	int irq_num;
	int reset_gpio;
	int intf_status;
	int before_booting;
	int i2c_enable;
	int altek_spi_mode;
	int be_set_to_bypass;
	int now_state;
	int spi_low_speed_mode;
	struct clk *isp_clk;
	struct mutex busy_lock;
};


struct irp_and_depth_information {
	u32 irp_image_address;
	u32 irp_width;
	u32 irp_height;
	u8 irp_format;/*Y_only:0  YUV:1*/
	u32 depth_image_address;
	u32 depth_width;
	u32 depth_height;
	u8 fov_mode;
} __attribute__ ((packed));

/******Public Function Prototype******/
extern irqreturn_t mini_isp_irq(int irq, void *handle);

/*miniisp_spi.c*/
extern struct misp_data *get_mini_isp_data(void);
extern void set_mini_isp_data(struct misp_data *data, int intf_type);
extern struct misp_data *get_mini_isp_intf(int i2c_type);
extern struct misp_global_variable *get_mini_isp_global_variable(void);
extern struct altek_statefsm *get_mini_isp_fsm(void);
extern int mini_isp_get_bulk(struct misp_data *devdata, u8 *response_buf,
					u32 total_size, u32 block_size);
//extern u16 mini_isp_get_status(void); //currently not use

/*miniisp_top.c*/
extern void mini_isp_e_to_a(void);
extern void mini_isp_a_to_e(void);
extern void mini_isp_chip_init(void);
extern void mini_isp_cp_mode_suspend_flow(void);
extern void mini_isp_cp_mode_resume_flow(void);
extern void mini_isp_check_and_leave_bypass_mode(void);
extern int mini_isp_pure_bypass(u16 mini_isp_mode);
extern void mini_isp_pure_bypass_debug(u16 mini_isp_mode);
extern int mini_isp_get_chip_id(u32 mini_isp_reg_addr, u8 *id_buf);
extern u32 mini_isp_register_read_then_write_file(u32 start_reg_addr,
				u32 end_reg_addr, char *module_name);
extern u32 mini_isp_register_read(u32 reg_addr, u32 *reg_value);
extern errcode mini_isp_debug_dump(void);
extern errcode mini_isp_memory_read_then_write_file(u32 start_addr, u32 len,
	char *file_name);
extern u32 mini_isp_memory_read(u32 start_addr, u8 *read_buffer, u32 len);
extern int mini_isp_get_altek_status(void *devdata, u32 *altek_status);
extern int mini_isp_wait_for_event(u32 MINI_ISP_EVENT);
extern u32 mini_isp_get_currentevent(void);
extern void mini_isp_register_write(u32 reg_addr, u32 reg_new_value);
extern void mini_isp_memory_write(u32 memory_addr, u8 *write_buffer,
				u32 write_len);
extern int mini_isp_debug_dump_img(void);
/*miniisp_utility.c*/
extern errcode mini_isp_utility_read_reg_e_mode(void);
extern errcode mini_isp_utility_read_reg_e_mode_for_bypass_use(void);
extern errcode mini_isp_utility_get_irp_and_depth_information(
	struct irp_and_depth_information *info);
extern u32 mini_isp_check_rx_dummy(u8 **recv_buffer, u32 rx_dummy_len);

/*miniisp_customer_define.c*/
extern void mini_isp_poweron(void);
extern void mini_isp_poweroff(void);
extern int mini_isp_gpio_init(struct device *spi,
			struct misp_data *drv_data,
			struct misp_global_variable *drv_global_variable);
#endif

