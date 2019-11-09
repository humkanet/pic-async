#include <xc.h>
#include "main.h"
#include "config.h"
#include "loop.h"


static LOOP_EVENT  event;
static LOOP_MUTEX  mutex;


inline void setup()
{
	// Power off all modules except, SYSCLK
	PMD0 = 0x7F;
	PMD1 = 0xFF;
	PMD2 = 0xFF;
	PMD3 = 0xFF;
	PMD4 = 0xFF;
	PMD5 = 0xFF;
	// Cant start with external XTAL, fallback to internak 8MHz
	if ((OSCCON2&0x70)!=0x70){
		OSCCON1 = 0x60;
		OSCFRQ  = 0x04;
	}
	// Sleep mode - IDLE
	IDLEN = 1;
}


/* --- Await example --- */
void task_await1()
{
	// ... code ...
	loop_sleep(100);
}
void task_await2()
{
	// ... code ...
	loop_sleep(200);
}
void task_await()
{
	// start tasks
	loop_task_start(task_await1);
	loop_task_start(task_await2);
	// wait tasks finished
	loop_await(task_await1);
	loop_await(task_await2);
}
/* -- End await example --- */

/* --- Event example ---*/
void task_event1()
{
	// wait event
	loop_wait(&event);
}
void task_event2()
{
	// wait event
	loop_wait(&event);
}
void task_event()
{
	// clear event
	event.flag = 0;
	// start tasks
	loop_task_start(task_event1);
	loop_task_start(task_event2);
	// ... code ...
	loop_sleep(100);
	// raise event
	event.flag = 1;
	// wait task finished
	loop_await(task_event1);
	loop_await(task_event2);
}
/* --- Event example ---*/

/* --- Mutex example --- */
void task_mutex1()
{
	static uint8_t n;
	for(n=0; n<3; n++){
		// acquire exclusive access
		loop_acquire(&mutex);
		// ... code ...
		loop_sleep(100);
		// release mutex
		mutex.lock = 0;
	}
}
void task_mutex2()
{
	static uint8_t n;
	for(n=0; n<3; n++){
		// acquire exclusive access
		loop_acquire(&mutex);
		// ... code ...
		loop_sleep(100);
		// release mutex
		mutex.lock = 0;
	}
}
void task_mutex()
{
	// clear mutex
	mutex.lock = 0;
	// start tasks
	loop_task_start(task_mutex1);
	loop_task_start(task_mutex2);
	// wait task finished
	loop_await(task_mutex1);
	loop_await(task_mutex2);
}
/* --- End mutex example --- */

/*--- Forever loop example --- */
void task_loop_forever()
{
	// loop forewer
	for(;;){
		// ... code ...
		asm("nop");
		// Async sleep 100 msec
		loop_sleep(100);
		// ... code ...
		asm("nop");
		// Async sleep 200 msec
		loop_sleep(200);
		// ... code ...
		asm("nop");
		// Async sleep 50 msec
		loop_sleep(50);
	}
}
/* --- End forewer loop example --- */

/* --- Loopt N-times example --- */
void task_loop()
{
	// !!! All variables must be static !!!
	static uint8_t n;
	// Loop 3 times
	for(n=0; n<3; n++){
		loop_sleep(250);
	}
}
/* --- ELoopt N-times example --- */

/* --- Run examples task --- */
void task_examples()
{
	/* 1. Run forever loop task */
	loop_task_start(task_loop_forever);
	/* Loop current task forever */
	for(;;){
		/* 2. Run loop N-times task and wait finished */
		loop_task_start(task_loop);
		loop_await(task_loop);
		/* 3. Run await multiple tasks example */
		loop_task_start(task_await);
		loop_await(task_await);
		/* 4. Run event example */
		loop_task_start(task_event);
		loop_await(task_event);
		/* 5. Run mutex example */
		loop_task_start(task_mutex);
		loop_await(task_mutex);
	}
}
/* --- End run examples task --- */


void main()
{
	di();
	setup();
	ei();

	// Initialize and start examples task
	loop_init();
	loop_task_start(task_examples);

	// Main loop
	uint16_t  msec = 0;
	for(;;){
		loop_tick(msec++);
	}
}
