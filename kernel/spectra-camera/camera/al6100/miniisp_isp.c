/*
 * File: miniisp_spi.c
 * Description: Mini ISP sample codes
 *
 * (C)Copyright altek Corporation 2017
 *
 *  2017/04/11; LouisWang; Initial version
 */

/************************************************************
*            Include File                                   *
*************************************************************/
/* Linux headers*/
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/buffer_head.h>
#include <linux/of_gpio.h>


#include "include/miniisp.h"
#include "include/miniisp_ctrl.h"
#include "include/miniisp_customer_define.h"
#include "include/miniisp_chip_base_define.h"
#include "include/altek_statefsm.h"

#include "include/error/miniisp_err.h"


#include "include/miniisp_ctrl_intf.h"
#ifdef ALTEK_TEST
#include "include/altek_test.h"
#endif

/****************************************************************************
*			             Private Constant Definition	                    *
****************************************************************************/
/*#define DEBUG_ALERT*/
#define MINI_ISP_LOG_TAG "[miniisp_isp]"
/*drv debug defination*/
#define _SPI_DEBUG

/****************************************************************************
*                        Private Global Variable                            *
****************************************************************************/
static struct misp_global_variable *misp_drv_global_variable;
static struct class *mini_isp_class;
static struct device *mini_isp_dev;
struct altek_statefsm *altek_state;
static int dump_reg_range;
// ALTEK_AL6100_ECHO >>>
struct file *l_internal_file[OTHER_MAX];
// ALTEK_AL6100_ECHO <<<
/************************************************************
*          Public Global Variable                           *
*************************************************************/

/************************************************************
*          Private Macro Definition                         *
*************************************************************/

/************************************************************
*          Public Function Prototype                        *
*************************************************************/
/// AL6100 debug tool >>>
extern struct device *miniisp_chdev_create(struct class *mini_isp_class);
/// AL6100 debug tool <<<
extern struct misp_data *get_mini_isp_intf_spi(void);
extern struct misp_data *get_mini_isp_intf_i2c(int i2c_type);
//extern struct misp_data *get_mini_isp_intf_cci(int i2c_type);

/************************************************************
*                    Private Function                       *
*************************************************************/

static ssize_t mini_isp_mode_config_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/*misp_info("%s - mini_isp_spi_send return %d", __func__, ret);*/
	return snprintf(buf, 32, "load fw:0 e_to_a:1 a_to_e:2\n");
}

static ssize_t mini_isp_mode_config_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	u8 buf_chip_id_use[4];

	if ('0' == buf[0]) {
		mini_isp_poweron();
		mini_isp_get_chip_id(CHIP_ID_ADDR, buf_chip_id_use);
		mini_isp_chip_init();
		mini_isp_e_to_a();
	} else if ('1' == buf[0]) {
		mini_isp_chip_init();
		mini_isp_e_to_a();
	} else if ('2' == buf[0]) {
		mini_isp_a_to_e();
	} else if ('4' == buf[0]) {
		mini_isp_drv_load_fw();
	} else if ('7' == buf[0]) {
		buf_chip_id_use[0] = 0;
		mini_isp_debug_dump_img();
		mini_isp_a_to_e();
		mini_isp_chip_base_dump_irp_and_depth_based_register();
		mini_isp_memory_write(0x10, buf_chip_id_use, 1);
		mini_isp_e_to_a();
// ALTEK_AL6100_ECHO >>>
	}  else if ('5' == buf[0]) {
	   struct isp_cmd_led_power_control control_param;
	   struct isp_cmd_active_ae active_ae;
	   memset(&control_param, 0, sizeof(struct isp_cmd_led_power_control));
	   memset(&active_ae, 0, sizeof(struct isp_cmd_active_ae));
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
		mini_isp_other_drv_read(l_internal_file[IQ_CODE], IQ_CODE); // IQ calibration data

		mini_isp_other_drv_open(DEPTHPACKDATA_FILE_LOCATION, DEPTH_CODE);
		mini_isp_other_drv_read(l_internal_file[DEPTH_CODE], DEPTH_CODE); // Depth calibration data

		mini_isp_drv_write_calibration_data(2, NULL, 0); // Scenario table
		mini_isp_drv_write_calibration_data(3, NULL, 0); // HDR Qmerge
		mini_isp_drv_write_calibration_data(4, NULL, 0); // IRP0 Qmerge
		mini_isp_drv_write_calibration_data(5, NULL, 0); // IRP1 Qmerge
		// mini_isp_drv_write_calibration_data(7, NULL, 0); // Blending table for ground depth
		// mini_isp_drv_write_calibration_data(8, NULL, 0); // Depth Qmerge

		// set depth output resolution
		mini_isp_drv_set_output_format(19, 7);// set depth output resolution
		active_ae.active_ae = 1;
		active_ae.f_number_x1000 = 2000;
		mini_isp_drv_active_ae(&active_ae);
		mini_isp_drv_set_sensor_mode(1,4,0,0,0);  // Set sensor mode
		//projector control
		control_param.led_on_off = 1;
		control_param.control_mode = 3;
		control_param.led_power_level = 255;
		control_param.control_projector_id = 0;
		mini_isp_drv_led_power_control(&control_param);
		mini_isp_drv_preview_stream_on_off(1,1); // open preview
		mini_isp_drv_isp_ae_control_mode_on_off(1);
		}else if ('6' == buf[0]) {
		 // close auto depth mode
		 struct isp_cmd_led_power_control control_param;
		 struct isp_cmd_active_ae active_ae;
		 memset(&control_param, 0, sizeof(struct isp_cmd_led_power_control));
		 memset(&active_ae, 0, sizeof(struct isp_cmd_active_ae));
		 mini_isp_drv_preview_stream_on_off(0,0); // close preview
		 mini_isp_drv_isp_ae_control_mode_on_off(0);
		 mini_isp_drv_led_power_control(&control_param);
		 mini_isp_drv_set_sensor_mode(0,0,0,0,0);  // Set sensor mode
		 mini_isp_drv_active_ae(&active_ae);
		 mini_isp_drv_set_output_format(0,0); // set depth output resolution:
		 // ALTEK_AL6100_ECHO >>>	                 // 0: Disable depth function (Depth engine is disable)
	}  else {
		//mini_isp_poweron();
		mini_isp_get_chip_id(CHIP_ID_ADDR, buf_chip_id_use);
		mini_isp_pure_bypass(1);
	}
	return size;
}

