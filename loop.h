#ifndef _LOOP_H
#define _LOOP_H

#include <stdint.h>


#define LOOP_MAX_TASKS  5


typedef void (*LOOP_TASK)(void);


typedef enum {
	LOOP_TASK_STATUS_RUN,
	LOOP_TASK_STATUS_FINISHED
} LOOP_TASK_STATUS;


void  loop_init(void);
void  loop_tick(uint16_t msec);
void  loop_task_start(LOOP_TASK pc);
void  loop_task_cancel(LOOP_TASK pc);
void  loop_task_finish(void);
LOOP_TASK_STATUS  loop_task_status(LOOP_TASK pc);

void  loop_sleep(uint16_t msec);
void  loop_await(LOOP_TASK pc);
void  loop_continue(void);


#endif
