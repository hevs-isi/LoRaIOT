

#ifndef _SYSVIEW_H
#define _SYSVIEW_H

//#include <systemview/SEGGER_SYSVIEW.h>

#define SYSVIEW_THREAD_STACK_SIZE		1024
#define SYSVIEW_THREAD_PRIORITY			K_PRIO_PREEMPT(3)

#define SYSVIEW_MAX_THREAD_NB			6
#define SYSVIEW_THREAD_NAME_MAX_LEN		20

void sysview_init(void);
void sysview_addTaskName(k_tid_t id, const char* name);

#endif /* _SYSVIEW_H */
