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

#include <string.h>

#include "lora.h"
#include <sensor.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#if CONFIG_LORA_IM881A
#include <im881a.h>
#endif

void main(void)
{
	LOG_DBG("main");
	lora_init();
}
