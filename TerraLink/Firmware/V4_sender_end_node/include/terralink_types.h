#pragma once
#include <Arduino.h>
#include "config.h"

enum SystemState : uint8_t {
    STATE_AWAKENING,
    STATE_WAIT_RELEASE,
    STATE_READY,
    STATE_HOLDING_SOS,
    STATE_SOS_ACTIVATED,
    STATE_SENDING,
    STATE_WAITING_ACK,
    STATE_SOS_SUCCESS,
    STATE_SOS_FAILURE,
    STATE_POST_SOS,
    STATE_HOLDING_RESEND,
    STATE_RESCUE_MESSAGE
};

struct RescueMessageRecord {
    int messageID;
    uint32_t sequence;
    int priority;
    char source[12];
    char message[MAX_RESCUE_MESSAGE_LENGTH + 1];
};
