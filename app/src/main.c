/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/sys_log.h>
#include <board_utils.h>
#include "shell_lora.h"
#include <kernel.h>
#include <device.h>
#include "stm32_lp.h"

#include <string.h>

#include "lora.h"
#include "gps.h"
#include "saved_config.h"
#include "global.h"
#include <sensor.h>

struct global_t global;

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#if CONFIG_LORA_IM881A
#include <im881a.h>
#endif

#include <device.h>
#include <gpio.h>
#include <crc.h>

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

struct periodic_timer_t
{
	u32_t next;
	u32_t period;
	u32_t enable;
};

static u32_t expiry(const struct periodic_timer_t *t, u32_t now)
{
	u32_t x = now - t->next;
	if (x > INT32_MAX)
	{
		return 0;
	}

	return x;
}

static u32_t expired(const struct periodic_timer_t *t, u32_t now)
{
	return (now - t->next) > 0 && t->enable;
}

static u32_t expired_restart(struct periodic_timer_t *t, u32_t now)
{
	u32_t ex = 0;

	if (expired(t, now))
	{
		ex = 1;

		t-> next = ((now / t->period) + 1) * t->period;
	}

	return ex;
}

static void restart_psnr(struct periodic_timer_t *t, u32_t now, u32_t pr)
{
	t->next = (((now / t->period) + 1) * t->period) + (pr % t->period);
}

static struct periodic_timer_t measure_timer =
{
	.next = 0,
	.period = 5*60,
	.enable = 1,
};

static struct periodic_timer_t tx_timer =
{
	.next = 0,
	.period = 15*60,
	.enable = 1,
};

static struct periodic_timer_t sync_timer =
{
	.next = 0,
	.period = 12*60*60,
	.enable = 1,
};

static const struct periodic_timer_t *timers[] =
{
	&measure_timer,
	&tx_timer,
	&sync_timer,
};

static u32_t expiry_min(const struct periodic_timer_t **ts, size_t nr, u32_t now)
{
	u32_t min = INT32_MAX;

	for (int i = 0 ; i < nr ; i++)
	{
		u32_t ex = expiry(ts[i], now);

		if (ex < min)
		{
			min = ex;
		}
	}

	return min;
}

static void tick(u32_t now)
{
	u32_t last_measure_ts = 0;

	if (expired_restart(&measure_timer, now))
	{
		// FIXME do the measure
		last_measure_ts = now;
	}

	if (expired(&tx_timer, now))
	{
		// FIXME do the TX
		u8_t devaddr[6] = {1,2,3,4,5,6}; // FIMXE
		u8_t data[10];
		memcpy(&data[0], devaddr, sizeof(devaddr));
		memcpy(&data[6], &last_measure_ts, sizeof(last_measure_ts));
		u32_t pr = crc32_ieee(data, sizeof(data));
		restart_psnr(&tx_timer, now, pr);
	}

	if (expired_restart(&sync_timer, now))
	{
		// FIXME request time or do gps sync
	}

	u32_t sleep = expiry_min(timers, ARRAY_SIZE(timers), now);
}


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
	psu_5v(0);
	psu_ind(0);
	psu_charge(1);
	psu_cpu_hp(1);
	saved_config_init();
	gps_off();

	lora_off();

	led0_set(0);
	led1_set(0);

	//stm32_swd_off();
	//stm32_sleep(10000);
	stm32_swd_on();
	//k_sleep(3000);
	//stm32_sleep(0);

	//gps_init();

	//
	lora_on();
	lora_init();
	//lora_shell_pm();
}
