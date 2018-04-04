#pragma once
#include "Arduino.h"




#define AND &&
#define OR ||
#define NOT !


using	PVoidFunc = void(*)(void);      // указатель на функцию без параметров, ничего не возвращающую: void AnyFunc(void)  


#ifdef __AVR_ATmega2560__
 #define     	MAXTIMERSCOUNT		16		// 	Максимальное число зарегистрированных таймеров для Mega2560 - 16
 #define	TIMER0_ONE_MS	247
#endif

#ifdef __AVR_ATMEGA8__
 #define	 MAXTIMERSCOUNT		8
 #define	 _1MSCONST		F_CPU/8/1000 //	задержка для 1 миллисекунды  12Мгц - 1500, 16Мгц - 2000 минус накладные расходы 
											//  между прерываниями 16 Мгц процессор выполнит примерно 10000 команд
#endif

#ifdef __AVR_ATmega328P__
#define  MAXTIMERSCOUNT		12		// 	Максимальное число зарегистрированных таймеров для прочих Uno - 12
#define	 TIMER0_ONE_MS	247
#endif

#ifdef __AVR_ATmega168P__
#define  MAXTIMERSCOUNT		12		// 	Максимальное число зарегистрированных таймеров для прочих Uno - 12
#define	 TIMER0_ONE_MS	247
#endif


/*   Раскомментить ЭТО

using	THandle = int8_t;			// можно замонстрячить от 0 до максимум 127 таймеров, только зачем?
static	const THandle INVALID_HANDLE PROGMEM = -1;
*/   

#pragma pack(push,1)
struct TCallStruct					    // внутренняя структура для хранения КАЖДОГО таймера
{
	PVoidFunc	CallingFunc;			// функция, которую нужно вызвать при срабатывании
	long		InitCounter;			// заданное кол-во миллисекунд
	long		WorkingCounter;			// рабочий счетчик. после начала счёта сюда копируется значение из InitCounter
	bool       	Active;	  			// и каждую миллисекунду уменьшается на 1. При достиженнии 0 вызывается функция 
							// CallingFunc, и снова записывается значение из InitCounter. Счёт начинается сначала. :)
};
#pragma pack(pop)
using PCallStruct = TCallStruct *;

class TTimerList
{
private:
	TCallStruct		Items[MAXTIMERSCOUNT];
	void			Init();
	bool			active;
	byte			count;

// проверка, что таймер существует, его номер правильный
// и callback функция присвоена
	bool			isValid(THandle hnd);				

public:
	TTimerList();


	THandle      Add(PVoidFunc AFunc, long timeMS);	        // функция AFunc, которую надо вызвать через timeMS миллисекунд

	THandle		 AddStopped(PVoidFunc AFunc, long AMS) {	// создать таймер не запуская его (в остановленном состоянии)
	   THandle result = Add(AFunc, AMS);					// AMS - время в миллисекундах	
	   if (result>=0) TimerStop(result);
	   return result;
	}

	THandle      AddSeconds(PVoidFunc AFunc, word timeSec); // то же, только интервал задается в секундах. 
	THandle      AddMinutes(PVoidFunc AFunc, word timeMin); // то же, только интервал задается в минутах. 
								// исключительно для удобства создания длинных интервалов 
// функции для списка в целом

		inline bool  	CanAdd() const;                // если можно добавить таймер, вернет true

		inline bool	IsActive() const;                  // true, если хоть один таймер запущен

		void		Delete(THandle hnd);               // удалить таймер hnd

		void		Step(void);                

		void		AllStart();                        // запустить рабочий цикл перебора таймеров

		void		AllStop();                         // выключить все таймеры, TimerList остановлен

		inline byte	Count() const {return count; };    // счетчик добавленных таймеров, всего. Можно не использовать, просто проверять возможность
		 	                                            // добавления, вызывая ф-цию CanAdd();

		inline byte MaxTimersCount() const { return MAXTIMERSCOUNT; }; // максимальное число таймеров


//  функции для одного конкретного таймера, заданного его номером (THandle)

		void	TimerPause(THandle hnd);	//приостановить таймер hnd

		void	TimerResume(THandle hnd);	// продолжить счёт для таймера hnd с остановленного места

		void	TimerReset(THandle hnd);	// для таймера hnd начать счёт с начала

		void	TimerStop(THandle hnd);		// остановить отдельный таймер 

		bool	TimerActive(THandle hnd);   // запущен ли конкретный таймер

		void    TimerStart(THandle hnd);    // запустить отдельный таймер (после остановки) 
		
		void	TimerNewInterval(THandle hnd, long newinterval); // назначить таймеру hnd новый интервал
};



extern TTimerList TimerList;                            // переменная будет видна в основном скетче

