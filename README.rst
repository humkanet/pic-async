Asynchronous event loop for **PIC enhanced mid-range MCU** (8-bit)


============
Features
============
- Running multiple tasks
- Asynchronous sleep
- Await task (or tasks) finnished
- Synchronization primitives:

  - Event

============
Usage
============

.. highlight:: c

Quick start:
::
	#include "loop.h"

	void task1()
	{
		// Run task forever
		for(;;){
			// ... code ...
			asm("nop");
			// Sleep 1000 milliseconds
			loop_sleep(1000);
		}
	}

	void task2()
	{
		// All task variables must be static!
		statig uint8_t n;
		// Loop 3 times
		for(n=0; n<3; n++){
			loop_sleep(123);
		}
		// Use loop_task_finish() to mark task as finished
		loop_task_finish();
	}

	...
	
	void main()
	{
		...
		// Initialize event loop
		loop_init();
		// Add tasks to event loop
		loop_task_start(task1);
		loop_task_start(task2);

		// Main loop
		uint16_t  msec = 0;
		for(;;){
			// Process event loop with current time
			loop_tick(msec++);
		}
	}

Awaiting tasks:
::
	void task1()
	{
		loop_sleep(100);
		loop_task_finish();
	}

	void task2()
	{
		loop_sleep(500);
		loop_task_finish();
	}

	void task3()
	{
		// Start task1
		loop_task_start(task1);
		// Wait task finished
		loop_await(task1);
		// Start both tasks
		loop_task_start(task1);
		loop_task_start(task2);
		// Wait both tasks finished
		loop_await(task1);
		loop_await(task2);
		// Mark current task as finished
		loop_task_finish();
	}

Wait pin changed
::
	void task_wait_pin()
	{
		...
		// Wait pin RA0 changed to "1"
		while(RA0!=1){
			// Return control to event loop
			loop_return();
		}
		...
	}


Synchronization by Event
::
	LOOP_EVENT  event;

	voit task1()
	{
		loop_wait(&event);
		loop_task_finish();
	}

	void task2()
	{
		loop_wait(&event);
		loop_task_finish();
	}

	void task3()
	{
		// Clear event
		event.flag = 0;
		// Start tasks
		loop_task_start(task1);
		loop_task_start(task2);
		// Small delay
		loop_sleep(100);
		// Raise event
		event.flag = 1;
		// Wait task finished
		loop_await(task1);
		loop_await(task2);
		loop_task_finish();
	}
