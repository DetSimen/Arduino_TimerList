#pragma once


using	PVoidFunc = void(*)(void);      // указатель на функцию без параметров, ничего не возвращающую: void AnyFunc(void)  

#ifdef ARDUINO_AVR_MEGA2560
 #define     MAXTIMERSCOUNT		16		// 	Максимальное число зарегистрированных таймеров для Mega2560 - 16
#else
 #define     MAXTIMERSCOUNT		8		// 	Максимальное число зарегистрированных таймеров для прочих Uno - 8
#endif

#define	    _1MSCONST			1990	//	задержка для 1 миллисекунды  12Мгц - 1500, 16Мгц - 2000 минус накладные расходы 
                                        //  между прерываниями 16 Мгц процессор выполнит примерно 10000 команд



extern bool InRange(int,int,int);

using THandle = int8_t;

#pragma pack(push,1)
struct TCallStruct					    // внутренняя структура для хранения КАЖДОГО таймера
{
	PVoidFunc	CallingFunc;			// функция, которую нужно вызвать при срабатывании
	long		InitCounter;			// заданное кол-во миллисекунд
	long		WorkingCounter;			// рабочий счетчик. после начала счёта сюда копируется значение из InitCounter
	bool        Active;	  			    // и каждую миллисекунду уменьшается на 1. При достиженнии 0 вызывается функция 
							            // CallingFunc, и снова записывается значение из InitCounter. Счёт начинается сначала. :)
};

class TTimerList
{
	private:
		TCallStruct		Items[MAXTIMERSCOUNT];
		void			Init();
		bool			active;
		byte			count;

	public:
		TTimerList();
			
    	THandle      Add(PVoidFunc AFunc, long timeMS);	        // функция AFunc, которую надо вызвать через timeMS миллисекунд
		THandle      AddSeconds(PVoidFunc AFunc, word timeSec); // то же, только интервал задается в секундах. 
		THandle      AddMinutes(PVoidFunc AFunc, word timeMin); // то же, только интервал задается в минутах. 
														        // исключительно для удобства создания длинных интервалов 

		bool         CanAdd();                          // если можно добавить таймер, вернет true

		bool		 IsActive();                        // true, если хоть один таймер запущен

		void		 Delete(THandle hnd);               // удалить таймер hnd

		void		 Step(void);                

		void		 Start();                           // запустить рабочий цикл перебора таймеров

		void		 Stop();                            // TimerList выключен, все таймеры остановлены

		byte		 Count();                           // счетчик добавленных таймеров, всего. Можно не использовать, просто проверять возможность
		                                                // добавления, вызывая ф-цию CanAdd();

		void         TimerStop(THandle hnd);            // остановить отдельный таймер по его хэндлу

		void         TimerStart(THandle hnd);           // запустить отдельный таймер (после остановки) 
		                                                //счет не продолжается, а начинается сначала
};

extern TTimerList TimerList;                            // переменная будет видна в основном скетче

