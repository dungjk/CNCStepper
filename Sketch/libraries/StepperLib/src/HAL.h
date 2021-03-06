////////////////////////////////////////////////////////
/*
  This file is part of CNCLib - A library for stepper motors.

  Copyright (c) 2013-2018 Herbert Aitenbichler

  CNCLib is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  CNCLib is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  http://www.gnu.org/licenses/
*/
////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////

#include <Arduino.h>
#include "ConfigurationStepperLib.h"

//////////////////////////////////////////

#ifdef _MSC_VER

#else

#define strcpy_s strcpy
#define strncpy_s strncpy

#define _itoa(a,b,c) itoa(a,b,c)
#define _ltoa(a,b,c) ltoa(a,b,c)

#ifndef CRITICAL_SECTION_START
#define CRITICAL_SECTION_START  irqflags_t _sreg = SREG; cli();
#define CRITICAL_SECTION_END    SREG = _sreg;
#endif //CRITICAL_SECTION_START

#endif

////////////////////////////////////////////////////////

#define TIMER0VALUE(freq)	((timer_t)((unsigned long)TIMER0FREQUENCE/(unsigned long)freq))
#define TIMER1VALUE(freq)	((timer_t)((unsigned long)TIMER1FREQUENCE/(unsigned long)freq))
#define TIMER2VALUE(freq)	((timer_t)((unsigned long)TIMER2FREQUENCE/(unsigned long)freq))
//#define TIMER3VALUE(freq)	((timer_t)((unsigned long)TIMER3FREQUENCE/(unsigned long)freq))
//#define TIMER4VALUE(freq)	((timer_t)((unsigned long)TIMER4FREQUENCE/(unsigned long)freq))
//#define TIMER5VALUE(freq)	((timer_t)((unsigned long)TIMER5FREQUENCE/(unsigned long)freq))

#define TIMER2MICROSEC					(1.0/(float) TIMER2FREQUENCE*1000000.0)
#define TIMER2VALUEFROMMICROSEC(msec)	((timer_t)((msec) / TIMER2MICROSEC))

////////////////////////////////////////////////////////

#if defined(__SAM3X8E__)

typedef uint32_t pin_t;

#define ALWAYSINLINE		__attribute__((__always_inline__)) 
#define ALWAYSINLINE_SAM	__attribute__((__always_inline__)) 
#define ALWAYSINLINE_AVR
#define NEVER_INLINE		__attribute__((__noinline__))
#define NEVER_INLINE_AVR	__attribute__((__noinline__))
#define NEVER_INLINE_SAM
#define ALIGN_WORD			__attribute__((aligned (4)))

#elif defined(__SAMD21G18A__)

typedef uint32_t pin_t;

#define ALWAYSINLINE		__attribute__((__always_inline__)) 
#define ALWAYSINLINE_SAM	__attribute__((__always_inline__)) 
#define ALWAYSINLINE_AVR
#define NEVER_INLINE		__attribute__((__noinline__))
#define NEVER_INLINE_AVR	__attribute__((__noinline__))
#define NEVER_INLINE_SAM
#define ALIGN_WORD			__attribute__((aligned (4)))

#define irqflags_t uint8_t

#elif defined(__AVR_ARCH__)

#define ALWAYSINLINE		__attribute__((__always_inline__)) 
#define ALWAYSINLINE_AVR	__attribute__((__always_inline__)) 
#define ALWAYSINLINE_SAM
#define NEVER_INLINE		__attribute__((__noinline__))
#define NEVER_INLINE_AVR	__attribute__((__noinline__))
#define NEVER_INLINE_SAM
#define ALIGN_WORD			__attribute__((aligned (2)))

#define irqflags_t uint8_t
typedef uint8_t pin_t;

#else

#define ALWAYSINLINE
#define ALWAYSINLINE_AVR
#define NEVER_INLINE
#define NEVER_INLINE_AVR
#define NEVER_INLINE_SAM
#define ALIGN_WORD

#define irqflags_t uint8_t
typedef uint8_t pin_t;

#endif


////////////////////////////////////////////////////////

class CHAL
{
public:

	typedef void(*HALEvent)();

	// min 8 bit, AVR: HW Timer 0
	static void InitTimer0(HALEvent evt);
	static void RemoveTimer0();
	static void StartTimer0(timer_t timer);
	static void StopTimer0();

	// min 16 bit (AVR: 2MHZ, HW Timer1) 
	static void InitTimer1OneShot(HALEvent evt);
	static void RemoveTimer1();
	static void StartTimer1OneShot(timer_t timer);
	static void StopTimer1();

