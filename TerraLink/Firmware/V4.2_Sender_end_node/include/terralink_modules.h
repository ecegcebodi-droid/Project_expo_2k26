#pragma once
#include <Arduino.h>
#include "terralink_types.h"
void loadPersistentData(void); void saveSequenceNumber(void); void savePendingSOS(void); void clearPendingSOS(void);
void buzzerOn(void); void buzzerOff(void); void beep(unsigned int duration); void buzzerWake(void); void buzzerSOSActivated(void); void buzzerSending(void); void buzzerWaiting(void); void buzzerSuccess(void); void buzzerFailure(void); void buzzerRescueMessage(void);
void handleLocalAlarm(void); void handleLEDButton(void);
void drawHeader(void); void drawNakedCheck(int x,int y); void drawNakedCross(int x,int y); void drawGPSIcon(int x,int y,bool fixed); void drawWarningIcon(int x,int y); void drawRadioWaves(int x,int y,int phase); void drawProgressBar(int x,int y,int width,int height,int percent); void drawRescueMessageScreen(void); void updateOLED(void);
void goToState(SystemState newState); void updateGPS(void); bool getPacketField(const String &packet,const char *field,String &value); bool packetHasType(const String &packet,const char *type); String createSOSPacket(void); void sendSOS(void); void sendMessageACK(int messageID,uint32_t sequence); void sendCurrentLocation(void); bool handleRescueMessage(const String &packet); bool handleLocationRequest(const String &packet); bool checkForFinalACK(const String &received); void processLoRaPacket(const String &received); void LoRaTask(void *parameter); void requestSOS(void); void startPersistentSOSRetry(void); void enterDeepSleep(void); void handleTouch(void);
