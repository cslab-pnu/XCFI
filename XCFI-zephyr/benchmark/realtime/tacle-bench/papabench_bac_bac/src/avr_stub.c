#pragma once

volatile unsigned int TCCR1A;
volatile unsigned int TCCR1B;
volatile unsigned int TCCR2;
volatile unsigned int TCNT1;
volatile unsigned int TCNT1L;
volatile unsigned int TIFR;
volatile unsigned int TIMSK;
volatile unsigned int OCR1A;

volatile unsigned int SPCR;
volatile unsigned int SPSR;
volatile unsigned int SPDR;

volatile unsigned int PORTB;

#define SPE 6
#define MSTR 4
#define SPR0 0
#define SPIF 7
#define SPIE 7
#define TOV2 6
#define OCF1A 4
#define OCIE1A 4

#define _BV(x) (1 << (x))

#define bit_is_set(reg, bit) ((reg) & (1 << (bit)))
#define sbi(reg, bit) ((reg) |= (1 << (bit)))
#define cbi(reg, bit) ((reg) &= ~(1 << (bit)))
