#include "Arduino.h"
#include "uTimerList.h"

#define AND &&
#define OR  ||
#define NOT !

TTimerList TimerList;    

/// Íàñòðîéêà òàéìåðîâ äëÿ ïåðâîãî èñïîëüçîâàíèÿ
/// íà ñðàáàòûâàíèå êàæäóþ 1 ìèëëèñåêóíäó
/// äëÿ Uno, Nano è ïðî÷èõ Micro ñ ATMega328, ATMega168 ðàáîòàåò íà òàéìåðå ¹1 
/// äëÿ Mega2560  ðàáîòàåò íà òàéìåðå #5
/// íà äðóãèõ ïðîòåñòèðîâàòü íåò âîçìîæíîñòè


#ifdef ARDUINO_AVR_MEGA2560
void TTimerList::Init()
{
	cli();
	TCCR5A = 0; TCCR5B = 0;
	TCNT5 = 0;

	OCR5A = _1MSCONST;
	TCCR5B |= (1 << WGM52);
	TCCR5B |= (1 << CS51);// | (1 << CS10);
	TIMSK5 |= (1 << OCIE5A);
	sei();
}
#else  // Uno, Nano, Micro
void TTimerList::Init()
{
	cli();
	TCCR1A = 0; TCCR1B = 0;
	TCNT1 = 0;

	OCR1A = _1MSCONST;
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << CS11);// | (1 << CS10);
	TIMSK1 |= (1 << OCIE1A);
	sei();
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
			cli();
			Items[i].CallingFunc	= AFunc;
			Items[i].InitCounter	= timeMS;
			Items[i].WorkingCounter = timeMS;
			Items[i].Active          = true;
			if (count<MAXTIMERSCOUNT) count++;
			if ((count>0) AND (NOT active)) Start();
			sei();
			return i;
		}
	}
	return -1;                                   // åñëè íåò - âåðíåì êîä àøûïêè (-1) 
}

THandle TTimerList::AddSeconds(PVoidFunc AFunc, word timeSec)
{
	return Add(AFunc, 1000L*timeSec);          // îíàëîãè÷íî äëÿ ñåêóíä 
}

THandle TTimerList::AddMinutes(PVoidFunc AFunc, word timeMin)
{
	return Add(AFunc, timeMin*60L*1000L);     // îíàëîãè÷íî äëÿ ìèíóò
}

bool TTimerList::CanAdd()
{
	for (byte i = 0; i < MAXTIMERSCOUNT; i++)
	{
		if (Items[i].CallingFunc != NULL) continue;  // åñëè â ñïèñêå åñòü ïóñòûå ìåñòà, êóäà ìîæíî äîáàâèòü òàéìåð
		return true;                                 // âåðíåì true
	}
	return false;                                    // åñëè íåò - òî false
}

bool TTimerList::IsActive()
{
	return active;                                   // åñëè õîòü îäèí òàéìåð àêòèâåí, âåðíåò true, åñëè âñå îñòàíîâëåíû - false
}

void TTimerList::Delete(THandle hnd)                 // óäàëèòü òàéìåð ñ õýíäëîì hnd.     
{
	if (InRange(hnd, 0, MAXTIMERSCOUNT - 1))
	{
		cli();
		Items[hnd].CallingFunc		= NULL;
		Items[hnd].InitCounter		= 0;
		Items[hnd].WorkingCounter	= 0;
		Items[hnd].Active           = false;
		if (count > 0)  count--;
		if (count==0) Stop();                        // åñëè âñå òàéìåðû óäàëåíû, îñòàíîâèòü è öèêë ïåðåáîðà òàéìåðîâ
		sei();
	}
}

/// ýòà ôóíêöèÿ âûçûâàåòñÿ ïðè àïïàðàòíîì ñðàáàòûâàíèè òàéìåðà êàæäóþ ìèëëèñåêóíäó
/// çàïóñêàÿ öèêë äåêðåìåíòà èíòåðâàëîâ ñðàáàòûâàíèÿ

void TTimerList::Step(void)
{
	if (!active) return;                            // åñëè âñå òàéìåðû îñòàíîâëåíû, òî è ñìûñëà íåò
	cli();                                          // ÷òîá íèêòî íå ïîìåøàë èçìåíèòü èíòåðâàëû, çàïðåùàåì ïðåðûâàíèÿ
// ñîïñíî ðàáî÷èé öèêë                      
	for (THandle i = 0; i < MAXTIMERSCOUNT; i++)   // ïðîáåãàåì ïî âñåìó ñïèñêó òàéìåðîâ
	{
		if (Items[i].CallingFunc == NULL)  continue;  // åñëè ôóíêöèÿ-îáðàáî÷÷èê íå íàçíà÷åíà, óõîäèì íà ñëåäóþùèé öèêë 
		if (NOT Items[i].Active)               continue; // åñëè òàéìåð îñòàíîâëåí - òîæå
		if (--Items[i].WorkingCounter > 0) continue;  // óìåíüøàåì íà 1 ðàáî÷èé ñ÷åò÷èê
		Items[i].CallingFunc();                       // åñëè äîñòèã 0, âûçûâàåì ôóíêöèþ-îáðàáîò÷èê
		Items[i].WorkingCounter = Items[i].InitCounter; // è çàïèñûâàåì â ðàáî÷èé ñ÷åò÷èê íà÷àëüíîå çíà÷åíèå äëÿ ñ÷åòà ñíà÷àëà
	}
	sei();                                            // òåïåðü è ïðåðûâàíèÿ ìîæíî ðàçðåøèòü
}

void TTimerList::Start()                              
{
	if (NOT active) Init();                              // ïðè äîáàâëåíèè ïåðâîãî òàéìåðà, èíèöèàëèçèðóåì àïïàðàòíûé òàéìåð
	active = true;
}

void TTimerList::Stop()
{
	active = false;                                   // îñòàíîâèòü âñå òàéìåðû
}

byte TTimerList::Count()
{
	return count;                                     // ñ÷åò÷èê äîáàâëåííûõ òàéìåðîâ
}

void TTimerList::TimerStop(THandle hnd)
{
	if (InRange(hnd, 0, MAXTIMERSCOUNT-1))
	{
		Items[hnd].Active = false;                    // îñòàíîâèòü òàéìåð íîìåð hnd
	}
}

void TTimerList::TimerStart(THandle hnd)              // çàïóñòèòü îñòàíîâëåííûé òàéìåð íîìåð hnd
{
	if (NOT InRange(hnd,0,MAXTIMERSCOUNT-1)) return;
	if (Items[hnd].CallingFunc == NULL)      return;
	if (Items[hnd].Active)                   return;
	cli();
	Items[hnd].WorkingCounter = Items[hnd].InitCounter;  // è íà÷àòü îòñ÷åò èíòåðâàëà ñíà÷àëà
	Items[hnd].Active = true;
	sei();
}

#ifdef ARDUINO_AVR_MEGA2560                              // îáðàáîò÷èê ïðåðûâàíèÿ àïïàðàòíîãî òàéìåðà
ISR(TIMER5_COMPA_vect)
{
	OCR5A = _1MSCONST;
	TimerList.Step();
}

#else
ISR(TIMER1_COMPA_vect)
{
	OCR1A = _1MSCONST;
	TimerList.Step();
}
#endif


bool InRange(int value, int min, int max)               // îòäàåò true, åñëè value ëåæèò â äèàïàçîíå îò min äî max (âêëþ÷èòåëüíî)
{
	return (value >= min) AND (value <= max);
}
