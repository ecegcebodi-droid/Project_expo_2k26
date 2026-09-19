#ifndef UART_DRIVER_H
#define UART_DRIVER_H
#include <stdint.h>
#include <stdbool.h>
void uart2_init(uint32_t baud);
bool uart2_available(void);
uint8_t uart2_read_byte(void);
void uart2_write_byte(uint8_t data);
void uart2_write(const char *s);
#endif
