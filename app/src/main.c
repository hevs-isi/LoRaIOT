/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/sys_log.h>
#include <board_utils.h>

#include <kernel.h>
#include <device.h>
#include <gpio.h>

#include <string.h>

#if CONFIG_SEGGER_SYSTEMVIEW
#include <systemview/SEGGER_SYSVIEW.h>
#include "sysview.h"
#endif

#if CONFIG_RTT_CONSOLE
#include <rtt/SEGGER_RTT.h>
#endif

#include "sensors.h"
#include "lora.h"
#include <sensor.h>

#include <i2c.h>
#include <kernel.h>

#if CONFIG_LORA_IM881A
#include <im881a.h>
#endif

#include <power.h>
#include <soc_power.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME 	1000

#define EDGE    (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)

K_THREAD_STACK_DEFINE(printer_stack, 1024);
K_THREAD_STACK_DEFINE(printer2_stack, 3000);
//K_THREAD_STACK_DEFINE(rttterm_stack, 2048);

static struct k_thread printer_thread_data;
static struct k_thread printer2_thread_data;
//static struct k_thread rttterm_thread_data;

extern void power_sensor(const char *name, u8_t power);
extern void enable_high_voltage(u8_t status);

#define SECONDS_TO_SLEEP	60

static void printer_thread()
{
	struct device *dev;
	struct sensor_value temp_value;

	//dev = device_get_binding(CONFIG_PYD1588_NAME);
	for (;;) {
		//SENSOR_ACTIVATE(CONFIG_SHTC1_NAME);
		//ret = sensor_sample_fetch(dev);
		//ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_value);
		//SENSOR_DEACTIVATE(CONFIG_SHTC1_NAME);

		//i2c_burst_read_addr(dev, 0x5a, tx_buf, 1, rx_buf, 2);

#if CONFIG_SEGGER_SYSTEMVIEW
		SEGGER_SYSVIEW_PrintfHost("Hello world %d", inc++);
#endif
		//printk("Printer thread says hello\n");
		//SYS_LOG_ERR("This is an error!");
		//SYS_LOG_WRN("This is a warning!");
		//SYS_LOG_INF("This is an info!");
		//SYS_LOG_DBG("Sensor value %d.%d", temp_value.val1, temp_value.val2);
		k_sleep(MSEC_PER_SEC*4);

	}
}

static void led_blink2()
{

	int cnt = 0;
	struct device *dev;

#if 0
	dev = device_get_binding(LEDS_PORT);
	/* Set LED pin as output */
	gpio_pin_configure(dev, LED_RED, GPIO_DIR_OUT);

	//blink_led(LED_GREEN, MSEC_PER_SEC/4, K_SECONDS(5));


	while (1) {

		/*if(cnt % 2){
			VDDH_DEACTIVATE();
		} else {
			VDDH_ACTIVATE();
		}*/

		//led_toggle(LED_RED);

		/* Set pin to HIGH/LOW every 1 second */
		gpio_pin_write(dev, LED_RED, cnt % 2);
		cnt++;
		//SEGGER_SYSVIEW_RecordString(32+5, "ABBA");
		//SYS_LOG_ERR("This is an error!");
		k_sleep(MSEC_PER_SEC*2);

	}
#endif

}

