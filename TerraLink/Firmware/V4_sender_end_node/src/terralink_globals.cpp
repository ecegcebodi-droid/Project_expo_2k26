#include "terralink_globals.h"

TinyGPSPlus gps;
HardwareSerial GPSserial(2);
Preferences preferences;
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

volatile SystemState currentState = STATE_AWAKENING;
volatile bool loraInitialized = false;
volatile bool sosTransmitRequest = false;
volatile bool finalAckReceived = false;
volatile uint32_t activeSequence = 0;
volatile uint32_t receivedAckSequence = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile double currentLatitude = 0.0;
volatile double currentLongitude = 0.0;
volatile bool gpsFixAvailable = false;

uint32_t sequenceNumber = 0;
bool pendingSOS = false;
uint32_t persistentSOSSequence = 0;
double persistentSOSLatitude = 0.0;
double persistentSOSLongitude = 0.0;
bool persistentSOSGPSFix = false;

unsigned long stateStartTime = 0;
unsigned long touchStartTime = 0;
unsigned long lastUIUpdate = 0;
unsigned long gpsWarningStart = 0;
unsigned long rescueMessageDisplayStart = 0;

bool touchWasPressed = false;
bool sosHoldTriggered = false;
bool localAlarmPressed = false;
bool ledButtonPressed = false;

volatile bool rescueMessageReceived = false;
volatile int receivedMessageID = 0;
volatile uint32_t receivedMessageSequence = 0;
volatile int receivedMessagePriority = 0;
char receivedRescueMessage[MAX_RESCUE_MESSAGE_LENGTH + 1] = "";
char currentRescueMessageSource[12] = "RESCUE";

int lastReceivedMessageID = -1;
uint32_t lastReceivedMessageSequence = 0;

RescueMessageRecord rescueMessageQueue[RESCUE_MESSAGE_QUEUE_SIZE];
volatile uint8_t rescueQueueHead = 0;
volatile uint8_t rescueQueueTail = 0;
volatile uint8_t rescueQueueCount = 0;
