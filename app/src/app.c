
#include <zephyr.h>
#include <logging/sys_log.h>
#include <board_utils.h>

#include <kernel.h>
#include <device.h>
#include <gpio.h>

#include <string.h>

#include "sensors.h"
#include "lora.h"
#include <sensor.h>
#include <lpcomp.h>

#if CONFIG_LORA
#include <wimod_lorawan_api.h>
#endif

#define MOTION_BLIND_TIME	10	// seconds
#define SOUND_BLIND_TIME	10	// seconds

#if CONFIG_PYD1588
static struct sensor_trigger motion_trig;

void enable_motion_detection(struct k_timer *timer_id)
{
	SENSOR_ACTIVATE(CONFIG_PYD1588_NAME);
	SYS_LOG_INF("Motion detection re-enabled\n");
}
K_TIMER_DEFINE(motion_timeout, enable_motion_detection, NULL);

static void motion_handler(struct device *dev, struct sensor_trigger *trig)
{
	u8_t data[3];
	SYS_LOG_INF("Motion detected 2\n");
	blink_led(LED_GREEN, MSEC_PER_SEC/4, K_SECONDS(4));
	SENSOR_DEACTIVATE(CONFIG_PYD1588_NAME);
	k_timer_start(&motion_timeout, K_SECONDS(MOTION_BLIND_TIME), 0);

	data[0] = 0x10;
#if CONFIG_LORA
	enable_uart();
	wimod_lorawan_send_c_radio_data(1, data, 1);
	disable_uart();
#endif
}
#endif

#if CONFIG_ANALOGMIC
static struct sensor_trigger sound_trig;

void enable_sound_detection(struct k_timer *timer_id)
{
	SENSOR_ACTIVATE(CONFIG_ANALOGMIC_NAME);
	SYS_LOG_INF("Sound detection re-enabled\n");
}
K_TIMER_DEFINE(sound_timeout, enable_sound_detection, NULL);

static void sound_handler(struct device *dev, struct sensor_trigger *trig)
{
	struct device *lpcomp;
	u8_t data[3];
	SYS_LOG_INF("Sound detected 2\n");

	lpcomp = device_get_binding(CONFIG_LPCOMP_NRF5_DEV_NAME);
	lpcomp_disable(lpcomp);

	blink_led(LED_BLUE, MSEC_PER_SEC/8, K_SECONDS(2));

	SENSOR_DEACTIVATE(CONFIG_ANALOGMIC_NAME);
	k_timer_start(&sound_timeout, K_SECONDS(SOUND_BLIND_TIME), 0);

	data[0] = 0x20;
#if CONFIG_LORA
	enable_uart();
	wimod_lorawan_send_c_radio_data(1, data, 1);
	disable_uart();
#endif

}
#endif

static void app()
{
	struct device *dev;
	SYS_LOG_DBG("Start application\n");

#if CONFIG_PYD1588

	dev = device_get_binding(CONFIG_PYD1588_NAME);
	motion_trig.type = SENSOR_TRIG_TAP;
	if(sensor_trigger_set(dev, &motion_trig, motion_handler) < 0){
		SYS_LOG_ERR("Cannot enable tap trigger for motion.");
	}

	SENSOR_ACTIVATE(CONFIG_PYD1588_NAME);
#endif

#if CONFIG_ANALOGMIC
	dev = device_get_binding(CONFIG_ANALOGMIC_NAME);
	sound_trig.type = SENSOR_TRIG_TAP;
	if(sensor_trigger_set(dev, &sound_trig, sound_handler) < 0){
		SYS_LOG_ERR("Cannot enable tap trigger for sound.");
	}

	SENSOR_ACTIVATE(CONFIG_ANALOGMIC_NAME);
#endif


/*
	while (1) {
		led_toggle(LED_RED);
		k_sleep(MSEC_PER_SEC*4);
		//k_yield();
	}*/
}


K_THREAD_DEFINE(app_thread_id, 2048, app, NULL, NULL, NULL,
		K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

