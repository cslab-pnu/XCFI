#ifndef FAKE_AVR_H
#define FAKE_AVR_H

#include <stdint.h>

// 1. 가짜 레지스터 정의 (컴파일 통과용)
extern volatile uint8_t  TCCR1A, TCCR1B, TCCR2, TCNT1, TCNT1L, TIFR, TIMSK;
extern volatile uint8_t  DDRB, PINB, PB0;
extern volatile uint8_t  TICIE1, ICES1, ICNC1, TOV2;

extern volatile uint8_t EIMSK, EIFR, INT4, INTF0;
extern volatile uint8_t ck_a, ck_b; // 모뎀 체크섬용

// 2. AVR 비트 조작 매크로 정의
#define _BV(bit) (1 << (bit))
#define sbi(sfr, bit) (_BV(bit))
#define cbi(sfr, bit) (~_BV(bit))
#define bit_is_set(sfr, bit) (sfr & _BV(bit))
#define bit_is_clear(sfr, bit) (!(sfr & _BV(bit)))

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#endif
