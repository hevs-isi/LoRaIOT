/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SENSOR_SHTC1_
#define _SENSOR_SHTC1_

#include <device.h>
#include <kernel.h>

#define SHTC1_I2C_ADDRESS		CONFIG_SHTC1_I2C_ADDR

#define SHTC1_CMD_SOFT_RESET	0x805D
#define SHTC1_CMD_READ_ID		0xEFC8

#if CONFIG_SHTC1_CLOCK_STRETCHING_EN
	#define SHTC1_CLOCK_STRETCHING_IDX	0
#elif CONFIG_SHTC1_CLOCK_STRETCHING_DIS
	#define SHTC1_CLOCK_STRETCHING_IDX	1
#endif

#if CONFIG_SHTC1_FIRST_MEASUREMENT_TEMP
	#define SHTC1_FIRST_MEASUREMENT_IDX	0
#elif CONFIG_SHTC1_FIRST_MEASUREMENT_HUM
	#define SHTC1_FIRST_MEASUREMENT_IDX	1
#endif

static const u16_t shtc1_measure_cmd[2][2] = {
	{0x7CA2, 0x5C24},
	{0x7866, 0x58E0}
};

struct shtc1_data {
	struct device *i2c;
	u16_t t_sample;
	u16_t rh_sample;
};

void shtc1_read_id_register(struct device *dev, u8_t *buf, u8_t num_bytes);

#define SYS_LOG_DOMAIN "SHTC1"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* _SENSOR_SHTC1_ */
