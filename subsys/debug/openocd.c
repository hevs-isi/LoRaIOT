/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>

#define OPENOCD_UNIMPLEMENTED	0xffffffff

#if defined(CONFIG_OPENOCD_SUPPORT) && defined(CONFIG_THREAD_MONITOR)
enum {
	OPENOCD_OFFSET_VERSION,
	OPENOCD_OFFSET_K_CURR_THREAD,
	OPENOCD_OFFSET_K_THREADS,
	OPENOCD_OFFSET_T_ENTRY,
	OPENOCD_OFFSET_T_NEXT_THREAD,
	OPENOCD_OFFSET_T_STATE,
	OPENOCD_OFFSET_T_USER_OPTIONS,
	OPENOCD_OFFSET_T_PRIO,
	OPENOCD_OFFSET_T_STACK_PTR,
	OPENOCD_OFFSET_T_NAME,
	OPENOCD_OFFSET_T_ARCH,
	OPENOCD_OFFSET_T_PREEMPT_FLOAT,
	OPENOCD_OFFSET_T_COOP_FLOAT,
};

/* Forward-compatibility notes: 1) Increment OPENOCD_OFFSET_VERSION element
 * each time an offset is added to this table.  2) Only append items to this
 * table; otherwise, OpenOCD versions that expects version 0 will read garbage
 * values.
 */
__attribute__((used, section(".openocd_dbg")))
size_t _kernel_openocd_offsets[] = {
	/* Version 0 starts */
	[OPENOCD_OFFSET_VERSION] = 1,
	[OPENOCD_OFFSET_K_CURR_THREAD] = offsetof(struct z_kernel, current),
	[OPENOCD_OFFSET_K_THREADS] = offsetof(struct z_kernel, threads),
	[OPENOCD_OFFSET_T_ENTRY] = offsetof(struct k_thread, entry),
	[OPENOCD_OFFSET_T_NEXT_THREAD] = offsetof(struct k_thread, next_thread),
	[OPENOCD_OFFSET_T_STATE] = offsetof(struct _thread_base, thread_state),
	[OPENOCD_OFFSET_T_USER_OPTIONS] = offsetof(struct _thread_base,
						   user_options),
	[OPENOCD_OFFSET_T_PRIO] = offsetof(struct _thread_base, prio),
#if defined(CONFIG_ARM)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.psp),
#elif defined(CONFIG_ARC)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_X86)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.esp),
#elif defined(CONFIG_NIOS2)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_RISCV32)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#else
	/* Use a special value so that OpenOCD knows that obtaining the stack
	 * pointer is not possible on this particular architecture.
	 */
#warning Please define OPENOCD_OFFSET_T_STACK_PTR for this architecture
	[OPENOCD_OFFSET_T_STACK_PTR] = OPENOCD_UNIMPLEMENTED,
#endif
	/* Version 0 ends */

	[OPENOCD_OFFSET_T_NAME] = offsetof(struct k_thread, name),
	[OPENOCD_OFFSET_T_ARCH] = offsetof(struct k_thread, arch),
#if defined(CONFIG_FLOAT) && defined(CONFIG_ARM)
	[OPENOCD_OFFSET_T_PREEMPT_FLOAT] = offsetof(struct _thread_arch,
						    preempt_float),
	[OPENOCD_OFFSET_T_COOP_FLOAT] = OPENOCD_UNIMPLEMENTED,
#elif defined(CONFIG_FLOAT) && defined(CONFIG_X86)
	[OPENOCD_OFFSET_T_PREEMPT_FLOAT] = offsetof(struct _thread_arch,
						    preempFloatReg),
	[OPENOCD_OFFSET_T_COOP_FLOAT] = offsetof(struct _thread_arch,
						 coopFloatReg),
#else
	[OPENOCD_OFFSET_T_PREEMPT_FLOAT] = OPENOCD_UNIMPLEMENTED,
	[OPENOCD_OFFSET_T_COOP_FLOAT] = OPENOCD_UNIMPLEMENTED,
#endif
};

__attribute__((used, section(".openocd_dbg")))
u8_t _kernel_openocd_size_t_size = (u8_t)sizeof(size_t);
#endif
