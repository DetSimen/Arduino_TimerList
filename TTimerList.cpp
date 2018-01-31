#include "Arduino.h"
#include "TTimerList.h"
#include "MyTypes.h"


/*
static bool InRange(int value, int min, int max)
{
	return (value >= min) AND(value <= max);
}
*/

TTimerList TimerList; 


/// Настройка таймеров для первого использования
/// на срабатывание каждую 1 миллисекунду
/// для Uno, Nano и прочих Micro с ATMega328, и на Mega2560  работает на таймере #0
/// на Atmega8 работает на таймере 1, на таймере 0 нет прерывания по совпадению
/// на других протестировать нет возможности

#if defined(__AVR_ATmega2560__) 
void TTimerList::Init()
{
	byte oldSREG = SREG;
	cli();

	TCCR0A = TCCR0A & 0b11111100;
	OCR0A = TIMER0_ONE_MS;
	TIMSK0 |= 0x3;
	TIFR0 = TIFR0 | 0x2;

	SREG = oldSREG;

}
#elif defined(__AVR_ATmega328P__)
void TTimerList::Init()
{
	byte oldSREG = SREG;
	cli();

	TCCR0A = TCCR0A & 0b11111100;
	OCR0A = TIMER0_ONE_MS;
	TIMSK0 |= 0x3;
	TIFR0 = TIFR0 | 0x2;

	SREG = oldSREG;

}
#elif defined(__AVR_ATMEGA8__)
void TTimerList::Init()
{
	byte oldSREG = SREG;
	cli();

	TCCR1A = 0; TCCR1B = 2;
	TCNT1 = 0;

	OCR1A = _1MSCONST;
	TCCR1B |= (1 << WGM12);
//	TCCR1B |= (1 << CS11) | (1 << CS10);
	
	TIMSK1 |= (1 << OCIE1A); 


	SREG = oldSREG;

}
#endif

TTimerList::TTimerList()
{
	count = 0;
	for (byte i = 0; i < MAXTIMERSCOUNT; i++)  // первоначальная инициализация, заполнение списка нулями
	{
		Items[i].CallingFunc    = NULL;
		Items[i].InitCounter    = 0;
		Items[i].WorkingCounter = 0;
		Items[i].Active          = false;
	}
	active = false;
}

THandle TTimerList::Add(PVoidFunc AFunc, long timeMS)
{
	for (THandle i = 0; i < MAXTIMERSCOUNT; i++)
	{
		if (Items[i].CallingFunc == NULL)         // пробегаемся по списку таймеров
		{                                         // если нашли пустую дырку, добавляем таймер
			byte sreg = SREG; cli();
			Items[i].CallingFunc	= AFunc;
			Items[i].InitCounter	= timeMS;
			Items[i].WorkingCounter = timeMS;
			Items[i].Active          = true;
			count++;
			if (NOT active)  AllStart();
			SREG = sreg;
			return i;
		}
	}
	return 0xFF;                                   // если нет - вернем код ашыпки (-1) 
}

THandle TTimerList::AddSeconds(PVoidFunc AFunc, word timeSec)
{
	return Add(AFunc, 1000L*timeSec);          // оналогично для секунд 
}

THandle TTimerList::AddMinutes(PVoidFunc AFunc, word timeMin)
{
	return Add(AFunc, timeMin*60L*1000L);     // оналогично для минут
}



bool TTimerList::CanAdd() const
{
	for (byte i = 0; i < MAXTIMERSCOUNT; i++)
	{
		if (Items[i].CallingFunc != NULL) continue;  // если в списке есть пустые места, куда можно добавить таймер
		return true;                                 // вернем true
	}
	return false;                                    // если нет - то false
}

bool TTimerList::IsActive() const
{
	return active;                                   // если хоть один таймер активен, вернет true, если все остановлены - false
}

