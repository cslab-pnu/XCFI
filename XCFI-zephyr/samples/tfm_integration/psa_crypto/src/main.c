/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>

#include "tfm_ns_interface.h"
#include "psa_attestation.h"
#include "psa_crypto.h"
#include "util_app_cfg.h"
#include "util_app_log.h"
#include "util_sformat.h"

#define DEMCR (*(volatile unsigned *) 0xE000EDFC)
#define DWT_CTRL (*(volatile unsigned *) 0xE0001000)
#define DWT_CYCCNT (*(volatile unsigned *) 0xE0001004)
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)

//extern unsigned sys_clock_cnt;

/** Declare a reference to the application logging interface. */
LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

/* Create an instance of the system config struct for the application. */
static struct cfg_data cfg;

int main(void)
{
	/* Initialise the logger subsys and dump the current buffer. */
	log_init();

    DEMCR |= (1 << 24);
    DWT_CYCCNT = 0;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    //unsigned start = DWT_CYCCNT;
    /*__asm__ volatile(
        "mov r10, #0\n"
    );*/
	/* Load app config struct from secure storage (create if missing). */
	if (cfg_load_data(&cfg)) {
		LOG_ERR("Error loading/generating app config data in SS.");
	}

	/* Get the entity attestation token (requires ~1kB stack memory!). */
	att_test();

	/* Crypto tests */
	crp_test();
    crp_test_rng();

	/* Generate Certificate Signing Request using Mbed TLS */
	crp_generate_csr();

    //unsigned end = DWT_CYCCNT;
    //printf("psa-crypto cycle count : %u\n", end-start);

    /*unsigned r10 = 0;
    __asm__ volatile(
        "mov %0, r10\n"
        :"=r"(r10)::
    );
    printf("psa-crypto instruction count : %u\n", r10);*/

	/* Dump any queued log messages, and wait for system events. */
	al_dump_log();

	LOG_INF("Done.");

	return 0;
}
