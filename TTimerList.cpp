#include "Arduino.h"
#include "TTimerList.h"

   
static bool InRange(int value, int min, int max)
{
	return (value >= min) AND(value <= max);
}


TTimerList TimerList; 


/// Íàñòðîéêà òàéìåðîâ äëÿ ïåðâîãî èñïîëüçîâàíèÿ
/// íà ñðàáàòûâàíèå êàæäóþ 1 ìèëëèñåêóíäó
/// äëÿ Uno, Nano è ïðî÷èõ Micro ñ ATMega328, è íà Mega2560  ðàáîòàåò íà òàéìåðå #0
/// íà Atmega8 ðàáîòàåò íà òàéìåðå 1, íà òàéìåðå 0 íåò ïðåðûâàíèÿ ïî ñîâïàäåíèþ
/// íà äðóãèõ ïðîòåñòèðîâàòü íåò âîçìîæíîñòè

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
	for (byte i = 0; i < MAXTIMERSCOUNT; i++)  // ïåðâîíà÷àëüíàÿ èíèöèàëèçàöèÿ, çàïîëíåíèå ñïèñêà íóëÿìè
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
		if (Items[i].CallingFunc == NULL)         // ïðîáåãàåìñÿ ïî ñïèñêó òàéìåðîâ
		{                                         // åñëè íàøëè ïóñòóþ äûðêó, äîáàâëÿåì òàéìåð
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
	return 0xFF;                                   // åñëè íåò - âåðíåì êîä àøûïêè (-1) 
}

THandle TTimerList::AddSeconds(PVoidFunc AFunc, word timeSec)
{
	return Add(AFunc, 1000L*timeSec);          // îíàëîãè÷íî äëÿ ñåêóíä 
}

THandle TTimerList::AddMinutes(PVoidFunc AFunc, word timeMin)
{
	return Add(AFunc, timeMin*60L*1000L);     // îíàëîãè÷íî äëÿ ìèíóò
}



bool TTimerList::CanAdd() const
{
	for (byte i = 0; i < MAXTIMERSCOUNT; i++)
	{
		if (Items[i].CallingFunc != NULL) continue;  // åñëè â ñïèñêå åñòü ïóñòûå ìåñòà, êóäà ìîæíî äîáàâèòü òàéìåð
		return true;                                 // âåðíåì true
	}
	return false;                                    // åñëè íåò - òî false
}

bool TTimerList::IsActive() const
{
	return active;                                   // åñëè õîòü îäèí òàéìåð àêòèâåí, âåðíåò true, åñëè âñå îñòàíîâëåíû - false
}

void TTimerList::Delete(THandle hnd)                 // óäàëèòü òàéìåð ñ õýíäëîì hnd.     
{
	if (InRange(hnd, 0, MAXTIMERSCOUNT - 1))
	{
		byte sreg = SREG; cli();
		Items[hnd].CallingFunc		= NULL;
		Items[hnd].InitCounter		= 0;
		Items[hnd].WorkingCounter	= 0;
		Items[hnd].Active           = false;
		if (count > 0)  count--;
		if (count==0) AllStop();                        // åñëè âñå òàéìåðû óäàëåíû, îñòàíîâèòü è öèêë ïåðåáîðà òàéìåðîâ
		SREG = sreg;
	}
}

/// ýòà ôóíêöèÿ âûçûâàåòñÿ ïðè àïïàðàòíîì ñðàáàòûâàíèè òàéìåðà êàæäóþ ìèëëèñåêóíäó
/// çàïóñêàÿ öèêë äåêðåìåíòà èíòåðâàëîâ ñðàáàòûâàíèÿ

void TTimerList::Step(void)
{
	if (NOT active) return;                         // åñëè âñå òàéìåðû îñòàíîâëåíû, òî è ñìûñëà íåò
	byte _sreg = SREG;								// çàïîìíèì ñîñòîÿíèå ïðåðûâàíèé
			// ñîïñíî ðàáî÷èé öèêë                      
	for (THandle i = 0; i < MAXTIMERSCOUNT; i++)   // ïðîáåãàåì ïî âñåìó ñïèñêó òàéìåðîâ
	{
		cli();                                          // ÷òîá íèêòî íå ïîìåøàë èçìåíèòü èíòåðâàëû, çàïðåùàåì ïðåðûâàíèÿ
		if (Items[i].CallingFunc == NULL)  continue;  // åñëè ôóíêöèÿ-îáðàáî÷÷èê íå íàçíà÷åíà, óõîäèì íà ñëåäóþùèé öèêë 
		if (NOT Items[i].Active)           continue; // åñëè òàéìåð îñòàíîâëåí - òîæå
		if (--Items[i].WorkingCounter > 0) continue;  // óìåíüøàåì íà 1 ðàáî÷èé ñ÷åò÷èê
		Items[i].WorkingCounter = Items[i].InitCounter; // è çàïèñûâàåì â ðàáî÷èé ñ÷åò÷èê íà÷àëüíîå çíà÷åíèå äëÿ ñ÷åòà ñíà÷àëà
		sei();
		Items[i].CallingFunc();                       // åñëè äîñòèã 0, âûçûâàåì ôóíêöèþ-îáðàáîò÷èê
	}
	SREG = _sreg;	                                  // òåïåðü è ïðåðûâàíèÿ ìîæíî âîññòàíîâèòü êàê áûëî
}

void TTimerList::AllStart()                              
{
	if (NOT active) Init();                              // ïðè äîáàâëåíèè ïåðâîãî òàéìåðà, èíèöèàëèçèðóåì àïïàðàòíûé òàéìåð
	active = true;
}

void TTimerList::AllStop()
{
	active = false;                                   // îñòàíîâèòü âñå òàéìåðû
}

void TTimerList::TimerStop(THandle hnd)
{
	if (InRange(hnd, 0, MAXTIMERSCOUNT-1))
	{
		Items[hnd].Active = false;                    // îñòàíîâèòü òàéìåð íîìåð hnd
	}
}

bool TTimerList::TimerActive(THandle hnd)
{
	if (NOT InRange(hnd, 0, MAXTIMERSCOUNT - 1)) return false;
	else return Items[hnd].Active;
}

void TTimerList::TimerStart(THandle hnd)              // çàïóñòèòü îñòàíîâëåííûé òàéìåð íîìåð hnd
{
	if (isValid(hnd))
	{
		if (Items[hnd].Active)                   return;
		byte old = SREG; 
		cli();
		Items[hnd].WorkingCounter = Items[hnd].InitCounter;  // è íà÷àòü îòñ÷åò èíòåðâàëà ñíà÷àëà
		Items[hnd].Active = true;
		SREG = old;
	}
}

void TTimerList::TimerResume(THandle hnd)                 // ïðîäîëæèòü ñ÷¸ò òàéìåðà ïîñëå ïàóçû
{
	if (isValid(hnd)) Items[hnd].Active = true;
}

void TTimerList::TimerPause(THandle hnd)				// ïîñòàâèòü òàéìåð íà ïàóçó
{
	if (isValid(hnd)) Items[hnd].Active = false;
}

bool TTimerList::isValid(THandle hnd)					// private ôóíêöèÿ, cíàðóæè íå âèäíà
{
	return (InRange(hnd, 0, MAXTIMERSCOUNT) AND (Items[hnd].CallingFunc != NULL));
}

void TTimerList::TimerNewInterval(THandle hnd, long newinterval)	// íàçíà÷èòü òàéìåðó íîâûé èíòåðâàë
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

