/*
 * Copyright (c) 2018-2024, Arm Limited. All rights reserved.
 * Copyright (c) 2022-2024 Cypress Semiconductor Corporation (an Infineon
 * company) or an affiliate of Cypress Semiconductor Corporation. All rights
 * reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <inttypes.h>

#include "compiler_ext_defs.h"
#include "config_spm.h"
#include "security_defs.h"
#include "region_defs.h"
#include "spm.h"
#include "svc_num.h"
#include "tfm_arch.h"
#include "tfm_hal_device_header.h"
#include "tfm_svcalls.h"
#include "utilities.h"
#include "core_ext.h"
#include "ffm/backend.h"

#if !defined(__ARM_ARCH_8M_MAIN__) && !defined(__ARM_ARCH_8_1M_MAIN__)
#error "Unsupported ARM Architecture."
#endif

/* Delcaraction flag to control the scheduling logic in PendSV. */
uint32_t scheduler_lock = SCHEDULER_UNLOCKED;

/* IAR Specific */
#if defined(__ICCARM__)

#pragma required = scheduler_lock
#pragma required = spm_svc_handler

#if CONFIG_TFM_SPM_BACKEND_IPC == 1

#pragma required = ipc_schedule
#pragma required = backend_abi_entering_spm
#pragma required = backend_abi_leaving_spm

#endif /* CONFIG_TFM_SPM_BACKEND_IPC == 1*/

#endif

#if CONFIG_TFM_SPM_BACKEND_IPC == 1

__naked
void tfm_arch_thread_fn_call(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    __asm volatile(
        "   push    {r4-r6, lr}             \n"
        "   push    {r0-r4, r12}            \n"
        "   cpsid   i                       \n"
        "   isb                             \n"
        "   bl      backend_abi_entering_spm\n" /* r0: new SP, r1: new PSPLIM */
        "   mrs     r5, psplim              \n"
        "   mov     r6, sp                  \n"
        "   cmp     r0, #0                  \n" /* Check whether the caller is
                                                 * NS agent(new SP == 0) or
                                                 * secure partition(new SP != 0)
                                                 */
        "   itttt   ne                      \n"
        "   movne   r2, #0                  \n"
        "   msrne   psplim, r2              \n" /* Clear PSPLIM before setting
                                                 * PSP to a new value. This can
                                                 * avoid potential stack
                                                 * overflow.
                                                 */
        "   movne   sp, r0                  \n" /* Switch to the SPM stack if
                                                 * caller is NOT an NS agent.
                                                 */
        "   msrne   psplim, r1              \n"
        "   cpsie   i                       \n"
        "   isb                             \n"
        "   ldmia   r6!, {r0-r4, r12}       \n" /* Load PSA interface input args
                                                 * and target function
                                                 */
#if CONFIG_ENABLE_XCFI
        "   str     r4, [sp, #-4]           \n"
        "   str     r5, [sp, #-8]           \n"
        "   mrs     r5, primask             \n"
        "   cpsid   i                       \n"
        "   movw    r10, #0xbeef            \n"
        "   ldr     r4, [r12, #-5]          \n"
        "   uxth.w  r4, r4                  \n"
        "   subs.w  r4, r4, r10             \n"
        "   msr     primask, r5             \n"
        "   ldr     r4, [sp, #-4]           \n"
        "   ldr     r4, [sp, #-8]           \n"
#endif
        "   blx     r12                     \n"
        "   cpsid   i                       \n"
        "   isb                             \n"
        "   bl      backend_abi_leaving_spm \n"
        "   mov     r2, #0                  \n" /* Back to caller new stack */
        "   msr     psplim, r2              \n"
        "   mov     sp, r6                  \n"
        "   msr     psplim, r5              \n"
        "   cpsie   i                       \n"
        "   isb                             \n"
        "   pop     {r4-r6, pc}             \n"
    );
}

#endif /* CONFIG_TFM_SPM_BACKEND_IPC == 1*/

