/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <logging/kernel_event_logger.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <zephyr.h>

#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>

#include "sysview.h"

static u32_t timestamp, interrupt;
static u8_t threads_name_nb;

struct sysview_thread {
	k_tid_t id;
	char name[SYSVIEW_THREAD_NAME_MAX_LEN];
};
static struct sysview_thread sysview_threads_name[SYSVIEW_MAX_THREAD_NB];

extern SEGGER_RTT_CB _SEGGER_RTT;

u32_t sysview_get_timestamp(void)
{
	return timestamp;
}

u32_t sysview_get_interrupt(void)
{
	return interrupt;
}

static void publish_context_switch(u32_t *event_data)
{
#if defined(CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH)
	SEGGER_SYSVIEW_OnTaskStartExec(event_data[1]);
#endif
}

static void publish_interrupt(u32_t *event_data)
{
#if defined(CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT)
	interrupt = event_data[1];

	/* FIXME: RecordEnter and RecordExit seem to be required to keep
	 * SystemView happy; however, there's currently no way to measure
	 * the time it takes for an ISR to execute.
	 */
	SEGGER_SYSVIEW_RecordEnterISR();
	//SEGGER_SYSVIEW_RecordExitISR();
	SEGGER_SYSVIEW_RecordExitISRToScheduler();
#endif
}

static void publish_sleep(u32_t *event_data)
{
#if defined(CONFIG_KERNEL_EVENT_LOGGER_SLEEP)
	SEGGER_SYSVIEW_OnIdle();
#endif
}

static void publish_task(u32_t *event_data)
{
#if defined(CONFIG_KERNEL_EVENT_LOGGER_THREAD)
	u32_t thread_id;

	thread_id = event_data[1];

	switch ((enum sys_k_event_logger_thread_event)event_data[2]) {
	case KERNEL_LOG_THREAD_EVENT_READYQ:
		SEGGER_SYSVIEW_OnTaskStartReady(thread_id);
		break;
	case KERNEL_LOG_THREAD_EVENT_PEND:
		SEGGER_SYSVIEW_OnTaskStopReady(thread_id, 3<<3);
		break;
	case KERNEL_LOG_THREAD_EVENT_EXIT:
		SEGGER_SYSVIEW_OnTaskTerminate(thread_id);
		break;
	}

	/* FIXME: Maybe we need a KERNEL_LOG_THREAD_EVENT_CREATE? */
#endif
}

static void send_system_desc(void)
{
	SEGGER_SYSVIEW_SendSysDesc("N=LoRaIoT");
	SEGGER_SYSVIEW_SendSysDesc("D=" CONFIG_BOARD " " CONFIG_SOC " " CONFIG_ARCH);
	SEGGER_SYSVIEW_SendSysDesc("O=Zephyr");
	SEGGER_SYSVIEW_SendSysDesc("I#16=Power Clock");
	SEGGER_SYSVIEW_SendSysDesc("I#18=UART0");
	SEGGER_SYSVIEW_SendSysDesc("I#22=GPIOTE");
	SEGGER_SYSVIEW_SendSysDesc("I#33=RTC1");
	SEGGER_SYSVIEW_SendSysDesc("I#35=LPCOMP");

}

static void sysview_api_send_task_list(void)
{
	struct k_thread *thr;

	struct sysview_thread* svth;

	for (thr = _kernel.threads; thr; thr = thr->next_thread) {
		char name[SYSVIEW_THREAD_NAME_MAX_LEN];

		snprintk(name, sizeof(name), "T%xE%x", (uintptr_t)thr,
								 (uintptr_t)thr->entry);

		for(svth=sysview_threads_name; svth<sysview_threads_name+SYSVIEW_MAX_THREAD_NB; svth++){
			if(svth->id == thr){
				snprintk(name, sizeof(name), "%s", svth->name);
				break;
			}
		}

		/* NOTE: struct k_thread is inside the stack on Zephyr 1.7.
		 * This is not guaranteed by the API, and is likely to change
		 * in the future.  Hence, StackBase/StackSize are not set here;
		 * these could be stored as part of the kernel event.
		 */
		SEGGER_SYSVIEW_SendTaskInfo(&(SEGGER_SYSVIEW_TASKINFO) {
			.TaskID = (u32_t)(uintptr_t)thr,
			.sName = name,
			.Prio = thr->base.prio,
		});
	}
}

