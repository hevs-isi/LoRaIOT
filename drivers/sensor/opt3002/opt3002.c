/*
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "opt3002.h"


#include <device.h>
#include <i2c.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/__assert.h>
#include <math.h>

/*
 * Optical power range: 1.2nW/cm2 to 10mW/cm2
 * In lux: 0.008196 to 68300 Lux
 */

int opt3002_write_command(struct opt3002_data *drv_data, u16_t cmd)
{
	u8_t tx_buf[2] = {cmd >> 8, cmd & 0xFF};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 OPT3002_I2C_ADDRESS);
}

int opt3002_write_reg(struct opt3002_data *drv_data, u8_t reg, u16_t val)
{
	u8_t tx_buf[3];

	tx_buf[0] = reg;
	tx_buf[1] = val >> 8;
	tx_buf[2] = val & 0xFF;

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 OPT3002_I2C_ADDRESS);
}

int opt3002_read_reg(struct opt3002_data *drv_data, u8_t reg, u8_t *val)
{
	u8_t reg_addr[1];
	reg_addr[0] = reg;
	return i2c_burst_read_addr(drv_data->i2c, OPT3002_I2C_ADDRESS, &reg, 1, val, 2);
}

static int opt3002_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct opt3002_data *drv_data = dev->driver_data;
	u8_t rx_buf[2];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT);

	opt3002_write_reg(drv_data, OPT3002_REG_CONFIG, OPT3002_CONFIG_RESET | OPT3002_CONF_MODE);
	// Set by default to maximum conversion time + security
	k_sleep(2000);
	if(opt3002_read_reg(drv_data, OPT3002_REG_RESULT, rx_buf) > 0){
		SYS_LOG_DBG("Failed to read result register!");
	}

	drv_data->sample = (rx_buf[0] << 8) | rx_buf[1];
	return 0;
}

static int opt3002_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct opt3002_data *drv_data = dev->driver_data;
	double opt_power;
	u8_t exp;
	u16_t mantissa;

	if (chan == SENSOR_CHAN_LIGHT) {
		/* Optical_Power = (2^E[3:0]) × R[11:0] × 1.2 [nW/cm2] */

		exp = drv_data->sample >> 12;
		mantissa = drv_data->sample & 0x0FFF;

		opt_power = pow(2, exp) * mantissa * 1.2;
		val->val1 = (s32_t)opt_power;
		val->val2 = (opt_power-val->val1) * 1000000;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api opt3002_driver_api = {
#ifdef CONFIG_OPT3002_TRIGGER
	.attr_set = opt3002_attr_set,
	.trigger_set = opt3002_trigger_set,
#endif
	.sample_fetch = opt3002_sample_fetch,
	.channel_get = opt3002_channel_get,
};

static int opt3002_init(struct device *dev)
{
	struct opt3002_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_OPT3002_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    CONFIG_OPT3002_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	// gives sensor time to start
	k_sleep(10);

#ifdef CONFIG_OPT3002_TRIGGER
	if (opt3002_init_interrupt(dev) < 0) {
		SYS_LOG_DBG("Failed to initialize interrupt");
		return -EIO;
	}
#endif


	return 0;
}

struct opt3002_data opt3002_driver;

DEVICE_AND_API_INIT(opt3002, CONFIG_OPT3002_NAME, opt3002_init, &opt3002_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &opt3002_driver_api);
