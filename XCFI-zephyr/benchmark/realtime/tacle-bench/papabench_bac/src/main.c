/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdbool.h>

// 1. 가짜 AVR 레지스터 실체 정의 (컴파일러가 변수처럼 취급하도록)
volatile uint8_t TCCR1A, TCCR1B, TCCR2, TCNT1, TCNT1L, TIFR, TIMSK;
volatile uint8_t DDRB, PINB, PB0;
volatile uint8_t TICIE1, ICES1, ICNC1, TOV2;

// 2. PapaBench 내부 전역 변수들 (fbw.c 등에서 사용)
uint8_t spi_was_interrupted = 0;
uint8_t last_radio = 0;
uint8_t last_radio_contains_avg_channels = 0;
uint8_t to_mega128 = 0; // 혹은 구조체라면 해당 타입으로 정의 필요

// 3. 링커가 찾지 못한 함수들 (빈 껍데기 Stub)
void spi_reset(void) {}
void uart_init_tx(void) {}
void uart_print_string(const char* s) {}
void fbw_adc_init(void) {}
void fbw_adc_buf_channel(uint8_t c, void* b, uint8_t s) {}
void servo_init(void) {}
void fbw_spi_init(void) {}
void servo_transmit(void) {}

uint8_t last_radio_from_ppm = 0;
bool ppm_valid = false;
bool mega128_receive_valid = false;

// from_mega128은 보통 구조체 타입입니다.
// 일단 링크 통과를 위해 메모리 공간만 할당합니다.
uint8_t from_mega128[32];

// 추가로 발견된 미정의 함수들
void servo_set(uint8_t servo_idx, uint16_t value) {}

extern void fbw_init(void);
extern void fbw_schedule(void);

uint8_t _1Hz = 0;
uint8_t _20Hz = 0;

/* src/main.c 에 추가 */
// Autopilot 레지스터 실체
volatile uint8_t EIMSK, EIFR, INT4, INTF0;
volatile uint8_t ck_a, ck_b;

// Autopilot에서 사용하는 추가 변수들 (에러 발생 시 추가)
uint8_t estimator_ir, estimator_rad, estimator_rad_of_ir;
uint8_t ir_roll_neutral, ir_pitch_neutral, ir_contrast;

int main(void)
{
#if defined(fbw)
	/*printf("Starting PapaBench in SINGLE mode on EK-RA8M1...\n");

	fbw_init();

	while(1) {
		fbw_schedule();

		_1Hz++;
		_20Hz++;

		if (_1Hz >= 60) { _1Hz = 0; }
		if (_20Hz >= 3) { _20Hz = 0; }

		k_msleep(16);
	}*/

    while(1) {
        uint32_t start = k_cycle_get_32();

        // PapaBench의 핵심 로직 수행
        fbw_schedule();

        uint32_t end = k_cycle_get_32();
        uint32_t elapsed = end - start;

        // 1초에 한 번씩만 '실시간 성능 리포트' 출력
        static int cnt = 0;
        if (cnt++ % 60 == 0) {
            uint32_t us = k_cyc_to_us_near32(elapsed);
            printf("[Report] Execution: %u cycles (%u us) | Deadline: 16.6ms\n", elapsed, us);
        }

        k_msleep(16); // 60Hz 제어 루프
    }
#elif defined(autopilot)
	autopilot_task();
#else
	printf("Select [fbw/ap]\n");
#endif

	return 0;
}
