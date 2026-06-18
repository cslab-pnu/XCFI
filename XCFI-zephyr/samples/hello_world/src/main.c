/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#define DEMCR (*(volatile unsigned *) 0xE000EDFC)
#define DWT_CTRL (*(volatile unsigned *) 0xe0001000)
#define DWT_CYCCNT (*(volatile unsigned *) 0xe0001004)
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)




int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
    DEMCR |= (1 << 24);
    DWT_CYCCNT = 0;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    __asm__ volatile(
        "movw lr, #0xffbd\n"
        "movt lr, #0xffff\n"
        "movw r6, #0x939c\n"
        "movt r6, #0x1100\n"
        :::"r6", "lr"
    );

    unsigned start, end;

/*    __asm__ volatile(
        "isb sy\n"
        "mrs r2, psp\n"
        "ands r3, lr, #0x20\n"
        "itt ne\n"
        "stmdbne r2!, {r4, r5, r6, r7, r8, r9, r10, r11}\n"
        "subne r2, #0x8\n"
        "stm.w r2, {r2, lr}\n"
        "ldm.w r1!, {r2, lr}\n"
        "ands r3, lr, #0x20\n"
        "itt ne\n"
        "addne r2, #0x8\n"
        "ldmne.w r2!, {r4, r5, r6, r7, r8, r9, r10, r11}\n"
        "ldr r3, [r1]\n"
        "msr psp, r2\n"
        "msr psplim, r3\n"
        :::"r2", "r3", "r1"
    );

    unsigned end = DWT_CYCCNT;

    __asm__ volatile(
        "msr psplim, r6\n"
    );

    printf("tfm pendsv interval : %u\n", end - start);
*/
    /*        "tst lr, #0x40\n"
        "it eq\n"
        "pacgeq r12, r10, lr\n"
        "beq 1f\n"
        "ite eq\n"
        "mrseq r6, msp\n"
        "mrsne r6, psp\n"
        "add r6, #20\n"
        "ldmia r6, {r7-r9}\n"
        "eor r7, r7, r8\n"
        "eor r7, r7, r9\n"
        "eor r7, r7, lr\n"
        "pacg r12, r10, r7\n"
        "1:\n"
        :::"r12", "r6", "r7", "r8", "r9"
    );

    unsigned end = DWT_CYCCNT;
    unsigned pro = end - start;

    start = DWT_CYCCNT;

    __asm__ volatile(
        "tst lr, #0x40\n"
        "it eq\n"
        "autgeq r12, r10, lr\n"
        "beq 2f\n"
        "tst lr, #0x4\n"
        "ite eq\n"
        "mrseq r6, msp\n"
        "mrsne r6, psp\n"
        "add r6, #20\n"
        "ldmia r6, {r7-r9}\n"
        "eor r7, r7, r8\n"
        "eor r7, r7, r9\n"
        "eor r7, r7, lr\n"
        "autg r12, r10, r7\n"
        "2:\n"
        "movw r6, #0xbeef\n"
        "movt r6, #0xdead\n"
        "eor r10, r6\n"
        :::"r12", "r6", "r7", "r8", "r9"
    );

    end = DWT_CYCCNT;
    unsigned epi = end - start;

    printf("exception pro : %u\n", pro);
    printf("exception epi : %u\n", epi);
*/
    /*__asm__ volatile(
        "bti\n"
        "ldr r10, [r6, #-4]\n"
        "pacg r8, r10, r6\n"
        :::"r8", "r6"
    );

    unsigned end = DWT_CYCCNT;
    unsigned pro = end - start;

    start = DWT_CYCCNT;
    __asm__ volatile(
        "movw r10, #0xbeef\n"
        "movt r10, #0xdead\n"
        "autg r8, r10, r6\n"
        :::"r8", "r6"
    );

    end = DWT_CYCCNT;
    unsigned epi = end - start;

    start = DWT_CYCCNT;
    __asm__ volatile(
        "mrs r3, PRIMASK\n"
        "cpsid i\n"
        "movw r10, #0xbeef\n"
        "ldr r4, [r6, #-4]\n"
        "uxth.w r4, r4\n"
        "subs.w r4, r4, r10\n"
        "bne.w XCFI_fault\n"
        "msr PRIMASK, r3\n"
        :::"r6", "r3", "r4"
    );

    end = DWT_CYCCNT;
    unsigned ic = end - start;

    start = DWT_CYCCNT;
    __asm__ volatile(
        "ldr r4, [r6, #-4]\n"
        "uxth.w r4, r4\n"
        "subs.w r4, r4, r10\n"
        "bne.w XCFI_fault\n"
        :::"r6", "r3", "r4"
    );

    end = DWT_CYCCNT;
    unsigned sg = end - start;
*/
    start = DWT_CYCCNT;
    __asm__ volatile(
        "mrs r3, PRIMASK\n"
        "cpsid i\n"
        "movw r10, #0xbeef\n"
        "ttt r4, r6\n"
        "cmp r4, #0\n"
        "beq 1f\n"
        "ldr r4, [r6, #-4]\n"
        "uxth.w r4, r4\n"
        "subs.w r4, r4, r10\n"
        "bne.w XCFI_fault\n"
        "1:\n"
        :::"r6", "r7", "r3", "r4"
    );

    end = DWT_CYCCNT;
    unsigned ic_s = end - start;
/*
    printf("pro cycle : %u\n", pro);
    printf("epi cycle : %u\n", epi);
    printf("ic cycle : %u\n", ic);
    printf("sg cycle : %u\n", sg);
*/    printf("ic secure cycle : %u\n", ic_s);

	return 0;
}
