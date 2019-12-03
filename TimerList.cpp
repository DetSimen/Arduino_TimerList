#include <Arduino.h>
#include <util/atomic.h>
#include "TimerList.h"
#include <avr\interrupt.h>


// глобально создается пустой, остановленный список щёччиков 
TTimerList TimerList;   // число таймеров = MAXTIMERSCOUNT

// Настройка таймеров для первого использования
// на срабатывание каждую 1 миллисекунду
// для Uno, Nano и прочих Micro с ATMega168, ATMega328, и на Mega2560  работает на таймере #0
// на Atmega8 работает на таймере 1, на таймере 0 нет прерывания по совпадению
// на других протестировать нет возможности

#if defined(__AVR_ATmega2560__) 
void TTimerList::Init()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		TCCR0A = TCCR0A & 0b11111100;
		OCR0A = TIMER0_ONE_MS;
		TIMSK0 |= 0x3;
		TIFR0 = TIFR0 | 0x2;

	}
}
#elif defined(__AVR_ATmega328P__)
void TTimerList::Init()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

		TCCR0A = TCCR0A & 0b11111100;  // Таймер в режиме Normal
		OCR0A = TIMER0_ONE_MS;		// загрузим регистр совпадения
		TIMSK0 |= 0x3;				// установить OCIE0A и TOIE0, разрешим совпадение А и переполнение
		TIFR0 |= 0x2;				// очистим флаг OCF0A если до этого он был установлен, ждём следущего совпадения
	}


}

#elif defined(__AVR_ATmega32U4__)
void TTimerList::Init()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

		TCCR0A = TCCR0A & 0b11111100;  // Таймер в режиме Normal
		OCR0A = TIMER0_ONE_MS;		// загрузим регистр совпадения
		TIMSK0 |= 0x3;				// установить OCIE0A и TOIE0, разрешим совпадение А и переполнение
		TIFR0 |= 0x2;				// очистим флаг OCF0A если до этого он был установлен, ждём следущего совпадения
	}


}


#elif defined(__AVR_ATmega168__)
void TTimerList::Init()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

		TCCR0A = TCCR0A & 0b11111100;
		OCR0A = TIMER0_ONE_MS;
		TIMSK0 |= 0x3;
		TIFR0 = TIFR0 | 0x2;

	}

}


#elif defined(__AVR_ATmega8__)
void TTimerList::Init()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		TCCR1A = 0; TCCR1B = 2;
		TCNT1 = 0;
		OCR1A = _1MSCONST;
		TCCR1B |= (1 << WGM12);
		TIMSK |= (1 << OCIE1A);
	}
}
#endif


#if defined(__AVR_ATmega8__)

ISR(TIMER1_COMPA_vect){
	OCR1A = _1MSCONST;
	TimerList.Tick();
}
#else

ISR(TIMER0_COMPA_vect){
	TCNT0 = 0xFF;		// чтоб на следующем шаге сработало переполнение и посчитался millis
	TimerList.Tick();
}
#endif


