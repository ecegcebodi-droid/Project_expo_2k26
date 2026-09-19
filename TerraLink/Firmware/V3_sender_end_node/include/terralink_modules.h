#pragma once
#include "terralink_types.h"

// System
void goToState(SystemState newState);

// SOS / persistence
void loadPersistentData();
void saveSequenceNumber();
void savePendingSOS();
void clearPendingSOS();
void requestSOS();
void startPersistentSOSRetry();

// LoRa
void initializeLoRa();
void sendSOS();
void LoRaTask(void *parameter);
void processLoRaPacket(const String &received);

// GPS
void initializeGPS();
void updateGPS();

// Display
void initializeDisplay();
void updateOLED();
void drawHeader();
void drawNakedCheck(int x, int y);
void drawNakedCross(int x, int y);
void drawGPSIcon(int x, int y, bool fixed);
void drawWarningIcon(int x, int y);
void drawRadioWaves(int x, int y, int phase);
void drawProgressBar(int x, int y, int width, int height, int percent);
void drawRescueMessageScreen();

// Buzzer
void initializeBuzzer();
void buzzerOn();
void buzzerOff();
void beep(unsigned int duration);
void buzzerWake();
void buzzerSOSActivated();
void buzzerSending();
void buzzerWaiting();
void buzzerSuccess();
void buzzerFailure();
void buzzerRescueMessage();
void buzzerPriorityMessage(int priority);

// Inputs
void initializeInputs();
void handleLocalAlarm();
void handleLEDButton();
void handleTouch();

// Messages
bool handleRescueMessage(const String &packet);
bool handleLocationRequest(const String &packet);
bool checkForFinalACK(const String &received);
void sendMessageACK(int messageID, uint32_t sequence);
void sendCurrentLocation();
bool popNextRescueMessage();

// Packet helpers
bool getPacketField(const String &packet, const char *field, String &value);
bool packetHasType(const String &packet, const char *type);
String createSOSPacket();

// Power
void enterDeepSleep();
