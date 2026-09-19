#ifndef TERRALINK_UART_MANAGER_H
#define TERRALINK_UART_MANAGER_H

#include <Arduino.h>

void uartManagerBegin();

void uartManagerLoop();

void sendCommandToRX1(const String &command);

void processRX1Message(const String &message);

#endif