/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SENSOR_PYD1588_
#define _SENSOR_PYD1588_

#include <device.h>
#include <kernel.h>
#include <gpio.h>
#include <sensor.h>

#define	PYD1588_CONF_DEFAULT			0x10

#define	PYD1588_CONF_PDM_SIGN_CHG		(0 << 0)
#define	PYD1588_CONF_PDM_SIGN_NOCHG		(1 << 0)
#define	PYD1588_CONF_HPF_0_4HZ			(0 << 2)
#define	PYD1588_CONF_HPF_0_2HZ			(1 << 2)
#define	PYD1588_CONF_FILTER_SRC_BPF		(0 << 5)
#define	PYD1588_CONF_FILTER_SRC_LPF		(1 << 5)
#define	PYD1588_CONF_FILTER_SRC_TEMP	(3 << 5)
#define	PYD1588_CONF_MODE_FORCED		(0 << 7)
#define	PYD1588_CONF_MODE_INTERRUPT		(1 << 7)
#define	PYD1588_CONF_MODE_WAKEUP		(2 << 7)
#define	PYD1588_CONF_WINDOW_2S			(0 << 9)
#define	PYD1588_CONF_WINDOW_4S			(1 << 9)
#define	PYD1588_CONF_WINDOW_6S			(2 << 9)
#define	PYD1588_CONF_WINDOW_8S			(3 << 9)
#define	PYD1588_CONF_PULSE_CNT_1		(0 << 11)
#define	PYD1588_CONF_PULSE_CNT_2		(1 << 11)
#define	PYD1588_CONF_PULSE_CNT_3		(2 << 11)
#define	PYD1588_CONF_PULSE_CNT_4		(3 << 11)
#define	PYD1588_CONF_BLIND_TIME_05S		(0 << 13)
#define	PYD1588_CONF_BLIND_TIME_4S		(7 << 13)
#define	PYD1588_CONF_BLIND_TIME_8S		(15 << 13)
#define	PYD1588_CONF_THRESHOLD_0		(0 << 17)
#define	PYD1588_CONF_THRESHOLD_2		(2 << 17)
#define	PYD1588_CONF_THRESHOLD_4		(4 << 17)
#define	PYD1588_CONF_THRESHOLD_8		(8 << 17)
#define	PYD1588_CONF_THRESHOLD_16		(16 << 17)
#define	PYD1588_CONF_THRESHOLD_32		(32 << 17)
#define	PYD1588_CONF_THRESHOLD_64		(64 << 17)
#define	PYD1588_CONF_THRESHOLD_128		(128 << 17)
#define	PYD1588_CONF_THRESHOLD_255		(255 << 17)


struct pyd1588_data {

	u16_t pir_val;
	u32_t statcfg;

	struct device *serin_gpio;
	struct device *dl_gpio;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;

#if defined(CONFIG_PYD1588_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_PYD1588_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_PYD1588_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif


};


int pyd1588_write_config(struct pyd1588_data *drv_data, u32_t val);

void pyd1588_read(struct pyd1588_data *drv_data);

int pyd1588_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int pyd1588_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int pyd1588_init_interrupt(struct device *dev);

void pyd1588_reset_interrupt(struct device *dev);


#define SYS_LOG_DOMAIN "PYD1588"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#endif /* _SENSOR_PYD1588_ */
