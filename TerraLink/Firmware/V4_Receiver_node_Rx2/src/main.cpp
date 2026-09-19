/*
============================================================
              TERRALINK RECEIVER #2
            OPERATOR CONSOLE / COMMAND
============================================================

main.cpp is ONLY the system controller.

Hardware-specific and application functions are implemented
in separate modules.
============================================================
*/

#include <Arduino.h>

#include "uart_manager.h"
#include "keypad_manager.h"
#include "command_manager.h"
#include "incident_manager.h"
#include "node_manager.h"
#include "message_manager.h"
#include "network_manager.h"
#include "display_manager.h"
#include "web_manager.h"

void setup()
{
    Serial.begin(9600);

    delay(500);

    Serial.println();
    Serial.println("======================================");
    Serial.println("       TERRALINK RECEIVER #2");
    Serial.println("        OPERATOR CONSOLE");
    Serial.println("======================================");

    uartManagerBegin();

    keypadManagerBegin();

    commandManagerBegin();

    displayManagerBegin();

    webManagerBegin();

    displayMenu();

    Serial.println("[RX2] SYSTEM READY");
}

void loop()
{
    uartManagerLoop();

    keypadManagerLoop();

    displayManagerLoop();

    webManagerLoop();

    updateNetwork();

    yield();
}