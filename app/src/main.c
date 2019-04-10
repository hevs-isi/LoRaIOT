/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/sys_log.h>
#include <board_utils.h>
#include "lora_shell.h"
#include <kernel.h>
#include <device.h>
#include "stm32_lp.h"

#include <string.h>

#include "lora.h"
#include "gps.h"
#include "saved_config.h"
#include <sensor.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#if CONFIG_LORA_IM881A
#include <im881a.h>
#endif

#include <device.h>
#include <gpio.h>

static void leds_init(void)
{
	struct device *led0_dev = device_get_binding(LED0_GPIO_CONTROLLER);
	struct device *led1_dev = device_get_binding(LED1_GPIO_CONTROLLER);
	gpio_pin_configure(led0_dev, LED0_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(led1_dev, LED1_GPIO_PIN, GPIO_DIR_OUT);

	gpio_pin_write(led0_dev, LED0_GPIO_PIN, 1);
	gpio_pin_write(led1_dev, LED1_GPIO_PIN, 1);
}

void main(void)
{
	int ret;

	LOG_DBG("main");

	struct device *dev;

	/* Power on radio module */
	dev = device_get_binding(DT_GPIO_STM32_GPIOD_LABEL);
	ret = gpio_pin_configure(dev, 2, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring pin PD%d!\n", 2);
	}

	ret = gpio_pin_write(dev, 2, 1);
	if (ret) {
		printk("Error set pin PD%d!\n", 2);
	}

	/* power down gps */
	dev = device_get_binding(DT_GPIO_STM32_GPIOA_LABEL);
	ret = gpio_pin_configure(dev, 12, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring pin PA%d!\n", 12);
	}

	ret = gpio_pin_write(dev, 12, 0);

	/* disable rx/tx radio for firmware update */
	#if 0
	{
		device_get_binding(DT_GPIO_STM32_GPIOA_LABEL);
		ret = gpio_pin_configure(dev, 9, (GPIO_DIR_IN));
		if (ret) {
			printk("Error configuring pin PA%d!\n", 9);
		}
		ret = gpio_pin_configure(dev, 10, (GPIO_DIR_IN));
		if (ret) {
			printk("Error configuring pin PA%d!\n", 10);
		}
	}
	#endif

	//gps_init();
	//leds_init();
	saved_config_init();
	lora_init();
	//lora_shell_pm();
}
