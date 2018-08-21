/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <version.h>
#include <sensor.h>
#include <device.h>
#include <gpio.h>
#include <board.h>
#include <board_utils.h>
#include <stdio.h>

#define SYS_LOG_DOMAIN			"sensor"
#include <logging/sys_log.h>

#include "sensors.h"

#include <pac1934.h>


struct device *temp_dev;

#if CONFIG_SHTC1
static int shell_cmd_temp(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *dev;
	struct sensor_value sense_value;
	int ret;
	double temp;

	SENSOR_ACTIVATE(CONFIG_SHTC1_NAME);
	dev = device_get_binding(CONFIG_SHTC1_NAME);
	ret = sensor_sample_fetch(dev);
	if (ret) {
		SYS_LOG_ERR("Sample fetch failed %d\n", ret);
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &sense_value);
	SENSOR_DEACTIVATE(CONFIG_SHTC1_NAME);

	temp = sensor_value_to_double(&sense_value);

	printf("Sensor value %.2f Celsius\n", temp);
	return 0;
}

static int shell_cmd_humidity(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *dev;
	struct sensor_value sense_value;
	int ret;

	SENSOR_ACTIVATE(CONFIG_SHTC1_NAME);
	dev = device_get_binding(CONFIG_SHTC1_NAME);
	ret = sensor_sample_fetch(dev);
	if (ret) {
		SYS_LOG_ERR("Sample fetch failed %d\n", ret);
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &sense_value);
	SENSOR_DEACTIVATE(CONFIG_SHTC1_NAME);

	printf("Sensor value %.2f%%\n", sensor_value_to_double(&sense_value));
	return 0;
}
#endif

#if CONFIG_OPT3002
static int shell_cmd_light(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *dev;
	struct sensor_value sense_value;
	int ret;

	SENSOR_ACTIVATE(CONFIG_OPT3002_NAME);
	dev = device_get_binding(CONFIG_OPT3002_NAME);
	ret = sensor_sample_fetch(dev);
	if (ret) {
		SYS_LOG_ERR("Sample fetch failed %d\n", ret);
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &sense_value);
	SENSOR_DEACTIVATE(CONFIG_OPT3002_NAME);

	printf("Sensor value %.2f\n", sensor_value_to_double(&sense_value));
	return 0;
}
#endif

#if CONFIG_PAC1934
static int shell_cmd_voltage(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *dev;
	struct sensor_value sense_value;
	int ret;

	SENSOR_ACTIVATE(CONFIG_PAC1934_NAME);

	//pac1934_read_register(PAC1934_REG_PRODUCT_ID, d, 1);

	dev = device_get_binding(CONFIG_PAC1934_NAME);
	ret = sensor_sample_fetch(dev);
	if (ret) {
		SYS_LOG_ERR("Sample fetch failed %d\n", ret);
		return ret;
	}
	SENSOR_DEACTIVATE(CONFIG_PAC1934_NAME);

	ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE_1, &sense_value);
	printf("Primary battery: %.3f\n", sensor_value_to_double(&sense_value));

	ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE_2, &sense_value);
	printf("Secondary battery: %.3f\n", sensor_value_to_double(&sense_value));

	ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE_3, &sense_value);
	printf("Load voltage: %.3f\n", sensor_value_to_double(&sense_value));

	ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE_4, &sense_value);
	printf("Solar voltage: %.3f\n", sensor_value_to_double(&sense_value));

	return 0;
}

static int shell_cmd_current(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *dev;
	struct sensor_value sense_value;
	int ret;

	SENSOR_ACTIVATE(CONFIG_PAC1934_NAME);

	dev = device_get_binding(CONFIG_PAC1934_NAME);
	ret = sensor_sample_fetch(dev);
	if (ret) {
		SYS_LOG_ERR("Sample fetch failed %d\n", ret);
		return ret;
	}
	SENSOR_DEACTIVATE(CONFIG_PAC1934_NAME);

	ret = sensor_channel_get(dev, SENSOR_CHAN_CURRENT_1, &sense_value);
	printf("Primary battery current: %.6f\n", sensor_value_to_double(&sense_value));

	ret = sensor_channel_get(dev, SENSOR_CHAN_CURRENT_2, &sense_value);
	printf("Secondary battery current: %.6f\n", sensor_value_to_double(&sense_value));

	ret = sensor_channel_get(dev, SENSOR_CHAN_CURRENT_3, &sense_value);
	printf("Load current: %.6f\n", sensor_value_to_double(&sense_value));

	ret = sensor_channel_get(dev, SENSOR_CHAN_CURRENT_4, &sense_value);
	printf("Solar current: %.6f\n", sensor_value_to_double(&sense_value));

	return 0;
}
#endif

/*
static int shell_cmd_params(int argc, char *argv[])
{
	int cnt;

	printk("argc = %d\n", argc);
	for (cnt = 0; cnt < argc; cnt++) {
		printk("  argv[%d] = %s\n", cnt, argv[cnt]);
	}
	return 0;
}
*/

#define SHELL_CMD_VERSION "version"
static int shell_cmd_version(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("Zephyr version %s\n", KERNEL_VERSION_STRING);
	return 0;
}

SHELL_REGISTER_COMMAND(SHELL_CMD_VERSION, shell_cmd_version,
		       "Show kernel version");


static int shell_cmd_temp_die(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);


	int r;
	struct sensor_value temp_value;


	printk("hello");


	r = sensor_sample_fetch(temp_dev);
	if (r) {
		SYS_LOG_ERR("sensor_sample_fetch failed return: %d", r);
		return r;
	}


	r = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &temp_value);
	if (r) {
		SYS_LOG_ERR("sensor_channel_get failed return: %d", r);
		return r;
	}

	printk("Temperature is %d.%dC\n", temp_value.val1, temp_value.val2/10000);




	return 0;
}




static struct shell_cmd commands[] = {
#if CONFIG_SHTC1
	{ "temp", shell_cmd_temp, NULL },
	{ "rh", shell_cmd_humidity, NULL },
#endif
#if CONFIG_OPT3002
	{ "light", shell_cmd_light, NULL },
#endif
#if CONFIG_PAC1934
	{ "voltage", shell_cmd_voltage, NULL },
	{ "current", shell_cmd_current, NULL },
#endif
	{ "temp_die", shell_cmd_temp_die, NULL },
	{ NULL, NULL, NULL }
};



void sensors_init()
{
	SHELL_REGISTER("sense", commands);

	temp_dev = device_get_binding("TEMP_0");
	__ASSERT_NO_MSG(temp_dev);

}
