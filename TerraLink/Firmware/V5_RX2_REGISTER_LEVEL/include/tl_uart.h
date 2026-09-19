#ifndef TL_UART_H
#define TL_UART_H

void tl_uart_init(void);
void tl_uart_process(void);
void tl_uart_send_command(const char *type, unsigned int node, const char *payload);

#endif
