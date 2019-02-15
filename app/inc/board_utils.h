/*************************************************************************//**
 * \file board_utils.h
 * \brief board include selection
 *
 * \author Marc Pignat
 * \copyright Copyright (c) 2019 Marc Pignat
 *
 **************************************************************************//*
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef INC_BOARD_UTILS_H
#define INC_BOARD_UTILS_H

#ifdef CONFIG_BOARD_NRF52840_LORAIOT
#include <board_utils.h>
#else

#define SENSOR_ACTIVATE(name)	do {} while (0)
#define SENSOR_DEACTIVATE(name)	do {} while (0)

#define VDDH_ACTIVATE()			do {} while (0)
#define VDDH_DEACTIVATE()		do {} while (0)
#define VDDH_IS_ENABLED()		do {} while (0)

#define NOT_IN_DEBUG()	( 1 )

#ifdef __cplusplus
extern "C"
{
#endif

static inline void power_sensor(const char *name, u8_t power){}
static inline void enable_high_voltage(u8_t status){}
static inline void led_on(u8_t led){}
static inline void led_off(u8_t led){}
static inline void led_toggle(u8_t led){}
static inline void blink_led(u8_t led, u32_t period_ms, u32_t duration_ms){}

#ifdef __cplusplus
}
#endif

#endif

#endif /* INC_BOARD_UTILS_H */
