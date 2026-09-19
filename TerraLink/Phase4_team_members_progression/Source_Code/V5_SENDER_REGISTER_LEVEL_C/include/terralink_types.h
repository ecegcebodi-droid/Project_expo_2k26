#ifndef TERRALINK_TYPES_H
#define TERRALINK_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TL_STATE_AWAKENING = 0,
    TL_STATE_WAIT_RELEASE,
    TL_STATE_READY,
    TL_STATE_HOLDING_SOS,
    TL_STATE_SOS_ACTIVATED,
    TL_STATE_SENDING,
    TL_STATE_WAITING_ACK,
    TL_STATE_SOS_SUCCESS,
    TL_STATE_SOS_FAILURE,
    TL_STATE_POST_SOS,
    TL_STATE_HOLDING_RESEND,
    TL_STATE_RESCUE_MESSAGE
} tl_state_t;

typedef struct {
    uint32_t sequence;
    uint8_t ttl;
    uint8_t hop;
    uint8_t src;
    uint8_t dst;
    uint8_t msg_id;
    uint8_t priority;
    char type[12];
    char status[16];
    char message[121];
    double latitude;
    double longitude;
    bool gps_fix;
} tl_packet_t;

typedef struct {
    double latitude;
    double longitude;
    uint8_t satellites;
    bool fix;
} tl_gps_data_t;

typedef struct {
    bool pending;
    uint32_t sequence;
    double latitude;
    double longitude;
    bool gps_fix;
} tl_persistent_sos_t;

typedef struct {
    uint8_t message_id;
    uint32_t sequence;
    uint8_t priority;
    char source[12];
    char message[121];
} tl_rescue_message_t;

#endif
