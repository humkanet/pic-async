#include <xc.h>
#include "clock.h"


#define U16(x) ((uint16_t) (x))


/*
	Таймер тактируется от FOSC/4, интервал в миллисекундах
	задан CLOCK_TICK_MSEC
	Таким образом, количество тактов для заданного интервала
	определяеся как FCPU*CLOCK_TICK_MSEC/4000
*/
#define TIMER_PERIOD  ((_XTAL_FREQ*CLOCK_TICK_MSEC/4000))


/*
	Таймер Timer0 8-битный, поэтому максимальное значение
	периода не может превышать 256. Так-же Timer0 умеет
	делить частоту тактирования на 2, 4, 8, 16, 32 ...
	Если значение периода получилось больше 256 пробуем
	найти делитель для тактирования таймера
*/
#if TIMER_PERIOD<=256
	#define TIMER_DIV   1
	#define TIMER_CKPS  0x00
#elif (TIMER_PERIOD/2)<=256
	#define TIMER_DIV   2
	#define TIMER_CKPS  0x01
#elif (TIMER_PERIOD/4)<=256
	#define TIMER_DIV   4
	#define TIMER_CKPS  0x02
#elif (TIMER_PERIOD/8)<=256
	#define TIMER_DIV   8
	#define TIMER_CKPS  0x03
#elif (TIMER_PERIOD/16)<=256
	#define TIMER_DIV   16
	#define TIMER_CKPS  0x04
#elif (TIMER_PERIOD/32)<=256
	#define TIMER_DIV   32
	#define TIMER_CKPS  0x05
#elif (TIMER_PERIOD/64)<=256
	#define TIMER_DIV   64
	#define TIMER_CKPS  0x06
#elif (TIMER_PERIOD/128)<=256
	#define TIMER_DIV   128
	#define TIMER_CKPS  0x07
#elif (TIMER_PERIOD/256)<=256
	#define TIMER_DIV   256
	#define TIMER_CKPS  0x08
#elif (TIMER_PERIOD/512)<=256
	#define TIMER_DIV   512
	#define TIMER_CKPS  0x09
#elif (TIMER_PERIOD/1024)<=256
	#define TIMER_DIV   1024
	#define TIMER_CKPS  0x0A
#elif (TIMER_PERIOD/2048)<=256
	#define TIMER_DIV   2048
	#define TIMER_CKPS  0x0B
#elif (TIMER_PERIOD/4096)<=256
	#define TIMER_DIV   4096
	#define TIMER_CKPS  0x0C
#elif (TIMER_PERIOD/8192)<=256
	#define TIMER_DIV   8192
	#define TIMER_CKPS  0x0D
#elif (TIMER_PERIOD/16384)<=256
	#define TIMER_DIV   16384
	#define TIMER_CKPS  0x0E
#elif (TIMER_PERIOD/32768)<=256
	#define TIMER_DIV   32768
	#define TIMER_CKPS  0x0F
#else
	#error "CLOCK_TICK_MSEC too LOW"
#endif


/*
	В одной секунде 1000 миллисекунд. Т.е. одна секунда
	пройдет тогда, когда произойдет 1000/CLOCK_TICK_MSEC
	интервалов таймера.
	Если остаток от деления 1000 на CLOCK_TICK_MSEC целое
	число, то отсчет можно чуть упростить, просто подсчитав
	количество периодов. Например, CLOCK_TICK_MSEC = 8мс,
	тогда секунда случится через 1000мс/8мс = 125 сработок
	таймера. Увеличивая переменную на 1 при каждой сработке
	таймера, секунда случится когда значение переменной
	станет равным 125:
		tmp ++;
		if (tmp>=125){
			seconds ++;
			tmp = 0;
		}
	Если же остаток от деления не равняются нулю, придется
	отсчитывать интервал в миллисекундах:
		tmp += CLOCK_TICK_MSEC;
		if (tmp>=1000){
			seconds ++;
			tmp -= 1000;
		}
*/
#define TMP  (1000/CLOCK_TICK_MSEC)
#if TMP*CLOCK_TICK_MSEC!=1000
	#define TMP_FULL_PROCESSING
	#define TMP_TYPE  uint16_t
	#undef  TMP
	#define TMP       1000
#elif TMP<256
	#define TMP_TYPE  uint8_t
#else
	#define TMP_TYPE  uint16_t
#endif


volatile struct {
	uint16_t  msec;
	uint16_t  sec;
	TMP_TYPE  tmp;
} clock = {
	.msec = 0,
	.sec  = 0,
	.tmp  = 0
};


inline void clock_isr()
{
	if (TMR0IF){
		// Сбрасываем флаг прерывания
		TMR0IF = 0;
		// Миллисекунды
		clock.msec += CLOCK_TICK_MSEC;
		// Секунды
		#ifdef TMP_FULL_PROCESSING
		clock.tmp  += CLOCK_TICK_MSEC;
		#else
		clock.tmp  ++;
		#endif
		if (clock.tmp>=TMP){
			clock.sec ++;
			#ifdef TMP_FULL_PROCESSING
			clock.tmp -= TMP;
			#else
			clock.tmp  = 0;
			#endif
		}
	}
}


inline void clock_init()
{
	// Подаем питанием на таймер
	TMR0MD = 0;
	asm("nop");
	// Настраиваем таймер
	T0CON0 = 0x00;
	T0CON1 = 0x50 | (TIMER_CKPS&0x0F);
	TMR0H  = (TIMER_PERIOD/TIMER_DIV)-1;
	TMR0L  = 0;
	// Настраиваем прерывания
	TMR0IF = 0;
	TMR0IE = 1;
	// Запускаем таймер
	T0EN   = 1;
}


/*
	Значение секунд и миллисекунд хранится в 16 битах,
	но сам МК 8-битный. Может случится ситуация, когда
	один байт значения был считан, но перед считыванием
	второго байта произошло прерывание и значение поменялось.
	Эта функция считывает 16-битное значение с проверкой
	на изменения
*/
uint16_t clock_read_u16(volatile uint16_t* ptr)
{
	uint16_t  val;
	do{
		val = *ptr;
	} while (val!=*ptr);
	// Возвращаем значение
	return val;
}


inline uint16_t clock_msec()
{
	return clock_read_u16(&clock.msec);
}


inline uint16_t clock_sec()
{
	return clock_read_u16(&clock.sec);
}


void clock_delay(uint16_t msec)
{
	uint16_t  ts = clock_msec();
	while (U16(clock_msec()-ts)<=msec){
		SLEEP();
	}
}
