#ifndef __BOARD_UTILS_H
#define __BOARD_UTILS_H

#include <board.h>

#define SENSOR_ACTIVATE(name)	power_sensor(name, 1)
#define SENSOR_DEACTIVATE(name)	power_sensor(name, 0)

#define VDDH_ACTIVATE()			enable_high_voltage(1)
#define VDDH_DEACTIVATE()		enable_high_voltage(0)
#define VDDH_IS_ENABLED()		get_vddh_status()

#define NOT_IN_DEBUG()	( ( CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk ) == 0 )

void power_sensor(const char *name, u8_t power);
void enable_high_voltage(u8_t status);
void led_on(u8_t led);
void led_off(u8_t led);
void led_toggle(u8_t led);
void blink_led(u8_t led, u32_t period_ms, u32_t duration_ms);

#endif /* __BOARD_UTILS_H */
