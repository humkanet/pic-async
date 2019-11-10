#include <xc.h>
#include "loop.h"


#define U16(x)  ((uint16_t) (x))


typedef union {
	void        *ptr;
	uint16_t    u8;
	uint16_t    u16;
	LOOP_TASK   pc;
} TASK_DATA;


typedef struct {
	LOOP_TASK         entry;       // Task entry point
	TASK_DATA         data;        // Task data
	LOOP_TASK_STATUS  status;      // Task status (running, finished, ...)
	LOOP_WAITER       pc;          // Task resume address
	uint8_t           context[6];  // MCU context (registers)
} TASK;


struct {
	uint16_t  msec;
	uint16_t  dmsec;
	uint8_t   finish : 1;
	TASK      *task;
	TASK      tasks[LOOP_MAX_TASKS];
} loop;


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
			task->entry   = entry;
			task->pc      = (LOOP_WAITER) entry;
			task->status  = LOOP_TASK_STATUS_RUN;
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


void loop_tick(uint16_t msec)
{
	loop.dmsec = U16(msec-loop.msec);
	loop.msec  = msec;
	// Loop over all tasks
	loop.task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, loop.task++){
		if (loop.task->status==LOOP_TASK_STATUS_RUN){
			loop.finish = 0;
			loop.task->pc();
			asm("LOOP_RESUME:");
			if (loop.finish){
				loop.task->status = LOOP_TASK_STATUS_FINISHED;
			}
		}
	}
}


void __loop_patch_context()
{
	/*
		Stack before:
			[STKPTR]   = loop_<wait function> resume PC
			[STKPTR-1] = task resume address
			[STKPTR-2] = loop_tick resume address
		Stack after:
			STKPTR     = STKPTR - 2
		Save task context:
			1. Save registers: WREG, BSR, FSR1
			2. Save INTCON
			3. Disable interrupts (GIE=0)
			4. Move current resume address to task->pc
			5. Save task resume address to task->context
			6. Enable interrupts and mark task as running
			7. Patch stack to event loop (LOOP_RESUME)
	*/
	uint8_t  intcon;
	FSR0 = (uint16_t) &loop.task->context;
	// Save registers
	asm("movwi     0[FSR0]");
	asm("movf      BSR, W");
	asm("movwi     1[FSR0]");
	asm("movf      FSR1H, W");
	asm("movwi     2[FSR0]");
	asm("movf      FSR1L, W");
	asm("movwi     3[FSR0]");
	// Save INTCON and disable interrupts
	intcon = INTCON;
	GIE    = 0;
	// Move current return PC to task->pc
	asm("banksel   TOSL");
	asm("movf      TOSL & 0x7F, W");
	asm("movwi     -2[FSR0]");
	asm("movf      TOSH & 0x7F, W");
	asm("movwi     -1[FSR0]");
	// Save task return PC and remove from stack
	asm("decf      STKPTR & 0x7F");
	asm("movf      TOSL & 0x7F, W");
	asm("movwi     4[FSR0]");
	asm("movf      TOSH & 0x7F, W");
	asm("movwi     5[FSR0]");
	// Return to event loop
	asm("decf      STKPTR & 0x7F");
	// Restore interrupts
	INTCON |= intcon & _INTCON_GIE_MASK;
	// Mark task as runned
	loop.finish = 0;
}


void __loop_restore_context()
{
	uint8_t  intcon;
	/*
		Restore task:
			1. Restore registers: BSR, FSR1
			2. Save INTCON and disable interrupts
			3. Restore task resume address
			4. Restore interrupts
			5. Mark task to finish after returning to event loop
			6. Restore WREG

	*/
	FSR0 = (uint16_t) &loop.task->context;
	// Restore context
	asm("moviw     1[FSR0]");
	asm("movwf     BSR");
	asm("moviw     2[FSR0]");
	asm("movwf     FSR1H");
	asm("moviw     3[FSR0]");
	asm("movwf     FSR1L");
	// Save and disable interrupts
	intcon = INTCON;
	GIE    = 0;
	// Restore stack
	asm("banksel   STKPTR");
	asm("moviw     4[FSR0]");
	asm("movwf     TOSL & 0x7F");
	asm("moviw     5[FSR0]");
	asm("movwf     TOSH & 0x7F");
	// Restore interrupts
	INTCON |= intcon & _INTCON_GIE_MASK;
	// Mark task as finished
	loop.finish = 1;
	// Restore WREG
	asm("moviw     0[FSR0]");
}


LOOP_TASK_STATUS loop_task_status(const LOOP_TASK entry)
{
	// Find task by entry point
	TASK  *task = loop.tasks;
	for(uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		if (task->entry==entry) return task->status;
	}
	// Task not found, return as finished
	return LOOP_TASK_STATUS_FINISHED;
}


void loop_return()
{
	// Patch context
	__loop_patch_context();
	// Continue
	__loop_restore_context();
}


void loop_sleep(uint16_t msec)
{
	// Calculate expire time
	loop.task->data.u16  = msec;
	// Patch context
	__loop_patch_context();
	// If timeout expired resume task
	FSR0 = (uint16_t) &loop.task->data;
	// Substract dmsec from sleep msec
	asm("banksel   _loop");
	asm("movf      (_loop+2), W");  // WREG = low(dmsec)
	asm("subwf     INDF0, F");      // low(msec) = low(msec) - WREG
	asm("moviw     FSR0++");        // FSR0 = FSR0 + 1
	asm("movf      (_loop+3), W");  // WREG = high(dmsec)
	asm("subwfb    INDF0, F");      // high(msec) = high(msec) - WREG - CARRY
	asm("btfss     STATUS, 0");     // Skip if CARRY
	asm("call     ___loop_restore_context");
}


void loop_await(const LOOP_TASK entry)
{
	// Save awaiting task entry point
	loop.task->data.pc = entry;
	// Patch context
	__loop_patch_context();
	// If awaiting task finished, resume task
	if (loop_task_status(loop.task->data.pc)==LOOP_TASK_STATUS_FINISHED){
		__loop_restore_context();
	}
}


void loop_wait(LOOP_EVENT *event)
{
	// Save event address
	loop.task->data.ptr = event;
	// Patch context
	__loop_patch_context();
	// If awaiting task finished, resume task
	if (((LOOP_EVENT*)loop.task->data.ptr)->flag){
		__loop_restore_context();
	}
}


void loop_acquire(LOOP_MUTEX *mutex)
{
	// Save event address
	loop.task->data.ptr = mutex;
	// If awaiting task finished, resume task
	mutex = loop.task->data.ptr;
	if (!mutex->lock){
		// Release mutex
		mutex->lock = 1;
		__loop_restore_context();
	}
}
