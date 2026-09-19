#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H
#include <stdint.h>
uint64_t timer_millis(void);
void timer_delay_ms(uint32_t ms);
#endif
