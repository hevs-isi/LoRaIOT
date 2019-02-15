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


#if CONFIG_LORA_IM881A
#include <im881a.h>
#endif

void main(void)
{
	lora_init();
}
