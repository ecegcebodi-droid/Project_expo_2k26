#include "network_manager.h"
#include "terralink_globals.h"
#include "node_manager.h"

void updateNetwork()
{
    unsigned long now = millis();

    for (int i = 0; i < nodeCount; i++)
    {
        if (now - nodeTable[i].lastSeen > 60000)
        {
            nodeTable[i].online = false;
        }
    }
}

void showNetworkStatus()
{
    Serial.println();
    Serial.println("=== TERRALINK NETWORK ===");

    Serial.print("NODES: ");
    Serial.println(nodeCount);

    Serial.print("INCIDENTS: ");
    Serial.println(incidentCount);

    Serial.print("RX: ");
    Serial.println(gatewayStatus.totalLoRaRx);

    Serial.print("TX: ");
    Serial.println(gatewayStatus.totalLoRaTx);

    Serial.print("SOS: ");
    Serial.println(gatewayStatus.totalSOS);

    Serial.print("ACK: ");
    Serial.println(gatewayStatus.totalACK);

    Serial.print("LOC: ");
    Serial.println(gatewayStatus.totalLOC);

    Serial.print("MSG: ");
    Serial.println(gatewayStatus.totalMSG);
}