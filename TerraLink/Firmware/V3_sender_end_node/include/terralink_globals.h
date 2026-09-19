#pragma once
#include <Arduino.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <Preferences.h>
#include <U8g2lib.h>
#include "terralink_types.h"

extern TinyGPSPlus gps;
extern HardwareSerial GPSserial;
extern Preferences preferences;
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C oled;

extern volatile SystemState currentState;
extern volatile bool loraInitialized;
extern volatile bool sosTransmitRequest;
extern volatile bool finalAckReceived;
extern volatile uint32_t activeSequence;
extern volatile uint32_t receivedAckSequence;
extern portMUX_TYPE timerMux;

extern volatile double currentLatitude;
extern volatile double currentLongitude;
extern volatile bool gpsFixAvailable;

extern uint32_t sequenceNumber;
extern bool pendingSOS;
extern uint32_t persistentSOSSequence;
extern double persistentSOSLatitude;
extern double persistentSOSLongitude;
extern bool persistentSOSGPSFix;

extern unsigned long stateStartTime;
extern unsigned long touchStartTime;
extern unsigned long lastUIUpdate;
extern unsigned long gpsWarningStart;
extern unsigned long rescueMessageDisplayStart;

extern bool touchWasPressed;
extern bool sosHoldTriggered;
extern bool localAlarmPressed;
extern bool ledButtonPressed;

extern volatile bool rescueMessageReceived;
extern volatile int receivedMessageID;
extern volatile uint32_t receivedMessageSequence;
extern volatile int receivedMessagePriority;
extern char receivedRescueMessage[MAX_RESCUE_MESSAGE_LENGTH + 1];
extern char currentRescueMessageSource[12];

extern int lastReceivedMessageID;
extern uint32_t lastReceivedMessageSequence;

extern RescueMessageRecord rescueMessageQueue[RESCUE_MESSAGE_QUEUE_SIZE];
extern volatile uint8_t rescueQueueHead;
extern volatile uint8_t rescueQueueTail;
extern volatile uint8_t rescueQueueCount;
