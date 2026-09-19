#ifndef TL_UART_H
#define TL_UART_H

#include <Arduino.h>

void tl_uart_init();
void tl_uart_send(const String &line);
void tl_uart_poll();

#endif
