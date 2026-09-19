#include "terralink_globals.h"

NodeInfo nodeTable[MAX_NODES];

int nodeCount = 0;

Incident incidentTable[MAX_INCIDENTS];

int incidentCount = 0;

RescueMessage messageTable[MAX_MESSAGES];

int messageCount = 0;

NetworkEntry networkTable[MAX_NETWORK_ENTRIES];

int networkCount = 0;

GatewayStatus gatewayStatus =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

int activeIncidentNode = -1;

unsigned long activeIncidentSeq = 0;

unsigned long commandSequence = 1000;