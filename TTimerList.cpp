#include <Arduino.h>
#include "TTimerList.h"

// создается пустой, остановленный список щёччиков 
TTimerList TimerList(MAXTIMERSCOUNT);

/// Настройка таймеров для первого использования
/// на срабатывание каждую 1 миллисекунду
/// для Uno, Nano и прочих Micro с ATMega168, ATMega328, и на Mega2560  работает на таймере #0
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

#elif defined(__AVR_ATmega168__)
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


#elif defined(__AVR_ATmega8__)
void TTimerList::Init()
{
	byte oldSREG = SREG;
	cli();

	TCCR1A = 0; TCCR1B = 2;
	TCNT1 = 0;

	OCR1A = _1MSCONST;
	TCCR1B |= (1 << WGM12);
//	TCCR1B |= (1 << CS11) | (1 << CS10);
	
	TIMSK |= (1 << OCIE1A); 


	SREG = oldSREG;

}
#endif


#if defined(__AVR_ATmega8__)

ISR(TIMER1_COMPA_vect){
	OCR1A = _1MSCONST;
	TCNT1 = 0xFFFF;		// чтоб на следующем шаге сработало переполнение и посчитался millis
	TimerList.Tick();
}
#else
ISR(TIMER0_COMPA_vect){
	TCNT0 = 0xFF;		// чтоб на следующем шаге сработало переполнение и посчитался millis
	TimerList.Tick();
}
#endif


