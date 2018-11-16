
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

#if CONFIG_SEGGER_SYSTEMVIEW
#include <systemview/SEGGER_SYSVIEW.h>
#include "sysview.h"
#endif

#if CONFIG_LORA
#include <wimod_lorawan_api.h>
#endif

#define SOUND_DETECTION_WINDOW		180 // seconds
#define SOUND_BLIND_TIME			120	// seconds
#define LORA_MOTION_SOUND_PORT		5

#if CONFIG_LORA
K_THREAD_STACK_DEFINE(lora_msg_stack, 1024);
static struct k_work_q lora_msg_work_q;
static u8_t sound_activated;

struct lora_msg {
	u8_t port;
	u8_t data[20];
	u8_t size;
	struct k_work work;
};

static struct lora_msg lmsg;
#endif // CONFIG_LORA


#if CONFIG_PYD1588
static struct sensor_trigger motion_trig;

#if CONFIG_ANALOGMIC
void disable_sound_detection(struct k_timer *timer_id)
{
	// No detection during SOUND_DETECTION_WINDOW seconds:
	// disable the microphone and re-enable back the motion detector

    SYS_LOG_INF("Microphone disabled. Motion re-enabled.\n");

#if CONFIG_LPCOMP_NRF5
	struct device *lpcomp;
	lpcomp = device_get_binding(CONFIG_LPCOMP_NRF5_DEV_NAME);
	lpcomp_disable(lpcomp);
#endif

	SENSOR_DEACTIVATE(CONFIG_ANALOGMIC_NAME);
	sound_activated = 0;
	SENSOR_ACTIVATE(CONFIG_PYD1588_NAME);
	blink_led(LED_RED, MSEC_PER_SEC/2, K_SECONDS(3));

#if CONFIG_SEGGER_SYSTEMVIEW
    SEGGER_SYSVIEW_PrintfHost("Microphone disabled. Motion re-enabled.");
#endif

#if CONFIG_LORA
	lmsg.data[0] = 0x30;
	lmsg.size = 1;
	lmsg.port = LORA_MOTION_SOUND_PORT;
	k_work_submit(&lmsg.work);
#endif
}
K_TIMER_DEFINE(sound_detection_timeout, disable_sound_detection, NULL);

void enable_sound_detection(struct k_timer *timer_id)
{
	SENSOR_ACTIVATE(CONFIG_ANALOGMIC_NAME);
	sound_activated = 1;
	SYS_LOG_INF("Sound detection re-enabled\n");
#if CONFIG_SEGGER_SYSTEMVIEW
    SEGGER_SYSVIEW_PrintfHost("Microphone enabled for %d", SOUND_DETECTION_WINDOW);
#endif
	k_timer_start(&sound_detection_timeout, K_SECONDS(SOUND_DETECTION_WINDOW), 0);
}
K_TIMER_DEFINE(sound_timeout, enable_sound_detection, NULL);

#endif

static void motion_handler(struct device *dev, struct sensor_trigger *trig)
{
	SYS_LOG_INF("Motion detected 2\n");
	blink_led(LED_GREEN, MSEC_PER_SEC/8, K_SECONDS(2));
	SENSOR_DEACTIVATE(CONFIG_PYD1588_NAME);
#if CONFIG_ANALOGMIC
	k_timer_start(&sound_timeout, K_SECONDS(SOUND_BLIND_TIME), 0);
#endif

#if CONFIG_SEGGER_SYSTEMVIEW
    SEGGER_SYSVIEW_PrintfHost("Motion detected");
#endif

#if CONFIG_LORA
	lmsg.data[0] = 0x10;
	lmsg.size = 1;
	lmsg.port = LORA_MOTION_SOUND_PORT;
	k_work_submit(&lmsg.work);
#endif
}
#endif

#if CONFIG_ANALOGMIC
static struct sensor_trigger sound_trig;

static void sound_handler(struct device *dev, struct sensor_trigger *trig)
{
	SYS_LOG_INF("Sound detected 2\n");
	k_timer_stop(&sound_detection_timeout);

#if CONFIG_LPCOMP_NRF5
	struct device *lpcomp;
	lpcomp = device_get_binding(CONFIG_LPCOMP_NRF5_DEV_NAME);
	lpcomp_disable(lpcomp);
#endif

	blink_led(LED_BLUE, MSEC_PER_SEC/4, K_SECONDS(1));

	SENSOR_DEACTIVATE(CONFIG_ANALOGMIC_NAME);
	k_timer_start(&sound_timeout, K_SECONDS(SOUND_BLIND_TIME), 0);

#if CONFIG_SEGGER_SYSTEMVIEW
    SEGGER_SYSVIEW_PrintfHost("Sound detected");
#endif

#if CONFIG_LORA
	lmsg.data[0] = 0x20;
	lmsg.size = 1;
	lmsg.port = LORA_MOTION_SOUND_PORT;
	k_work_submit(&lmsg.work);
#endif

}
#endif

