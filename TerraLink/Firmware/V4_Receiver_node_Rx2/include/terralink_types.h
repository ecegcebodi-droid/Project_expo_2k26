#ifndef TERRALINK_TYPES_H
#define TERRALINK_TYPES_H

#include <Arduino.h>

enum IncidentState
{
    INCIDENT_NONE,
    INCIDENT_ACTIVE,
    INCIDENT_ACKNOWLEDGED,
    INCIDENT_LOCATING,
    INCIDENT_RESPONDING,
    INCIDENT_RESOLVED
};

struct NodeInfo
{
    int nodeID;

    float latitude;
    float longitude;

    int hop;
    int rssi;

    bool gpsValid;
    bool online;
    bool sosActive;

    unsigned long lastSeen;
};

struct Incident
{
    int nodeID;

    unsigned long seq;

    float latitude;
    float longitude;

    int hop;
    int rssi;

    IncidentState state;

    unsigned long receivedAt;
    unsigned long lastUpdate;
};

struct RescueMessage
{
    int source;
    int destination;

    unsigned long seq;

    String message;

    bool priority;

    bool delivered;

    unsigned long timestamp;
};

struct NetworkEntry
{
    int source;
    int destination;

    int hop;

    int rssi;

    bool active;

    unsigned long lastSeen;
};

struct GatewayStatus
{
    unsigned long totalLoRaRx;
    unsigned long totalLoRaTx;

    unsigned long totalSOS;
    unsigned long totalACK;
    unsigned long totalLOC;
    unsigned long totalMSG;

    unsigned long totalCommands;
};

#endif