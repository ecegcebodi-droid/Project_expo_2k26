#ifndef TERRALINK_INCIDENT_MANAGER_H
#define TERRALINK_INCIDENT_MANAGER_H

#include <Arduino.h>

void processIncomingSOS(const String &data);

void showLatestSOS();

void showIncidentList();

void showLatestLocation();

void showSenderInfo();

#endif