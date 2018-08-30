/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 ARM Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for the Nordic Semiconductor nRF5X GPIO module.
 */
#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <logging/sys_log.h>

#include <lpcomp.h>

#include <nrfx.h>
#include <nrfx_config.h>
#include <nrfx_lpcomp.h>

#include "nrf_common.h"

#ifdef CONFIG_LPCOMP_NRF5
DEVICE_DECLARE(lpcomp_nrf5);
#endif

struct lpcomp_nrf5_data {
	lpcomp_callback_handler_t callback;
};

static struct lpcomp_nrf5_data lpcomp_nrf5_dev_data = {
	.callback = NULL,
};

static u8_t lpcomp_status;

enum {
	UNINITIALIZED,
	INITIALIZED,
	ON
};

/** Configuration data */
/*struct lpcomp_nrf5_config {
	nrf_lpcomp_config_t config;
	nrfx_lpcomp_config_t xconfig;
};*/

/**
 * @brief Configure analog input, ...
 */
/*
static int lpcomp_nrf5_config(struct device *dev, u32_t pin, int flags)
{
	struct lpcomp_nrf5_config *cfg = dev->config->config_info;
	nrfx_err_t lpcomp_error;

	lpcomp_error = nrfx_lpcomp_init(&cfg, lpcomp_event_handler);
	if(lpcomp_error != NRFX_SUCCESS){
		SYS_LOG_ERR("Error initializing LPCOMP: %d", lpcomp_error);
		return 1;
	}

	return 0;
}*/

void lpcomp_event_handler(nrf_lpcomp_event_t event)
{
	struct device *dev;
	struct lpcomp_nrf5_data *dev_data;

	SYS_LOG_DBG("LPCOMP event");

	dev = DEVICE_GET(lpcomp_nrf5);
	dev_data = ((struct lpcomp_nrf5_data *)(dev)->driver_data);

	dev_data->callback();

	if (event == NRF_LPCOMP_EVENT_CROSS)
	{
		SYS_LOG_DBG("Input voltage crossed the threshold");
	}
}

static void lpcomp_nrf5_enable(struct device *dev)
{
	if(lpcomp_status == INITIALIZED){
		nrfx_lpcomp_enable();
		lpcomp_status = ON;
	}
}

static void lpcomp_nrf5_disable(struct device *dev)
{
	if(lpcomp_status == ON){
		nrfx_lpcomp_disable();
		lpcomp_status = INITIALIZED;
	}
}

/** Set the callback function */
static void lpcomp_nrf5_set_callback(struct device *dev, lpcomp_callback_handler_t cb)
{

	struct lpcomp_nrf5_data * const dev_data = \
				((struct lpcomp_nrf5_data * const)(dev)->driver_data);

	dev_data->callback = cb;
}


static const struct lpcomp_driver_api lpcomp_nrf5_drv_api_funcs = {
	.enable = lpcomp_nrf5_enable,
	.disable = lpcomp_nrf5_disable,
	.set_callback = lpcomp_nrf5_set_callback,
};



/* Initialization */
#ifdef CONFIG_LPCOMP_NRF5
static int lpcomp_nrf5_init(struct device *dev)
{
	const nrfx_lpcomp_config_t *cfg = dev->config->config_info;
	nrfx_err_t lpcomp_error;

	IRQ_CONNECT(NRF5_IRQ_LPCOMP_IRQn, 0, nrfx_lpcomp_irq_handler, NULL, 0);
	irq_enable(NRF5_IRQ_LPCOMP_IRQn);

	lpcomp_error = nrfx_lpcomp_init(cfg, lpcomp_event_handler);
	if(lpcomp_error != NRFX_SUCCESS){
		SYS_LOG_ERR("Error initializing LPCOMP: %d", lpcomp_error);
		return 1;
	}

	lpcomp_status = INITIALIZED;

	return 0;
}
/*
static const nrfx_lpcomp_config_t lpcomp_nrf5_cfg = {
		.hal    = { (nrf_lpcomp_ref_t)NRFX_LPCOMP_CONFIG_REFERENCE ,    \
					(nrf_lpcomp_detect_t)NRFX_LPCOMP_CONFIG_DETECTION,  \
					(nrf_lpcomp_hysteresis_t)NRFX_LPCOMP_CONFIG_HYST }, \
		.input  = (nrf_lpcomp_input_t)NRFX_LPCOMP_CONFIG_INPUT,         \
		.interrupt_priority = NRFX_LPCOMP_CONFIG_IRQ_PRIORITY           \
};
*/

static const nrfx_lpcomp_config_t lpcomp_nrf5_cfg = {
		.hal    = { (nrf_lpcomp_ref_t)NRF_LPCOMP_REF_SUPPLY_5_8 ,    \
					(nrf_lpcomp_detect_t)NRF_LPCOMP_DETECT_CROSS,  \
					(nrf_lpcomp_hysteresis_t)NRF_LPCOMP_HYST_NOHYST }, \
		.input  = (nrf_lpcomp_input_t)NRF_LPCOMP_INPUT_5,         \
		.interrupt_priority = 0           \
};


DEVICE_AND_API_INIT(lpcomp_nrf5, CONFIG_LPCOMP_NRF5_DEV_NAME, lpcomp_nrf5_init,
		    &lpcomp_nrf5_dev_data, &lpcomp_nrf5_cfg,
		    POST_KERNEL, CONFIG_LPCOMP_NRF5_INIT_PRIORITY,
		    &lpcomp_nrf5_drv_api_funcs);

#endif /* CONFIG_LPCOMP_NRF5 */

