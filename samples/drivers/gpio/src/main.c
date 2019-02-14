/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver sample
 *
 * This sample toggles LED1 and wait interrupt on BUTTON1.
 * Note that an internet pull-up is set on BUTTON1 as the button
 * only drives low when pressed.
 */
#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <gpio.h>
#include <misc/util.h>

#if defined(SW0_GPIO_CONTROLLER) && defined(LED0_GPIO_CONTROLLER)
#define GPIO_OUT_DRV_NAME LED0_GPIO_CONTROLLER
#define GPIO_OUT_PIN  LED0_GPIO_PIN
#define GPIO_IN_DRV_NAME SW0_GPIO_CONTROLLER
#define GPIO_INT_PIN  SW0_GPIO_PIN
#else
#error Change the pins based on your configuration. This sample \
	defaults to built-in buttons and LEDs
#endif

void gpio_callback(struct device *port,
		   struct gpio_callback *cb, u32_t pins)
{
	printk("Pin %d triggered\n", GPIO_INT_PIN);
}

static struct gpio_callback gpio_cb;

void main(void)
{
	struct device *gpio_out_dev, *gpio_in_dev;
	int ret;
	int toggle = 1;

	gpio_out_dev = device_get_binding(GPIO_OUT_DRV_NAME);
	if (!gpio_out_dev) {
		printk("Cannot find %s!\n", GPIO_OUT_DRV_NAME);
		return;
	}

	gpio_in_dev = device_get_binding(GPIO_IN_DRV_NAME);
	if (!gpio_in_dev) {
		printk("Cannot find %s!\n", GPIO_IN_DRV_NAME);
		return;
	}

	/* Setup GPIO output */
	ret = gpio_pin_configure(gpio_out_dev, GPIO_OUT_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring pin %d!\n", GPIO_OUT_PIN);
	}

	/* Setup GPIO input, and triggers on rising edge. */
#ifdef CONFIG_SOC_CC2650
	ret = gpio_pin_configure(gpio_in_dev, GPIO_INT_PIN,
				 (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH |
				  GPIO_INT_DEBOUNCE | GPIO_PUD_PULL_UP));
#else
	ret = gpio_pin_configure(gpio_in_dev, GPIO_INT_PIN,
				 (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH |
				  GPIO_INT_DEBOUNCE));
#endif
	if (ret) {
		printk("Error configuring pin %d!\n", GPIO_INT_PIN);
	}

	gpio_init_callback(&gpio_cb, gpio_callback, BIT(GPIO_INT_PIN));

	ret = gpio_add_callback(gpio_in_dev, &gpio_cb);
	if (ret) {
		printk("Cannot setup callback!\n");
	}

	ret = gpio_pin_enable_callback(gpio_in_dev, GPIO_INT_PIN);
	if (ret) {
		printk("Error enabling callback!\n");
	}

	while (1) {
		printk("Toggling pin %d\n", GPIO_OUT_PIN);

		ret = gpio_pin_write(gpio_out_dev, GPIO_OUT_PIN, toggle);
		if (ret) {
			printk("Error set pin %d!\n", GPIO_OUT_PIN);
		}

		if (toggle) {
			toggle = 0;
		} else {
			toggle = 1;
		}

		k_sleep(SECONDS(2));
	}
}
