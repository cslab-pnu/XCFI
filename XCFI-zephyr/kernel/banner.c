/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/version.h>

#if defined(CONFIG_BOOT_DELAY) && (CONFIG_BOOT_DELAY > 0)
#define DELAY_STR STRINGIFY(CONFIG_BOOT_DELAY)
#define BANNER_POSTFIX " (delayed boot " DELAY_STR "ms)"
#else
#define BANNER_POSTFIX ""
#endif /* defined(CONFIG_BOOT_DELAY) && (CONFIG_BOOT_DELAY > 0) */

#ifndef BANNER_VERSION
#if defined(BUILD_VERSION) && !IS_EMPTY(BUILD_VERSION)
#define BANNER_VERSION STRINGIFY(BUILD_VERSION)
#else
#define BANNER_VERSION KERNEL_VERSION_STRING
#endif /* BUILD_VERSION */
#endif /* !BANNER_VERSION */

void boot_banner(void)
{
#if defined(CONFIG_BOOT_DELAY) && (CONFIG_BOOT_DELAY > 0)
#ifdef CONFIG_BOOT_BANNER
	printk("***** delaying boot " DELAY_STR "ms (per build configuration) *****\n");
#endif /* CONFIG_BOOT_BANNER */
	k_busy_wait(CONFIG_BOOT_DELAY * USEC_PER_MSEC);
#endif /* defined(CONFIG_BOOT_DELAY) && (CONFIG_BOOT_DELAY > 0) */

#if defined(CONFIG_BOOT_CLEAR_SCREEN)
	 /* \x1b[ = escape sequence
	  * 3J = erase scrollback
	  * 2J = erase screen
	  * H = move cursor to top left
	  */
	printk("\x1b[3J\x1b[2J\x1b[H");
#endif /* CONFIG_BOOT_CLEAR_SCREEN */

    // non-secure 0x28080400
    // secure 0x38080400
    /*unsigned resp = 0;
    __asm__ volatile(
        "movw r9, #0x0650\n"
        "movt r9, #0x3800\n"
        "tt   %0, r9"
        :"=r"(resp)::"r9"
    );

    printk("****** secure tt resp : 0x%08x ******\n", resp);

    __asm__ volatile(
        "movw r9, #0x0400\n"
        "movt r9, #0x2808\n"
        "tt   %0, r9"
        :"=r"(resp)::"r9"
    );

    printk("****** non-secure tt resp : 0x%08x ******\n", resp);
*/



#ifdef CONFIG_BOOT_BANNER
	printk("*** " CONFIG_BOOT_BANNER_STRING " " BANNER_VERSION BANNER_POSTFIX " ***\n");
#ifdef ENABLE_XCFI
	printk("*** XCFI enabled ***\n");
#endif
#endif /* CONFIG_BOOT_BANNER */
}