void TTimerList::Delete(THandle hnd)                 // удалить таймер с хэндлом hnd.     
{
	if (InRange(hnd, 0, MAXTIMERSCOUNT - 1))
	{
		byte sreg = SREG; cli();
		Items[hnd].CallingFunc		= NULL;
		Items[hnd].InitCounter		= 0;
		Items[hnd].WorkingCounter	= 0;
		Items[hnd].Active           = false;
		if (count > 0)  count--;
		if (count==0) AllStop();                        // если все таймеры удалены, остановить и цикл перебора таймеров
		SREG = sreg;
	}
}

/// эта функция вызывается при аппаратном срабатывании таймера каждую миллисекунду
/// запуская цикл декремента интервалов срабатывания

void TTimerList::Step(void)
{
	if (NOT active) return;                         // если все таймеры остановлены, то и смысла нет
	byte _sreg = SREG;								// запомним состояние прерываний
			// сопсно рабочий цикл                      
	for (THandle i = 0; i < MAXTIMERSCOUNT; i++)   // пробегаем по всему списку таймеров
	{
		cli();                                          // чтоб никто не помешал изменить интервалы, запрещаем прерывания
		if (Items[i].CallingFunc == NULL)  continue;  // если функция-обрабоччик не назначена, уходим на следующий цикл 
		if (NOT Items[i].Active)           continue; // если таймер остановлен - тоже
		if (--Items[i].WorkingCounter > 0) continue;  // уменьшаем на 1 рабочий счетчик
		Items[i].WorkingCounter = Items[i].InitCounter; // и записываем в рабочий счетчик начальное значение для счета сначала
		sei();
		Items[i].CallingFunc();                       // если достиг 0, вызываем функцию-обработчик
	}
	SREG = _sreg;	                                  // теперь и прерывания можно восстановить как было
}

void TTimerList::AllStart()                              
{
	if (NOT active) Init();                              // при добавлении первого таймера, инициализируем аппаратный таймер
	active = true;
}

void TTimerList::AllStop()
{
	active = false;                                   // остановить все таймеры
}

void TTimerList::TimerStop(THandle hnd)
{
	if (InRange(hnd, 0, MAXTIMERSCOUNT-1))
	{
		Items[hnd].Active = false;                    // остановить таймер номер hnd
	}
}

bool TTimerList::TimerActive(THandle hnd)
{
	if (NOT InRange(hnd, 0, MAXTIMERSCOUNT - 1)) return false;
	else return Items[hnd].Active;
}

void TTimerList::TimerStart(THandle hnd)              // запустить остановленный таймер номер hnd
{
	if (isValid(hnd))
	{
		if (Items[hnd].Active)                   return;
		byte old = SREG; 
		cli();
		Items[hnd].WorkingCounter = Items[hnd].InitCounter;  // и начать отсчет интервала сначала
		Items[hnd].Active = true;
		SREG = old;
	}
}

void TTimerList::TimerResume(THandle hnd)                 // продолжить счёт таймера после паузы
{
	if (isValid(hnd)) Items[hnd].Active = true;
}

void TTimerList::TimerPause(THandle hnd)				// поставить таймер на паузу
{
	if (isValid(hnd)) Items[hnd].Active = false;
}

bool TTimerList::isValid(THandle hnd)					// private функция, cнаружи не видна
{
	return (InRange(hnd, 0, MAXTIMERSCOUNT) AND (Items[hnd].CallingFunc != NULL));
}

void TTimerList::TimerNewInterval(THandle hnd, long newinterval)	// назначить таймеру новый интервал
{
	if (isValid(hnd))
	{
		TimerStop(hnd);
		Items[hnd].InitCounter = newinterval;
		TimerStart(hnd);
	}
}

#if defined(__AVR_ATMEGA8__)

ISR(TIMER1_COMPA_vect)
{
	OCR1A = _1MSCONST;
	TimerList.Step();
}
#else
ISR(TIMER0_COMPA_vect)
{

	TCNT0 = 0xFF;
	TimerList.Step();
}
#endif

ISR(PCINT0_vect)
{

}

