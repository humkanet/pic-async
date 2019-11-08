#include <xc.h>
#include "loop.h"


typedef struct {
	LOOP_TASK         entry;       // Task entry point
	LOOP_TASK         pc;          // Task resume address
	LOOP_TASK_STATUS  status;      // Task status (running, finished, ...)
	union {
		const uint8_t *ptr;        // Task local data
		uint8_t       data[2];
	};
	uint8_t           context[6];  // MCU context (registers)
} TASK;


typedef struct {
	uint16_t  until;
} SLEEP;


typedef struct {
	LOOP_TASK  entry;
} AWAIT;


typedef struct {
	LOOP_EVENT  *event;
} EVENT;


typedef struct {
	uint8_t  lock;
} MUTEX;


__persistent struct {
	uint16_t  msec;
	TASK      *task;
	TASK      tasks[LOOP_MAX_TASKS];
} loop;


__persistent static uint8_t __loop_int_mask;


void loop_init()
{
	// Mark all tasks as finished
	TASK  *task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		task->status = LOOP_TASK_STATUS_FINISHED;
	}
}


void loop_task_start(const LOOP_TASK entry)
{
	TASK  *task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		// Task already started, return
		if ((task->entry==entry)&&(task->status!=LOOP_TASK_STATUS_FINISHED)) break;
		// Find first empty slot and save task to it
		if (task->status==LOOP_TASK_STATUS_FINISHED){
			task->pc     = entry;
			task->entry  = entry;
			task->status = LOOP_TASK_STATUS_RUN;
			break;
		}
	}
}


void loop_task_cancel(const LOOP_TASK entry)
{
	TASK  *task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		// Find task by entry point and mark finished
		if (task->entry==entry){
			task->status = LOOP_TASK_STATUS_FINISHED;
			break;
		}
	}
}


void loop_task_finish()
{
	// Mark task as finished
	loop.task->status = LOOP_TASK_STATUS_FINISHED;
}


void loop_tick(uint16_t msec)
{
	// Save system clock
	loop.msec = msec;
	// Loop over all tasks
	loop.task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, loop.task++){
		switch (loop.task->status){
			case LOOP_TASK_STATUS_RUN:
				loop.task->pc();
				asm("LOOP_RESUME:");
				break;
			default:
				break;
		}
	}
}


void __loop_save_context()
{
	/* !!! FSR0 must be point to task context !!! */
	// Save registers
	asm("movwi     0[FSR0]");
	asm("movf      BSR, W");
	asm("movwi     1[FSR0]");
	asm("movf      FSR1H, W");
	asm("movwi     2[FSR0]");
	asm("movf      FSR1L, W");
	asm("movwi     3[FSR0]");
	// Save and disable interrupts
	asm("movf      INTCON, W");
	asm("bcf       INTCON, 7");
	asm("andlw     0x80");
	asm("movwf     ___loop_int_mask");
	// Save stack
	asm("banksel   TOSL");
	asm("decf      STKPTR & 0x7F");
	asm("movf      TOSL & 0x7F, W");
	asm("movwi     4[FSR0]");
	asm("movf      TOSH & 0x7F, W");
	asm("movwi     5[FSR0]");
	// Change stack to loop
	asm("movlw     low(LOOP_RESUME)");
	asm("movwf     TOSL & 0x7F");
	asm("movlw     high(LOOP_RESUME)");
	asm("movwf     TOSH & 0x7F");
	asm("incf      STKPTR & 0x7F");
	// Restore interrupts
	asm("movf      ___loop_int_mask, W");
	asm("iorwf     INTCON");
}


void __loop_restore_context()
{
	/* !!! FSR0 must be point to task context !!! */
	asm("moviw     1[FSR0]");
	asm("movwf     BSR");
	asm("moviw     2[FSR0]");
	asm("movwf     FSR1H");
	asm("moviw     3[FSR0]");
	asm("movwf     FSR1L");
	asm("moviw     0[FSR0]");
}


