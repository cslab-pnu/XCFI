#include <stdlib.h>

void _exit(int status)
{
    (void)status;
    while (1) {
        __asm volatile ("wfi");  // Wait forever (never return)
    }
}

