/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "analogmic.h"

static k_tid_t tid;

extern struct analogmic_data analogmic_driver;

int analogmic_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	return 0;
}

static void analogmic_lpcomp_callback()
{
	// TODO: find the proper way to use the line bellow
	/*struct analogmic_data *drv_data =
		CONTAINER_OF(cb, struct analogmic_data, lpcomp_cb);*/

	SYS_LOG_DBG("analogmic_lpcomp_callback");

#if defined(CONFIG_PYD1588_TRIGGER_OWN_THREAD)
	k_sem_give(&analogmic_driver.comp_sem);
#elif defined(CONFIG_PYD1588_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void analogmic_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct analogmic_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("Sound detected");

	if (drv_data->handler != NULL) {
		drv_data->handler(dev, &drv_data->trigger);
	}

}

#ifdef CONFIG_ANALOGMIC_TRIGGER_OWN_THREAD
static void analogmic_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct analogmic_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		SYS_LOG_DBG("Sound thread waiting semaphore");
		k_sem_take(&drv_data->comp_sem, K_FOREVER);
		analogmic_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_ANALOGMIC_TRIGGER_GLOBAL_THREAD
static void analogmic_work_cb(struct k_work *work)
{
	struct analogmic_data *drv_data =
		CONTAINER_OF(work, struct analogmic_data, work);

	analogmic_thread_cb(drv_data->dev);
}
#endif

int analogmic_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct analogmic_data *drv_data = dev->driver_data;

	if (trig->type != SENSOR_TRIG_TAP) {
		return -ENOTSUP;
	}

	//gpio_pin_disable_callback(drv_data->dl_gpio, CONFIG_ANALOGMIC_OUT_GPIO_PIN_NUM);
	drv_data->handler = handler;
	drv_data->trigger = *trig;
	//gpio_pin_enable_callback(drv_data->dl_gpio, CONFIG_ANALOGMIC_DL_GPIO_PIN_NUM);
	SYS_LOG_DBG("Sound trigger set");
	return 0;
}

int analogmic_init_interrupt(struct device *dev)
{
	struct analogmic_data *drv_data = dev->driver_data;

	if(tid){
		return 0;
	}

	drv_data->comp = device_get_binding(CONFIG_LPCOMP_NRF5_DEV_NAME);
	lpcomp_callback_set(drv_data->comp, (lpcomp_callback_handler_t)analogmic_lpcomp_callback);
	lpcomp_enable(drv_data->comp);

	SYS_LOG_DBG("Init analogmic trigger!");

#if defined(CONFIG_ANALOGMIC_TRIGGER_OWN_THREAD)

	k_sem_init(&drv_data->comp_sem, 0, UINT_MAX);

	tid = k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ANALOGMIC_THREAD_STACK_SIZE,
			(k_thread_entry_t)analogmic_thread, POINTER_TO_INT(dev),
			0, NULL, K_PRIO_COOP(CONFIG_ANALOGMIC_THREAD_PRIORITY),
			0, 0);

#elif defined(CONFIG_ANALOGMIC_TRIGGER_GLOBAL_THREAD)
	tid = 1;
	drv_data->work.handler = analogmic_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
