#ifndef TERRALINK_GLOBALS_H
#define TERRALINK_GLOBALS_H

#include "terralink_types.h"

/* =========================================================
                       NODE DATABASE
   ========================================================= */

extern NodeInfo nodeTable[MAX_NODES];

extern int nodeCount;

/* =========================================================
                       INCIDENT DATABASE
   ========================================================= */

extern Incident incidentTable[MAX_INCIDENTS];

extern int incidentCount;

/* =========================================================
                       MESSAGE DATABASE
   ========================================================= */

extern RescueMessage messageTable[MAX_MESSAGES];

extern int messageCount;

/* =========================================================
                       NETWORK DATABASE
   ========================================================= */

extern NetworkEntry networkTable[MAX_NETWORK_ENTRIES];

extern int networkCount;

/* =========================================================
                       GATEWAY STATUS
   ========================================================= */

extern GatewayStatus gatewayStatus;

/* =========================================================
                    ACTIVE INCIDENT
   ========================================================= */

extern int activeIncidentNode;

extern unsigned long activeIncidentSeq;

/* =========================================================
                     COMMAND SEQUENCE
   ========================================================= */

extern unsigned long commandSequence;

#endif