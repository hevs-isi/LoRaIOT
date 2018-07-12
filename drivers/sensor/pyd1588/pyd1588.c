#include "pyd1588.h"

#include <device.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/__assert.h>

struct pyd1588_data pyd1588_driver;

static void configure_pir_sensor(struct k_timer *timer_id)
{
	pyd1588_write_config(&pyd1588_driver, PYD1588_CONF_THRESHOLD_128 |
			PYD1588_CONF_MODE_WAKEUP |
			PYD1588_CONF_WINDOW_8S |
			PYD1588_CONF_BLIND_TIME_8S);
	//pyd1588_write_config(drv_data, 0x00304D10);
	SYS_LOG_INF("Write config");
}

K_TIMER_DEFINE(configure_delay, configure_pir_sensor, NULL);

int pyd1588_write_config(struct pyd1588_data *drv_data, u32_t val)
{
	u8_t i;
	u8_t nextbit;
	u32_t regmask = 0x1000000;
	u32_t conf = PYD1588_CONF_DEFAULT;

	conf |= val;

	for(i=0;i < 25;i++){
		nextbit = (conf&regmask)!=0;
		regmask >>= 1;
		gpio_pin_write(drv_data->serin_gpio, CONFIG_PYD1588_SERIN_GPIO_PIN_NUM, 0);
		gpio_pin_write(drv_data->serin_gpio, CONFIG_PYD1588_SERIN_GPIO_PIN_NUM, 1);
		k_busy_wait(40);
		gpio_pin_write(drv_data->serin_gpio, CONFIG_PYD1588_SERIN_GPIO_PIN_NUM, nextbit);
		k_busy_wait(100);
	}

	gpio_pin_write(drv_data->serin_gpio, CONFIG_PYD1588_SERIN_GPIO_PIN_NUM, 0);
	k_busy_wait(600);

	return 0;

}

void pyd1588_read(struct pyd1588_data *drv_data)
{
	u8_t i;
	u16_t uibitmask;
	u32_t ulbitmask;
	u32_t dl_val;

	SYS_LOG_DBG("Read data");

	gpio_pin_read(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, &dl_val);

	// If DL is already in a high state and it is not required to generate the first pulse
	if(!dl_val){
		// Set DL = High, to force fast uC controlled DL read out
		gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_OUT);
		gpio_pin_write(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, 1);
		k_busy_wait(200);
	} else {
		//gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_OUT);
	}
	//gpio_pin_write(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, 0);

	 // get first 15bit out-off-range and ADC value
	uibitmask = 0x4000;              // Set BitPos
	drv_data->pir_val = 0;
	for (i=0; i < 15; i++)
	{
		//  create low to high transition
		// Set DL = Low, duration must be > 200 ns (tL)
		gpio_pin_write(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, 0);
		// Configure DL as Output
		gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_OUT);
		// number of nop dependant processor speed (200ns min.)
		//__asm("nop");
		k_busy_wait(1);
		// Set DL = High, duration must be > 200 ns (tH)
		gpio_pin_write(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, 1);
		// Configure DL as Input
		gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_IN);

		k_busy_wait(1);                 // Wait for stable low signal

		gpio_pin_read(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, &dl_val);
		// If DL High set masked bit in PIRVal
		if (dl_val) drv_data->pir_val |= uibitmask;
		uibitmask >>= 1;
	}

    // get 25bit status and config
    ulbitmask   = 0x1000000;                // Set BitPos
    drv_data->statcfg = 0;
    for (i=0; i < 25; i++)
    {
        //  create low to high transition
        // Set DL = Low, duration must be > 200 ns (tL)
    	gpio_pin_write(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, 0);
        // Configure DL as Output
    	gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_OUT);
    	// number of nop dependant processor speed (200ns min.)
    	//__asm("nop");
    	k_busy_wait(1);
        // Set DL = High, duration must be > 200 ns (tH)
    	gpio_pin_write(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, 1);
        // Configure DL as Input
    	gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_IN);

    	// Wait for stable low signal, tbd empirically using scope
    	k_busy_wait(1);

    	gpio_pin_read(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, &dl_val);
        // If DL High set masked bit
        if (dl_val) drv_data->statcfg |= ulbitmask;
        ulbitmask>>=1;
    }

    // Set DL = Low
	gpio_pin_write(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, 0);
	// Configure DL as Output
	gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_OUT);
	__asm("nop");
	gpio_pin_configure(drv_data->dl_gpio, CONFIG_PYD1588_DL_GPIO_PIN_NUM, GPIO_DIR_IN);

	// clear unused bit
	drv_data->pir_val &= 0x3FFF;

	if (!(drv_data->statcfg & 0x60)){
		// ADC source to PIR band pass
		// number in 14bit two's complement
		if(drv_data->pir_val & 0x2000) drv_data->pir_val -= 0x4000;
	}

}


static int pyd1588_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct pyd1588_data *drv_data = dev->driver_data;

	//pyd1588_write_config(drv_data, 0x00304D10);
	pyd1588_read(drv_data);

	SYS_LOG_DBG("PIR value 0x%x", drv_data->pir_val);
	SYS_LOG_DBG("Config value 0x%x", drv_data->statcfg);
	return 0;
}

static int pyd1588_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	return 0;
}

static const struct sensor_driver_api pyd1588_driver_api = {
	.attr_set = pyd1588_attr_set,
	.trigger_set = pyd1588_trigger_set,
	.sample_fetch = pyd1588_sample_fetch,
	.channel_get = pyd1588_channel_get,
};


static int pyd1588_init(struct device *dev)
{
	struct pyd1588_data *drv_data = dev->driver_data;

	/* setup serial in gpio */
	drv_data->serin_gpio = device_get_binding(CONFIG_PYD1588_SERIN_GPIO_DEV_NAME);
	if (drv_data->serin_gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
			CONFIG_PYD1588_SERIN_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->serin_gpio, CONFIG_PYD1588_SERIN_GPIO_PIN_NUM, GPIO_DIR_OUT);
	gpio_pin_write(drv_data->serin_gpio, CONFIG_PYD1588_SERIN_GPIO_PIN_NUM, 0);


	if (pyd1588_init_interrupt(dev) < 0) {
		SYS_LOG_DBG("Failed to initialize interrupt");
		return -EIO;
	}

	// give some time to the sensor to settle before sending configuration
	k_timer_start(&configure_delay, K_SECONDS(5), 0);


	return 0;
}

DEVICE_AND_API_INIT(pyd1588, CONFIG_PYD1588_NAME, pyd1588_init, &pyd1588_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &pyd1588_driver_api);
