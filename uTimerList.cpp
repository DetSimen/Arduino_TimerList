#include "Arduino.h"
#include "uTimerList.h"
#include "MyTypes.h"

static bool InRange(int value, int min, int max)               // отдает true, если value лежит в диапаносе от min до max (включительно)
{
	return (value >= min) AND(value <= max);
}



TTimerList TimerList; 


/// Настройка таймеров для первого использования
/// на срабатывание каждую 1 миллисекунду
/// для Uno, Nano и прочих Micro с ATMega328, ATMega168 работает на таймере №1 
/// для Mega2560  работает на таймере #5
/// на других протестировать нет возможности


void TTimerList::Init()
{
	cli();

	TCCR0A = TCCR0A & 0b11111100;
	OCR0A = TIMER0_ONE_MS;
	TIMSK0 |= 0x3;
	TIFR0 = TIFR0 | 0x2;

	sei();

}

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
			cli();
			Items[i].CallingFunc	= AFunc;
			Items[i].InitCounter	= timeMS;
			Items[i].WorkingCounter = timeMS;
			Items[i].Active          = true;
			count++;
			if (NOT active)  Start();
			sei();
			return i;
		}
	}
	return -1;                                   // если нет - вернем код ашыпки (-1) 
}

THandle TTimerList::AddSeconds(PVoidFunc AFunc, word timeSec)
{
	return Add(AFunc, 1000L*timeSec);          // оналогично для секунд 
}

THandle TTimerList::AddMinutes(PVoidFunc AFunc, word timeMin)
{
	return Add(AFunc, timeMin*60L*1000L);     // оналогично для минут
}

byte TTimerList::AvailableCount(void)
{
	return MAXTIMERSCOUNT-count;
}

bool TTimerList::CanAdd()
{
	for (byte i = 0; i < MAXTIMERSCOUNT; i++)
	{
		if (Items[i].CallingFunc != NULL) continue;  // если в списке есть пустые места, куда можно добавить таймер
		return true;                                 // вернем true
	}
	return false;                                    // если нет - то false
}

bool TTimerList::IsActive()
{
	return active;                                   // если хоть один таймер активен, вернет true, если все остановлены - false
}

void TTimerList::Delete(THandle hnd)                 // удалить таймер с хэндлом hnd.     
{
	if (InRange(hnd, 0, MAXTIMERSCOUNT - 1))
	{
		cli();
		Items[hnd].CallingFunc		= NULL;
		Items[hnd].InitCounter		= 0;
		Items[hnd].WorkingCounter	= 0;
		Items[hnd].Active           = false;
		if (count > 0)  count--;
		if (count==0) Stop();                        // если все таймеры удалены, остановить и цикл перебора таймеров
		sei();
	}
}

/// эта функция вызывается при аппаратном срабатывании таймера каждую миллисекунду
/// запуская цикл декремента интервалов срабатывания

void TTimerList::Step(void)
{
	if (NOT active) return;                         // если все таймеры остановлены, то и смысла нет
	byte _sreg = SREG;								// запомним состояние прерываний
	cli();                                          // чтоб никто не помешал изменить интервалы, запрещаем прерывания
			// сопсно рабочий цикл                      
	for (THandle i = 0; i < MAXTIMERSCOUNT; i++)   // пробегаем по всему списку таймеров
	{
		if (Items[i].CallingFunc == NULL)  continue;  // если функция-обрабоччик не назначена, уходим на следующий цикл 
		if (NOT Items[i].Active)           continue; // если таймер остановлен - тоже
		if (--Items[i].WorkingCounter > 0) continue;  // уменьшаем на 1 рабочий счетчик
		Items[i].CallingFunc();                       // если достиг 0, вызываем функцию-обработчик
		Items[i].WorkingCounter = Items[i].InitCounter; // и записываем в рабочий счетчик начальное значение для счета сначала
	}
	SREG = _sreg;	                                  // теперь и прерывания можно восстановить как было
}

void TTimerList::Start()                              
{
	if (NOT active) Init();                              // при добавлении первого таймера, инициализируем аппаратный таймер
	active = true;
}

void TTimerList::Stop()
{
	active = false;                                   // остановить все таймеры
}

byte TTimerList::Count()
{
	return count;                                     // счетчик добавленных таймеров
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
	if (NOT InRange(hnd,0,MAXTIMERSCOUNT-1)) return;
	if (Items[hnd].CallingFunc == NULL)      return;
	if (Items[hnd].Active)                   return;
	cli();
	Items[hnd].WorkingCounter = Items[hnd].InitCounter;  // и начать отсчет интервала сначала
	Items[hnd].Active = true;
	sei();
}

void TTimerList::TimerNewInterval(THandle hnd, long newinterval)
{
	if (NOT InRange(hnd, 0, MAXTIMERSCOUNT - 1)) 
	{
		Serial << "NotInRange\n"; return;
	}
	PCallStruct item = &Items[hnd];
	if (item->CallingFunc == NULL)
	{
		Serial << "NULL\n";
		return;
	}
	cli();
	item->InitCounter = newinterval;
	item->WorkingCounter = newinterval;
	item->Active = true;
	sei();
}


ISR(TIMER0_COMPA_vect)
{

	TCNT0 = 0xFF;
	TimerList.Step();
}

