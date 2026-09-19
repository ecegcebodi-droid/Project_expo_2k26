#ifndef INTERRUPT_DRIVER_H
#define INTERRUPT_DRIVER_H
#include <stdint.h>
void interrupt_init(void);
void interrupt_enable_gpio(uint8_t pin);
#endif
