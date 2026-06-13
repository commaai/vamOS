/*
 * File: miniisp_customer_define.c
 * Description: Mini ISP sample codes
 *
 * (C)Copyright altek Corporation 2017
 *
 *  2017/03/14;LouisWang; Initial version
 */

/******Include File******/
/* Linux headers*/
#include <linux/delay.h>
#include  <linux/of_gpio.h>

#include "include/miniisp_customer_define.h"
#include "include/miniisp.h"
#include "include/miniisp_ctrl.h"
#include "include/altek_statefsm.h"

#define MINI_ISP_LOG_TAG "[miniisp_customer_define]"

extern void mini_isp_poweron(void)
{
	errcode ret = 0;
	void *devdata;
	struct misp_global_variable *dev_global_variable;
	struct altek_statefsm *fsm;

	misp_info("%s - enter", __func__);
	fsm = get_mini_isp_fsm();
	dev_global_variable = get_mini_isp_global_variable();
	/*reset mini-isp keep low for at least 200us, release to high for 20ms*/
	dev_global_variable->before_booting = 1;
	dev_global_variable->be_set_to_bypass = 0;

	devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);

	/*state check*/
	ret = altek_statefsmispdrv_open(fsm, ((void *)devdata));
	if (ret != 0)
		misp_err("%s err, %x", __func__, ret);

	dev_global_variable->now_state = 1;
}
EXPORT_SYMBOL(mini_isp_poweron);

extern void mini_isp_poweroff(void)
{
	void *devdata;
	struct misp_global_variable *dev_global_variable;
	int ret = 0;
	struct altek_statefsm *fsm;

	fsm = get_mini_isp_fsm();
	misp_err("[miniISP]mini_isp_poweroff");
	dev_global_variable = get_mini_isp_global_variable();
    /*
	if (dev_global_variable->i2c_enable)
		devdata = (void *)get_mini_isp_i2c_slave_data();
	else
		devdata = (void *)get_mini_isp_data();
    */
    devdata = get_mini_isp_intf(MINIISP_I2C_SLAVE);

	/*state check*/
	ret = altek_statefsmispdrv_close(fsm, devdata);
	if (ret != 0) {
		misp_err("%s err, %x", __func__, ret);
		return;
	}

	dev_global_variable->now_state = 0;
	misp_info("%s - X", __func__);
}
EXPORT_SYMBOL(mini_isp_poweroff);

#if 0 //not used currently
static int misp_ts_pinctrl_init(struct device *dev, int on)
{
	int retval = 0;
	struct pinctrl *misp_pinctrl;
	struct pinctrl_state *gpio_state_active;
	struct pinctrl_state *gpio_state_suspend;

	struct pinctrl_state *pins_state;

	/* Get pinctrl if target uses pinctrl */
	misp_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(misp_pinctrl)) {
		dev_dbg(&spi->dev,
			"Target does not use pinctrl");
		retval = PTR_ERR(misp_pinctrl);
		misp_pinctrl = NULL;
		goto misp_ts_pinctrl_init_end;
	}

	gpio_state_active
		= pinctrl_lookup_state(misp_pinctrl,
			"pmx_ts_active");
	if (IS_ERR_OR_NULL(gpio_state_active)) {
		dev_dbg(dev),
			"Can not get ts default pinstate");
		retval = PTR_ERR(gpio_state_active);
		misp_pinctrl = NULL;
		goto misp_ts_pinctrl_init_end;
	}

	gpio_state_suspend
		= pinctrl_lookup_state(misp_pinctrl,
			"pmx_ts_suspend");
	if (IS_ERR_OR_NULL(gpio_state_suspend)) {
		dev_err(dev,
			"Can not get ts sleep pinstate");
		retval = PTR_ERR(gpio_state_suspend);
		misp_pinctrl = NULL;
		goto misp_ts_pinctrl_init_end;
	}

	pins_state = on ? gpio_state_active
		: gpio_state_suspend;
	if (!IS_ERR_OR_NULL(pins_state)) {
		retval = pinctrl_select_state(misp_pinctrl, pins_state);
		if (retval) {
			dev_err(dev,
				"can not set %s pins",
				on ? "pmx_ts_active" : "pmx_ts_suspend");
			goto misp_ts_pinctrl_init_end;
		}
	} else {
		dev_err(dev,
			"not a valid '%s' pinstate",
				on ? "pmx_ts_active" : "pmx_ts_suspend");
	}

misp_ts_pinctrl_init_end:
	return retval;
}
#endif

extern int mini_isp_gpio_init(struct device *dev,
			struct misp_data *drv_data,
			struct misp_global_variable *drv_global_variable)
{
	int ret = 0;

	if (VCC1_GPIO != NULL) {
		drv_global_variable->vcc1_gpio =
			of_get_named_gpio(dev->of_node, VCC1_GPIO, 0);
		misp_info("%s - probe vcc1-gpios = %d", __func__,
			drv_global_variable->vcc1_gpio);

		ret = devm_gpio_request(dev,
			drv_global_variable->vcc1_gpio, VCC1_GPIO);
		if (ret) {
			misp_err("%s -step 4. request vcc1-gpio error",
				__func__);
			goto err_gpio1_config;
		}

		gpio_direction_output(drv_global_variable->vcc1_gpio, 1);
		msleep(20);
		gpio_set_value(drv_global_variable->vcc1_gpio, 1);
		msleep(20);
	}

