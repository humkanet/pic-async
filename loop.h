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
void  loop_task_start(const LOOP_TASK pc);
void  loop_task_cancel(const LOOP_TASK pc);
void  loop_task_finish(void);
LOOP_TASK_STATUS  loop_task_status(const LOOP_TASK pc);

/* Sleep *msec* interval */
void  loop_sleep(uint16_t msec);

/* Await task with entry point *pc* finished */
void  loop_await(const LOOP_TASK pc);

/* Return contorl to event loop */
void  loop_return(void);


#endif