#if CONFIG_TFM_SPM_BACKEND_IPC == 1
__attribute__((naked)) void PendSV_Handler(void)
{
    __ASM volatile(
        SYNTAX_UNIFIED
        "   movs    r0, #"M2S(EXC_RETURN_S)"            \n"
        "   ands    r0, lr                              \n" /* NS interrupted */
#if CONFIG_TFM_SCHEDULE_WHEN_NS_INTERRUPTED == 0
        "   beq     v8m_pendsv_exit                     \n" /* No schedule */
#endif
        "   add     r10, r10, #1                        \n"
#if CONFIG_ENABLE_XCFI
        "   ldr     r10, [pc, #-24]                     \n"
        "   tst     lr, #0x4                            \n"
        "   ite     ne                                  \n"
        "   mrsne   r0, psp                             \n"
        "   mrseq   r0, msp                             \n"
        "   sub     r0, #20                             \n"
        "   ldmia   r0, {r1-r3}                         \n"
        "   eor     r1, r1, r2                          \n"
        "   eor     r1, r1, r3                          \n"
        "   eor     r1, r1, lr                          \n"
        "   pacg    r12, r10, r1                        \n"
        "   push    {r0, r12, lr}                       \n"
#else
        "   push    {r0, lr}                            \n" /* Save R0, LR */
#endif
        "   mov     r0, lr                              \n" /* Pass the EXC_RETURN value as
                                                             * parameter
                                                             */
        "   bl      ipc_schedule                        \n"
#if CONFIG_ENABLE_XCFI
        "   pop     {r2, r12, lr}                       \n"
        "   movw    r10, #0xbeef                        \n"
        "   movt    r10, #0xdead                        \n"
        "   tst     lr, #0x4                            \n"
        "   ite     ne                                  \n"
        "   mrsne   r0, psp                             \n"
        "   mrseq   r0, msp                             \n"
        "   sub     r0, #20                             \n"
        "   ldmia   r0, {r1-r3}                         \n"
        "   eor     r1, r1, r2                          \n"
        "   eor     r1, r1, r3                          \n"
        "   eor     r1, r1, lr                          \n"
        "   autg    r12, r10, r1                        \n"
#else
        "   pop     {r2, lr}                            \n"
#endif
        "   cmp     r0, r1                              \n" /* curr, next ctx */
        "   beq     v8m_pendsv_exit                     \n" /* No schedule */
        "   cpsid   i                                   \n"
        "   isb                                         \n"
        "   mrs     r2, psp                             \n"
#if (CONFIG_TFM_FLOAT_ABI >= 1)
        "   vstmdb  r2!, {s16-s31}                      \n" /* Store s16-s31. */
#endif
        "   ands    r3, lr, #"M2S(EXC_RETURN_DCRS)"     \n" /* Check DCRS */
        "   itt     ne                                  \n" /* Skip saving callee */
        "   stmdbne r2!, {r4-r11}                       \n" /* Save callee */
        "   subne   r2, #8                              \n" /* SP offset for
                                                             * reserved additional state context,
                                                             * integrity signature
                                                             */
        "   stmia   r0, {r2, lr}                        \n" /* Save curr ctx:
                                                             * PSP, LR
                                                             */
        "   ldmia   r1!, {r2, lr}                       \n" /* Load next ctx:
                                                             * PSP, LR
                                                             */
        "   ands    r3, lr, #"M2S(EXC_RETURN_DCRS)"     \n" /* Check DCRS */
        "   itt     ne                                  \n" /* Skip loading callee */
        "   addne   r2, #8                              \n" /* SP offset for
                                                             * reserved additional state context,
                                                             * integrity signature
                                                             */
        "   ldmiane r2!, {r4-r11}                       \n" /* Load callee */
#if (CONFIG_TFM_FLOAT_ABI >= 1)
        "   vldmia  r2!, {s16-s31}                      \n" /* Load s16-s31. */
#endif
        "   ldr     r3, [r1]                            \n" /* Load sp_limit */
        "   msr     psp, r2                             \n"
        "   msr     psplim, r3                          \n"
        "   cpsie   i                                   \n"
        "   isb                                         \n"
        "v8m_pendsv_exit:                               \n"
        "   bx      lr                                  \n"
    );
}
#endif

