#include "display_manager.h"
#include "terralink_globals.h"

void displayManagerBegin()
{
    Serial.println("[DISPLAY] RX2 operator console ready");
}

void displayManagerLoop()
{
    /* UI refresh can be implemented here. */
}

void displaySOS()
{
    Serial.println("[DISPLAY] *** ACTIVE SOS ***");
}

void displayMenu()
{
    Serial.println();
    Serial.println("1 Latest SOS");
    Serial.println("2 SOS Events");
    Serial.println("3 Location");
    Serial.println("4 Sender Info");
    Serial.println("5 Message");
    Serial.println("6 GPS Request");
    Serial.println("7 Resolve");
    Serial.println("8 Emergency");
    Serial.println("9 Network");
    Serial.println("0 Status");
}