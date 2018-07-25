#include <device.h>
#include <kernel.h>
#include <sensor.h>

#include "analogmic.h"

struct analogmic_data analogmic_driver;


/* will be used to sample the microphone output with the internal adc */
static int analogmic_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	//struct analogmic_data *drv_data = dev->driver_data;

	return 0;
}

/* will be used to retrieve the sampled data */
static int analogmic_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	return 0;
}

static const struct sensor_driver_api analogmic_driver_api = {
	.attr_set = analogmic_attr_set,
	.trigger_set = analogmic_trigger_set,
	.sample_fetch = analogmic_sample_fetch,
	.channel_get = analogmic_channel_get,
};


static int analogmic_init(struct device *dev)
{
	if (analogmic_init_interrupt(dev) < 0) {
		SYS_LOG_DBG("Failed to initialize interrupt");
		return -EIO;
	}

	return 0;
}

DEVICE_AND_API_INIT(analogmic, CONFIG_ANALOGMIC_NAME, analogmic_init, &analogmic_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &analogmic_driver_api);