static DEVICE_ATTR(mini_isp_mode_config, 0660, mini_isp_mode_config_show,
		mini_isp_mode_config_store);

static ssize_t mini_isp_reset_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = -EINVAL;

	ret = gpio_get_value(misp_drv_global_variable->reset_gpio);
	misp_info("%s - reset_gpio is %d", __func__, ret);

	return snprintf(buf, 32, "%d", ret);
}

static ssize_t mini_isp_reset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	if ('0' == buf[0])
		gpio_set_value(misp_drv_global_variable->reset_gpio, 0);
	else
		gpio_set_value(misp_drv_global_variable->reset_gpio, 1);

	misp_info("%s - ", __func__);

	return size;
}

static DEVICE_ATTR(mini_isp_reset, 0660, mini_isp_reset_show, mini_isp_reset_store);


static ssize_t mini_isp_poweron_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = -EINVAL;

	ret = gpio_get_value(misp_drv_global_variable->vcc1_gpio);
	misp_info("%s - vcc1_gpio is %d", __func__, ret);
	return snprintf(buf, 32, "%d", ret);
}

static ssize_t mini_isp_poweron_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	/*int ret = -EINVAL;*/
	if ('1' == buf[0])
		mini_isp_poweron();
	else
		mini_isp_poweroff();

	misp_info("%s - ", __func__);

	return size;
}

static DEVICE_ATTR(mini_isp_poweron, S_IRUSR | S_IWUSR, mini_isp_poweron_show,
		mini_isp_poweron_store);

static ssize_t mini_isp_dump_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = -EINVAL;

	misp_info("%s - enter", __func__);
	mini_isp_a_to_e();
	if (dump_reg_range == 0)
		ret = mini_isp_utility_read_reg_e_mode();
	else if (dump_reg_range == 1)
		ret = mini_isp_utility_read_reg_e_mode_for_bypass_use();
	else
		ret = mini_isp_chip_base_dump_irp_and_depth_based_register();

	if (!ret)
		return snprintf(buf, 32, "dump reg success!!\n");
	else
		return snprintf(buf, 32, "dump reg fail!!\n");
}

static ssize_t mini_isp_dump_reg_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	/*int ret = -EINVAL;*/
	/*0 means dump all reg value, 1 means dump bypass mode reg value */
	if ('0' == buf[0])
		dump_reg_range = 0;
	else if ('1' == buf[0])
		dump_reg_range = 1;
	else
		dump_reg_range = 2;
	misp_info("%s - ", __func__);

	return size;
}

static DEVICE_ATTR(mini_isp_dump_reg, 0660, mini_isp_dump_reg_show,
		mini_isp_dump_reg_store);



/************************************************************
*                    Public Function                        *
*************************************************************/

