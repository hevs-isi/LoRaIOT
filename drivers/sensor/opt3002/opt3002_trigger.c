/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "opt3002.h"

static u16_t opt3002_power_processed_to_raw(const struct sensor_value *val)
{
	u8_t exp;
	u16_t mantissa;

	/* TODO: convert optical power to register */
	exp = (u8_t)*val->val1;
	mantissa = (u16_t)*val->val2;

	return (exp << 12) & 0xF000 + mantissa & 0x0FFF;

}

int opt3002_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	struct opt3002_data *drv_data = dev->driver_data;
	u8_t reg;
	u16_t reg_val;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		reg = OPT3002_REG_LIM_LOW;
	} else if (attr == SENSOR_ATTR_UPPER_THRESH) {
		reg = OPT3002_REG_LIM_HIGH;
	} else {
		return -ENOTSUP;
	}

	reg_val = opt3002_power_processed_to_raw(val);

	if (opt3002_write_reg(drv_data, reg, reg_val) < 0) {
		SYS_LOG_DBG("Failed to write threshold value!");
		return -EIO;
	}

	return 0;
}

static void opt3002_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, u32_t pins)
{
	struct opt3002_data *drv_data =
		CONTAINER_OF(cb, struct opt3002_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, CONFIG_OPT3002_GPIO_PIN_NUM);

#if defined(CONFIG_OPT3002_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_OPT3002_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void opt3002_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct opt3002_data *drv_data = dev->driver_data;
	u16_t status;

	if (opt3002_read_reg(drv_data, OPT3002_REG_CONFIG, &status) < 0) {
		return;
	}

	if ((status & OPT3002_CONF_CRF) &&
		drv_data->drdy_handler != NULL) {
		drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
	}

	if (((status & OPT3002_CONF_FL) || (status & OPT3002_CONF_FH)) &&
		drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, &drv_data->th_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_OPT3002_GPIO_PIN_NUM);
}

#ifdef CONFIG_OPT3002_TRIGGER_OWN_THREAD
static void opt3002_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct opt3002_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		opt3002_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_OPT3002_TRIGGER_GLOBAL_THREAD
static void opt3002_work_cb(struct k_work *work)
{
	struct opt3002_data *drv_data =
		CONTAINER_OF(work, struct opt3002_data, work);

	opt3002_thread_cb(drv_data->dev);
}
#endif

int opt3002_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct opt3002_data *drv_data = dev->driver_data;

	if (trig->type != SENSOR_TRIG_THRESHOLD && trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	gpio_pin_disable_callback(drv_data->gpio, CONFIG_OPT3002_GPIO_PIN_NUM);

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = *trig;
	} else if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->th_handler = handler;
		drv_data->th_trigger = *trig;
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_OPT3002_GPIO_PIN_NUM);

	return 0;
}

int opt3002_init_interrupt(struct device *dev)
{
	struct opt3002_data *drv_data = dev->driver_data;

	// enable end-of-conversion
	if (opt3002_write_reg(drv_data, OPT3002_REG_LIM_LOW, 0xC000) < 0) {
		SYS_LOG_DBG("Failed to write register %X!", OPT3002_REG_LIM_LOW);
		return -EIO;
	}

	/* setup gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_OPT3002_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    CONFIG_OPT3002_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_OPT3002_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   opt3002_gpio_callback,
			   BIT(CONFIG_OPT3002_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_OPT3002_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_OPT3002_THREAD_STACK_SIZE,
			(k_thread_entry_t)opt3002_thread, POINTER_TO_INT(dev),
			0, NULL, K_PRIO_COOP(CONFIG_OPT3002_THREAD_PRIORITY),
			0, 0);
#elif defined(CONFIG_OPT3002_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = opt3002_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
