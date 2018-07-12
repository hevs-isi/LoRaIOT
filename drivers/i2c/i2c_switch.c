
#include <i2c_switch.h>


int i2c_switch_enable_channel(u8_t channel){

	struct device *dev;
	u8_t tx_buf[1] = {1 << channel};

	dev = device_get_binding(CONFIG_I2C_SWITCH_I2C_MASTER_DEV_NAME);

	return i2c_write(dev, tx_buf, 1, CONFIG_I2C_SWITCH_ADDR);


}
