/*
 * Copyright (c) 2018 Darko Petrovic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <board.h>
#include <gpio.h>
#include <misc/printk.h>

#include <string.h>
#include <i2c_switch.h>
#include <logging/sys_log.h>

#include <board_utils.h>

#define NB_LEDS		3

struct sensor_pwr_ctrl_cfg {
	const char *name;
	const char *port;
	u32_t pin;
	u8_t polarity;
	s8_t i2c_channel;
};

struct led_blink_data {
	u8_t led;
	u32_t period;
	u32_t duration;
	struct k_timer timer;
	struct k_thread thread_data;
	k_thread_stack_t *thread_stack;
	u8_t active;
};

K_THREAD_STACK_DEFINE(led_blink_stack1, 256);
K_THREAD_STACK_DEFINE(led_blink_stack2, 256);
K_THREAD_STACK_DEFINE(led_blink_stack3, 256);

static u8_t vddh_active;
static struct device *led_dev;
static u32_t led_status;
static struct led_blink_data led_data[NB_LEDS];

static const struct sensor_pwr_ctrl_cfg pwr_ctrl_cfg[] = {
#if CONFIG_SHTC1
	{	.name = CONFIG_SHTC1_NAME,
		.port = SHT_POWER_PORT,
		.pin = SHT_POWER_PIN,
		.polarity = 1,
		.i2c_channel = SHT_I2C_CHANNEL
	},
#endif
#if CONFIG_OPT3002
	{
		.name = CONFIG_OPT3002_NAME,
		.port = OPT_POWER_PORT,
		.pin = OPT_POWER_PIN,
		.polarity = 1,
		.i2c_channel = OPT_I2C_CHANNEL
	},
#endif
#if CONFIG_CCS811
	{
		.name = CONFIG_CCS811_NAME,
		.port = CCS_POWER_PORT,
		.pin = CCS_POWER_PIN,
		.polarity = 0,
		.i2c_channel = CCS_I2C_CHANNEL
	},
#endif
#if CONFIG_PYD1588
	{
		.name = CONFIG_PYD1588_NAME,
		.port = PYD_POWER_PORT,
		.pin = PYD_POWER_PIN,
		.polarity = 1,
		.i2c_channel = -1
	},
#endif
#if CONFIG_ANALOGMIC
	{
		.name = CONFIG_ANALOGMIC_NAME,
		.port = MIC_POWER_PORT,
		.pin = MIC_POWER_PIN,
		.polarity = 1,
		.i2c_channel = -1
	},
#endif
	{
		.name = "PAC1934",
		.port = PAC1934_POWER_PORT,
		.pin = PAC1934_POWER_PIN,
		.polarity = 1,
		.i2c_channel = -1
	}
};

static int pwr_ctrl_init(struct device *dev)
{
	u8_t i;
	const struct sensor_pwr_ctrl_cfg *cfg = dev->config->config_info;
	struct device *gpio;

	// enable i2c switch by setting the n_reset pin
	gpio = device_get_binding(MUX_RESET_PORT);
	gpio_pin_configure(gpio, MUX_RESET_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio, MUX_RESET_PIN, 1);

	// turn off all sensors by default
	for(i=0;i<sizeof(pwr_ctrl_cfg)/sizeof(pwr_ctrl_cfg[0]);i++){
		gpio = device_get_binding(cfg[i].port);
		if (!gpio) {
			SYS_LOG_ERR("Could not bind device \"%s\"\n", cfg[i].port);
			return -ENODEV;
		}

		gpio_pin_configure(gpio, cfg[i].pin, GPIO_DIR_OUT);
		gpio_pin_write(gpio, cfg[i].pin, !cfg[i].polarity);
	}

	//gpio = device_get_binding(MIC_OUTPUT_PORT);
	//gpio_pin_configure(gpio, MIC_OUTPUT_PIN, GPIO_DIR_IN);

	// TODO: disable boost converter when not used
	gpio = device_get_binding(DISABLE_BOOST_PORT);
	gpio_pin_configure(gpio, DISABLE_BOOST_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio, DISABLE_BOOST_PIN, 1);

	gpio = device_get_binding(EN_VDDH_PORT);
	gpio_pin_configure(gpio, EN_VDDH_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio, EN_VDDH_PIN, 0);


	gpio = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);
	gpio_pin_configure(gpio, 11, GPIO_DIR_OUT);
	gpio_pin_write(gpio, 11, 0);


	gpio_pin_configure(gpio, 4, GPIO_DIR_OUT);
	gpio_pin_write(gpio, 4, 0);

	gpio_pin_configure(gpio, 5, GPIO_DIR_OUT);
	gpio_pin_write(gpio, 5, 0);

	gpio_pin_configure(gpio, CCS_POWER_PIN, GPIO_DIR_IN | GPIO_PUD_PULL_UP);
	//gpio_pin_write(gpio, CCS_POWER_PIN, 0);


	return 0;
}

void power_sensor(const char *name, u8_t power){
	struct device *gpio;
	struct device *dev;
	u8_t i, found=0;
	u8_t buf[1];

	// power on/off the sensor and de/activate the corresponding I2C channel
	for(i=0;i<sizeof(pwr_ctrl_cfg)/sizeof(pwr_ctrl_cfg[0]);i++){
		if(strcmp(name, pwr_ctrl_cfg[i].name) == 0){
			found = 1;
			// turn on device before I2C switch configuration
			// (bug when turned off before disabling I2C channel)
			if(power){
				gpio = device_get_binding(pwr_ctrl_cfg[i].port);
				gpio_pin_write(gpio, pwr_ctrl_cfg[i].pin, !(power ^ pwr_ctrl_cfg[i].polarity));
				// TODO_ need to be adjusted
				k_busy_wait(100);
			}

#if CONFIG_I2C_SWITCH
			// enable or disable the I2C switch channel
			if(pwr_ctrl_cfg[i].i2c_channel >= 0){
				SYS_LOG_DBG("Configure I2C channel -> device: %s, power: %d.", name, power);
				dev = device_get_binding(CONFIG_I2C_SWITCH_I2C_MASTER_DEV_NAME);
				// read i2c switch status
				if(i2c_read(dev, buf, 1, CONFIG_I2C_SWITCH_ADDR) > 0){
					SYS_LOG_ERR("Failed to read data sample!");
				}

				SYS_LOG_DBG("I2C Switch status: 0x%x", buf[0]);
				if(power){
					buf[0] |= (1 << pwr_ctrl_cfg[i].i2c_channel);
				} else {
					buf[0] &= ~(1 << pwr_ctrl_cfg[i].i2c_channel);
				}

				i2c_write(dev, buf, 1, CONFIG_I2C_SWITCH_ADDR);
				SYS_LOG_DBG("Writing I2C switch status 0x%x", buf[0]);
			}
#endif

			/* Execute init function of the device when powered on */
			if(power){
				dev = device_get_binding(pwr_ctrl_cfg[i].name);
				if(dev){
					SYS_LOG_DBG("Init device: %s", pwr_ctrl_cfg[i].name);
					dev->config->init(dev);
				}
			}

			// turn off device after I2C switch configuration
			else {
				gpio = device_get_binding(pwr_ctrl_cfg[i].port);
				gpio_pin_write(gpio, pwr_ctrl_cfg[i].pin, !(power ^ pwr_ctrl_cfg[i].polarity));
			}


			break;
		}
	}

	if(!found){
		SYS_LOG_ERR("Sensor %s not found!", name);
	}

}

