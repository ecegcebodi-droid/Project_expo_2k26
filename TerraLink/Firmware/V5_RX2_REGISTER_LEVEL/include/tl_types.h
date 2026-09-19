#ifndef TL_TYPES_H
#define TL_TYPES_H

#include <Arduino.h>

typedef struct {
    bool valid;
    uint16_t nodeId;
    uint32_t sequence;
    String latitude;
    String longitude;
    int hop;
    int rssi;
    bool sosActive;
    bool resolved;
    unsigned long firstSeen;
    unsigned long lastSeen;
} TL_NodeRecord;

typedef struct {
    bool valid;
    String incidentId;
    uint16_t nodeId;
    uint32_t sequence;
    String latitude;
    String longitude;
    int hop;
    int rssi;
    String status;
    unsigned long createdAt;
    unsigned long updatedAt;
} TL_IncidentRecord;

typedef struct {
    bool valid;
    uint16_t nodeId;
    uint32_t sequence;
    String message;
    String status;
    unsigned long timestamp;
} TL_MessageRecord;

typedef enum {
    TL_UI_HOME,
    TL_UI_NODE_SELECT,
    TL_UI_NODE_CONTROL,
    TL_UI_ACTION_CONFIRM,
    TL_UI_NODE_INFO,
    TL_UI_RESOLVE_CONFIRM,
    TL_UI_SENT_STATUS
} TL_UIState;

typedef enum {
    TL_ACT_NONE,
    TL_ACT_STAY_CALM,
    TL_ACT_RESCUE,
    TL_ACT_DO_NOT_MOVE,
    TL_ACT_LOCATION_AGAIN,
    TL_ACT_HELP_APPROACHING,
    TL_ACT_GPS_REQUEST,
    TL_ACT_EMERGENCY,
    TL_ACT_RESOLVE
} TL_ActionType;

#endif
