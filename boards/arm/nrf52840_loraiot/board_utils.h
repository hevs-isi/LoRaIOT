#ifndef __BOARD_UTILS_H
#define __BOARD_UTILS_H

#include <board.h>

#define SENSOR_ACTIVATE(name)	power_sensor(name, 1)
#define SENSOR_DEACTIVATE(name)	power_sensor(name, 0)

#define VDDH_ACTIVATE()			enable_high_voltage(1)
#define VDDH_DEACTIVATE()		enable_high_voltage(0)
#define VDDH_IS_SET()			get_vddh_status()

void power_sensor(const char *name, u8_t power);
void enable_high_voltage(u8_t status);


#endif /* __BOARD_UTILS_H */
