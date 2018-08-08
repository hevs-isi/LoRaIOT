/* rtt_console.c - Console messages to a RAM buffer that is then read by
 * the Segger J-Link debugger
 */

/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel.h>
#include <misc/printk.h>
#include <device.h>
#include <init.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>
#include <logging/sys_log.h>
#include <console/console.h>

#include <string.h>

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

static bool host_present;

static struct k_fifo *avail_queue;
static struct k_fifo *lines_queue;

K_THREAD_STACK_DEFINE(rttterm_stack, 2048);
static struct k_thread rttterm_thread_data;


static void RTT_terminal()
{
	u32_t rx;
	u8_t buf[20];

	while(1){
		static struct console_input *cmd;
		if(SEGGER_RTT_HASDATA(0)){
			//memset(buf, 0, sizeof(buf));
			rx = SEGGER_RTT_Read(0, buf, 20);

			if (rx < 0) {
				continue;
			}

			if (!cmd) {
				cmd = k_fifo_get(avail_queue, K_NO_WAIT);
				if (!cmd) {
					continue;
				}
			}

			memcpy(cmd->line, buf, rx);
			cmd->line[rx] = '\0';

			k_fifo_put(lines_queue, cmd);

		}
		k_yield();
	}
}


static int rtt_console_out(int character)
{
	unsigned int key;
	char c = (char)character;

	key = irq_lock();
	SEGGER_RTT_WriteNoLock(0, &c, 1);
	irq_unlock(key);

	return character;
}

/** @brief Wait for fixed period.
 *
 */
static void wait(void)
{
	if (k_is_in_isr()) {
		if (IS_ENABLED(CONFIG_RTT_TX_RETRY_IN_INTERRUPT)) {
			k_busy_wait(1000*500);
		}
	} else {
		k_sleep(500);
	}
}

#if 0
static int rtt_console_out(int character)
{
	unsigned int key;
	char c = (char)character;
	unsigned int cnt;
	int max_cnt = 30;

	do {
		key = irq_lock();
		cnt = SEGGER_RTT_WriteNoLock(0, &c, 1);
		irq_unlock(key);

		/* There are two possible reasons for not writing any data to
		 * RTT:
		 * - The host is not connected and not reading the data.
		 * - The buffer got full and will be read by the host.
		 * These two situations are distinguished using the following
		 * algorithm:
		 * At the beginning, the module assumes that the host is active,
		 * so when no data is read, it busy waits and retries.
		 * If, after retrying, the host reads the data, the module
		 * assumes that the host is active. If it fails, the module
		 * assumes that the host is inactive and stores that
		 * information. On next call, only one attempt takes place.
		 * The host is marked as active if the attempt is successful.
		 */
		if (cnt) {
			/* byte processed - host is present. */
			host_present = true;
		} else if (host_present) {
			if (max_cnt) {
				wait();
				max_cnt--;
				continue;
			} else {
				host_present = false;
			}
		}

		break;
	} while (1);

	return character;
}
#endif

static int rtt_console_init(struct device *arg)
{
	ARG_UNUSED(arg);

	SEGGER_RTT_Init();

	__printk_hook_install(rtt_console_out);
	__stdout_hook_install(rtt_console_out);


	k_thread_create(&rttterm_thread_data, rttterm_stack,
				K_THREAD_STACK_SIZEOF(rttterm_stack),
				(k_thread_entry_t)RTT_terminal,
				NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, 0);



	SYS_LOG_INF("RTT console initialized\n");

	return 0;
}

void rtt_register_input(struct k_fifo *avail, struct k_fifo *lines,
			 u8_t (*completion)(char *str, u8_t len))
{
	ARG_UNUSED(completion);
	avail_queue = avail;
	lines_queue = lines;
}

/* RTT console */

SYS_INIT(rtt_console_init,
		POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);