void __loop_restore_sp()
{
	/* !!! FSR0 must be point to task context !!! */
	// Save and disable interrupts
	asm("movf      INTCON, W");
	asm("bcf       INTCON, 7");
	asm("andlw     0x80");
	asm("movwf     ___loop_int_mask");
	// Restore stack
	asm("banksel   STKPTR");
	asm("moviw     4[FSR0]");
	asm("movwf     TOSL & 0x7F");
	asm("moviw     5[FSR0]");
	asm("movwf     TOSH & 0x7F");
	// Restore interrupts
	asm("movf      ___loop_int_mask, W");
	asm("iorwf     INTCON");
}


LOOP_TASK_STATUS loop_task_status(const LOOP_TASK entry)
{
	TASK  *task = loop.tasks;
	// Find task by entry point
	for(uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		if (task->entry==entry) return task->status;
	}
	// Task not found, return as finished
	return LOOP_TASK_STATUS_FINISHED;
}


void loop_return()
{
	// Save context
	FSR0 = (uint16_t) &loop.task->context;
	__loop_save_context();
	// Patch stack
	FSR0 = (uint16_t) &loop.task->pc;
	asm("movlw     low(CONTINUE_RESUME)");
	asm("movwi     0[FSR0]");
	asm("movlw     high(CONTINUE_RESUME)");
	asm("movwi     1[FSR0]");
	// Return to loop
	asm("return");
	// Restore stack and resume task
	asm("CONTINUE_RESUME:");
	FSR0 = (uint16_t) &loop.task->context;
	__loop_restore_context();
	__loop_restore_sp();
}


void loop_sleep(uint16_t msec)
{
	// Calculate expire time
	((SLEEP*)&loop.task->data)->until = loop.msec+msec;
	// Save context
	FSR0 = (uint16_t) &loop.task->context;
	__loop_save_context();
	// Patch stack
	FSR0 = (uint16_t) &loop.task->pc;
	asm("movlw     low(SLEEP_RESUME)");
	asm("movwi     0[FSR0]");
	asm("movlw     high(SLEEP_RESUME)");
	asm("movwi     1[FSR0]");
	asm("return");
	// Restore context
	asm("SLEEP_RESUME:");
	FSR0 = (uint16_t) &loop.task->context;
	__loop_restore_context();
	// If timeout expired resume task
	if (loop.msec>=((SLEEP*)&loop.task->data)->until){
		__loop_restore_sp();
	}
}


void loop_await(const LOOP_TASK entry)
{
	// Save awaiting task entry point
	((AWAIT*)&loop.task->data)->entry = entry;
	// Save context
	FSR0 = (uint16_t) &loop.task->context;
	__loop_save_context();
	// Patch stack
	FSR0 = (uint16_t) &loop.task->pc;
	asm("movlw     low(AWAIT_RESUME)");
	asm("movwi     0[FSR0]");
	asm("movlw     high(AWAIT_RESUME)");
	asm("movwi     1[FSR0]");
	asm("return");
	// Restore context
	asm("AWAIT_RESUME:");
	FSR0 = (uint16_t) &loop.task->context;
	__loop_restore_context();
	// If awaiting task finished, resume task
	if (loop_task_status(((AWAIT*)&loop.task->data)->entry)==LOOP_TASK_STATUS_FINISHED){
		__loop_restore_sp();
	}
}


void loop_wait(LOOP_EVENT *event)
{
	// Save event address
	((EVENT*)&loop.task->data)->event = event;
	// Save context
	FSR0 = (uint16_t) &loop.task->context;
	__loop_save_context();
	// Patch stack
	FSR0 = (uint16_t) &loop.task->pc;
	asm("movlw     low(EVENT_RESUME)");
	asm("movwi     0[FSR0]");
	asm("movlw     high(EVENT_RESUME)");
	asm("movwi     1[FSR0]");
	asm("return");
	// Restore context
	asm("EVENT_RESUME:");
	FSR0 = (uint16_t) &loop.task->context;
	__loop_restore_context();
	// If awaiting task finished, resume task
	if (((EVENT*)&loop.task->data)->event->flag){
		__loop_restore_sp();
	}
}
