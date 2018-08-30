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


#define POWER_MANAGEMENT	0
#define MCU_GPIO_TEST		0

#define EDGE    (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)

K_THREAD_STACK_DEFINE(printer_stack, 1024);
K_THREAD_STACK_DEFINE(printer2_stack, 3000);
//K_THREAD_STACK_DEFINE(rttterm_stack, 2048);

static struct k_thread printer_thread_data;
static struct k_thread printer2_thread_data;
//static struct k_thread rttterm_thread_data;

extern void power_sensor(const char *name, u8_t power);
extern void enable_high_voltage(u8_t status);

#if POWER_MANAGEMENT

#define SECONDS_TO_SLEEP	60


/* In Tickless Kernel mode, time is passed in milliseconds instead of ticks */
#ifdef CONFIG_TICKLESS_KERNEL
#define TICKS_TO_SECONDS_MULTIPLIER 1000
#define TIME_UNIT_STRING "milliseconds"
#else
#define TICKS_TO_SECONDS_MULTIPLIER CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define TIME_UNIT_STRING "ticks"
#endif
#if defined(CONFIG_SYS_POWER_DEEP_SLEEP)
#define MAX_POWER_SAVING_STATE 3
#else
#define MAX_POWER_SAVING_STATE 2
#endif
#define NO_POWER_SAVING_STATE 0

#define MIN_TIME_TO_SUSPEND	((SECONDS_TO_SLEEP * \
				  TICKS_TO_SECONDS_MULTIPLIER) - \
				  (TICKS_TO_SECONDS_MULTIPLIER / 2))
#define GPIO_CFG_SENSE_LOW (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos)

/* To Create Ordered List of devices */
static void create_device_list(void);
/* Suspend Devices based on Ordered List */
static void suspend_devices(void);
/* Resume Devices */
static void resume_devices(void);
static struct device *device_list;
static int device_count;
static int post_ops_done = 1;

/*
 * Example ordered list to store devices on which
 * device power policies would be executed.
 */
#define DEVICE_POLICY_MAX 15
static char device_ordered_list[DEVICE_POLICY_MAX];
static char device_retval[DEVICE_POLICY_MAX];
static int pm_state;
static struct device *gpiob;

/* change this to use another GPIO port */
#define PORT    SW0_GPIO_NAME
#define PIN	SW0_GPIO_PIN
#define LOW	0

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application creates Idleness, Due to which System Idle Thread is\n"\
	"scheduled and it enters into various Low Power States.\n"\
	"Press Button 1 on Board to wake device from Low Power State.\n"

extern int nrf_gpiote_interrupt_enable(uint32_t mask);
extern void nrf_gpiote_clear_port_event(void);

void configure_ram_retention(void)
{
	/* Place Holder for RAM retention */
	/* Turn Off the RAM Blocks which are not needed */
}

#endif


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

#if MCU_GPIO_TEST
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
		k_sleep(1000);
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

#if POWER_MANAGEMENT
	create_device_list();
#endif

#if MCU_GPIO_TEST
	test_gpios();
#endif

#if CONFIG_LORA
	lora_init();
#endif

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

#if POWER_MANAGEMENT
	//_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP);
	while (1) {
		printk("\nApplication Thread\n");
		/* Create Idleness to make Idle thread run */
		k_sleep(SECONDS_TO_SLEEP * 1000);
	}
#endif

}

#if POWER_MANAGEMENT

/* This Function decides which Low Power Mode to Enter based on Idleness.
 * In actual Application this decision can be made using time (ticks)
 * And RTC timer can be programmed to wake the device after time elapsed.
 */
static int check_pm_policy(s32_t ticks)
{
	static int policy = NO_POWER_SAVING_STATE;
	int power_states[] = {SYS_POWER_STATE_MAX, SYS_POWER_STATE_CPU_LPS,
			SYS_POWER_STATE_CPU_LPS_1, SYS_POWER_STATE_DEEP_SLEEP};
	/*
	 * Compare time available with wake latencies and select
	 * appropriate power saving policy
	 * For the demo we will alternate between following policies
	 * 0 = no power saving operation
	 * 1 = low power state Constant Latency
	 * 2 = Low Power State
	 * 3 = System Off State - Device will reset on Exit from this State
	 */
	policy = ++policy > MAX_POWER_SAVING_STATE ?
					NO_POWER_SAVING_STATE : policy;
	/* Pick Platform specific State */
	return power_states[policy];
}

static void low_power_state_exit(void)
{
	/* Perform some Application specific task on Low Power Mode Exit */

	/* Turn on suspended peripherals/clocks as necessary */
	printk("---- Low power state exit !\n");
}

static void deep_sleep_exit(void)
{
	/* Turn on peripherals and restore device states as necessary */
	resume_devices();
	printk("====Deep sleep exit!\n");
}

