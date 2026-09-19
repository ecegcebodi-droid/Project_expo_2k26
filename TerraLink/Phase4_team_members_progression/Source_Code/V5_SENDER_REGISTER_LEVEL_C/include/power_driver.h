#ifndef POWER_DRIVER_H
#define POWER_DRIVER_H
#include <stdint.h>
void power_init(void);
void power_deep_sleep_seconds(uint32_t seconds);
#endif
