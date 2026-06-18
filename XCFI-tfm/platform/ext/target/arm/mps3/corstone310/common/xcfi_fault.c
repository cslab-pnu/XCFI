#include <stdio.h>
#include "cmsis_compiler.h"

__attribute__((noreturn, used, section(".XCFI_fault")))
void XCFI_fault(void)
{
    printf("[XCFI] Control-flow violation detected in Secure world!\n");
    while (1) {
        __WFI();
    }
}

