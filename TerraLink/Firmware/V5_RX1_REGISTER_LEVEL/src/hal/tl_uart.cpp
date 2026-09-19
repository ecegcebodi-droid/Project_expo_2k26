#include <Arduino.h>
#include "tl_config.h"
#include "tl_uart.h"

static String uartBuffer;

void tl_uart_init()
{
    Serial.begin(TL_UART_BAUD);
    uartBuffer.reserve(160);
}

bool tl_uart_available()
{
    return Serial.available() > 0;
}

String tl_uart_read_line()
{
    if (!Serial.available())
        return "";

    String line = Serial.readStringUntil('\n');
    line.trim();

    return line;
}

void tl_uart_send(const String &message)
{
    Serial.println(message);
}