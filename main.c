#include <xc.h>
#include "main.h"
#include "config.h"
#include "loop.h"
#include "clock.h"


void __interrupt() isr()
{
	clock_isr();
}


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
	// Инициализация периферии
	clock_init();
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
	}
}


void main()
{
	di();
	setup();
	ei();

	// Инициализируем и добавляем задачи в очередь
	loop_init();
	loop_task_start(task1);
	loop_task_start(task2);

	// Основной цикл
	for(;;){
		// Считываем системные часы
		uint16_t msec = clock_msec();
		// Обрабатываем очередь
		loop_tick(msec);
		// Спим до след. события
		SLEEP();
	}
}
