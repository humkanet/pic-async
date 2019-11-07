#include <xc.h>
#include "loop.h"


typedef struct {
	LOOP_TASK         pc;          // Адрес, с которого продолжить обработку
	LOOP_TASK_STATUS  status;      // Состояние задачи
	uint8_t           data[2];     // Локальные данные задачи
	uint8_t           context[8];  // Контекст для сохранения регистров
} TASK;


typedef struct {
	uint16_t  until;
} SLEEP;


typedef struct {
	volatile void  *pc;
} AWAIT;


typedef struct {
	uint8_t  lock;
} MUTEX;


typedef struct {
	uint8_t  set;
} EVENT;


struct {
	uint16_t  msec;
	TASK      *task;
	TASK      tasks[LOOP_MAX_TASKS];
} loop;


static uint8_t __loop_int_mask;


void loop_init()
{
	// Помечаем все задачи как завершенные
	TASK  *task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		task->status = LOOP_TASK_STATUS_FINISHED;
	}
}


void loop_task_start(LOOP_TASK pc)
{
	TASK  *task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		// Проверяем, что задача не запущена
		if ((task->pc==pc)&&(task->status!=LOOP_TASK_STATUS_FINISHED)) break;
		// Ищем первый свободный слот и сохраняем в него задачу
		if (task->status==LOOP_TASK_STATUS_FINISHED){
			task->pc     = pc;
			task->status = LOOP_TASK_STATUS_RUN;
			break;
		}
	}
}


void loop_task_cancel(LOOP_TASK pc)
{
	TASK  *task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		// Ищем и отменяем задачу
		if (task->pc==pc){
			task->status = LOOP_TASK_STATUS_FINISHED;
			break;
		}
	}
}


void loop_task_finish()
{
	// Помечаем задачу, как завершенную
	loop.task->status = LOOP_TASK_STATUS_FINISHED;
}


void loop_tick(uint16_t msec)
{
	// Проверяем время
	if (msec==loop.msec) return;
	loop.msec = msec;
	// Проходимся по всем задачам
	loop.task = loop.tasks;
	for (uint8_t n=0; n<LOOP_MAX_TASKS; n++, loop.task++){
		switch (loop.task->status){
			case LOOP_TASK_STATUS_RUN:
				if (loop.task->pc) loop.task->pc();
				asm("LOOP_RESUME:");
				break;
			default:
				break;
		}
	}
}


void __loop_save_context()
{
	/* !!! FSR0 должен указывать на на контекст !!! */
	// Сохраняем регистры
	asm("movwi     0[FSR0]");
	asm("movf      BSR, W");
	asm("movwi     1[FSR0]");
	asm("movf      FSR0H, W");
	asm("movwi     2[FSR0]");
	asm("movf      FSR0L, W");
	asm("movwi     3[FSR0]");
	asm("movf      FSR1H, W");
	asm("movwi     4[FSR0]");
	asm("movf      FSR1L, W");
	asm("movwi     5[FSR0]");
	// Сохраняем флаг и запрещаем прерывания
	asm("movf      INTCON, W");
	asm("bcf       INTCON, 7");
	asm("andlw     0x80");
	asm("movwf     ___loop_int_mask");
	// Сохраняем пред. точку возврата
	asm("banksel   TOSL");
	asm("decf      STKPTR & 0x7F");
	asm("movf      TOSL & 0x7F, W");
	asm("movwi     6[FSR0]");
	asm("movf      TOSH & 0x7F, W");
	asm("movwi     7[FSR0]");
	// Перенацеливаем пред. точку возврата на управление очередью
	asm("movlw     low(LOOP_RESUME)");
	asm("movwf     TOSL & 0x7F");
	asm("movlw     high(LOOP_RESUME)");
	asm("movwf     TOSH & 0x7F");
	asm("incf      STKPTR & 0x7F");
	// Восстанавливаем прерывания
	asm("movf      ___loop_int_mask, W");
	asm("iorwf     INTCON");
}