__attribute__((naked)) void SVC_Handler(void)
{
    __ASM volatile(
    SYNTAX_UNIFIED
    "MRS     r0, MSP                         \n"
    "MOV     r1, lr                          \n"
    "MRS     r2, PSP                         \n"
    "MRS     r3, PSPLIM                      \n"
    "PUSH    {r2, r3}                        \n" /* PSP PSPLIM */
    "PUSH    {r1, r2}                        \n" /* Orig_exc_return, dummy */
    "BL      spm_svc_handler                 \n"
    "MOV     lr, r0                          \n"
    "LDR     r1, [sp]                        \n" /* Get orig_exc_return value */
    "AND     r0, #8                          \n" /* Mode bit */
    "AND     r1, #8                          \n"
    "SUBS    r0, r1                          \n" /* Compare EXC_RETURN values */
    "BGT     to_flih_func                    \n"
    "BLT     from_flih_func                  \n"
    "ADD     sp, #16                         \n" /*
                                                  * "Unstack" unused orig_exc_return, dummy,
                                                  * PSP, PSPLIM pushed by current handler
                                                  */
    "BX      lr                              \n"
    "to_flih_func:                           \n"
#if (CONFIG_TFM_FLOAT_ABI >= 1)
    "VPUSH   {s16-s31}                       \n" /* Save callee FPU registers */
#endif
    "ANDS    r3, lr, #"M2S(EXC_RETURN_DCRS)" \n" /* Check DCRS */
#if CONFIG_ENABLE_XCFI
    "BEQ     1f                              \n"
    "MRS     r0, psp                         \n"
    "LDMIA   r0, {r1-r3}                     \n"
    "EOR     r1, r1, r2                      \n"
    "EOR     r1, r1, r3                      \n"
    "MOVW    r10, #0xbeef                    \n"
    "MOVT    r10, #0xdead                    \n"
    "PACG    r12, r10, r1                    \n"
    "PUSH    {r4-r12}                        \n"
    "SUB     sp, #8                          \n"
    "1:                                      \n"
#else
    "ITT     ne                              \n" /* Skip saving callee */
    "PUSHNE  {r4-r11}                        \n" /* Save callee */
    "SUBSNE  sp, #8                          \n" /* SP offset for
                                                  * reserved additional state context,
                                                  * integrity signature
                                                  */
#endif
    "LDR     r4, ="M2S(STACK_SEAL_PATTERN)"  \n" /* clear r4-r11 */
    "MOV     r5, r4                          \n"
    "MOV     r6, r4                          \n"
    "MOV     r7, r4                          \n"
    "MOV     r8, r4                          \n"
    "MOV     r9, r4                          \n"
    "MOV     r10, r4                         \n"
    "MOV     r11, r4                         \n"
    "PUSH    {r4, r5}                        \n" /* Seal stack before EXC_RET */
    "BX      lr                              \n"
    "from_flih_func:                         \n"
    "ADD     sp, #16                         \n" /*
                                                  * "Unstack" unused orig_exc_return, dummy,
                                                  * PSP, PSPLIM pushed by current handler
                                                  */
    "POP     {r4, r5}                        \n" /* Seal stack */
    "ANDS    r3, lr, #"M2S(EXC_RETURN_DCRS)" \n" /* Check DCRS */
#if CONFIG_ENABLE_XCFI
    "BEQ     2f                              \n"
    "MRS     r0, psp                         \n"
    "LDMIA   r0, {r1-r3}                     \n"
    "EOR     r1, r1, r2                      \n"
    "EOR     r1, r1, r3                      \n"
    "MOVW    r10, #0xbeef                    \n"
    "MOVT    r10, #0xdead                    \n"
    "ADDS    sp, #8                          \n"
    "POP     {r4-r12}                        \n"
    "AUTG    r12, r10, r1                    \n"
    "2:                                      \n"
#else
    "ITT     ne                              \n" /* Skip loading callee */
    "ADDSNE  sp, #8                          \n" /* SP offset for
                                                  * reserved additional state context,
                                                  * integrity signature
                                                  */
    "POPNE   {r4-r11}                        \n" /* Load callee */
#endif
#if (CONFIG_TFM_FLOAT_ABI >= 1)
    "VPOP    {s16-s31}                       \n" /* Load callee FPU registers */
#endif
    "ADD     sp, #16                         \n" /*
                                                  * "Unstack" unused orig_exc_return, dummy,
                                                  * PSP, PSPLIM pushed by the previous
                                                  * TFM_SVC_PREPARE_DEPRIV_FLIH request
                                                  */
    "BX      lr                              \n"
    );
}

void tfm_arch_set_secure_exception_priorities(void)
{
    uint32_t VECTKEY;
    SCB_Type *scb = SCB;
    uint32_t AIRCR;

    /* Set PRIS flag in AIRCR */
    AIRCR = scb->AIRCR;
    VECTKEY = (~AIRCR & SCB_AIRCR_VECTKEYSTAT_Msk);
    scb->AIRCR = SCB_AIRCR_PRIS_Msk |
                 VECTKEY |
                 (AIRCR & ~SCB_AIRCR_VECTKEY_Msk);
    /* Set fault priority to less than 0x80 (with AIRCR.PRIS set) to prevent
     * Non-secure from pre-empting faults that may indicate corruption of Secure
     * state.
     */
    NVIC_SetPriority(MemoryManagement_IRQn, MemoryManagement_IRQnLVL);
    NVIC_SetPriority(BusFault_IRQn, BusFault_IRQnLVL);
    NVIC_SetPriority(SecureFault_IRQn, SecureFault_IRQnLVL);

    NVIC_SetPriority(SVCall_IRQn, SVCall_IRQnLVL);
    /*
     * Set secure PendSV priority to the lowest in SECURE state.
     */
    NVIC_SetPriority(PendSV_IRQn, PENDSV_PRIO_FOR_SCHED);
}

