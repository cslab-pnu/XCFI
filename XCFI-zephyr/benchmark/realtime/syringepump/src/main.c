/*
 * Copyright (c) 2024, Your Name <your.email@example.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Syringe Pump Control for Zephyr RTOS (Benchmark Version)
 *
 * This code is a port of a RIOT OS benchmark sketch for controlling a stepper
 * motor-based syringe pump and measuring performance characteristics.
 * It does not require an LCD or keypad shield.
 *
 * --- Zephyr Configuration Requirements ---
 *
 * 1. prj.conf:
 *    Make sure your prj.conf file includes these configurations.
 *
 *    CONFIG_GPIO=y
 *    CONFIG_UART_CONSOLE=y
 *    CONFIG_FPU=y
 *    CONFIG_CBPRINTF_FP_SUPPORT=y
 *    CONFIG_ARCH_HAS_DWT=y
 *
 * 2. app.overlay (Devicetree Overlay):
 *    Define the motor control pins for your board.
 *
 *    / {
 *        aliases {
 *            motor-dir = &motor_dir_pin;
 *            motor-step = &motor_step_pin;
 *            led0 = &led0_pin; // For visual feedback
 *        };
 *
 *        gpios {
 *            compatible = "gpio-leds";
 *            motor_dir_pin: motor_dir_pin {
 *                gpios = <&gpio0 2 0>; // Example: Arduino D2
 *            };
 *            motor_step_pin: motor_step_pin {
 *                gpios = <&gpio0 3 0>; // Example: Arduino D3
 *            };
 *            led0_pin: led0_pin {
 *                gpios = <&gpio0 13 0>; // Example: Arduino on-board LED
 *                label = "User LED";
 *            };
 *        };
 *    };
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <cmsis_core.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* -- Constants -- */
#define SYRINGE_VOLUME_ML 30.0
#define SYRINGE_BARREL_LENGTH_MM 80.0

#define THREADED_ROD_PITCH 1.25
#define STEPS_PER_REVOLUTION 200.0
#define MICROSTEPS_PER_STEP 16.0

#define SPEED_MICROSECONDS_DELAY 100 // longer delay = lower speed

/* Calculate steps per unit */
#define USTEPS_PER_ML ((MICROSTEPS_PER_STEP * STEPS_PER_REVOLUTION * SYRINGE_BARREL_LENGTH_MM) / (SYRINGE_VOLUME_ML * THREADED_ROD_PITCH))

/* -- Pin definitions from Devicetree -- */
static const struct gpio_dt_spec motor_dir_pin = GPIO_DT_SPEC_GET(DT_ALIAS(motor_dir), gpios);
static const struct gpio_dt_spec motor_step_pin = GPIO_DT_SPEC_GET(DT_ALIAS(motor_step), gpios);
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* -- Enums and constants -- */
enum { PUSH, PULL }; // syringe movement direction
enum { MAIN, BOLUS_MENU }; // UI states

const int mLBolusStepsLength = 9;
static const float mLBolusSteps[9] = {0.001, 0.005, 0.010, 0.050, 0.100, 0.500, 1.000, 5.000, 10.000};

/* -- Default Parameters -- */
float mLBolus = 0.500; // default bolus size
float mLUsed = 0.0;
int mLBolusStepIdx = 3; // 0.05 mL increments at first
float mLBolusStep;

// menu stuff
int uiState = MAIN;

// serial
char serialStr[40] = {0};
bool serialStrReady = false;

/* Function prototypes */
void processSerial(void);
void bolus(int direction);
void updateScreen(void);

#define DEMCR (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t *) 0xE0001004)

void enable_dwt(){
    if(DWT_CTRL != 0){
        DEMCR |= (1 << 24);
    // uint64_t start_cycles = k_cycle_get_64();
        DWT_CYCCNT = 0;
        DWT_CTRL |= 1;
    }
}

void bolus(int direction) {
    long steps = (long)(mLBolus * USTEPS_PER_ML);
    if (direction == PUSH) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_set_dt(&motor_dir_pin, 1);
        mLUsed += mLBolus;
    } else if (direction == PULL) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_set_dt(&motor_dir_pin, 0);
        mLUsed -= mLBolus;
        if (mLUsed < 0) {
            mLUsed = 0;
        }
    }

    uint32_t usDelay = SPEED_MICROSECONDS_DELAY;
    for (long i = 0; i < steps; i++) {
        gpio_pin_set_dt(&motor_step_pin, 1);
        k_busy_wait(usDelay);
        gpio_pin_set_dt(&motor_step_pin, 0);
        k_busy_wait(usDelay);
    }
}

void updateScreen(void) {
    if (uiState == MAIN) {
        // printk("\rUsed %.3f mL | Bolus %.3f mL", mLUsed, mLBolus);
        printk("\rUsed %d mL | Bolus %d mL\n", (int)mLUsed, (int)mLBolus);
    } else if (uiState == BOLUS_MENU) {
        printk("\rMenu> BolusStep: %.3f\n", mLBolusStep);
    }
}

void processSerial(void) {
    if (serialStr[0] == '+') {
        bolus(PUSH);
        updateScreen();
    } else if (serialStr[0] == '-') {
        bolus(PULL);
        updateScreen();
    } else {
        int uLbolus = atoi(serialStr);
        if (uLbolus != 0) {
            mLBolus = (float)uLbolus / 1000.0;
            updateScreen();
        } else {
            printk("Invalid command: [%s]\n", serialStr);
        }
    }
    serialStrReady = false;
    memset(serialStr, 0, sizeof(serialStr));
}

void loop(int count) {
	mLBolus = 0.001 * count;
	serialStr[0] = '+';
	serialStrReady = true;

	if (serialStrReady) {
		processSerial();
	}
}

int main(void) {
    printk("SyringePump v2.0\n");

    enable_dwt();

    uint32_t freq = sys_clock_hw_cycles_per_sec();
    printk("CPU freq: %u Hz\n", freq);

    /* Initialize mLBolusStep */
    mLBolusStep = mLBolusSteps[mLBolusStepIdx];

    /* Motor Setup */
    if (!device_is_ready(motor_dir_pin.port) || !device_is_ready(motor_step_pin.port) || !device_is_ready(led0.port)) {
        printk("GPIO device not ready.\n");
        return 0;
    }
    gpio_pin_configure_dt(&motor_dir_pin, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&motor_step_pin, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);

    uint32_t start = DWT_CYCCNT;
    int count = 1;
    while (count < 62) {
        count += 10;
        loop(count);
    }
    uint32_t cycles = DWT_CYCCNT - start;

    printf("time cycles: %u\n", cycles);

	printf("=============================================\n\n");

    return 0;
}
