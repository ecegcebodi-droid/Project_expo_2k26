#include "message_manager.h"
#include "terralink_globals.h"

void processIncomingACK(
    const String &data
)
{
    gatewayStatus.totalACK++;

    Serial.print("[ACK] ");
    Serial.println(data);
}

void processIncomingMSGACK(
    const String &data
)
{
    Serial.print("[MSG ACK] ");
    Serial.println(data);

    /*
       Later:
       mark corresponding message as delivered.
    */
}

void processIncomingResolveACK(
    const String &data
)
{
    Serial.print("[RESOLVE ACK] ");
    Serial.println(data);

    if (incidentCount > 0)
    {
        incidentTable[
            (incidentCount - 1) % MAX_INCIDENTS
        ].state =
            INCIDENT_RESOLVED;
    }
}

void processTXStatus(
    const String &data
)
{
    Serial.print("[TX STATUS] ");
    Serial.println(data);
}