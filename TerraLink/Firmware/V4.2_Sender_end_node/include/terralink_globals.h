#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <Preferences.h>
#include "esp_sleep.h"
#include "config.h"
#include "terralink_types.h"
extern TinyGPSPlus gps; extern HardwareSerial GPSserial; extern Preferences preferences; extern U8G2_SH1106_128X64_NONAME_F_HW_I2C oled;
extern volatile SystemState currentState; extern volatile bool loraInitialized; extern volatile bool sosTransmitRequest; extern volatile bool finalAckReceived; extern volatile uint32_t activeSequence; extern volatile uint32_t receivedAckSequence; extern portMUX_TYPE timerMux;
extern volatile double currentLatitude,currentLongitude; extern volatile bool gpsFixAvailable; extern uint32_t sequenceNumber;
extern bool pendingSOS; extern uint32_t persistentSOSSequence; extern double persistentSOSLatitude,persistentSOSLongitude; extern bool persistentSOSGPSFix;
extern unsigned long stateStartTime,touchStartTime,lastUIUpdate,gpsWarningStart; extern bool touchWasPressed,sosHoldTriggered,localAlarmPressed,ledButtonPressed;
extern volatile bool rescueMessageReceived; extern volatile int receivedMessageID; extern volatile uint32_t receivedMessageSequence; extern char receivedRescueMessage[MAX_RESCUE_MESSAGE_LENGTH+1]; extern unsigned long rescueMessageDisplayStart;
extern int lastReceivedMessageID; extern uint32_t lastReceivedMessageSequence;
