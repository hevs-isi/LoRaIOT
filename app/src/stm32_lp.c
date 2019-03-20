#include <zephyr.h>

#include <soc_registers.h>
#include <soc.h>
#include <power.h>
#include <counter.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(lp, LOG_LEVEL_DBG);

#define ALARM_CHANNEL_ID 0

static struct counter_alarm_cfg alarm_cfg;

static void counter_isr(struct device *counter_dev, u8_t chan_id,
	u32_t ticks, void *user_data)
{
	// nop!
	LOG_DBG("here");
}

void stm32_sleep(u32_t duration)
{
	struct device *counter_dev;
	int err;

	LOG_DBG("stm32_sleep(%d)", duration);

	counter_dev = device_get_binding(DT_RTC_0_NAME);

	counter_start(counter_dev);

	alarm_cfg.absolute = false;
	alarm_cfg.ticks = counter_us_to_ticks(counter_dev, duration*1000);
	alarm_cfg.callback = counter_isr;
	alarm_cfg.user_data = &alarm_cfg;

	err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, &alarm_cfg);

	//HAL_PWR_EnterSTOPMode(HAL_PWR_EnterSLEEPMode, PWR_STOPENTRY_WFI);
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
	//HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
	//HAL_PWREx_DisableLowPowerRunMode();
}

enum power_states sys_suspend(s32_t ticks)
{
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
	#ifdef CONFIG_TRACING
		z_sys_trace_idle(ticks);
	#endif /* CONFIG_TRACING */

	/* clear BASEPRI so wfi is awakened by incoming interrupts */
	__asm__("eors.n r0, r0\nmsr BASEPRI, r0\n");

	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

	return SYS_POWER_STATE_AUTO;
}

void sys_resume(void)
{
	
}