#ifdef TFM_FIH_PROFILE_ON
FIH_RET_TYPE(int32_t) tfm_arch_verify_secure_exception_priorities(void)
{
    SCB_Type *scb = SCB;

    if ((scb->AIRCR & SCB_AIRCR_PRIS_Msk) != SCB_AIRCR_PRIS_Msk) {
        FIH_RET(FIH_FAILURE);
    }
    (void)fih_delay();
    if ((scb->AIRCR & SCB_AIRCR_PRIS_Msk) != SCB_AIRCR_PRIS_Msk) {
        FIH_RET(FIH_FAILURE);
    }
    if (fih_not_eq(fih_int_encode(NVIC_GetPriority(MemoryManagement_IRQn)),
                  fih_int_encode(MemoryManagement_IRQnLVL))) {
        FIH_RET(FIH_FAILURE);
    }
    if (fih_not_eq(fih_int_encode(NVIC_GetPriority(BusFault_IRQn)),
                  fih_int_encode(BusFault_IRQnLVL))) {
        FIH_RET(FIH_FAILURE);
    }
    if (fih_not_eq(fih_int_encode(NVIC_GetPriority(SecureFault_IRQn)),
                  fih_int_encode(SecureFault_IRQnLVL))) {
        FIH_RET(FIH_FAILURE);
    }
    if (fih_not_eq(fih_int_encode(NVIC_GetPriority(SVCall_IRQn)),
                  fih_int_encode(SVCall_IRQnLVL))) {
        FIH_RET(FIH_FAILURE);
    }
    if (fih_not_eq(fih_int_encode(NVIC_GetPriority(PendSV_IRQn)),
                  fih_int_encode(PENDSV_PRIO_FOR_SCHED))) {
        FIH_RET(FIH_FAILURE);
    }
    FIH_RET(FIH_SUCCESS);
}
#endif

