/*
 * Copyright (c) 2019,2020, 2021, 2023 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "tfm_ns_interface.h"
#ifdef TFM_PSA_API
#include "psa_manifest/sid.h"
#include "psa/crypto.h"
#endif

#define DEMCR (*(volatile unsigned *) 0xE000EDFC)
#define DWT_CTRL (*(volatile unsigned *) 0xE0001000)
#define DWT_CYCCNT (*(volatile unsigned *) 0xE0001004)
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)

//extern unsigned sys_clock_cnt;

/**
 * \brief Retrieve the version of the PSA Framework API.
 *
 * \note This is a functional test only and doesn't
 *       mean to test all possible combinations of
 *       input parameters and return values.
 */
static void tfm_get_version(void)
{
	uint32_t version;

	version = psa_framework_version();
	if (version == PSA_FRAMEWORK_VERSION) {
		printk("The version of the PSA Framework API is %d.\n",
		       version);
	} else {
		printk("The version of the PSA Framework API is not valid!\n");
		return;
	}
}

#ifdef TFM_PSA_API
/**
 * \brief Retrieve the minor version of a RoT Service.
 */
static void tfm_get_sid(void)
{
	uint32_t version;

	version = psa_version(TFM_CRYPTO_SID);
	if (version == PSA_VERSION_NONE) {
		printk("RoT Service is not implemented or caller is not ");
		printk("authorized to access it!\n");
		return;
	}

	/* Valid version number */
	printk("The PSA Crypto service minor version is %d.\n", version);
}

/**
 * \brief Generates random data using the TF-M crypto service.
 */
void tfm_psa_crypto_rng(void)
{
	psa_status_t status;
	uint8_t outbuf[256] = { 0 };

	status = psa_generate_random(outbuf, 256);
	printk("Generating 256 bytes of random data:");
	for (uint16_t i = 0; i < 256; i++) {
		if (!(i % 16)) {
			printk("\n");
		}
		printk("%02X ", (uint8_t)(outbuf[i] & 0xFF));
	}
	printk("\n");
}
#endif

int main(void)
{
	printk("TF-M IPC on %s\n", CONFIG_BOARD);

    DEMCR |= (1 << 24);
    DWT_CYCCNT = 0;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    unsigned start = DWT_CYCCNT;

	tfm_get_version();
#ifdef TFM_PSA_API
	tfm_get_sid();
	tfm_psa_crypto_rng();
#endif

    unsigned end = DWT_CYCCNT;
    printf("tfm_ipc cycle count : %u\n", end-start);

	return 0;
}