#if CONFIG_LORA
static void lora_msg_send(struct k_work *item)
{
	struct lora_msg *msg = CONTAINER_OF(item, struct lora_msg, work);

	enable_uart();
	wimod_lorawan_send_c_radio_data(msg->port, msg->data, msg->size);
	disable_uart();

	VDDH_DEACTIVATE();

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

#if CONFIG_LPCOMP_NRF5
	struct device *lpcomp;
	lpcomp = device_get_binding(CONFIG_LPCOMP_NRF5_DEV_NAME);
	lpcomp_disable(lpcomp);
#endif
#endif

#if CONFIG_LORA
	k_work_q_start(&lora_msg_work_q, lora_msg_stack,
		   K_THREAD_STACK_SIZEOF(lora_msg_stack), K_PRIO_PREEMPT(0));

	k_work_init(&lmsg.work, lora_msg_send);
#endif
}

static void periodic_app()
{
	struct device *dev;
	struct sensor_value sense_value;
	int ret;
	double temp;
	u8_t tx_data[15];
	u16_t value;

	while (1) {

#if 1
		tx_data[0] = 0x01;

		/* Temperature & Humidity */
		/***********************************************************************/
		SENSOR_ACTIVATE(CONFIG_SHTC1_NAME);

		dev = device_get_binding(CONFIG_SHTC1_NAME);
		ret = sensor_sample_fetch(dev);
		if (ret) {
			SYS_LOG_ERR("Sample fetch failed %d\n", ret);
		}

		SENSOR_DEACTIVATE(CONFIG_SHTC1_NAME);

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &sense_value);
		temp = sensor_value_to_double(&sense_value);
		value = (int)(temp*100);
		tx_data[1] = value >> 8;
		tx_data[2] = value;

		ret = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &sense_value);
		temp = sensor_value_to_double(&sense_value);
		value = (int)(temp*100);
		tx_data[3] = value >> 8;
		tx_data[4] = value;

		/* Light */
		/***********************************************************************/
		SENSOR_ACTIVATE(CONFIG_OPT3002_NAME);
		dev = device_get_binding(CONFIG_OPT3002_NAME);
		ret = sensor_sample_fetch(dev);
		if (ret) {
			SYS_LOG_ERR("Sample fetch failed %d\n", ret);
		}
		SENSOR_DEACTIVATE(CONFIG_OPT3002_NAME);

		ret = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &sense_value);
		temp = sensor_value_to_double(&sense_value);
		value = (int)(temp/1000); // in uW/cm2
		tx_data[5] = value >> 8;
		tx_data[6] = value;


		/* Voltages */
		/***********************************************************************/
		VDDH_ACTIVATE();
		SENSOR_ACTIVATE(CONFIG_PAC1934_NAME);
		dev = device_get_binding(CONFIG_PAC1934_NAME);
		ret = sensor_sample_fetch(dev);
		if (ret) {
			SYS_LOG_ERR("Sample fetch failed %d\n", ret);
		}
		SENSOR_DEACTIVATE(CONFIG_PAC1934_NAME);

		// Primary battery voltage
		ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE_1, &sense_value);
		temp = sensor_value_to_double(&sense_value);
		value = (int)(temp*1000);
		tx_data[7] = value >> 8;
		tx_data[8] = value;

		// Secondary battery voltage
		ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE_2, &sense_value);
		temp = sensor_value_to_double(&sense_value);
		value = (int)(temp*1000);
		tx_data[9] = value >> 8;
		tx_data[10] = value;

		// Solar voltage
		ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE_4, &sense_value);
		temp = sensor_value_to_double(&sense_value);
		value = (int)(temp*1000);
		tx_data[11] = value >> 8;
		tx_data[12] = value;

		// Solar current
		ret = sensor_channel_get(dev, SENSOR_CHAN_CURRENT_4, &sense_value);
		temp = sensor_value_to_double(&sense_value);
		value = (int)(temp*1e6); // in uA
		tx_data[13] = value >> 8;
		tx_data[14] = value;
#endif

#if CONFIG_LORA
		memcpy(lmsg.data, tx_data, 15);
		lmsg.size = 15;
		lmsg.port = 1;
		k_work_submit(&lmsg.work);
#endif

		// BUG: Patch: need to deactivate the microphone while sending lora message.
		// Somehow the voltage drops below 1.8V when the a lora packet is sent while
		// the microphone is activated.
		if(sound_activated){
		    SENSOR_ACTIVATE(CONFIG_ANALOGMIC_NAME);
		}
		k_sleep(MSEC_PER_SEC*300);
		if(sound_activated){
#if CONFIG_LPCOMP_NRF5
            struct device *lpcomp;
            lpcomp = device_get_binding(CONFIG_LPCOMP_NRF5_DEV_NAME);
            lpcomp_disable(lpcomp);
#endif
		    SENSOR_DEACTIVATE(CONFIG_ANALOGMIC_NAME);
		}
	}
}

K_THREAD_DEFINE(app_thread_id, 1024, app, NULL, NULL, NULL,
        K_PRIO_PREEMPT(0), 0, K_NO_WAIT);


K_THREAD_DEFINE(periodic_app_thread_id, 1024, periodic_app, NULL, NULL, NULL,
		K_PRIO_PREEMPT(10), 0, K_SECONDS(5));


