/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <tfm_ns_interface.h>

#include "dummy_partition.h"

#define DEMCR (*(volatile unsigned *) 0xE000EDFC)
#define DWT_CTRL (*(volatile unsigned *) 0xE0001000)
#define DWT_CYCCNT (*(volatile unsigned *) 0xE0001004)
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)

//extern unsigned sys_clock_cnt;

int main(void)
{
    DEMCR |= (1 << 24);
    DWT_CYCCNT = 0;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    //unsigned start = DWT_CYCCNT;

    /*__asm__ volatile(
        "mov r10, #0\n"
    );*/
	uint8_t digest[32];

	for (int key = 0; key < 6; key++) {
		psa_status_t status = dp_secret_digest(key, digest, sizeof(digest));

		if (status == PSA_ERROR_INVALID_ARGUMENT && key == 5) {
			printk("No valid secret for key, received expected error code\n");
		} else if (status != PSA_SUCCESS) {
			printk("Status: %d\n", status);
		} else {
			printk("Digest: ");
			for (int i = 0; i < 32; i++) {
				printk("%02x", digest[i]);
			}
			printk("\n");
		}
	}

    //unsigned end = DWT_CYCCNT;
    //printf("tfm_secure_partition cycle count : %u\n", end-start);

    /*unsigned r10 = 0;
    __asm__ volatile(
        "mov %0, r10\n"
        :"=r"(r10)::
    );
    printf("tfm_secure_partition instruction count : %u\n", r10);*/

    //printf("sys_clock_isr count : %u\n", sys_clock_cnt);
	return 0;
}
