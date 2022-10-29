/*
  wiring_private.h - Internal header file.
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
*/

/* #ifndef WiringPrivate_h
#define WiringPrivate_h

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdarg.h>

#include "Arduino.h"
*/
#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )

// the prescaler is set so that timer2 ticks every .. clock cycles, and the
// the overflow handler is called every 256 ticks.
#define PRESCALER0 (0)
#define PRESCALER8 (1<<3)
#define PRESCALER32 (1<<5)
#define PRESCALER64 (1<<6)
#define PRESCALER128 (1<<7)
#define PRESCALER256 (1<<8)
#define PRESCALER1024 (1<<10)

#define PRESCALER PRESCALER32
#define MICROSECONDS_PER_TIMER2_OVERFLOW (clockCyclesToMicroseconds(PRESCALER * 256))

// the whole number of milliseconds per timer2 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER2_OVERFLOW / 1000)

// the fractional number of milliseconds per timer2 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER2_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void timer_init();

unsigned long micros();

unsigned long millis();