	if (VCC2_GPIO != NULL) {
		drv_global_variable->vcc2_gpio = of_get_named_gpio(
			dev->of_node, VCC2_GPIO, 0);
		misp_info("%s - probe vcc2-gpios = %d", __func__,
			drv_global_variable->vcc2_gpio);

		ret = devm_gpio_request(dev,
			drv_global_variable->vcc2_gpio, VCC2_GPIO);
		if (ret) {
			misp_err("%s -step 4. request vcc2-gpios error",
				__func__);
			goto err_gpio2_config;
		}

		gpio_direction_output(drv_global_variable->vcc2_gpio, 1);
		msleep(20);
		gpio_set_value(drv_global_variable->vcc2_gpio, 1);
		msleep(20);
	}

	if (VCC3_GPIO != NULL) {
		drv_global_variable->vcc3_gpio = of_get_named_gpio(
			dev->of_node, VCC3_GPIO, 0);
		misp_err("%s - probe vcc3-gpios = %d", __func__,
					drv_global_variable->vcc3_gpio);

		ret = devm_gpio_request(dev,
			drv_global_variable->vcc3_gpio, VCC3_GPIO);
		if (ret) {
			misp_err("%s -step 4. request vcc3-gpio error",
				__func__);
			goto err_gpio_config;
		}

		gpio_direction_output(drv_global_variable->vcc3_gpio, 1);
		gpio_set_value(drv_global_variable->vcc3_gpio, 1);
		msleep(20);

	}
	if (ISP_CLK != NULL) {
		drv_global_variable->isp_clk = devm_clk_get(dev,
						ISP_CLK);
		misp_err("clk_ptr = %p", drv_global_variable->isp_clk);
		ret = clk_set_rate(drv_global_variable->isp_clk, 19200000L);
		if (ret < 0)
			misp_err("clk_set_rate failed, not fatal\n");

		misp_err("clk_get_rate %ld\n", clk_get_rate(
					drv_global_variable->isp_clk));
		ret = clk_prepare_enable(drv_global_variable->isp_clk);
		if (ret < 0) {
			misp_err("clk_prepare_enable failed\n");
			goto err_clk_config;
		}
		msleep(20);
	}

	if (RESET_GPIO != NULL) {
		drv_global_variable->reset_gpio =
			of_get_named_gpio(dev->of_node, RESET_GPIO, 0);
		misp_info("%s - probe reset_gpio = %d", __func__,
			drv_global_variable->reset_gpio);

		ret = devm_gpio_request(dev,
			drv_global_variable->reset_gpio, RESET_GPIO);
		if (ret) {
			misp_err("%s -step 4. request reset gpio error",
				__func__);
			goto err_reset_config;
		}

		gpio_direction_output(drv_global_variable->reset_gpio, 0);
		gpio_set_value(drv_global_variable->reset_gpio, 0);
		msleep(20);

	}

	if (IRQ_GPIO != NULL) {

		drv_global_variable->irq_gpio = of_get_named_gpio(dev->of_node, IRQ_GPIO, 0);

		//drv_global_variable->irq_gpio = of_get_named_gpio(dev->of_node, IRQ_GPIO, &gpio_flag);
		misp_info("%s - probe irq_gpio = %d", __func__, drv_global_variable->irq_gpio);

		/*ret = devm_gpio_request(dev,
			drv_global_variable->irq_gpio, IRQ_GPIO);*/
		ret = gpio_request(drv_global_variable->irq_gpio, IRQ_GPIO);
		if (ret) {
			misp_err("%s -step 4. request irq gpio error",
				__func__);
			goto err_irq_config;
		}
		gpio_direction_input(drv_global_variable->irq_gpio);

		drv_global_variable->irq_num = gpio_to_irq(drv_global_variable->irq_gpio);

		misp_err("%s - probe spi->irq = %d %d ",
			__func__, drv_global_variable->irq_num,
			gpio_to_irq(drv_global_variable->irq_gpio));

		ret = request_threaded_irq(drv_global_variable->irq_num, NULL, mini_isp_irq,
		IRQF_TRIGGER_HIGH | IRQF_ONESHOT, "mini_isp", drv_data);

		if (ret) {
			misp_err("%s - step4. probe - request irq error",
				__func__);
			goto err_dev_attr;
		}
		misp_info("%s - step4 done. irq number:%d", __func__,
			drv_global_variable->irq_num);

		free_irq(drv_global_variable->irq_num, drv_data);
	}

	/*step 5:other additional config*/

	misp_info("%s - step5 done", __func__);

	if (RESET_GPIO != NULL) {
		gpio_direction_output(drv_global_variable->reset_gpio, 1);
		gpio_set_value(drv_global_variable->reset_gpio, 1);
		msleep(20);
	}

	return ret;

err_dev_attr:
	free_irq(drv_global_variable->irq_num, drv_data);
err_irq_config:
	if (IRQ_GPIO != NULL)
		gpio_free(drv_global_variable->irq_gpio);

err_reset_config:
	if (RESET_GPIO != NULL)
		gpio_free(drv_global_variable->reset_gpio);

err_clk_config:
	if (ISP_CLK != NULL)
		clk_disable_unprepare(drv_global_variable->isp_clk);

err_gpio_config:
	if (VCC2_GPIO != NULL)
		gpio_free(drv_global_variable->vcc2_gpio);
err_gpio2_config:
	if (VCC1_GPIO != NULL)
		gpio_free(drv_global_variable->vcc1_gpio);

err_gpio1_config:

	return ret;
}

