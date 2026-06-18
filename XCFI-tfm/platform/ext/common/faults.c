/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "tfm_hal_device_header.h"
#include "utilities.h"
#include "tfm_spm_log.h"
/* "exception_info.h" must be the last include because of the IAR pragma */
#include "exception_info.h"

void C_HardFault_Handler(void)
{
    /* A HardFault may indicate corruption of secure state, so it is essential
     * that Non-secure code does not regain control after one is raised.
     * Returning from this exception could allow a pending NS exception to be
     * taken, so the current solution is not to return.
     */
    tfm_core_panic();
}

__attribute__((naked)) void HardFault_Handler(void)
{
    EXCEPTION_INFO();

    __ASM volatile(
        "bl        C_HardFault_Handler     \n"
        "b         .                       \n"
    );
}

void C_MemManage_Handler(void)
{
    /* A MemManage fault may indicate corruption of secure state, so it is
     * essential that Non-secure code does not regain control after one is
     * raised. Returning from this exception could allow a pending NS exception
     * to be taken, so the current solution is to panic.
     */
    tfm_core_panic();
}

__attribute__((naked)) void MemManage_Handler(void)
{
    EXCEPTION_INFO();

    __ASM volatile(
        "bl        C_MemManage_Handler     \n"
        "b         .                       \n"
    );
}

void C_BusFault_Handler(void)
{
    /* A BusFault may indicate corruption of secure state, so it is essential
     * that Non-secure code does not regain control after one is raised.
     * Returning from this exception could allow a pending NS exception to be
     * taken, so the current solution is to panic.
     */
    tfm_core_panic();
}

__attribute__((naked)) void BusFault_Handler(void)
{
    EXCEPTION_INFO();

    __ASM volatile(
        "bl        C_BusFault_Handler      \n"
        "b         .                       \n"
    );
}

void C_SecureFault_Handler(void)
{
    /* A SecureFault may indicate corruption of secure state, so it is essential
     * that Non-secure code does not regain control after one is raised.
     * Returning from this exception could allow a pending NS exception to be
     * taken, so the current solution is to panic.
     */
    tfm_core_panic();
}

__attribute__((naked)) void SecureFault_Handler(void)
{
    EXCEPTION_INFO();

    __ASM volatile(
        "bl        C_SecureFault_Handler   \n"
        "b         .                       \n"
    );
}

__attribute__((no_instrument_function))
void C_UsageFault_Handler(void)
{
#ifdef TFM_EXCEPTION_INFO_DUMP
    struct exception_info_t ctx;
    uint32_t ufsr;
    uint32_t ufsr_ns;
    uint32_t invstate;

    tfm_exception_info_get_context(&ctx);
    ufsr = (ctx.CFSR & SCB_CFSR_USGFAULTSR_Msk) >> SCB_CFSR_USGFAULTSR_Pos;
    ufsr_ns = (ctx.CFSR_NS & SCB_CFSR_USGFAULTSR_Msk) >> SCB_CFSR_USGFAULTSR_Pos;
    invstate = SCB_CFSR_INVSTATE_Msk >> SCB_CFSR_USGFAULTSR_Pos;

    SPMLOG_ERRMSGVAL("[XCFI] CFSR: ", ctx.CFSR);
    SPMLOG_ERRMSGVAL("[XCFI] UFSR: ", ufsr);
    SPMLOG_ERRMSGVAL("[XCFI] CFSR_NS: ", ctx.CFSR_NS);
    SPMLOG_ERRMSGVAL("[XCFI] UFSR_NS: ", ufsr_ns);
    if (((ufsr | ufsr_ns) & invstate) != 0U) {
        SPMLOG_ERRMSG("[XCFI] UFSR.INVSTATE set; PAC/AUT failure is possible.\r\n");
    } else {
        SPMLOG_ERRMSG("[XCFI] UFSR.INVSTATE is clear; PAC/AUT failure is not confirmed by fault status.\r\n");
    }
#else
    SPMLOG_ERRMSG("[XCFI] Secure UsageFault occurred; enable TFM_EXCEPTION_INFO_DUMP to inspect UFSR.\r\n");
#endif

    tfm_core_panic();
}

__attribute__((naked)) void UsageFault_Handler(void)
{
    EXCEPTION_INFO();

    __ASM volatile(
        "bl        C_UsageFault_Handler   \n"
        "b         .                      \n"
    );
}
