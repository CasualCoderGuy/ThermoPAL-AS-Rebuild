/*
  wiring.c - Partial implementation of the Wiring API for the ATmega8.
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

//#include "wiring_private.h"
#include "grbl.h"

volatile unsigned long timer2_overflow_count = 0;
volatile unsigned long timer2_millis = 0;
static unsigned char timer2_fract = 0;

ISR(TIMER2_OVF_vect)
{
	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer2_millis;
	unsigned char f = timer2_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer2_fract = f;
	timer2_millis = m;
	timer2_overflow_count++;
}

unsigned long millis()
{
	unsigned long m;
	uint8_t oldSREG = SREG;

	// disable interrupts while we read timer0_millis or we might get an
	// inconsistent value (e.g. in the middle of a write to timer0_millis)
	cli();
	m = timer2_millis;
	SREG = oldSREG;

	return m;
}

unsigned long micros() {
	unsigned long m;
	uint8_t oldSREG = SREG, t;
	
	cli();
	m = timer2_overflow_count;
	t = TCNT2;
	if ((TIFR2 & _BV(TOV2)) && (t < 255))
		m++;
	SREG = oldSREG;

	return ((m << 8) + t) * (PRESCALER / clockCyclesPerMicrosecond());
}

void enable_timer2()
{
#if (PRESCALER == PRESCALER64 || PRESCALER == PRESCALER128 || PRESCALER == PRESCALER256 || PRESCALER == PRESCALER1024)
	sbi(TCCR2B, CS22);
#endif
#if (PRESCALER == PRESCALER8 || PRESCALER == PRESCALER32 || PRESCALER == PRESCALER256 || PRESCALER == PRESCALER1024)
	sbi(TCCR2B, CS21);
#endif
#if (PRESCALER == PRESCALER0 || PRESCALER == PRESCALER32 || PRESCALER == PRESCALER128 || PRESCALER == PRESCALER1024)
	sbi(TCCR2B, CS20);
#endif
}

void disable_timer2()
{
	TCCR2B = 0;
}

void timer_init()
{
	TIMSK2 &= ~((1<<OCIE2B) | (1<<OCIE2A) | (1<<TOIE2)); // Disconnect OC0 outputs and OVF interrupt.
	TCCR2A = 0; // Normal operation
	TCCR2B = 0; // Disable Timer0 until needed
	TIMSK2 |= (1<<TOIE2); // Enable Timer0 overflow interrupt
}
