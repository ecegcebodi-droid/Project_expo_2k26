#include "node_manager.h"
#include "terralink_globals.h"

void updateNodeFromIncident(
    const Incident &incident
)
{
    int existing = -1;

    for (int i = 0; i < nodeCount; i++)
    {
        if (nodeTable[i].nodeID ==
            incident.nodeID)
        {
            existing = i;
            break;
        }
    }

    if (existing < 0)
    {
        if (nodeCount >= MAX_NODES)
            return;

        existing = nodeCount++;

        nodeTable[existing].nodeID =
            incident.nodeID;
    }

    nodeTable[existing].latitude =
        incident.latitude;

    nodeTable[existing].longitude =
        incident.longitude;

    nodeTable[existing].hop =
        incident.hop;

    nodeTable[existing].rssi =
        incident.rssi;

    nodeTable[existing].gpsValid =
        true;

    nodeTable[existing].online =
        true;

    nodeTable[existing].sosActive =
        true;

    nodeTable[existing].lastSeen =
        millis();
}

void showNodeRegistry()
{
    Serial.println();
    Serial.println("=== NODE REGISTRY ===");

    for (int i = 0; i < nodeCount; i++)
    {
        Serial.print("NODE ");
        Serial.print(nodeTable[i].nodeID);

        Serial.print(" HOP=");
        Serial.print(nodeTable[i].hop);

        Serial.print(" RSSI=");
        Serial.print(nodeTable[i].rssi);

        Serial.print(" ONLINE=");
        Serial.println(
            nodeTable[i].online ? "YES" : "NO"
        );
    }
}