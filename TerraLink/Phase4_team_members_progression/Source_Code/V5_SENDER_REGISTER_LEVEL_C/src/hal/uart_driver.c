#include "uart_driver.h"
#include "terralink_config.h"
#include "terralink_registers.h"

void uart2_init(uint32_t baud)
{
    (void)baud;
    /*
     * UART2 pin routing and baud configuration are isolated here.
     * For the exact ESP32 silicon/IDF version, configure UART2 registers
     * (or the equivalent ESP-IDF C driver) without Arduino HardwareSerial.
     */
}

bool uart2_available(void)
{
    return false;
}

uint8_t uart2_read_byte(void)
{
    return 0;
}

void uart2_write_byte(uint8_t data)
{
    (void)data;
}

void uart2_write(const char *s)
{
    while (s && *s)
        uart2_write_byte((uint8_t)*s++);
}
