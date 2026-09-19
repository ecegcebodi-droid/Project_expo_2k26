#include "incident_manager.h"
#include "terralink_globals.h"
#include "node_manager.h"

static String field(
    const String &data,
    const String &key
)
{
    String search = key + ":";

    int start = data.indexOf(search);

    if (start < 0)
        return "";

    start += search.length();

    int end = data.indexOf(',', start);

    if (end < 0)
        end = data.length();

    return data.substring(start, end);
}

void processIncomingSOS(const String &data)
{
    /*
       RX1 sends:

       SOS,
       NODE,
       SEQ,
       LAT,
       LON,
       HOP,
       RSSI,
       TTL
    */

    int p[8];

    p[0] = data.indexOf(',');
    if (p[0] < 0) return;

    String body = data.substring(p[0] + 1);

    String values[8];

    int index = 0;

    while (body.length() && index < 8)
    {
        int comma = body.indexOf(',');

        if (comma < 0)
        {
            values[index++] = body;
            break;
        }

        values[index++] =
            body.substring(0, comma);

        body =
            body.substring(comma + 1);
    }

    if (index < 8)
        return;

    Incident &incident =
        incidentTable[
            incidentCount % MAX_INCIDENTS
        ];

    incident.nodeID = values[0].toInt();
    incident.seq = values[1].toInt();

    incident.latitude = values[2].toFloat();
    incident.longitude = values[3].toFloat();

    incident.hop = values[4].toInt();
    incident.rssi = values[5].toInt();

    incident.state = INCIDENT_ACTIVE;

    incident.receivedAt = millis();
    incident.lastUpdate = millis();

    if (incidentCount < MAX_INCIDENTS)
        incidentCount++;

    activeIncidentNode =
        incident.nodeID;

    activeIncidentSeq =
        incident.seq;

    updateNodeFromIncident(incident);

    Serial.println();
    Serial.println("==============================");
    Serial.println("       ACTIVE SOS");
    Serial.println("==============================");

    Serial.print("NODE: ");
    Serial.println(incident.nodeID);

    Serial.print("SEQ : ");
    Serial.println(incident.seq);

    Serial.print("LAT : ");
    Serial.println(incident.latitude, 6);

    Serial.print("LON : ");
    Serial.println(incident.longitude, 6);

    Serial.print("HOP : ");
    Serial.println(incident.hop);

    Serial.print("RSSI: ");
    Serial.println(incident.rssi);

    Serial.println("==============================");
}

void showLatestSOS()
{
    if (incidentCount == 0)
    {
        Serial.println("NO SOS EVENTS");
        return;
    }

    Incident &incident =
        incidentTable[
            (incidentCount - 1) % MAX_INCIDENTS
        ];

    Serial.print("SOS NODE: ");
    Serial.println(incident.nodeID);

    Serial.print("SEQ: ");
    Serial.println(incident.seq);

    Serial.print("STATE: ");
    Serial.println(incident.state);
}

void showIncidentList()
{
    Serial.println();
    Serial.println("=== SOS EVENTS ===");

    for (int i = 0; i < incidentCount; i++)
    {
        Serial.print(i + 1);
        Serial.print(". NODE ");
        Serial.print(incidentTable[i].nodeID);
        Serial.print(" SEQ ");
        Serial.println(incidentTable[i].seq);
    }
}

void showLatestLocation()
{
    if (incidentCount == 0)
    {
        Serial.println("NO LOCATION");
        return;
    }

    Incident &incident =
        incidentTable[
            (incidentCount - 1) % MAX_INCIDENTS
        ];

    Serial.print("LAT: ");
    Serial.println(incident.latitude, 6);

    Serial.print("LON: ");
    Serial.println(incident.longitude, 6);
}

void showSenderInfo()
{
    if (activeIncidentNode < 0)
    {
        Serial.println("NO ACTIVE NODE");
        return;
    }

    Serial.print("ACTIVE NODE: ");
    Serial.println(activeIncidentNode);
}