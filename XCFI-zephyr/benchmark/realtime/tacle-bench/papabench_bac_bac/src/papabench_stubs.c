#include <zephyr/kernel.h>

/* hardware init stubs */

void modem_init(void) {}
void adc_init(void) {}
void spi_init(void) {}
void link_fbw_init(void) {}
void gps_init(void) {}
void ir_init(void) {}
void fbw_init(void) {}

/* runtime stubs */

void periodic_task(void) {}
void parse_gps_msg(void) {}
void send_gps_pos(void) {}
void send_radIR(void) {}
void send_takeOff(void) {}
void radio_control_task(void) {}

int gps_msg_received = 0;
int link_fbw_receive_complete = 0;

/* estimator dependency */

double ir_rad_of_ir(double x) { return x; }
