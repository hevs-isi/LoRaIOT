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

static void led0_set(u32_t value)
{
	struct device *led0_dev = device_get_binding(LED0_GPIO_CONTROLLER);
	gpio_pin_write(led0_dev, LED0_GPIO_PIN, value);
}

static void led1_set(u32_t value)
{
	struct device *led1_dev = device_get_binding(LED1_GPIO_CONTROLLER);
	gpio_pin_write(led1_dev, LED1_GPIO_PIN, value);
}

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <soc.h>
#include <init.h>
#include <uart.h>
#include <clock_control.h>

#include <linker/sections.h>
#include <clock_control/stm32_clock_control.h>
#include "../drivers/serial/uart_stm32.h"
#define DEV_CFG(dev)							\
	((const struct uart_stm32_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct uart_stm32_data * const)(dev)->driver_data)
#define UART_STRUCT(dev)					\
	((USART_TypeDef *)(DEV_CFG(dev))->uconf.base)

void main(void)
{
	int ret;

	LOG_DBG("main");

	struct device *dev;

	/* Disable vbat measure */
	dev = device_get_binding(DT_GPIO_STM32_GPIOC_LABEL);
	ret = gpio_pin_configure(dev, 0, (GPIO_DIR_IN));
	dev = device_get_binding(DT_GPIO_STM32_GPIOA_LABEL);
	ret = gpio_pin_configure(dev, 8, (GPIO_DIR_IN));

	/* disable rx/tx radio for firmware update */
	#if 0
	{
		dev = device_get_binding(DT_GPIO_STM32_GPIOA_LABEL);
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

	/*
	USART_TypeDef *UartInstance = UART_STRUCT(device_get_binding("UART_2"));

	printk("UART_2->BRR:%"PRIu32"\n", UartInstance->BRR);
	*/

	lp_init();
	leds_init();
	saved_config_init();
	gps_off();

	lora_off();

	led0_set(0);
	led1_set(0);

	//stm32_sleep(10000);
	//k_sleep(3000);
	//stm32_sleep(0);

	//gps_init();

	//
	//lora_init();
	//lora_shell_pm();
}
