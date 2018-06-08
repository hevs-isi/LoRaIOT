/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SENSOR_OPT3002_
#define _SENSOR_OPT3002_

#include <device.h>
#include <kernel.h>

#define OPT3002_I2C_ADDRESS		CONFIG_OPT3002_I2C_ADDR

#define OPT3002_REG_RESULT		0x00
#define OPT3002_REG_CONFIG		0x01
#define OPT3002_REG_LIM_LOW		0x02
#define OPT3002_REG_LIM_HIGH	0x03
#define OPT3002_REG_MANID		0x7E

#define OPT3002_CONF_FC			BIT(0)
#define OPT3002_CONF_FC_MASK	BIT(0) & BIT(1)
#define OPT3002_CONF_ME			BIT(2)
#define OPT3002_CONF_POL		BIT(3)
#define OPT3002_CONF_L			BIT(4)
#define OPT3002_CONF_FL			BIT(5)
#define OPT3002_CONF_FH			BIT(6)
#define OPT3002_CONF_CRF		BIT(7)
#define OPT3002_CONF_OVF		BIT(8)
#define OPT3002_CONF_MODE		BIT(9)
#define OPT3002_CONF_MODE_MASK	BIT(9) & BIT(10)
#define OPT3002_CONF_CT			BIT(11)
#define OPT3002_CONF_RN			BIT(12)
#define OPT3002_CONF_RN_MASK	BIT(12) & BIT(13) & BIT(14) & BIT(15)

#define OPT3002_MODE_SHUTDOWN	0x00
#define OPT3002_MODE_SINGLESHOT	0x01
#define OPT3002_MODE_CONTINUOUS	0x11

struct opt3002_data {
	struct device *i2c;
	u16_t conf_reg;
	u16_t sample;

#ifdef CONFIG_OPT3002_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	u16_t opt_low;
	u16_t opt_high;

	sensor_trigger_handler_t drdy_handler;
	struct sensor_trigger drdy_trigger;

	sensor_trigger_handler_t th_handler;
	struct sensor_trigger th_trigger;

#if defined(CONFIG_OPT3002_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_OPT3002_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_OPT3002_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_OPT3002_TRIGGER */
};

#ifdef CONFIG_OPT3002_TRIGGER
int opt3002_write_reg(struct opt3002_data *drv_data, u8_t reg, u16_t val);

int opt3002_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int opt3002_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int opt3002_init_interrupt(struct device *dev);
#endif

#define SYS_LOG_DOMAIN "OPT3002"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* _SENSOR_OPT3002_ */
