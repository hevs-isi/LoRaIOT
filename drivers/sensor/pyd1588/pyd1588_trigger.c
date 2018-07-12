/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "pyd1588.h"

static k_tid_t tid;

int pyd1588_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{

	return 0;
}

static void pyd1588_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, u32_t pins)
{
	struct pyd1588_data *drv_data =
		CONTAINER_OF(cb, struct pyd1588_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, CONFIG_PYD1588_DL_GPIO_PIN_NUM);

#if defined(CONFIG_PYD1588_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_PYD1588_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void pyd1588_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct pyd1588_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("Motion detected");

	pyd1588_read(drv_data);

	if (drv_data->handler != NULL) {
		drv_data->handler(dev, &drv_data->trigger);
	}

	// reset interrupt
	gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_OUT);
	gpio_pin_write(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, 0);

	gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM,
				   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				   GPIO_INT_ACTIVE_HIGH);

	gpio_pin_enable_callback(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM);

}

#ifdef CONFIG_PYD1588_TRIGGER_OWN_THREAD
static void pyd1588_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct pyd1588_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	SYS_LOG_DBG("Motion thread started");
	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		pyd1588_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_PYD1588_TRIGGER_GLOBAL_THREAD
static void pyd1588_work_cb(struct k_work *work)
{
	struct pyd1588_data *drv_data =
		CONTAINER_OF(work, struct pyd1588_data, work);

	pyd1588_thread_cb(drv_data->dev);
}
#endif

int pyd1588_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct pyd1588_data *drv_data = dev->driver_data;

	if (trig->type != SENSOR_TRIG_TAP) {
		return -ENOTSUP;
	}

	gpio_pin_disable_callback(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM);
	drv_data->handler = handler;
	drv_data->trigger = *trig;
	gpio_pin_enable_callback(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM);

	SYS_LOG_DBG("Motion trigger set");

	return 0;
}

int pyd1588_init_interrupt(struct device *dev)
{
	struct pyd1588_data *drv_data = dev->driver_data;

	if(tid){
		return 0;
	}

	/* setup gpio interrupt */
	drv_data->dl_gpio = device_get_binding(CONFIG_PYD1588_DL_GPIO_DEV_NAME);
	if (drv_data->dl_gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    CONFIG_PYD1588_DL_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH);

	gpio_init_callback(&drv_data->gpio_cb,
			   pyd1588_gpio_callback,
			   BIT(CONFIG_PYD1588_DL_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->dl_gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

	SYS_LOG_DBG("Init trigger!");

#if defined(CONFIG_PYD1588_TRIGGER_OWN_THREAD)

	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	tid = k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_PYD1588_THREAD_STACK_SIZE,
			(k_thread_entry_t)pyd1588_thread, POINTER_TO_INT(dev),
			0, NULL, K_PRIO_COOP(CONFIG_PYD1588_THREAD_PRIORITY),
			0, 0);

#elif defined(CONFIG_PYD1588_TRIGGER_GLOBAL_THREAD)
	tid = 1;
	drv_data->work.handler = pyd1588_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
