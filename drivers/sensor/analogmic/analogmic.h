/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SENSOR_ANALOGMIC_
#define _SENSOR_ANALOGMIC_

#include <device.h>
#include <kernel.h>
#include <gpio.h>
#include <sensor.h>
#include <lpcomp.h>

struct analogmic_data {

	struct device *comp;
	lpcomp_callback_handler_t lpcomp_cb;

	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;

#if defined(CONFIG_ANALOGMIC_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_ANALOGMIC_THREAD_STACK_SIZE);
	struct k_sem comp_sem;
	struct k_thread thread;
#elif defined(CONFIG_ANALOGMIC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif


};


int analogmic_write_config(struct analogmic_data *drv_data, u32_t val);

void analogmic_read(struct analogmic_data *drv_data);

int analogmic_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int analogmic_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int analogmic_init_interrupt(struct device *dev);


#define SYS_LOG_DOMAIN "ANALOGMIC"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#endif /* _SENSOR_ANALOGMIC_ */
