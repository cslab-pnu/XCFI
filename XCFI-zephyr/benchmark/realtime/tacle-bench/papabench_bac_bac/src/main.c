#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

extern int papabench_mainloop(void);

int main(void)
{
    uint32_t start = k_cycle_get_32();

    papabench_mainloop();

    uint32_t end = k_cycle_get_32();

    printf("cycles: %u\n", end - start);

    return 0;
}
