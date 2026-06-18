/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Direct NSC veneers for ret2ns security evaluation.
 */

#pragma clang optimize off

#include <arm_cmse.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "security_defs.h"
#include "tfm_hal_device_header.h"

#define NONSECURE_START (0x00200000u)
#define MAX_LEN 8
#define _TIME_STAMP "UNKNOWN"
#define _SYSTEM_STATUS "OK"

int32_t _driver_LCD_ready(void) __attribute__((optnone));
int32_t _driver_LCD_ready(void)
{
    return 1;
}

int32_t _driver_LCD_print(char *msg) __attribute__((optnone));
int32_t _driver_LCD_print(char *msg)
{
    printf("%s\r\n", msg);
    return 0;
}

void default_callback(void);
void default_callback(void)
{
    __BKPT(0);
}

typedef int __attribute__((cmse_nonsecure_call)) ret2ns_ns_callback_t(void);
static ret2ns_ns_callback_t *fp = (ret2ns_ns_callback_t *)default_callback;

__tz_c_veneer
int32_t ret2ns_register_callback_nsc(ret2ns_ns_callback_t *callback)
{
    cmse_address_info_t tt_payload = cmse_TTA_fptr(callback);

    if (!tt_payload.flags.nonsecure_read_ok)
    {
        return -1;
    }

    fp = cmse_nsfptr_create(callback);
    return 0;
}

__tz_c_veneer
int32_t print_LCD_nsc(char *msg)
{
    printf("secure state\n");
    printf("msg ptr : %p\nmsg size : %u\n", msg, sizeof(msg));
    char buf[MAX_LEN] = {0};
    int32_t val = -1;
    if (_driver_LCD_ready())
    {
        // sprintf(buf, "%s %s: %c%c%c%c", _TIME_STAMP, _SYSTEM_STATUS, msg[0], msg[1], msg[2], msg[3]);
        memcpy(buf, msg, 28);

        val = _driver_LCD_print(buf);
    }
    return val;
}

__tz_c_veneer
int32_t blxns_nsc(char *msg)
{
    printf("secure state\n");
    printf("msg ptr : %p\nmsg size : %u\n", msg, sizeof(msg));
    int32_t val = -1;
    int driver_status;
    ret2ns_ns_callback_t *check_fp;
    // if (fp == NULL)
    // {
    //     return val;
    // }

    check_fp = cmse_nsfptr_create(fp);

    char buf[MAX_LEN];
    // sprintf(buf, "%s %s: %c%c%c%c", _TIME_STAMP, _SYSTEM_STATUS, msg[0], msg[1], msg[2], msg[3]);
    memcpy(buf, msg, 28);

    driver_status = check_fp();
    if (driver_status == 0)
    {
        val = _driver_LCD_print(buf);
    }
    return val;
}

#pragma clang optimize on