#if 0
static void test_gpios()
{

	int cnt = 0;
	struct device *dev0;
	struct device *dev1;
	int i;

	dev0 = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	/* Set all pins as output */

	for(i=0;i<32;i++){
		if(i==0 || i==1 || i==9 || i==10 || i==12 || i==18 || i==25){
			continue;
		}
		gpio_pin_configure(dev0, i, GPIO_DIR_OUT);
	}

	dev1 = device_get_binding(CONFIG_GPIO_NRF5_P1_DEV_NAME);
	for(i=0;i<16;i++){
		if(i==3 || i==5 || i==7 || i==9){
			continue;
		}
		gpio_pin_configure(dev1, i, GPIO_DIR_OUT);
	}


	while (1) {
		for(i=0;i<32;i++){
			if(i==0 || i==1 || i==9 || i==10 || i==12 || i==18 || i==25){
				continue;
			}
			gpio_pin_write(dev0, i, cnt % 2);
		}

		for(i=0;i<16;i++){
			if(i==3 || i==5 || i==7 || i==9){
				continue;
			}
			gpio_pin_write(dev1, i, cnt % 2);
		}

		cnt++;
		k_sleep(100);

	}


	/*gpio_pin_configure(dev, 2, GPIO_DIR_OUT);
	gpio_pin_write(dev, 2, 1);

	gpio_pin_configure(dev, 3, GPIO_DIR_OUT);
	gpio_pin_write(dev, 3, 1);

	gpio_pin_configure(dev, 4, GPIO_DIR_OUT);
	gpio_pin_write(dev, 4, 1);

	gpio_pin_configure(dev, 5, GPIO_DIR_OUT);
		gpio_pin_write(dev, 5, 1);*/

}
#endif

void button_pressed(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
{
	//SEGGER_SYSVIEW_PrintfHost("Button pressed at %d\n", k_cycle_get_32());
	//VDDH_ACTIVATE();
	//led_on(LED_RED);

}

static struct gpio_callback gpio_cb;

static void button_led()
{
	struct device *gpiob;

	gpiob = device_get_binding(SW0_GPIO_NAME);
	if (!gpiob) {
		SYS_LOG_ERR("Get button error");
		return;
	}

	gpio_pin_configure(gpiob, SW0_GPIO_PIN,
			   GPIO_DIR_IN | GPIO_INT |  GPIO_PUD_PULL_UP | EDGE | (2 << 16));

	gpio_init_callback(&gpio_cb, button_pressed, BIT(SW0_GPIO_PIN));

	gpio_add_callback(gpiob, &gpio_cb);
	gpio_pin_enable_callback(gpiob, SW0_GPIO_PIN);

	/*while (1) {
		u32_t val = 0;

		//gpio_pin_read(gpiob, SW0_GPIO_PIN, &val);
		k_sleep(SLEEP_TIME);
	}*/
}


/*
static void RTT_terminal()
{
	u8_t buf[20];
	while(1){
		if(SEGGER_RTT_HASDATA(0)){
			SEGGER_RTT_Read(0, buf, 20);
			printk("Data read: %s\n", buf);
			memset(buf, 0, sizeof(buf));
		}
		k_sleep(200);
	}
}

*/




void main(void)
{

	struct device *dev;

#if CONFIG_SEGGER_SYSTEMVIEW
	sysview_init();
#endif

	//sensors_init();

	//button_led();

	k_tid_t tid_tmp;


	//VDDH_ACTIVATE();

	//

	//test_gpios();

#if CONFIG_LORA
	lora_init();
#endif

	//_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP);

/*
	while (1) {
		printk("\nApplication Thread\n");
		k_sleep(SECONDS_TO_SLEEP * 1000);
	}
*/
	//blink_led(LED_RED, MSEC_PER_SEC/4, K_SECONDS(2));
	//led_on(LED_RED);

/*

	tid_tmp = k_thread_create(&printer_thread_data, printer_stack,
				K_THREAD_STACK_SIZEOF(printer_stack),
				(k_thread_entry_t)led_blink2,
				NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, 0);*/
#if CONFIG_SEGGER_SYSTEMVIEW
	sysview_addTaskName(tid_tmp, "Led");
#endif

/*
	tid_tmp = k_thread_create(&printer2_thread_data, printer2_stack,
				K_THREAD_STACK_SIZEOF(printer2_stack),
				(k_thread_entry_t)printer_thread,
				NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_SECONDS(15));*/
#if CONFIG_SEGGER_SYSTEMVIEW
	sysview_addTaskName(tid_tmp, "Sensor");
#endif
/*
	tid_tmp = k_thread_create(&rttterm_thread_data, rttterm_stack,
				K_THREAD_STACK_SIZEOF(rttterm_stack),
				(k_thread_entry_t)RTT_terminal,
				NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, 0);
	sysview_addTaskName(tid_tmp, "RTT Terminal");
*/


}

