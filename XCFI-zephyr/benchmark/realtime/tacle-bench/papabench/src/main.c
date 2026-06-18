#include <zephyr/pm/device.h>

extern void autopilot_main(void);

int main(void)
{
    while (1) {
        autopilot_main();
        k_sleep(K_MSEC(10));
    }
}