static int low_power_state_entry(void)
{
	printk("---->Low power state entry ");
	if (pm_state == SYS_POWER_STATE_CPU_LPS) {
		printk("- CONSTANT LATENCY MODE-");
	} else {
		printk("- LOW POWER MODE -");
	}
	configure_ram_retention();
	_sys_soc_set_power_state(pm_state);
	return SYS_PM_LOW_POWER_STATE;
}

static int deep_sleep_entry(void)
{
	printk("===> Entry Into Deep Sleep ==");
	/* Don't need pm idle exit event notification */
	_sys_soc_pm_idle_exit_notification_disable();
	/* Save device states and turn off peripherals as necessary */
	suspend_devices();
	configure_ram_retention();
	_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP);
	 /* Exiting from Deep Sleep State */
	deep_sleep_exit();

	return SYS_PM_DEEP_SLEEP;
}

/* Function name : _sys_soc_suspend
 * Return Value : Power Mode Entered Success/Failure
 * Input : Idleness in number of Ticks
 *
 * Description: All request from Idle thread to Enter into
 * Low Power Mode or Deep Sleep State land in this function
 */
int _sys_soc_suspend(s32_t ticks)
{
	int ret = SYS_PM_NOT_HANDLED;
	u32_t level = 0;

	post_ops_done = 0;

	if ((ticks != K_FOREVER) && (ticks < MIN_TIME_TO_SUSPEND)) {
		printk("Not enough time for PM operations " TIME_UNIT_STRING
							" : %d).\n", ticks);
		return SYS_PM_NOT_HANDLED;
	}

	pm_state = check_pm_policy(ticks);

	switch (pm_state) {
	case SYS_POWER_STATE_CPU_LPS:
	case SYS_POWER_STATE_CPU_LPS_1:
		/* Do CPU LPS operations */
		ret = low_power_state_entry();
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
		/* Do deep sleep operations */
		ret = deep_sleep_entry();
		break;
	default:
		/* No PM operations */
		printk("\nNo PM operations done\n");
		ret = SYS_PM_NOT_HANDLED;
		break;
	}

	if (ret != SYS_PM_NOT_HANDLED) {
		/*
		 * Do any arch or soc specific post operations specific to the
		 * power state.
		 */
		if (!post_ops_done) {
			if ((pm_state == SYS_POWER_STATE_CPU_LPS_1) ||
					(pm_state == SYS_POWER_STATE_CPU_LPS)) {
				low_power_state_exit();
			}
			post_ops_done = 1;
			_sys_soc_power_state_post_ops(pm_state);
		}
	}

	return ret;
}

void _sys_soc_resume(void)
{
	/*
	 * This notification is called from the ISR of the event
	 * that caused exit from kernel idling after PM operations.
	 *
	 * Some CPU low power states require enabling of interrupts
	 * atomically when entering those states. The wake up from
	 * such a state first executes code in the ISR of the interrupt
	 * that caused the wake. This hook will be called from the ISR.
	 * For such CPU LPS states, do post operations and restores here.
	 * The kernel scheduler will get control after the ISR finishes
	 * and it may schedule another thread.
	 *
	 * Call _sys_soc_pm_idle_exit_notification_disable() if this
	 * notification is not required.
	 */
	if (!post_ops_done) {
		if (pm_state == SYS_POWER_STATE_CPU_LPS) {
			low_power_state_exit();
		}
		post_ops_done = 1;
		_sys_soc_power_state_post_ops(pm_state);
	}
}

static void suspend_devices(void)
{

	for (int i = device_count - 1; i >= 0; i--) {
		int idx = device_ordered_list[i];

		/* If necessary  the policy manager can check if a specific
		 * device in the device list is busy as shown below :
		 * if(device_busy_check(&device_list[idx])) {do something}
		 */
		device_retval[i] = device_set_power_state(&device_list[idx],
						DEVICE_PM_SUSPEND_STATE);
	}
}

static void resume_devices(void)
{
	int i;

	for (i = 0; i < device_count; i++) {
		if (!device_retval[i]) {
			int idx = device_ordered_list[i];

			device_set_power_state(&device_list[idx],
						DEVICE_PM_ACTIVE_STATE);
		}
	}
}

static void create_device_list(void)
{
	int count;
	int i;

	/*
	 * Following is an example of how the device list can be used
	 * to suspend devices based on custom policies.
	 *
	 * Create an ordered list of devices that will be suspended.
	 * Ordering should be done based on dependencies. Devices
	 * in the beginning of the list will be resumed first.
	 */
	device_list_get(&device_list, &count);
	device_count = 4; /* Reserve for 32KHz, 16MHz, system clock, and uart */

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "clk_k32src")) {
			device_ordered_list[0] = i;
		} else if (!strcmp(device_list[i].config->name, "clk_m16src")) {
			device_ordered_list[1] = i;
		} else if (!strcmp(device_list[i].config->name, "sys_clock")) {
			device_ordered_list[2] = i;
		} else if (!strcmp(device_list[i].config->name, "UART_0")) {
			device_ordered_list[3] = i;
		} else {
			device_ordered_list[device_count++] = i;
		}
	}
}

#endif