void enable_high_voltage(u8_t status)
{
	struct device *gpio;
	gpio = device_get_binding(EN_VDDH_PORT);
	gpio_pin_configure(gpio, EN_VDDH_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio, EN_VDDH_PIN, !status);

	vddh_active = status;
}

u8_t get_vddh_status(void)
{
	return vddh_active;
}

static void led_blink(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct led_blink_data *one_led_data = arg1;

	SYS_LOG_DBG("Start thread for led blinking %d\n", one_led_data->led);

	while (one_led_data->active) {
		/*if(cnt % 2){
			VDDH_DEACTIVATE();
		} else {
			VDDH_ACTIVATE();
		}*/


		led_toggle(one_led_data->led);
		k_sleep(one_led_data->period);
	}
	led_off(one_led_data->led);
}


/*
K_THREAD_DEFINE(led_blinking_thread_id, 1024, led_blink, NULL, NULL, NULL,
		K_PRIO_PREEMPT(0), 0, K_NO_WAIT);*/


void led_blink_timeout(struct k_timer *timer_id)
{
	u8_t i;

	for(i=0;i<NB_LEDS;i++){
		if(timer_id == &led_data[i].timer){
			led_data[i].active = 0;
			break;
		}
	}

}

void led_on(u8_t led)
{
	gpio_pin_write(led_dev, led, 0);
	led_status |= 1 << led;
}

void led_off(u8_t led)
{
	gpio_pin_write(led_dev, led, 1);
	led_status &= ~(1 << led);
}

void led_toggle(u8_t led)
{
	if(led_status & (1 << led)){
		led_off(led);
	} else {
		led_on(led);
	}
}

void blink_led(u8_t led, u32_t period_ms, u32_t duration_ms)
{
	u8_t i;

	for(i=0;i<NB_LEDS;i++){
		if(led == led_data[i].led){

			if(led_data[i].active){
				break;
			}

			led_data[i].period = period_ms;
			led_data[i].duration = duration_ms;
			led_data[i].active = 1;

			k_thread_create(&led_data[i].thread_data, led_data[i].thread_stack,
					K_THREAD_STACK_SIZEOF(led_data[i].thread_stack),
					(k_thread_entry_t)led_blink,
					&led_data[i], NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);

			k_timer_start(&led_data[i].timer, duration_ms, 0);
			SYS_LOG_DBG("Blink led %d\n", led_data[i].led);
			break;
		}

	}

}

static int board_init(struct device *dev)
{
	pwr_ctrl_init(dev);

	led_dev = device_get_binding(LEDS_PORT);

	/* Set LED pins as output */
	gpio_pin_configure(led_dev, LED_GREEN, GPIO_DIR_OUT);
	gpio_pin_configure(led_dev, LED_BLUE, GPIO_DIR_OUT);
	gpio_pin_configure(led_dev, LED_RED, GPIO_DIR_OUT);

	// turn off
	gpio_pin_write(led_dev, LED_GREEN, 1);
	gpio_pin_write(led_dev, LED_BLUE, 1);
	gpio_pin_write(led_dev, LED_RED, 1);

	led_data[0].led = LED_GREEN;
	led_data[1].led = LED_BLUE;
	led_data[2].led = LED_RED;

	led_data[0].thread_stack = led_blink_stack1;
	led_data[1].thread_stack = led_blink_stack2;
	led_data[2].thread_stack = led_blink_stack3;

	k_timer_init(&led_data[0].timer, led_blink_timeout, NULL);
	k_timer_init(&led_data[1].timer, led_blink_timeout, NULL);
	k_timer_init(&led_data[2].timer, led_blink_timeout, NULL);

	return 0;
}


DEVICE_INIT(vdd_pwr_ctrl, "SENSOR_PWR_CTRL", board_init, NULL, pwr_ctrl_cfg,
	    POST_KERNEL, 5);

