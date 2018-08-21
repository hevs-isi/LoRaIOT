/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SENSOR_PAC1934_
#define _SENSOR_PAC1934_

#include <device.h>
#include <kernel.h>

#define PAC1934_I2C_ADDRESS		CONFIG_PAC1934_I2C_ADDR

#define PAC1934_RSENSE_CH1		1.5	// Ohm
#define PAC1934_RSENSE_CH2		3.0 // Ohm
#define PAC1934_RSENSE_CH3		1.5 // Ohm
#define PAC1934_RSENSE_CH4		3.0 // Ohm

struct pac1934_data {
	struct device *i2c;
	u16_t vbus_ch1;
	u16_t vbus_ch2;
	u16_t vbus_ch3;
	u16_t vbus_ch4;
	u16_t vsense_ch1;
	u16_t vsense_ch2;
	u16_t vsense_ch3;
	u16_t vsense_ch4;
};

/* Registers */
#define PAC1934_REG_CMD_REFRESH		0x00
#define PAC1934_REG_CTRL			0x01

#define PAC1934_REG_VBUS_CH1		0x07
#define PAC1934_REG_VBUS_CH2		0x08
#define PAC1934_REG_VBUS_CH3		0x09
#define PAC1934_REG_VBUS_CH4		0x0A

#define PAC1934_REG_VSENSE_CH1		0x0B
#define PAC1934_REG_VSENSE_CH2		0x0C
#define PAC1934_REG_VSENSE_CH3		0x0D
#define PAC1934_REG_VSENSE_CH4		0x0E

#define PAC1934_REG_CHANNEL_DIS		0x1C
#define PAC1934_REG_NEG_PWR			0x1D

#define PAC1934_REG_CTRL_ACT		0x21

#define PAC1934_REG_PRODUCT_ID		0xFD
#define PAC1934_REG_MANUF_ID		0xFE
#define PAC1934_REG_REVISION_ID		0xFF

/* Control register bits (0x01) */
#define PAC1934_CFG_OVF_ALERT				(1 << 1)
#define PAC1934_CFG_END_OF_CONVERSION		(1 << 2)
#define PAC1934_CFG_ALERT_PIN				(1 << 3)
#define PAC1934_CFG_SINGLE_SHOT				(1 << 4)
#define PAC1934_CFG_SLEEP					(1 << 5)
#define PAC1934_CFG_SAMPLES_8				(3 << 6)
#define PAC1934_CFG_SAMPLES_64				(2 << 6)
#define PAC1934_CFG_SAMPLES_256				(1 << 6)
#define PAC1934_CFG_SAMPLES_1024			(0 << 6)

/* Negative power register (0x1D) */
#define PAC1934_BIDV_CH4				(1 << 0)
#define PAC1934_BIDV_CH3				(1 << 1)
#define PAC1934_BIDV_CH2				(1 << 2)
#define PAC1934_BIDV_CH1				(1 << 3)
#define PAC1934_BIDI_CH4				(1 << 4)
#define PAC1934_BIDI_CH3				(1 << 5)
#define PAC1934_BIDI_CH2				(1 << 6)
#define PAC1934_BIDI_CH1				(1 << 7)

/* Public functions */

void pac1934_read_register(u8_t reg, u8_t *buf, u8_t num_bytes);
void pac1934_write_register(u8_t reg, u8_t value);

#define SYS_LOG_DOMAIN "PAC1934"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* _SENSOR_PAC1934_ */