static u32_t zephyr_to_sysview(int event_type)
{
	static const u32_t lut[] = {
		[KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID] =
			SYSVIEW_EVTMASK_TASK_START_EXEC,
		[KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID] =
			SYSVIEW_EVTMASK_ISR_ENTER |
			SYSVIEW_EVTMASK_ISR_EXIT,
		[KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID] =
			SYSVIEW_EVTMASK_IDLE,
		[KERNEL_EVENT_LOGGER_THREAD_EVENT_ID] =
			SYSVIEW_EVTMASK_TASK_CREATE |
			SYSVIEW_EVTMASK_TASK_START_READY |
			SYSVIEW_EVTMASK_TASK_STOP_READY |
			SYSVIEW_EVTMASK_TASK_STOP_EXEC,
	};

	return lut[event_type];
}

#define MUST_LOG(event) \
	(IS_ENABLED(CONFIG_ ## event) && \
	sys_k_must_log_event(event ## _EVENT_ID))

#define ENABLE_SYSVIEW_EVENT(event) \
	(MUST_LOG(event) ? zephyr_to_sysview(event ## _EVENT_ID) : 0)

static void sysview_setup(void)
{
	static const SEGGER_SYSVIEW_OS_API api = {
		.pfGetTime = NULL, /* sysview_get_timestamp() used instead */
		.pfSendTaskList = sysview_api_send_task_list,
	};
	u32_t evs;

	printk("RTT block address is %p\n", &_SEGGER_RTT);


	/* NOTE: SysView does not support dynamic frequency scaling */
	SEGGER_SYSVIEW_Init(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			    CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			    &api, send_system_desc);

#if defined(CONFIG_PHYS_RAM_ADDR)	/* x86 */
	SEGGER_SYSVIEW_SetRAMBase(CONFIG_PHYS_RAM_ADDR);
#elif defined(CONFIG_SRAM_BASE_ADDRESS)	/* arm, default */
	SEGGER_SYSVIEW_SetRAMBase(CONFIG_SRAM_BASE_ADDRESS);
#else
	/* Setting RAMBase is just an optimization: this value is subtracted
	 * from all pointers in order to save bandwidth.  It's not an error
	 * if a platform does not set this value.
	 */
#endif

	evs = SYSVIEW_EVTMASK_PRINT_FORMATTED |
		ENABLE_SYSVIEW_EVENT(KERNEL_EVENT_LOGGER_SLEEP) |
		ENABLE_SYSVIEW_EVENT(KERNEL_EVENT_LOGGER_CONTEXT_SWITCH) |
		ENABLE_SYSVIEW_EVENT(KERNEL_EVENT_LOGGER_INTERRUPT) |
		ENABLE_SYSVIEW_EVENT(KERNEL_EVENT_LOGGER_THREAD);
	SEGGER_SYSVIEW_EnableEvents(evs);


}

#undef ENABLE_SYSVIEW_EVENT
#undef MUST_LOG

#if CONFIG_KERNEL_EVENT_LOGGER
static void sysview_thread(void)
{
	sysview_setup();

	for (;;) {
		u32_t event_data[4];
		u16_t event_id;
		u8_t dropped;
		u8_t event_data_size = (u8_t)ARRAY_SIZE(event_data);
		int ret;

		ret = sys_k_event_logger_get_wait(&event_id,
						  &dropped,
						  event_data,
						  &event_data_size);
		if (ret < 0) {
			continue;
		}

		timestamp = event_data[0];

		switch (event_id) {
		case KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID:
			publish_context_switch(event_data);
			break;
		case KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID:
			publish_interrupt(event_data);
			break;
		case KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID:
			publish_sleep(event_data);
			break;
		case KERNEL_EVENT_LOGGER_THREAD_EVENT_ID:
			publish_task(event_data);
			break;
		}
	}
}

K_THREAD_DEFINE(sysview_thread_id, SYSVIEW_THREAD_STACK_SIZE, sysview_thread, NULL, NULL, NULL,
		SYSVIEW_THREAD_PRIORITY, 0, K_NO_WAIT);


void sysview_init()
{
	sysview_threads_name[0].id = sysview_thread_id;
	snprintk(sysview_threads_name[0].name, SYSVIEW_THREAD_NAME_MAX_LEN, "SysView");
	threads_name_nb = 1;
	//SEGGER_SYSVIEW_Start();
}
#endif

void sysview_addTaskName(k_tid_t id, const char* name)
{
	sysview_threads_name[threads_name_nb].id = id;
	snprintk(sysview_threads_name[threads_name_nb].name, SYSVIEW_THREAD_NAME_MAX_LEN, "%s", name);
	threads_name_nb++;
}

