#include <xc.h>
#include "main.h"
#include "config.h"
#include "loop.h"


static LOOP_EVENT  evt;


inline void setup()
{
	// Отключаем питание со всех модулей (кроме SYSCLK)
	PMD0 = 0x7F;
	PMD1 = 0xFF;
	PMD2 = 0xFF;
	PMD3 = 0xFF;
	PMD4 = 0xFF;
	PMD5 = 0xFF;
	// Если не удалость запуститься с внешнего кварца, то переходим на внутренний 8МГц
	if ((OSCCON2&0x70)!=0x70){
		OSCCON1 = 0x60;
		OSCFRQ  = 0x04;
	}
	// Режим сна - IDLE
	IDLEN  = 1;
	// Разрешаем прерывания от периферии
	PEIE   = 1;
}


void task1()
{
	for(;;){
		// ... my cool code ...
		asm("nop");
		// Async sleep 1000 msec
		loop_sleep(1000);
		// ... my cool code ...
		asm("nop");
		// Async sleep 2000 msec
		loop_sleep(2000);
		// ... my cool code ...
		asm("nop");
		// Async sleep 500 msec
		loop_sleep(500);
	}
}


void task3()
{
	// !!! All variables must be static !!!
	static uint8_t n;
	// Iterate 3 times
	for(n=0; n<3; n++){
		loop_sleep(250);
	}
	// Mark task as finished
	loop_task_finish();
}


void task4()
{
	static uint8_t n;
	for(n=0; n<5; n++){
		loop_sleep(250);
	}
	loop_task_finish();
}


void task5()
{
	loop_sleep(500);
	evt.flag = 1;
	loop_sleep(500);
	loop_task_finish();
}

void task6()
{
	loop_wait(&evt);
	loop_task_finish();
}


void task2()
{
	for(;;){
		loop_sleep(100);
		loop_sleep(200);
		loop_sleep(300);
		// Start task3
		loop_task_start(task3);
		// Wait task finished
		loop_await(task3);
		// Start two tasks: task3 and task4
		loop_task_start(task3);
		loop_task_start(task4);
		// Wait both tasks finished
		loop_await(task3);
		loop_await(task4);
		// Clear event and start task5 that raise event, task6 wait event
		evt.flag = 0;
		loop_task_start(task5);
		loop_task_start(task6);
		// Wait event raised and task5 finished
		loop_wait(&evt);
		loop_await(task5);
		loop_await(task6);
	}
}


void main()
{
	di();
	setup();
	ei();

	// Initialize and add tasks to event loop
	loop_init();
	loop_task_start(task1);
	loop_task_start(task2);

	// Main loop
	uint16_t  msec;
	for(;;){
		loop_tick(msec++);
	}
}
