#include "command_manager.h"
#include "uart_manager.h"
#include "terralink_globals.h"
#include "message_manager.h"
#include "incident_manager.h"

void commandManagerBegin()
{
    commandSequence = 1000;
}

void processOperatorKey(char key)
{
    /*
       This is intentionally kept as a navigation layer.

       Example future menu:

       1 -> Latest SOS
       2 -> SOS Events
       3 -> Location
       4 -> Sender Info
       5 -> Send Message
       6 -> GPS Request
       7 -> Resolve
       8 -> Emergency Priority
       9 -> Network
       0 -> Status
    */

    switch (key)
    {
        case '1':
            showLatestSOS();
            break;

        case '2':
            showIncidentList();
            break;

        case '3':
            showLatestLocation();
            break;

        case '4':
            showSenderInfo();
            break;

        case '5':
            Serial.println("[MENU] MESSAGE");
            break;

        case '6':
            Serial.println("[MENU] GPS REQUEST");
            break;

        case '7':
            Serial.println("[MENU] RESOLVE");
            break;

        case '8':
            Serial.println("[MENU] EMERGENCY PRIORITY");
            break;

        case '9':
            showNetworkStatus();
            break;

        case '0':
            sendCommandToRX1("GET_STATUS");
            break;
    }
}

void sendMessageCommand(int node, const String &message)
{
    commandSequence++;

    String command =
        "TL,MSG," +
        String(node) +
        "," +
        message;

    sendCommandToRX1(command);
}

void requestGPS(int node)
{
    sendCommandToRX1(
        "TL,GPS_REQUEST," +
        String(node)
    );
}

void requestLocationAgain(int node)
{
    sendCommandToRX1(
        "TL,LOC_AGAIN," +
        String(node)
    );
}

void resolveIncident(int node)
{
    sendCommandToRX1(
        "TL,RESOLVE," +
        String(node)
    );
}

void sendEmergencyPriority(int node)
{
    sendCommandToRX1(
        "TL,EMERGENCY," +
        String(node)
    );
}