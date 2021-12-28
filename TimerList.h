#pragma once

#include <Arduino.h>
//#include <Consts.h>

using bsize_t = uint8_t;   // size_t  размером 1 байт
using THandle = int8_t;

static const int8_t INVALID_HANDLE = -1;


#ifdef __AVR_ATmega2560__
 #define    MAXTIMERSCOUNT	16		// 	Максимальное число зарегистрированных таймеров для Mega2560 - 16
 #define	TIMER0_ONE_MS	247
#endif

#ifdef __AVR_ATmega8__
 #define	 MAXTIMERSCOUNT	8
 #define	 _1MSCONST		F_CPU/8/1000 //	задержка для 1 миллисекунды  12Мгц - 1500, 16Мгц - 2000 минус накладные расходы 
											//  между прерываниями 16 Мгц процессор выполнит примерно 10000 команд
#endif


#ifdef __AVR_ATmega328P__
#define  MAXTIMERSCOUNT		10		// 	Максимальное число зарегистрированных таймеров для прочих Uno - 10
#define	 TIMER0_ONE_MS		245
											
#endif

#ifdef  __AVR_ATmega32__
#define  MAXTIMERSCOUNT		10		// 	Максимальное число зарегистрированных таймеров для прочих Uno - 10
#define	 TIMER0_ONE_MS		245
									//  между прерываниями 16 Мгц процессор выполнит примерно 10000 команд
#endif //  


#ifdef __AVR_ATmega32U4__
 #define  MAXTIMERSCOUNT		10		// 	Максимальное число зарегистрированных таймеров для Lяnardo - 10
 #define	 TIMER0_ONE_MS		245
#endif

#ifdef __AVR_ATmega168__
#define  MAXTIMERSCOUNT		8		// 	Максимальное число зарегистрированных таймеров для прочих Old Uno - 8
#define	 TIMER0_ONE_MS		240
											
#endif



using pvfCallback = void(*)(void); // тип pointer to void function Callback


class TCounterDown;						// опережающее обявление класса TCounterDown
using  PCounterDown = TCounterDown *;	// и указателя на нее



class TCounterDown {
protected:
	bool		fActive;        // 1 byte   true - если счетчик считает, false, если остановлен
	uint32_t	fWorkCount;	    // 4 bytes	Рабочий счетчик, уменьшается на 1 каждый Tick()
	uint32_t	fInitCount;		// 4 bytes  Начальный счетчик, хранит значение от которого считать до 0
	pvfCallback fCallback;		// 2 bytes  Адрес функции, которая вызовется, когда счётчик досчитает до 0
								// sizeof(TCounterDown) = 11 bytes;

	// гарантированно очистить (заполнить нулями) все поля структуры
	void		Clear(void) { memset(this, 0, sizeof(TCounterDown)); };

	// пустой конструктор, не разрешен
	TCounterDown() {};
public:
	
	// конструктор. Принимает первоначальное значение, которое потом будет уменьшаться при каждом вызове 
	// Tick() и указатель на void функцию обратного вызова.
	TCounterDown(uint32_t aTimeMS, pvfCallback aCallback) {
		Clear();							//очистить все поля
		fInitCount = fWorkCount = aTimeMS;  // задать начальные значения 
		fCallback = aCallback;
		fActive = true;						// и сразу разрешить запуск
	}

	// остановить счетчик (поставить на паузу)
	inline void Stop(void) {
		fActive = false; 
	};

	// продолжить считать с того же места, где остановили	
	inline void Start(void) { 
		fActive = true; 
	};		

	// сбросить счетчик, считать сначала
	inline void Reset(void) { 
		Stop(); 
		fWorkCount = fInitCount; 
		Start(); 
	}; 

	// отдает true - если счетчик запущен и считает, и false, если остановлен
	inline bool isActive(void) const { 
		return fActive; 
	};			

	// отдает true - если функция Callback не назначена
	inline bool isEmpty(void) const { 
		return (fCallback == NULL); 
	};

	// установить новый интервал для счета
	void setInterval(uint32_t anewinterval) { 
		Stop(); 
		fInitCount = fWorkCount = anewinterval; 
		Start(); 
	};

	// Оператор постдекремента. Уменьшает рабочий счетчик на 1 за вызов, если он не остановлен. 
	// Если счетчик дошел до 0, и назначена функция Callback, то она вызывается
	// 
	TCounterDown &operator --(int) {

		uint8_t sreg = SREG;   // на всякий случай запомним состояние прерываний

		cli();				// запрещаем прерывания

		if ((isActive()) && (!isEmpty()) && ((--fWorkCount) == 0)) {  // если счетчик досчитал до 0
			fWorkCount = fInitCount; // перезагружаем рабочий счётчик начальным значением, чтобы считать заново
			sei();				// разрешаем прерывания
			fCallback();		// и вызываем нашу фунцию обратного вызова
		}
		
		SREG = sreg;     // восстанавливаем прерывания
		return *this;
	}

	// то же самое в виде функции, а не оператора
	void Tick(void) { (*this)--; };  // просто вызываем оператор постдекремента

	uint32_t getCount() const { return fWorkCount; };
};



class TTimerList;				// опережающее обьявление класса 
using PTimerList = TTimerList *; // и указателя на него

extern TTimerList TimerList;   // глобальная переменная нашего списка счетчиков



class TTimerList   {
protected:

