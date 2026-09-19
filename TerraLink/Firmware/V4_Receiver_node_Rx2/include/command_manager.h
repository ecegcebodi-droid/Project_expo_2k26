#ifndef TERRALINK_COMMAND_MANAGER_H
#define TERRALINK_COMMAND_MANAGER_H

#include <Arduino.h>

void commandManagerBegin();

void processOperatorKey(char key);

void sendMessageCommand(
    int node,
    const String &message
);

void requestGPS(int node);

void requestLocationAgain(int node);

void resolveIncident(int node);

void sendEmergencyPriority(int node);

#endif