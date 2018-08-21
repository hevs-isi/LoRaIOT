#include "pac1934.h"

#include <device.h>
#include <i2c.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/__assert.h>


#define PAC1934_FSC_CH1		(double)0.1/PAC1934_RSENSE_CH1
#define PAC1934_FSC_CH2		(double)0.1/PAC1934_RSENSE_CH2
#define PAC1934_FSC_CH3		(double)0.1/PAC1934_RSENSE_CH3
#define PAC1934_FSC_CH4		(double)0.1/PAC1934_RSENSE_CH4

struct pac1934_data pac1934_driver;

int pac1934_write_command(struct pac1934_data *drv_data, u16_t cmd)
{
	u8_t tx_buf[2] = {cmd >> 8, cmd & 0xFF};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 PAC1934_I2C_ADDRESS);
}

void pac1934_read_register(u8_t reg, u8_t *buf, u8_t num_bytes)
{
	struct device *i2c = pac1934_driver.i2c;

	if(i2c_burst_read(i2c, PAC1934_I2C_ADDRESS, reg, buf, num_bytes) > 0){
		SYS_LOG_DBG("Failed to read register!");
	}
}

void pac1934_write_register(u8_t reg, u8_t value)
{
	struct device *i2c = pac1934_driver.i2c;

	if(i2c_reg_write_byte(i2c, PAC1934_I2C_ADDRESS, reg, value) > 0){
		SYS_LOG_DBG("Failed to write register!");
	}
}

static int pac1934_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct pac1934_data *drv_data = dev->driver_data;
	u8_t rx_buf[8];
	u8_t tx_buf[1];

	tx_buf[0] = PAC1934_REG_CMD_REFRESH;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	// send REFRESH command
	i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf), PAC1934_I2C_ADDRESS);
	k_sleep(1);
	if(i2c_burst_read(drv_data->i2c, PAC1934_I2C_ADDRESS, PAC1934_REG_VBUS_CH1, rx_buf, 8) > 0){
		SYS_LOG_DBG("Failed to read VBUS registers !");
		return EIO;
	}

	drv_data->vbus_ch1 = rx_buf[1] + (rx_buf[0] << 8);
	drv_data->vbus_ch2 = rx_buf[3] + (rx_buf[2] << 8);
	drv_data->vbus_ch3 = rx_buf[5] + (rx_buf[4] << 8);
	drv_data->vbus_ch4 = rx_buf[7] + (rx_buf[6] << 8);

	if(i2c_burst_read(drv_data->i2c, PAC1934_I2C_ADDRESS, PAC1934_REG_VSENSE_CH1, rx_buf, 8) > 0){
		SYS_LOG_DBG("Failed to read VBUS registers !");
		return EIO;
	}

	drv_data->vsense_ch1 = rx_buf[1] + (rx_buf[0] << 8);
	drv_data->vsense_ch2 = rx_buf[3] + (rx_buf[2] << 8);
	drv_data->vsense_ch3 = rx_buf[5] + (rx_buf[4] << 8);
	drv_data->vsense_ch4 = rx_buf[7] + (rx_buf[6] << 8);


	return 0;
}

static int pac1934_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct pac1934_data *drv_data = dev->driver_data;
	u32_t integer;
	double value, fractional;

	switch(chan) {
		case SENSOR_CHAN_VOLTAGE_1:
			value = (double)(32*drv_data->vbus_ch1)/0xFFFF;
			break;
		case SENSOR_CHAN_VOLTAGE_2:
			value = (double)(32*drv_data->vbus_ch2)/0xFFFF;
			break;
		case SENSOR_CHAN_VOLTAGE_3:
			value = (double)(32*drv_data->vbus_ch3)/0xFFFF;
			break;
		case SENSOR_CHAN_VOLTAGE_4:
			value = (double)(32*drv_data->vbus_ch4)/0xFFFF;
			break;
		case SENSOR_CHAN_CURRENT_1:
			value = (double)(PAC1934_FSC_CH1*drv_data->vsense_ch1)/0xFFFF;
			break;
		case SENSOR_CHAN_CURRENT_2:
			// bidirectional current
			value = (double)(PAC1934_FSC_CH2*drv_data->vsense_ch2)/0x7FFF;
			break;
		case SENSOR_CHAN_CURRENT_3:
			value = (double)(PAC1934_FSC_CH3*drv_data->vsense_ch3)/0xFFFF;
			break;
		case SENSOR_CHAN_CURRENT_4:
			value = (double)(PAC1934_FSC_CH4*drv_data->vsense_ch4)/0xFFFF;
			break;
		default:
			value = 0;
	}

	integer = (u32_t)value;
	fractional = value-integer;
	val->val1 = integer;
	val->val2 = 1e6*fractional;

	return 0;
}

int pac1934_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{

	return 0;
}

static const struct sensor_driver_api pac1934_driver_api = {
	.attr_set = pac1934_attr_set,
	.sample_fetch = pac1934_sample_fetch,
	.channel_get = pac1934_channel_get,
};

static int pac1934_init(struct device *dev)
{
	struct pac1934_data *drv_data = dev->driver_data;

	k_sleep(15);

	drv_data->i2c = device_get_binding(CONFIG_PAC1934_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    CONFIG_PAC1934_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	// configure device
	pac1934_write_register(PAC1934_REG_CTRL,
			PAC1934_CFG_ALERT_PIN | PAC1934_CFG_OVF_ALERT |
			PAC1934_CFG_SINGLE_SHOT);

	pac1934_write_register(PAC1934_REG_NEG_PWR, PAC1934_BIDI_CH2);

	SYS_LOG_DBG("Sensor initialized.");

	return 0;
}

DEVICE_AND_API_INIT(pac1934, CONFIG_PAC1934_NAME, pac1934_init, &pac1934_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &pac1934_driver_api);