	bool		factive;	// Активность всего списка, если false - все таймеры остановлены
	bsize_t		fcount;		// Количество добавленных таймеров

	PCounterDown Items[MAXTIMERSCOUNT];	// массив указателей на счетчики TCounterDown

	// отдает true если THandle счётчика правильный [0..fsize-1]
	// и сам счетчик не удален 
	bool isValid(THandle ahandle) {
		return ((ahandle >= 0) && (ahandle < MAXTIMERSCOUNT)) && (Items[ahandle] != NULL);
	}

	// начальная инициализация аппаратного таймера
	// реализация в cpp файле для разных камней разная
	void Init(void);


public:
	// создает пустой список размера asize, для хранения счетчиков
	// при создании забивает список NULL-ами
	TTimerList() {
		for (bsize_t i = 0; i < MAXTIMERSCOUNT; i++) 
			 Items[i] = NULL;				  // забиваем выделенную память нулями
		factive = false;					  // пока ни один счетчик не добавлен, список неактивен	
		fcount = 0;							  // и число счётчиков пока == 0
	};

/* for oversmart boys: uncomment this 

	~TTimerList() {
		for (bsize_t i = 0; i < fsize; i++) delete Items[i];
	}
*/

	// добавить счетчик в список
	// ainterval задается в миллисекундах
	// acallback - адрес функции, которая вызывается, когда счетчик досчитает до 0
	// 
	// возвращает Handle щёччика, или специальное значение INVALID_HANDLE, если 
	// добавить не удалось
	THandle Add(uint32_t ainterval, pvfCallback acallback, const bool AStopped = false) {
		
		if (fcount == MAXTIMERSCOUNT) return INVALID_HANDLE; // список заполнен до отказа, мест нет

		PCounterDown counter = new TCounterDown(ainterval, acallback); // попытаемся хапнуть память под щёччик
		
		if (counter == NULL) return INVALID_HANDLE;  // не удалось, уходим с ошибкой

		if (AStopped) counter->Stop();

		if (fcount == 0) {		// если добавляемый счетчик - первый, 
			Init();				// то сначала инициализируем аппаратный таймер
			Start();			// и разрешаем запустить весь этот цыкал перебора
		}

		for (THandle i = 0; i < MAXTIMERSCOUNT; i++) {
			if (Items[i] == NULL) {   // ищем свободную "дырку" в массиве Items
				Items[i] = counter;	  // если нашли - пхаем туда созданный щёччик	
				fcount++;			  // кол-во счетчиков увеличиваем на 1	
				return i;			  // возвращаем индекс добавленного счётчика как его Handle
			}
		}

		return INVALID_HANDLE;		  // ну, а если места не нашлось - извините. 

	}

	THandle AddSeconds(uint32_t aSeconds, pvfCallback aCallback, const bool AStopped = false) {
		return Add(1000UL * aSeconds, aCallback, AStopped);
	}

	THandle AddMinutes(const uint16_t AMinutes, const pvfCallback ACallBack, const bool AStopped = false) {
		return Add(1000UL * 60UL * AMinutes, ACallBack, AStopped);
	}

	// запустить цикл перебора счетчиков
	inline void Start(void) { 
		factive = true; 
	};

	// остановить цикл перебора счётчиков
	inline void Stop(void) { 
		factive = false; 
	};

	// отдает true - если цикл перебора запущен
	inline bool isActive(void) const { 
		return factive; 
	};

	// функции для работы с конкретным счётчиком в списке, по его Handle

	// запустить счётчик с номером hnd
	void Start(THandle hnd) {
		if (isValid(hnd)) Items[hnd]->Start();
	}

	// остановить счётчик hnd
	void Stop(THandle hnd) {
		if (isValid(hnd)) Items[hnd]->Stop();
	}

	// Перезапустить счётчик hnd сначала
	void Reset(THandle hnd) {
		if (isValid(hnd)) Items[hnd]->Reset();
	}

	// Установить счетчику hnd новый интервал счёта
	void setNewInterval(THandle hnd, uint32_t anewinterval) {
		if (isValid(hnd)) Items[hnd]->setInterval(anewinterval);
	}
	
	// проверить, активен ли счётчик hnd
	bool isActive(THandle hnd) {
		if (isValid(hnd)) return Items[hnd]->isActive();
		return false;
	}

	// остановить и удалить счётчик hnd из массива
	void Delete(THandle hnd) {
		if (!isValid(hnd)) return;
		PCounterDown cntptr = Items[hnd];
		if (cntptr != NULL) cntptr->Stop();
		delete(cntptr);
		Items[hnd] = NULL;
		if (--fcount == 0) Stop(); // Если счётчиков не осталось - остановить цикл перебора
	}

	// отдает количество добавленных в список щёччиков
	bsize_t getCount() const {
		return fcount;
	};


	// сюда приходят тики от аппаратного таймера. Если список активен, запускается цикл
	// перебора счетчиков в списке.  На каждом тике каждый добавленный счётчик уменьшается на 1
	void Tick(void) {

		if (isActive()) {
			for (bsize_t i = 0; i < MAXTIMERSCOUNT; i++)
				if (Items[i] != NULL) (*Items[i])--;
		}

	}

	uint32_t getCount(THandle hnd) {
		if (isValid(hnd)) return Items[hnd]->getCount();
		return 0;
	}

};


