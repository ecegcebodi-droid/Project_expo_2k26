#ifndef TERRALINK_MESSAGE_MANAGER_H
#define TERRALINK_MESSAGE_MANAGER_H

#include <Arduino.h>

void processIncomingACK(
    const String &data
);

void processIncomingMSGACK(
    const String &data
);

void processIncomingResolveACK(
    const String &data
);

void processTXStatus(
    const String &data
);

#endif