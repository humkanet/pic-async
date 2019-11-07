#ifndef _CLOCK_H
#define	_CLOCK_H

/*
	СИСТЕМНЫЕ ЧАСЫ

	Системные часы ведут отсчет времени (секунды и миллисекунды) от момента
	инициализации МК. В любой момент времени можно получить значение
	системных часов вызвав методы:
		clock_msec() - для получения миллисекунд
		clock_sec()  - для получения секунд

	Системные часы изпользуют Timer0 для отсчета времени. Timer0 работает
	в 8-битном режиме и тактируется от FOSC/4

	Системные часы начинают отсчет с момента вызова метода clock_init().
	Для корректной работа системных часов необходимо добавить обработчик
	прерываний clock_isr() в основной обработчик программы:

	main.c:

		include "clock.h"
		...
		void __interrupt() isr()
		{
			...
			clock_isr();
			...
		}
*/


#include <stdint.h>


#define CLOCK_TICK_MSEC  4


inline void      clock_isr(void);
inline void      clock_init(void);
inline uint16_t  clock_msec(void);
inline uint16_t  clock_sec(void);
void             clock_delay(uint16_t msec);


#endif