struct misp_data *get_mini_isp_intf(int i2c_type)
{
	if (misp_drv_global_variable->intf_status & INTF_SPI_READY) {
		return get_mini_isp_intf_spi();
	} else if (misp_drv_global_variable->intf_status & INTF_I2C_READY) {
		return get_mini_isp_intf_i2c(i2c_type);
	/*} else if (misp_drv_global_variable->intf_status & INTF_CCI_READY) {
		return get_mini_isp_intf_cci(i2c_type);*/
	} else {
		misp_err("%s - error i2c type %d", __func__,i2c_type);
		return NULL;
	}
}
void set_mini_isp_data(struct misp_data *data, int intf_type)
{
	if (!misp_drv_global_variable) {
		misp_err("%s - set global_variable error", __func__);
	} else {
		misp_drv_global_variable->intf_status |= intf_type;
	}
}

struct misp_global_variable *get_mini_isp_global_variable(void)
{
	if (!misp_drv_global_variable) {
		misp_err("%s - get global_variable error", __func__);
		return NULL;
	} else {
		return misp_drv_global_variable;
	}
}

struct altek_statefsm *get_mini_isp_fsm(void)
{
	if (!altek_state) {
		misp_err("%s - get fsm error", __func__);
		return NULL;
	} else {
		return altek_state;
	}
}


int mini_isp_setup_resource(struct device *dev, struct misp_data *drv_data)
{
	int status = 0;
	misp_info("%s - start", __func__);
	if (misp_drv_global_variable != NULL ) {
		misp_err("%s - resource already been setupped", __func__);
		goto setup_done;
	}

	/*step 1: alloc misp_drv_global_variable*/
	misp_drv_global_variable = kzalloc(sizeof(*misp_drv_global_variable), GFP_KERNEL);
	if (!misp_drv_global_variable) {
		misp_info("%s - Out of memory", __func__);
		status = -ENOMEM;
		goto alloc_fail;
	}
	misp_info("%s - step1 done.", __func__);

	/*step 2: init mutex and gpio resource*/
	mutex_init(&misp_drv_global_variable->busy_lock);
	status = mini_isp_gpio_init(dev, drv_data, misp_drv_global_variable);
	if (status < 0) {
		misp_info("%s - gpio init fail", __func__);
		goto setup_fail;
	}
	misp_info("%s - step2 done.", __func__);

	misp_drv_global_variable->before_booting = 1;


	/*step 3: register to VFS as character device*/
	mini_isp_class = class_create(THIS_MODULE, "mini_isp");
	if (IS_ERR(mini_isp_class))
		misp_err("Failed to create class(mini_isp_class)!");
	mini_isp_dev = miniisp_chdev_create(mini_isp_class);
	if (IS_ERR(mini_isp_dev))
		misp_err("Failed to create device(mini_isp_dev)!");

	status = device_create_file(mini_isp_dev, &dev_attr_mini_isp_mode_config);
	if (status < 0)
		misp_err("Failed to create device file(%s)!", dev_attr_mini_isp_mode_config.attr.name);

	if (RESET_GPIO != NULL) {
		status = device_create_file(mini_isp_dev, &dev_attr_mini_isp_reset);
		if (status < 0)
			misp_err("Failed to create device file(%s)!", dev_attr_mini_isp_reset.attr.name);
	}

	status = device_create_file(mini_isp_dev, &dev_attr_mini_isp_poweron);
	if (status < 0)
		misp_err("Failed to create device file(%s)!", dev_attr_mini_isp_poweron.attr.name);

	status = device_create_file(mini_isp_dev, &dev_attr_mini_isp_dump_reg);
	if (status < 0)
		misp_err("Failed to create device file(%s)!", dev_attr_mini_isp_dump_reg.attr.name);

	misp_info("%s - step3 done.", __func__);

	misp_info("%s - success.", __func__);
	goto setup_done;

setup_fail:
	mutex_destroy(&misp_drv_global_variable->busy_lock);
	kfree(misp_drv_global_variable);

alloc_fail:
	misp_drv_global_variable = NULL;

setup_done:
	return status;
}

static int __init mini_isp_init(void)
{
	int ret = 0;
	//struct altek_statefsm *fsm = NULL;

	misp_info("%s - start", __func__);

	//fsm = altek_statefsmcreate();
	//altek_state = fsm;

	misp_info("%s - success", __func__);

	return ret;
}

static void __exit mini_isp_exit(void)
{
	misp_info("%s", __func__);
	//free_irq(misp_drv_data->spi->irq, misp_drv_data);
	if (misp_drv_global_variable->irq_gpio)
		gpio_free(misp_drv_global_variable->irq_gpio);

	if (misp_drv_global_variable)
		kfree(misp_drv_global_variable);
	//spi_unregister_driver(&mini_isp_drv);

	altek_statefsmdelete(altek_state);
	altek_state = NULL;
}

module_init(mini_isp_init);
module_exit(mini_isp_exit);
MODULE_LICENSE("Dual BSD/GPL");
