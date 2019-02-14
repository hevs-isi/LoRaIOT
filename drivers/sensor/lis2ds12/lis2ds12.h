/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_

#include <zephyr/types.h>
#include <sensor.h>
#include <gpio.h>

#define LIS2DS12_REG_WHO_AM_I			0x0F
#define LIS2DS12_VAL_WHO_AM_I			0x43

#define LIS2DS12_REG_CTRL1			0x20
#define LIS2DS12_MASK_CTRL1_ODR			(BIT(7) | BIT(6) | \
						 BIT(5) | BIT(4))
#define LIS2DS12_SHIFT_CTRL1_ODR		4
#define LIS2DS12_MASK_CTRL1_FS			(BIT(3) | BIT(2))
#define LIS2DS12_SHIFT_CTRL1_FS			2
#define LIS2DS12_MASK_CTRL1_HF_ODR		BIT(1)
#define LIS2DS12_MASK_CTRL1_HF_BDU		BIT(0)

#define LIS2DS12_REG_CTRL2			0x21
#define LIS2DS12_SOFT_RESET			0x40

#define LIS2DS12_REG_CTRL3			0x22
#define LIS2DS12_SHIFT_LIR			0x02
#define LIS2DS12_MASK_LIR			0x04

#define LIS2DS12_REG_CTRL4			0x23
#define LIS2DS12_SHIFT_INT1_DRDY		0x00
#define LIS2DS12_MASK_INT1_DRDY			0x01

#define LIS2DS12_REG_OUT_T			0x26

#define LIS2DS12_REG_STATUS			0x27
#define LIS2DS12_INT_DRDY			0x01

#define LIS2DS12_REG_OUTX_L			0x28
#define LIS2DS12_REG_OUTX_H			0x29
#define LIS2DS12_REG_OUTY_L			0x2A
#define LIS2DS12_REG_OUTY_H			0x2B
#define LIS2DS12_REG_OUTZ_L			0x2C
#define LIS2DS12_REG_OUTZ_H			0x2D


#if (CONFIG_LIS2DS12_ODR == 0)
#define LIS2DS12_ODR_RUNTIME			1
#define LIS2DS12_DEFAULT_ODR			0
#else
#define LIS2DS12_DEFAULT_ODR			(CONFIG_LIS2DS12_ODR << 4)
#endif

/* Accel sensor sensitivity unit is 0.061 mg/LSB */
#define GAIN_XL				(61LL / 1000.0)

#if CONFIG_LIS2DS12_FS == 0
#define LIS2DS12_FS_RUNTIME 1
#define LIS2DS12_DEFAULT_FS			(0 << 2)
#define LIS2DS12_DEFAULT_GAIN			GAIN_XL
#elif CONFIG_LIS2DS12_FS == 2
#define LIS2DS12_DEFAULT_FS			(0 << 2)
#define LIS2DS12_DEFAULT_GAIN			GAIN_XL
#elif CONFIG_LIS2DS12_FS == 4
#define LIS2DS12_DEFAULT_FS			(2 << 2)
#define LIS2DS12_DEFAULT_GAIN			(2.0 * GAIN_XL)
#elif CONFIG_LIS2DS12_FS == 8
#define LIS2DS12_DEFAULT_FS			(3 << 2)
#define LIS2DS12_DEFAULT_GAIN			(4.0 * GAIN_XL)
#elif CONFIG_LIS2DS12_FS == 16
#define LIS2DS12_DEFAULT_FS			(1 << 2)
#define LIS2DS12_DEFAULT_GAIN			(8.0 * GAIN_XL)
#else
#error "Bad LIS2DS12 FS value (should be 0, 2, 4, 8, 16)"
#endif

struct lis2ds12_config {
	char *comm_master_dev_name;
	int (*bus_init)(struct device *dev);
};

struct lis2ds12_data;

struct lis2ds12_transfer_function {
	int (*read_data)(struct lis2ds12_data *data, u8_t reg_addr,
			 u8_t *value, u8_t len);
	int (*write_data)(struct lis2ds12_data *data, u8_t reg_addr,
			  u8_t *value, u8_t len);
	int (*read_reg)(struct lis2ds12_data *data, u8_t reg_addr,
			u8_t *value);
	int (*write_reg)(struct lis2ds12_data *data, u8_t reg_addr,
			u8_t value);
	int (*update_reg)(struct lis2ds12_data *data, u8_t reg_addr,
			  u8_t mask, u8_t value);
};

struct lis2ds12_data {
	struct device *comm_master;
	int sample_x;
	int sample_y;
	int sample_z;
	float gain;
	const struct lis2ds12_transfer_function *hw_tf;

#ifdef CONFIG_LIS2DS12_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LIS2DS12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_LIS2DS12_TRIGGER */
};

int lis2ds12_spi_init(struct device *dev);
int lis2ds12_i2c_init(struct device *dev);

#ifdef CONFIG_LIS2DS12_TRIGGER
int lis2ds12_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int lis2ds12_trigger_init(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_ */
