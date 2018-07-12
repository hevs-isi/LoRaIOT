#include "shtc1.h"

#include <device.h>
#include <i2c.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/__assert.h>


/*
 * CRC algorithm parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
static u8_t shtc1_compute_crc(u16_t value)
{
	u8_t buf[2] = {value >> 8, value & 0xFF};
	u8_t crc = 0xFF;
	u8_t polynom = 0x31;
	int i, j;

	for (i = 0; i < 2; ++i) {
		crc  = crc ^ buf[i];
		for (j = 0; j < 8; ++j) {
			if (crc & 0x80) {
				crc = (crc << 1) ^ polynom;
			} else {
				crc = crc << 1;
			}
		}
	}

	return crc;
}

int shtc1_write_command(struct shtc1_data *drv_data, u16_t cmd)
{
	u8_t tx_buf[2] = {cmd >> 8, cmd & 0xFF};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 SHTC1_I2C_ADDRESS);
}

void shtc1_read_id_register(struct device *dev, u8_t *buf, u8_t num_bytes)
{

	i2c_burst_read16(dev, SHTC1_I2C_ADDRESS, SHTC1_CMD_READ_ID, buf, 2);
}

static int shtc1_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct shtc1_data *drv_data = dev->driver_data;
	u8_t rx_buf[6];
	u16_t t_sample, rh_sample;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	u8_t tx_buf[2] = {
			shtc1_measure_cmd[SHTC1_CLOCK_STRETCHING_IDX][SHTC1_FIRST_MEASUREMENT_IDX] >> 8,
			shtc1_measure_cmd[SHTC1_CLOCK_STRETCHING_IDX][SHTC1_FIRST_MEASUREMENT_IDX] & 0xFF
	};

	i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf), SHTC1_I2C_ADDRESS);
	k_sleep(20);
	if(i2c_read(drv_data->i2c, rx_buf, sizeof(rx_buf), SHTC1_I2C_ADDRESS) > 0){
		SYS_LOG_DBG("Failed to read ID!");
	}

	t_sample = (rx_buf[0+SHTC1_FIRST_MEASUREMENT_IDX*3] << 8) |
			rx_buf[1+SHTC1_FIRST_MEASUREMENT_IDX*3];
	if (shtc1_compute_crc(t_sample) != rx_buf[2+SHTC1_FIRST_MEASUREMENT_IDX*3]) {
		SYS_LOG_DBG("Received invalid temperature CRC!");
		return -EIO;
	}

	rh_sample = (rx_buf[3-SHTC1_FIRST_MEASUREMENT_IDX*3] << 8) |
			rx_buf[4-SHTC1_FIRST_MEASUREMENT_IDX*3];
	if (shtc1_compute_crc(rh_sample) != rx_buf[5-SHTC1_FIRST_MEASUREMENT_IDX*3]) {
		SYS_LOG_DBG("Received invalid relative humidity CRC!");
		return -EIO;
	}

	drv_data->t_sample = t_sample;
	drv_data->rh_sample = rh_sample;

	return 0;
}

static int shtc1_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct shtc1_data *drv_data = dev->driver_data;
	u64_t tmp;

	/*
	 * See datasheet "Conversion of Signal Output" section
	 * for more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* val = -45 + 175 * sample / (2^16 -1) */
		tmp = 175 * (u64_t)drv_data->t_sample;
		val->val1 = (s32_t)(tmp / 0xFFFF) - 45;
		val->val2 = (1000000 * (tmp % 0xFFFF)) / 0xFFFF;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		/* val = 100 * sample / (2^16 -1) */
		u32_t tmp2 = 100 * (u32_t)drv_data->rh_sample;
		val->val1 = tmp2 / 0xFFFF;
		/* x * 100000 / 65536 == x * 15625 / 1024 */
		val->val2 = (tmp2 % 0xFFFF) * 15625 / 1024;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api shtc1_driver_api = {
	.sample_fetch = shtc1_sample_fetch,
	.channel_get = shtc1_channel_get,
};

static int shtc1_init(struct device *dev)
{
	struct shtc1_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_SHTC1_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    CONFIG_SHTC1_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	return 0;
}

struct shtc1_data shtc1_driver;

DEVICE_AND_API_INIT(shtc1, CONFIG_SHTC1_NAME, shtc1_init, &shtc1_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &shtc1_driver_api);
