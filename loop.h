#ifndef _LOOP_H
#define _LOOP_H

#include <stdint.h>


#define LOOP_MAX_TASKS  5


#if !defined(_PIC14EX) && !defined(_PIC14E)
	#error Not an Enhanced Midrange PIC!
#endif


typedef void (*LOOP_TASK)(void);
typedef void (*LOOP_WAITER)(void);


typedef enum {
	LOOP_TASK_STATUS_RUN,
	LOOP_TASK_STATUS_FINISHED
} LOOP_TASK_STATUS;


typedef struct {
	uint8_t  flag : 1;
} LOOP_EVENT;


typedef struct {
	uint8_t  lock : 1;
} LOOP_MUTEX;


void  loop_init(void);
void  loop_tick(uint16_t msec);
void  loop_task_start(const LOOP_TASK entry);
void  loop_task_cancel(const LOOP_TASK entry);
LOOP_TASK_STATUS  loop_task_status(const LOOP_TASK entry);


/* Sleep *msec* interval */
void  loop_sleep(uint16_t msec);

/* Wait *event* raised */
void  loop_wait(LOOP_EVENT *event);

/* Acquire exclusive access */
void  loop_acquire(LOOP_MUTEX *mutex);

/* Wait task with entry point *pc* finished */
void  loop_await(const LOOP_TASK pc);

/* Return contorl to event loop */
void  loop_return(void);


#endif