void tfm_arch_config_extensions(void)
{
#if defined(__ARM_FEATURE_PAUTH) && (__ARM_FEATURE_PAUTH == 1)
    CONTROL_Type ctrl;

    /*
     * XCFI injects PACG/AUTG encodings even when TF-M's upstream
     * CONFIG_TFM_BRANCH_PROTECTION_FEAT remains disabled for LLVM builds.
     * Enable PAC execution explicitly so return authentication faults
     * materialize as architectural UsageFaults instead of alias-like control
     * transfers.
     */
    ctrl.w = __get_CONTROL();
    ctrl.b.UPAC_EN = 1;
    ctrl.b.PAC_EN = 1;
    __set_CONTROL(ctrl.w);
    __ISB();
#endif

#if defined(CONFIG_TFM_ENABLE_CP10CP11)
    /*
     * Enable SPE privileged and unprivileged access to the FP Extension.
     * Note: On Armv8-M, if Non-secure access to the FPU is needed, Secure
     * access to the FPU must be enabled first in order to avoid No Coprocessor
     * (NOCP) usage fault when a Non-secure to Secure service call is
     * interrupted while CONTROL.FPCA=1 is set by Non-secure. This is needed
     * even if SPE will not use the FPU directly.
     */
    SCB->CPACR |= (3U << 10U*2U)     /* enable CP10 full access */
                  | (3U << 11U*2U);  /* enable CP11 full access */
    __DSB();
    __ISB();
    /*
     * Permit Non-secure access to the Floating-point Extension.
     * Note: It is still necessary to set CPACR_NS to enable the FP Extension
     * in the NSPE. This configuration is left to NS privileged software.
     */
    SCB->NSACR |= SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk;
#elif defined(CONFIG_TFM_DISABLE_CP10CP11)
    /*
     * Disable privileged and unprivileged access to the FP Extension for SPE and
     * NSPE located on the same core.
     */
    SCB->CPACR    &= ~((3U << 10U*2U)     /* disable CP10 full access for SPE */
                     | (3U << 11U*2U));   /* disable CP11 full access for SPE */
    SCB_NS->CPACR &= ~((3U << 10U*2U)     /* disable CP10 full access for NSPE */
                     | (3U << 11U*2U));   /* disable CP11 full access for NSPE */
    __DSB();
    __ISB();

    /* Disable Non-secure access to the Floating-point Extension. */
    SCB->NSACR &= ~(SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk);
#endif /* defined(CONFIG_TFM_ENABLE_CP10CP11) */

#if (CONFIG_TFM_FLOAT_ABI >= 1)

#ifdef CONFIG_TFM_LAZY_STACKING
    /* Enable lazy stacking. */
    FPU->FPCCR |= FPU_FPCCR_LSPEN_Msk;
#else
    /* Disable lazy stacking. */
    FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;
#endif

    /*
     * If the SPE will ever use the floating-point registers for sensitive
     * data, then FPCCR.ASPEN, FPCCR.TS, FPCCR.CLRONRET and FPCCR.CLRONRETS
     * must be set at initialisation and not changed again afterwards.
     * Let SPE decide the S/NS shared setting (LSPEN and CLRONRET) to avoid the
     * possible side-path brought by flexibility. This is not needed
     * if the SPE will never use floating-point but enables the FPU only for
     * avoiding NOCP faults during interrupted NSPE to SPE calls.
     */
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk
                  | FPU_FPCCR_TS_Msk
                  | FPU_FPCCR_CLRONRET_Msk
                  | FPU_FPCCR_CLRONRETS_Msk
                  | FPU_FPCCR_LSPENS_Msk;

    /* Prevent non-secure from modifying FPU’s power setting. */
#if defined(__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ == 1)
    ICB->CPPWR |=
#else
    SCnSCB->CPPWR |=
#endif
        SCnSCB_CPPWR_SUS11_Msk | SCnSCB_CPPWR_SUS10_Msk;

#endif /* CONFIG_TFM_FLOAT_ABI >= 1 */

#if defined(__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ == 1)
    SCB->CCR |= SCB_CCR_TRD_Msk;
#endif

#if defined(__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ == 1)
    /*
     * \note Set Data Independent Timing (DIT) bit in AIRCR_S. This makes sure
     *       that instructions which might have variable duration based on the
     *       value of their operands, are always taking the same amount of
     *       cycles to complete, for example UMLAL instruction used by the
     *       P256M ECDSA driver.
     *       If the implementation is not capable of guaranteeing this, the
     *       write is ignored
     */
    SCB->AIRCR |= SCB_AIRCR_DIT_Msk;
#endif
}

#if (CONFIG_TFM_FLOAT_ABI > 0)
__attribute__((naked, noinline, used)) void tfm_arch_clear_fp_data(void)
{
    __ASM volatile(
                    "eor  r0, r0, r0         \n"
                    "vmsr fpscr, r0          \n"
#if (defined(__ARM_ARCH_8_1M_MAIN__))
/* IAR throws an error if the S0-S31 syntax is used.
 * Splitting the command into two parts solved the issue.
 */
#if defined(__ICCARM__)
                    "vscclrm {s0-s30,vpr}    \n"
                    "vscclrm {s31,vpr}       \n"
#else
                    "vscclrm {s0-s31,vpr}    \n"
#endif
#else
                    "vmov s0, r0             \n"
                    "vmov s1, r0             \n"
                    "vmov s2, r0             \n"
                    "vmov s3, r0             \n"
                    "vmov s4, r0             \n"
                    "vmov s5, r0             \n"
                    "vmov s6, r0             \n"
                    "vmov s7, r0             \n"
                    "vmov s8, r0             \n"
                    "vmov s9, r0             \n"
                    "vmov s10, r0            \n"
                    "vmov s11, r0            \n"
                    "vmov s12, r0            \n"
                    "vmov s13, r0            \n"
                    "vmov s14, r0            \n"
                    "vmov s15, r0            \n"
                    "vmov s16, r0            \n"
                    "vmov s17, r0            \n"
                    "vmov s18, r0            \n"
                    "vmov s19, r0            \n"
                    "vmov s20, r0            \n"
                    "vmov s21, r0            \n"
                    "vmov s22, r0            \n"
                    "vmov s23, r0            \n"
                    "vmov s24, r0            \n"
                    "vmov s25, r0            \n"
                    "vmov s26, r0            \n"
                    "vmov s27, r0            \n"
                    "vmov s28, r0            \n"
                    "vmov s29, r0            \n"
                    "vmov s30, r0            \n"
                    "vmov s31, r0            \n"
#endif
                    "bx   lr                 \n"
                  );
}
#endif
