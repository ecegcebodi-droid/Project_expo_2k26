#include "uart_manager.h"
#include "command_manager.h"
#include "incident_manager.h"
#include "message_manager.h"
#include "node_manager.h"
#include "terralink_globals.h"
#include "config.h"

SoftwareSerial rx1Serial(D8, D0);

String uartBuffer = "";

void uartManagerBegin()
{
    rx1Serial.begin(UART_BAUD);
}

void sendCommandToRX1(const String &command)
{
    rx1Serial.println(command);
    rx1Serial.flush();

    Serial.print("[RX2 -> RX1] ");
    Serial.println(command);
}

void processRX1Message(const String &message)
{
    Serial.print("[RX1 -> RX2] ");
    Serial.println(message);

    if (message.startsWith("SOS,"))
    {
        processIncomingSOS(message);
        return;
    }

    if (message.startsWith("LOCATION,"))
    {
        processIncomingLocation(message);
        return;
    }

    if (message.startsWith("ACK,"))
    {
        processIncomingACK(message);
        return;
    }

    if (message.startsWith("MSG_ACK,"))
    {
        processIncomingMSGACK(message);
        return;
    }

    if (message.startsWith("RESOLVE_ACK,"))
    {
        processIncomingResolveACK(message);
        return;
    }

    if (message.startsWith("RX1_STATUS,"))
    {
        processGatewayStatus(message);
        return;
    }

    if (message == "RX1,ONLINE")
    {
        Serial.println("[RX1] ONLINE");
        return;
    }

    if (message.startsWith("TX_STATUS,"))
    {
        processTXStatus(message);
        return;
    }
}

void uartManagerLoop()
{
    while (rx1Serial.available())
    {
        char c = rx1Serial.read();

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            uartBuffer.trim();

            if (uartBuffer.length() > 0)
                processRX1Message(uartBuffer);

            uartBuffer = "";

            continue;
        }

        if (uartBuffer.length() < 250)
            uartBuffer += c;
        else
            uartBuffer = "";
    }
}