void __loop_restore_context()
{
	/* !!! FSR0 должен указывать на на контекст !!! */
	asm("moviw     1[FSR0]");
	asm("movwf     BSR");
	asm("moviw     2[FSR0]");
	asm("movwf     FSR0H");
	asm("moviw     3[FSR0]");
	asm("movwf     FSR0L");
	asm("moviw     4[FSR0]");
	asm("movwf     FSR1H");
	asm("moviw     5[FSR0]");
	asm("movwf     FSR1L");
	asm("moviw     0[FSR0]");
}


void __loop_restore_sp()
{
	/* !!! FSR0 должен указывать на на контекст !!! */
	// Сохраняем флаг и запрещаем прерывания
	asm("movf      INTCON, W");
	asm("bcf       INTCON, 7");
	asm("andlw     0x80");
	asm("movwf     ___loop_int_mask");
	// Восстанавливаем SP
	asm("banksel   STKPTR");
	asm("moviw     6[FSR0]");
	asm("movwf     TOSL & 0x7F");
	asm("moviw     7[FSR0]");
	asm("movwf     TOSH & 0x7F");
	// Восстанавливаем прерывания
	asm("movf      ___loop_int_mask, W");
	asm("iorwf     INTCON");
}


LOOP_TASK_STATUS loop_task_status(LOOP_TASK pc)
{
	TASK  *task = loop.tasks;
	// Ищем задачу
	for(uint8_t n=0; n<LOOP_MAX_TASKS; n++, task++){
		if (task->pc==pc) return task->status;
	}
	// Задача не найдена
	return LOOP_TASK_STATUS_FINISHED;
}


void loop_continue()
{
	// Сохраняем контекст
	FSR0 = (uint16_t) &loop.task->context;
	__loop_save_context();
	// Изменяем точку возврата и возвращаем управление очереде
	FSR0 = (uint16_t) &loop.task->pc;
	asm("movlw     low(CONTINUE_RESUME)");
	asm("movwi     0[FSR0]");
	asm("movlw     high(CONTINUE_RESUME)");
	asm("movwi     1[FSR0]");
	// Возвращаем управление очереди
	asm("return");
	// Востанавливаем контекст и передаем управление задаче
	asm("CONTINUE_RESUME:");
	FSR0 = (uint16_t) &loop.task->context;
	__loop_restore_context();
	__loop_restore_sp();
}


void loop_sleep(uint16_t msec)
{
	// Расчитыем время завершения
	((SLEEP*)&loop.task->data)->until = loop.msec+msec;
	// Сохраняем контекст
	FSR0 = (uint16_t) &loop.task->context;
	__loop_save_context();
	// Изменяем точку возврата и возвращаем управление очереде
	FSR0 = (uint16_t) &loop.task->pc;
	asm("movlw     low(SLEEP_RESUME)");
	asm("movwi     0[FSR0]");
	asm("movlw     high(SLEEP_RESUME)");
	asm("movwi     1[FSR0]");
	asm("return");
	// Восстанавливаем контекст
	asm("SLEEP_RESUME:");
	FSR0 = (uint16_t) &loop.task->context;
	__loop_restore_context();
	// Если время ожидания истекло возвращаем управление задаче
	if (loop.msec>=((SLEEP*)&loop.task->data)->until){
		__loop_restore_sp();
	}
}


void loop_await(LOOP_TASK pc)
{
	// Запоминаем задачу
	((AWAIT*)&loop.task->data)->pc = pc;
	// Сохраняем контекст
	FSR0 = (uint16_t) &loop.task->context;
	__loop_save_context();
	// Изменяем точку возврата
	FSR0 = (uint16_t) &loop.task->pc;
	asm("movlw     low(AWAIT_RESUME)");
	asm("movwi     0[FSR0]");
	asm("movlw     high(AWAIT_RESUME)");
	asm("movwi     1[FSR0]");
	asm("return");
	// Восстанавливаем контекст
	asm("AWAIT_RESUME:");
	FSR0 = (uint16_t) &loop.task->context;
	__loop_restore_context();
	// Если задача завершена возвращаем управление
	if (loop_task_status(((AWAIT*)&loop.task->data)->pc)==LOOP_TASK_STATUS_FINISHED){
		__loop_restore_sp();
	}
}