	static HALEvent _TimerEvent0;
	static HALEvent _TimerEvent1;

#if !defined( __AVR_ATmega328P__)

	// min 8 bit, (AVR: HW Timer5) 
	static void InitTimer2OneShot(HALEvent evt);
	static void RemoveTimer2();
	static void StartTimer2OneShot(timer_t timer);
	static void ReStartTimer2OneShot(timer_t timer);
	static void StopTimer2();

	static HALEvent _TimerEvent2;

#endif

#if defined(__SAM3X8E__) || defined(__SAMD21G18A__)

	static void BackgroundRequest();
	static void InitBackground(HALEvent evt);

	static HALEvent _BackgroundEvent;

#endif

#if defined(__SAMD21G18A__)
	
#define EEPROM_SIZE	512		// must be x*256

	static const uint8_t _flashStorage[EEPROM_SIZE] __attribute__((__aligned__(256)));
	static uint8_t _flashBuffer[EEPROM_SIZE] __attribute__((__aligned__(4)));

	static void FlashWriteWords(uint32_t *flash_ptr, const uint32_t *data, uint32_t nwords);
	static void FlashErase(void *flash_ptr, uint32_t size);
	static void FlashEraseRow(void *flash_ptr);
	static void FlashRead(const void *flash_ptr, void *data, uint32_t size);

#endif

	static inline void DisableInterrupts() ALWAYSINLINE;
	static inline void EnableInterrupts() ALWAYSINLINE;

	static inline void DelayMicroseconds(unsigned int us) ALWAYSINLINE ;
	static inline void DelayMicroseconds0250() ALWAYSINLINE;		// delay 1/4 us (4 nop on AVR)
	static inline void DelayMicroseconds0312() ALWAYSINLINE;		// delay 0.312us (5 nop on AVR)
	static inline void DelayMicroseconds0375() ALWAYSINLINE;		// delay 0.312us (6 nop on AVR)
	static inline void DelayMicroseconds0438() ALWAYSINLINE;		// delay 0.312us (7 nop on AVR)
	static inline void DelayMicroseconds0500() ALWAYSINLINE;		// delay 1/2 (8 nop on AVR)

	static inline irqflags_t GetSREG() ALWAYSINLINE;
	static inline void SetSREG(irqflags_t) ALWAYSINLINE;

	static inline void pinMode(pin_t pin, uint8_t mode);
	static inline void pinModeInput(pin_t pin) NEVER_INLINE_AVR;
	static inline void pinModeInputPullUp(pin_t pin) NEVER_INLINE_AVR;
	static inline void pinModeOutput(pin_t pin) NEVER_INLINE_AVR;

	static void digitalWrite(pin_t pin, uint8_t lowOrHigh);
	static uint8_t digitalRead(pin_t pin);

	static void analogWrite8(pin_t pin, uint8_t val);

	static unsigned short analogRead(pin_t pin);

	static void attachInterruptPin(pin_t pin, void(*userFunc)(void), int mode) ALWAYSINLINE;

	static bool HaveEeprom() ALWAYSINLINE;
	static void InitEeprom() ALWAYSINLINE;
	static void FlushEeprom() ALWAYSINLINE;
	static bool NeedFlushEeprom() ALWAYSINLINE;

	static void eeprom_write_dword(uint32_t *  __p, uint32_t  	__value) ALWAYSINLINE;
	static uint32_t eeprom_read_dword(const uint32_t * __p) ALWAYSINLINE;
	//static uint8_t eeprom_read_byte(const uint8_t * __p) ALWAYSINLINE;

	static uint32_t* GetEepromBaseAdr() ALWAYSINLINE;

#if defined(_MSC_VER)

	static void SetEepromFilename(char* filename) { _eepromFileName = filename; }

private:

	static char* _eepromFileName;
	static uint32_t _eepromBuffer[2048];

#else

#endif
};

//////////////////////////////////////////

class CCriticalRegion
{
private:
	irqflags_t _sreg;

public:

	inline CCriticalRegion() ALWAYSINLINE :_sreg(CHAL::GetSREG()) {  CHAL::DisableInterrupts(); };
	inline ~CCriticalRegion() ALWAYSINLINE { CHAL::SetSREG(_sreg); }
};

//////////////////////////////////////////

#include "HAL_AVR.h"
#include "HAL_Sam3x8e.h"
#include "HAL_SamD21g18a.h"
#include "HAL_Msvc.h"

//////////////////////////////////////////
