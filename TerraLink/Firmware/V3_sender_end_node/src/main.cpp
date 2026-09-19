/*
   ============================================================
                       TERRALINK SENDER
   ============================================================

   REFINED DYNAMIC VERSION
   ------------------------------------------------------------
   Persistent SOS + periodic wake-up retransmission
   + Rescue/local/website response message reception
   + OLED message notification
   + Message ACK
   + Message queue
   + Optional SOURCE and PRIORITY support

   MCU:
      ESP32-WROOM-32E / ESP32-WROOM-32S

   LoRa:
      AI-Thinker RA-02 / SX1278
      Frequency = 433 MHz

   GPS:
      u-blox NEO-M8N

   OLED:
      0.96" SH1106
      128 x 64
      I2C

   INPUTS:
      Touch/SOS       = GPIO32
      Local Alarm     = GPIO33
      LED Button      = GPIO13

   OUTPUTS:
      Buzzer          = GPIO25
      LED / Torch     = GPIO4

   ============================================================
                       CORE DISTRIBUTION
   ============================================================

   CORE 0:
      - LoRa
      - SOS transmission
      - ACK reception
      - Rescue message reception
      - MSGACK transmission
      - Location response

   CORE 1:
      - GPS
      - Touch/SOS button
      - Local alarm button
      - LED button
      - OLED
      - Buzzer control
      - LED control
      - State machine
      - Persistent SOS management

   ============================================================
                    PERSISTENT SOS SYSTEM
   ============================================================

   New SOS:
          ↓
   Create sequence number
          ↓
   Save SOS to ESP32 NVS flash
          ↓
   Transmit SOS
          ↓
   Wait for ACK
       ↙       ↘
     ACK       NO ACK
      ↓           ↓
   Clear NVS    Keep NVS
      ↓           ↓
   SUCCESS    Deep Sleep
                  ↓
             Timer Wake
                  ↓
             Retransmit
                  ↓
             Wait for ACK
                  ↓
                 Repeat

   SAME sequence number is used for every retransmission.

   ============================================================
                 RESCUE MESSAGE SYSTEM
   ============================================================

   Receiver #1 can send:

   SRC:4,DST:1,SEQ:54,TTL:5,HOP:0,
   TYPE:MSG,PRIORITY:0,MSG:STAY CALM

   Optional enhanced packet:

   SRC:4,DST:1,SEQ:54,TTL:5,HOP:0,
   MSGID:12,SOURCE:LOCAL,PRIORITY:1,
   TYPE:MSG,MSG:STAY CALM

   Sender:
      ↓
   Validate routing
      ↓
   Extract message
      ↓
   Generate MSGID if missing
      ↓
   Send MSGACK
      ↓
   Queue message
      ↓
   OLED display
      ↓
   Buzzer notification

   ============================================================
*/


#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <Preferences.h>
#include "esp_sleep.h"


// ============================================================
//                    PIN CONFIGURATION
// ============================================================

// ---------------- LoRa RA-02 / SX1278 ----------------

#define LORA_SCK       18
#define LORA_MISO      19
#define LORA_MOSI      23
#define LORA_SS        5
#define LORA_RST       27
#define LORA_DIO0      26


// ---------------- GPS NEO-M8N ----------------

#define GPS_RX_PIN     16
#define GPS_TX_PIN     17

// ---------------- OLED SH1106 ----------------

#define OLED_SDA       21
#define OLED_SCL       22


// ---------------- User Interface ----------------

#define TOUCH_PIN          32
#define LOCAL_ALARM_PIN    33
#define LED_BUTTON_PIN     13

#define BUZZER_PIN         25
#define TORCH_PIN          4


// ---------------- Node ID ----------------

#define NODE_ID        1


// ============================================================
//                    ROUTING CONFIGURATION
// ============================================================

#define RECEIVER_NODE_ID       4
#define BROADCAST_ID           255

#define SOS_INITIAL_TTL        5
#define REVERSE_PACKET_TTL     5


// ============================================================
//                    LORA CONFIGURATION
// ============================================================

#define LORA_FREQUENCY          433E6

#define LORA_SYNC_WORD          0xF3

#define LORA_SPREADING_FACTOR   7

#define LORA_BANDWIDTH          125E3

#define LORA_CODING_RATE        5

#define LORA_TX_POWER           17


// ============================================================
//                    TIMING CONFIGURATION
// ============================================================

#define AWAKENING_TIME_MS          2000UL

#define NORMAL_AWAKE_TIME_MS       60000UL

#define POST_SOS_AWAKE_TIME_MS     60000UL

#define SOS_LONG_PRESS_MS          2000UL

#define GPS_MESSAGE_TIME_MS        4000UL

#define ACK_WAIT_TIMEOUT_MS        15000UL

#define SUCCESS_DISPLAY_MS         5000UL

#define FAILURE_DISPLAY_MS         5000UL

#define SENDING_DISPLAY_MS         1500UL

#define UI_UPDATE_MS               120UL

#define RESCUE_MESSAGE_DISPLAY_MS  8000UL


// ============================================================
//              PERSISTENT SOS / POWER MANAGEMENT
// ============================================================

#define SOS_RETRY_INTERVAL_SEC     30

#define NORMAL_SLEEP_INTERVAL_SEC  60


// ============================================================
//              RESCUE MESSAGE CONFIGURATION
// ============================================================

#define MAX_RESCUE_MESSAGE_LENGTH  120

#define RESCUE_MESSAGE_QUEUE_SIZE  5


// ============================================================
//                    OBJECTS
// ============================================================

TinyGPSPlus gps;

HardwareSerial GPSserial(2);

Preferences preferences;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE
);


// ============================================================
//                    SYSTEM STATES
// ============================================================

enum SystemState
{
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


// ============================================================
//                    SHARED VARIABLES
// ============================================================

volatile SystemState currentState =
  STATE_AWAKENING;


volatile bool loraInitialized =
  false;


volatile bool sosTransmitRequest =
  false;


volatile bool finalAckReceived =
  false;


volatile uint32_t activeSequence =
  0;


volatile uint32_t receivedAckSequence =
  0;


portMUX_TYPE timerMux =
  portMUX_INITIALIZER_UNLOCKED;


// ============================================================
//                    GPS VARIABLES
// ============================================================

volatile double currentLatitude =
  0.0;


volatile double currentLongitude =
  0.0;


volatile bool gpsFixAvailable =
  false;


// ============================================================
//                    SEQUENCE
// ============================================================

uint32_t sequenceNumber =
  0;


// ============================================================
//               PERSISTENT SOS VARIABLES
// ============================================================

bool pendingSOS =
  false;


uint32_t persistentSOSSequence =
  0;


double persistentSOSLatitude =
  0.0;


double persistentSOSLongitude =
  0.0;


bool persistentSOSGPSFix =
  false;


// ============================================================
//                    TIMERS
// ============================================================

unsigned long stateStartTime =
  0;


unsigned long touchStartTime =
  0;


unsigned long lastUIUpdate =
  0;


unsigned long gpsWarningStart =
  0;


unsigned long rescueMessageDisplayStart =
  0;


// ============================================================
//                    TOUCH VARIABLES
// ============================================================

bool touchWasPressed =
  false;


bool sosHoldTriggered =
  false;


// ============================================================
//                    LOCAL ALARM
// ============================================================

bool localAlarmPressed =
  false;


// ============================================================
//                    LED
// ============================================================

bool ledButtonPressed =
  false;


// ============================================================
//              RESCUE MESSAGE VARIABLES
// ============================================================

volatile bool rescueMessageReceived =
  false;


volatile int receivedMessageID =
  0;


volatile uint32_t receivedMessageSequence =
  0;


volatile int receivedMessagePriority =
  0;


char receivedRescueMessage[
  MAX_RESCUE_MESSAGE_LENGTH + 1
] = "";


char currentRescueMessageSource[
  12
] = "RESCUE";


// ============================================================
//          DUPLICATE MESSAGE PROTECTION
// ============================================================

int lastReceivedMessageID =
  -1;


uint32_t lastReceivedMessageSequence =
  0;


// ============================================================
//              RESCUE MESSAGE QUEUE
// ============================================================

struct RescueMessageRecord
{
  int messageID;

  uint32_t sequence;

  int priority;

  char source[12];

  char message[
    MAX_RESCUE_MESSAGE_LENGTH + 1
  ];
};


RescueMessageRecord rescueMessageQueue[
  RESCUE_MESSAGE_QUEUE_SIZE
];


volatile uint8_t rescueQueueHead =
  0;


volatile uint8_t rescueQueueTail =
  0;


volatile uint8_t rescueQueueCount =
  0;


// ============================================================
//              PERSISTENT SOS FUNCTIONS
// ============================================================

void loadPersistentData()
{
  preferences.begin(
    "terralink",
    false
  );


  sequenceNumber =
    preferences.getUInt(
      "sequence",
      0
    );


  pendingSOS =
    preferences.getBool(
      "pending",
      false
    );


  if (pendingSOS)
  {
    persistentSOSSequence =
      preferences.getUInt(
        "sos_seq",
        0
      );


    persistentSOSLatitude =
      preferences.getDouble(
        "lat",
        0.0
      );


    persistentSOSLongitude =
      preferences.getDouble(
        "lon",
        0.0
      );


    persistentSOSGPSFix =
      preferences.getBool(
        "gpsfix",
        false
      );


    activeSequence =
      persistentSOSSequence;


    Serial.println();
    Serial.println(
      "================================"
    );

    Serial.println(
      "PERSISTENT SOS FOUND IN FLASH"
    );

    Serial.print(
      "Sequence: "
    );

    Serial.println(
      persistentSOSSequence
    );


    if (persistentSOSGPSFix)
    {
      Serial.print(
        "Stored LAT: "
      );

      Serial.println(
        persistentSOSLatitude,
        6
      );


      Serial.print(
        "Stored LON: "
      );

      Serial.println(
        persistentSOSLongitude,
        6
      );
    }
    else
    {
      Serial.println(
        "Stored GPS: NO FIX"
      );
    }


    Serial.println(
      "SOS WILL BE RETRANSMITTED"
    );


    Serial.println(
      "================================"
    );
  }
  else
  {
    Serial.println(
      "No pending SOS in flash."
    );
  }
}


// ============================================================
//                    SAVE SEQUENCE
// ============================================================

void saveSequenceNumber()
{
  preferences.putUInt(
    "sequence",
    sequenceNumber
  );
}


// ============================================================
//                    SAVE SOS
// ============================================================

void savePendingSOS()
{
  preferences.putBool(
    "pending",
    true
  );


  preferences.putUInt(
    "sos_seq",
    persistentSOSSequence
  );


  preferences.putDouble(
    "lat",
    persistentSOSLatitude
  );


  preferences.putDouble(
    "lon",
    persistentSOSLongitude
  );


  preferences.putBool(
    "gpsfix",
    persistentSOSGPSFix
  );


  pendingSOS =
    true;


  Serial.println();
  Serial.println(
    "SOS SAVED TO ESP32 FLASH"
  );


  Serial.print(
    "Persistent sequence = "
  );


  Serial.println(
    persistentSOSSequence
  );
}


// ============================================================
//                    CLEAR SOS
// ============================================================

void clearPendingSOS()
{
  preferences.putBool(
    "pending",
    false
  );


  preferences.remove(
    "sos_seq"
  );


  preferences.remove(
    "lat"
  );


  preferences.remove(
    "lon"
  );


  preferences.remove(
    "gpsfix"
  );


  pendingSOS =
    false;


  persistentSOSSequence =
    0;


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "PERSISTENT SOS CLEARED"
  );

  Serial.println(
    "FINAL ACK CONFIRMED"
  );

  Serial.println(
    "================================"
  );
}


// ============================================================
//                    BUZZER FUNCTIONS
// ============================================================

void buzzerOn()
{
  digitalWrite(
    BUZZER_PIN,
    HIGH
  );
}


void buzzerOff()
{
  digitalWrite(
    BUZZER_PIN,
    LOW
  );
}


void beep(
  unsigned int duration
)
{
  buzzerOn();

  delay(duration);

  buzzerOff();
}


void buzzerWake()
{
  beep(100);
}


void buzzerSOSActivated()
{
  beep(120);

  delay(120);

  beep(120);

  delay(120);

  beep(120);
}


void buzzerSending()
{
  beep(80);

  delay(80);

  beep(80);

  delay(80);

  beep(80);
}


void buzzerWaiting()
{
  beep(70);

  delay(70);

  beep(70);
}


void buzzerSuccess()
{
  beep(120);

  delay(120);

  beep(120);

  delay(300);

  beep(300);
}


void buzzerFailure()
{
  beep(350);

  delay(350);

  beep(350);
}


// ------------------------------------------------------------
// Rescue message notification
// ------------------------------------------------------------

void buzzerRescueMessage()
{
  /*
     Two short beeps.

     This is intentionally different from:
       - SOS activation
       - SOS transmission
       - ACK success
       - ACK failure
  */

  beep(120);

  delay(100);

  beep(120);
}


// ------------------------------------------------------------
// Priority notification
// ------------------------------------------------------------

void buzzerPriorityMessage(
  int priority
)
{
  if (priority >= 2)
  {
    // Critical: three rapid beeps

    beep(150);

    delay(80);

    beep(150);

    delay(80);

    beep(150);
  }
  else if (priority == 1)
  {
    // Important: two beeps

    beep(120);

    delay(100);

    beep(120);
  }
  else
  {
    // Normal

    buzzerRescueMessage();
  }
}


// ============================================================
//                    LOCAL ALARM
// ============================================================

void handleLocalAlarm()
{
  localAlarmPressed =
    (
      digitalRead(
        LOCAL_ALARM_PIN
      ) == LOW
    );


  if (localAlarmPressed)
  {
    buzzerOn();
  }
  else
  {
    buzzerOff();
  }
}


// ============================================================
//                    LED CONTROL
// ============================================================

void handleLEDButton()
{
  ledButtonPressed =
    (
      digitalRead(
        LED_BUTTON_PIN
      ) == LOW
    );


  if (ledButtonPressed)
  {
    digitalWrite(
      TORCH_PIN,
      HIGH
    );
  }
  else
  {
    digitalWrite(
      TORCH_PIN,
      LOW
    );
  }
}


// ============================================================
//                    OLED HEADER
// ============================================================

void drawHeader()
{
  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    0,
    9,
    "TERRALINK"
  );


  oled.setCursor(
    103,
    9
  );


  oled.print("N");

  oled.print(NODE_ID);


  oled.drawLine(
    0,
    13,
    127,
    13
  );
}


// ============================================================
//                    CHECK ICON
// ============================================================

void drawNakedCheck(
  int x,
  int y
)
{
  oled.drawLine(
    x - 5,
    y,
    x,
    y + 5
  );


  oled.drawLine(
    x,
    y + 5,
    x + 9,
    y - 6
  );
}


// ============================================================
//                    CROSS ICON
// ============================================================

void drawNakedCross(
  int x,
  int y
)
{
  oled.drawLine(
    x - 5,
    y - 5,
    x + 5,
    y + 5
  );


  oled.drawLine(
    x + 5,
    y - 5,
    x - 5,
    y + 5
  );
}


// ============================================================
//                    GPS ICON
// ============================================================

void drawGPSIcon(
  int x,
  int y,
  bool fixed
)
{
  oled.drawCircle(
    x,
    y,
    5
  );


  oled.drawLine(
    x,
    y + 5,
    x,
    y + 9
  );


  if (fixed)
  {
    drawNakedCheck(
      x + 7,
      y
    );
  }
  else
  {
    oled.drawLine(
      x - 3,
      y - 3,
      x + 3,
      y + 3
    );


    oled.drawLine(
      x + 3,
      y - 3,
      x - 3,
      y + 3
    );
  }
}


// ============================================================
//                    WARNING SYMBOL
// ============================================================

void drawWarningIcon(
  int x,
  int y
)
{
  oled.drawLine(
    x,
    y - 7,
    x - 7,
    y + 6
  );


  oled.drawLine(
    x - 7,
    y + 6,
    x + 7,
    y + 6
  );


  oled.drawLine(
    x + 7,
    y + 6,
    x,
    y - 7
  );


  oled.drawLine(
    x,
    y - 3,
    x,
    y + 2
  );


  oled.drawPixel(
    x,
    y + 4
  );
}


// ============================================================
//                    RADIO WAVES
// ============================================================

void drawRadioWaves(
  int x,
  int y,
  int phase
)
{
  oled.drawDisc(
    x,
    y,
    2
  );


  if (phase >= 1)
  {
    oled.drawCircle(
      x,
      y,
      6
    );
  }


  if (phase >= 2)
  {
    oled.drawCircle(
      x,
      y,
      10
    );
  }


  if (phase >= 3)
  {
    oled.drawCircle(
      x,
      y,
      14
    );
  }
}


// ============================================================
//                    PROGRESS BAR
// ============================================================

void drawProgressBar(
  int x,
  int y,
  int width,
  int height,
  int percent
)
{
  percent =
    constrain(
      percent,
      0,
      100
    );


  oled.drawFrame(
    x,
    y,
    width,
    height
  );


  int fillWidth =
    (
      (width - 2)
      *
      percent
    )
    /
    100;


  if (fillWidth > 0)
  {
    oled.drawBox(
      x + 1,
      y + 1,
      fillWidth,
      height - 2
    );
  }
}


// ============================================================
//              RESCUE MESSAGE OLED SCREEN
// ============================================================

void drawRescueMessageScreen()
{
  oled.clearBuffer();


  drawHeader();


  // ----------------------------------------------------------
  // Title
  // ----------------------------------------------------------

  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    28,
    24,
    "RESCUE MESSAGE"
  );


  oled.drawLine(
    0,
    27,
    127,
    27
  );


  // ----------------------------------------------------------
  // Source
  // ----------------------------------------------------------

  oled.setFont(
    u8g2_font_5x8_tf
  );


  oled.drawStr(
    2,
    37,
    "FROM:"
  );


  oled.setCursor(
    30,
    37
  );


  oled.print(
    currentRescueMessageSource
  );


  // ----------------------------------------------------------
  // Priority
  // ----------------------------------------------------------

  oled.setCursor(
    82,
    37
  );


  oled.print(
    "P"
  );


  oled.print(
    receivedMessagePriority
  );


  // ----------------------------------------------------------
  // Message
  // ----------------------------------------------------------

  String message =
    String(
      receivedRescueMessage
    );


  int line =
    0;


  String currentLine =
    "";


  for (
    unsigned int i = 0;
    i < message.length();
    i++
  )
  {
    currentLine +=
      message[i];


    if (
      currentLine.length() >= 21
      ||
      i == message.length() - 1
    )
    {
      oled.drawStr(
        2,
        47 + (line * 8),
        currentLine.c_str()
      );


      currentLine =
        "";


      line++;


      if (line >= 2)
      {
        break;
      }
    }
  }


  // ----------------------------------------------------------
  // ACK indication
  // ----------------------------------------------------------

  oled.setCursor(
    2,
    62
  );


  oled.print(
    "ID:"
  );


  oled.print(
    receivedMessageID
  );


  oled.print(
    " ACK"
  );


  oled.sendBuffer();
}


// ============================================================
//                    UPDATE OLED
// ============================================================

void updateOLED()
{
  if (
    millis() -
    lastUIUpdate <
    UI_UPDATE_MS
  )
  {
    return;
  }


  lastUIUpdate =
    millis();


  // ----------------------------------------------------------
  // Rescue message screen has priority.
  // ----------------------------------------------------------

  if (
    currentState ==
    STATE_RESCUE_MESSAGE
  )
  {
    drawRescueMessageScreen();

    return;
  }


  oled.clearBuffer();


  drawHeader();


  // ==========================================================
  // AWAKENING
  // ==========================================================

  if (
    currentState ==
    STATE_AWAKENING
  )
  {
    oled.setFont(
      u8g2_font_6x10_tf
    );


    oled.drawStr(
      36,
      30,
      "STARTING"
    );


    oled.drawStr(
      30,
      46,
      "PLEASE WAIT"
    );


    int phase =
      (
        millis() / 250
      )
      %
      4;


    for (
      int i = 0;
      i < 4;
      i++
    )
    {
      if (
        i == phase
      )
      {
        oled.drawDisc(
          49 + (i * 10),
          57,
          2
        );
      }
      else
      {
        oled.drawCircle(
          49 + (i * 10),
          57,
          2
        );
      }
    }


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // WAIT RELEASE
  // ==========================================================

  if (
    currentState ==
    STATE_WAIT_RELEASE
  )
  {
    oled.setFont(
      u8g2_font_6x10_tf
    );


    oled.drawStr(
      28,
      29,
      "BUTTON HELD"
    );


    oled.drawStr(
      18,
      43,
      "RELEASE FIRST"
    );


    oled.setFont(
      u8g2_font_5x8_tf
    );


    oled.drawStr(
      24,
      58,
      "THEN HOLD 2s"
    );


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // READY
  // ==========================================================

  if (
    currentState ==
    STATE_READY
  )
  {
    oled.setFont(
      u8g2_font_6x10_tf
    );


    oled.drawStr(
      48,
      27,
      "READY"
    );


    oled.setFont(
      u8g2_font_5x8_tf
    );


    oled.drawStr(
      30,
      48,
      "EMERGENCY SOS"
    );


    oled.drawFrame(
      27,
      51,
      74,
      12
    );


    oled.drawStr(
      36,
      60,
      "HOLD 2 SEC"
    );


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // HOLDING SOS
  // ==========================================================

  if (
    currentState ==
    STATE_HOLDING_SOS
  )
  {
    oled.setFont(
      u8g2_font_6x10_tf
    );


    oled.drawStr(
      36,
      27,
      "SOS HOLDING"
    );


    unsigned long held =
      millis() -
      touchStartTime;


    int percent =
      map(
        constrain(
          (long)held,
          0L,
          (long)SOS_LONG_PRESS_MS
        ),
        0,
        SOS_LONG_PRESS_MS,
        0,
        100
      );


    float remaining =
      (
        float
      )(
        SOS_LONG_PRESS_MS -
        held
      )
      /
      1000.0;


    if (
      remaining < 0
    )
    {
      remaining =
        0;
    }


    oled.setFont(
      u8g2_font_7x13B_tf
    );


    oled.setCursor(
      50,
      43
    );


    oled.print(
      remaining,
      1
    );


    oled.print(
      "s"
    );


    drawProgressBar(
      10,
      50,
      108,
      9,
      percent
    );


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // GPS / SOS ACTIVATED
  // ==========================================================

  if (
    currentState ==
    STATE_SOS_ACTIVATED
  )
  {
    oled.setFont(
      u8g2_font_5x8_tf
    );


    if (
      gpsFixAvailable
    )
    {
      drawGPSIcon(
        13,
        24,
        true
      );


      oled.drawStr(
        24,
        27,
        "GPS LOCATION OK"
      );


      oled.drawStr(
        0,
        39,
        "LAT:"
      );


      oled.setCursor(
        22,
        39
      );


      oled.print(
        currentLatitude,
        6
      );


      oled.drawStr(
        0,
        50,
        "LON:"
      );


      oled.setCursor(
        22,
        50
      );


      oled.print(
        currentLongitude,
        6
      );


      oled.drawStr(
        28,
        61,
        "PREPARING SOS..."
      );
    }
    else
    {
      drawWarningIcon(
        12,
        25
      );


      oled.setFont(
        u8g2_font_6x10_tf
      );


      oled.drawStr(
        25,
        27,
        "GPS SIGNAL WEAK"
      );


      oled.setFont(
        u8g2_font_5x8_tf
      );


      oled.drawStr(
        38,
        39,
        "MOVE TO OPEN"
      );


      oled.drawStr(
        51,
        49,
        "SPACE"
      );


      oled.drawStr(
        22,
        60,
        "SOS WILL STILL SEND"
      );
    }


    if (
      !gpsFixAvailable
    )
    {
      unsigned long elapsed =
        millis() -
        gpsWarningStart;


      int percent =
        map(
          constrain(
            (long)elapsed,
            0L,
            (long)GPS_MESSAGE_TIME_MS
          ),
          0,
          GPS_MESSAGE_TIME_MS,
          0,
          100
        );


      drawProgressBar(
        0,
        61,
        20,
        3,
        percent
      );
    }


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // SENDING SOS
  // ==========================================================

  if (
    currentState ==
    STATE_SENDING
  )
  {
    oled.setFont(
      u8g2_font_6x10_tf
    );


    oled.drawStr(
      38,
      25,
      "SENDING SOS"
    );


    oled.setFont(
      u8g2_font_5x8_tf
    );


    oled.drawStr(
      0,
      37,
      "LAT:"
    );


    oled.setCursor(
      22,
      37
    );


    if (
      gpsFixAvailable
    )
    {
      oled.print(
        currentLatitude,
        6
      );
    }
    else
    {
      oled.print(
        "0.000000"
      );
    }


    oled.drawStr(
      0,
      47,
      "LON:"
    );


    oled.setCursor(
      22,
      47
    );


    if (
      gpsFixAvailable
    )
    {
      oled.print(
        currentLongitude,
        6
      );
    }
    else
    {
      oled.print(
        "0.000000"
      );
    }


    if (
      gpsFixAvailable
    )
    {
      oled.drawStr(
        0,
        58,
        "GPS: FIX"
      );
    }
    else
    {
      oled.drawStr(
        0,
        58,
        "GPS: NO FIX"
      );
    }


    int phase =
      (
        millis() / 180
      )
      %
      4;


    oled.drawDisc(
      87,
      54,
      2
    );


    if (
      phase >= 1
    )
    {
      oled.drawCircle(
        87,
        54,
        5
      );
    }


    if (
      phase >= 2
    )
    {
      oled.drawCircle(
        87,
        54,
        9
      );
    }


    if (
      phase >= 3
    )
    {
      oled.drawCircle(
        87,
        54,
        13
      );
    }


    oled.drawStr(
      106,
      58,
      "TX"
    );


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // WAITING FOR RESCUE
  // ==========================================================

  if (
    currentState ==
    STATE_WAITING_ACK
  )
  {
    oled.setFont(
      u8g2_font_5x8_tf
    );


    oled.drawStr(
      25,
      22,
      "WAITING FOR RESCUE"
    );


    oled.drawStr(
      43,
      34,
      "HELP LINK"
    );


    unsigned long elapsed =
      millis() -
      stateStartTime;


    long remaining =
      (
        (
          long
        )ACK_WAIT_TIMEOUT_MS -
        (
          long
        )elapsed
      )
      /
      1000;


    if (
      remaining < 0
    )
    {
      remaining =
        0;
    }


    oled.setFont(
      u8g2_font_7x13B_tf
    );


    oled.setCursor(
      42,
      49
    );


    oled.print(
      remaining
    );


    oled.print(
      "s"
    );


    int phase =
      (
        millis() / 220
      )
      %
      4;


    drawRadioWaves(
      88,
      43,
      phase
    );


    int percent =
      map(
        constrain(
          (long)elapsed,
          0L,
          (long)ACK_WAIT_TIMEOUT_MS
        ),
        0,
        ACK_WAIT_TIMEOUT_MS,
        100,
        0
      );


    drawProgressBar(
      10,
      56,
      108,
      6,
      percent
    );


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // SUCCESS
  // ==========================================================

  if (
    currentState ==
    STATE_SOS_SUCCESS
  )
  {
    oled.setFont(
      u8g2_font_6x10_tf
    );


    drawNakedCheck(
      60,
      29
    );


    oled.drawStr(
      26,
      45,
      "HELP RECEIVED"
    );


    oled.setFont(
      u8g2_font_5x8_tf
    );


    oled.drawStr(
      29,
      59,
      "RESCUE ALERTED"
    );


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // FAILURE
  // ==========================================================

  if (
    currentState ==
    STATE_SOS_FAILURE
  )
  {
    oled.setFont(
      u8g2_font_6x10_tf
    );


    oled.drawStr(
      38,
      28,
      "SOS SENT"
    );


    oled.drawLine(
      43,
      37,
      51,
      37
    );


    oled.drawLine(
      57,
      37,
      65,
      37
    );


    oled.drawLine(
      71,
      37,
      79,
      37
    );


    oled.drawStr(
      46,
      46,
      "NO REPLY"
    );


    oled.setFont(
      u8g2_font_5x8_tf
    );


    oled.drawStr(
      22,
      59,
      "HOLD 2s TO RESEND"
    );


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // POST SOS
  // ==========================================================

  if (
    currentState ==
    STATE_POST_SOS
  )
  {
    oled.setFont(
      u8g2_font_5x8_tf
    );


    oled.drawStr(
      30,
      24,
      "HELP MODE ACTIVE"
    );


    int pulse =
      (
        millis() / 400
      )
      %
      2;


    if (pulse)
    {
      oled.drawDisc(
        22,
        36,
        3
      );
    }
    else
    {
      oled.drawCircle(
        22,
        36,
        3
      );
    }


    oled.drawStr(
      32,
      39,
      "LINK ACTIVE"
    );


    oled.drawFrame(
      20,
      45,
      88,
      13
    );


    oled.drawStr(
      29,
      54,
      "HOLD 2s RESEND"
    );


    oled.sendBuffer();

    return;
  }


  // ==========================================================
  // HOLDING RESEND
  // ==========================================================

  if (
    currentState ==
    STATE_HOLDING_RESEND
  )
  {
    oled.setFont(
      u8g2_font_6x10_tf
    );


    oled.drawStr(
      36,
      27,
      "RESEND SOS"
    );


    unsigned long held =
      millis() -
      touchStartTime;


    int percent =
      map(
        constrain(
          (long)held,
          0L,
          (long)SOS_LONG_PRESS_MS
        ),
        0,
        SOS_LONG_PRESS_MS,
        0,
        100
      );


    float remaining =
      (
        float
      )(
        SOS_LONG_PRESS_MS -
        held
      )
      /
      1000.0;


    if (
      remaining < 0
    )
    {
      remaining =
        0;
    }


    oled.setFont(
      u8g2_font_7x13B_tf
    );


    oled.setCursor(
      50,
      43
    );


    oled.print(
      remaining,
      1
    );


    oled.print(
      "s"
    );


    drawProgressBar(
      10,
      50,
      108,
      9,
      percent
    );


    oled.sendBuffer();

    return;
  }
}


// ============================================================
//                    STATE TRANSITION
// ============================================================

void goToState(
  SystemState newState
)
{
  currentState =
    newState;


  stateStartTime =
    millis();


  lastUIUpdate =
    0;
}


// ============================================================
//                    GPS UPDATE
// ============================================================

void updateGPS()
{
  while (
    GPSserial.available()
  )
  {
    gps.encode(
      GPSserial.read()
    );
  }


  if (
    gps.location.isValid()
  )
  {
    currentLatitude =
      gps.location.lat();


    currentLongitude =
      gps.location.lng();


    gpsFixAvailable =
      true;
  }
  else
  {
    gpsFixAvailable =
      false;
  }
}


// ============================================================
//                    ROUTING HELPERS
// ============================================================

bool getPacketField(
  const String &packet,
  const char *field,
  String &value
)
{
  String token =
    String(field);


  int searchFrom =
    0;


  while (true)
  {
    int pos =
      packet.indexOf(
        token,
        searchFrom
      );


    if (
      pos < 0
    )
    {
      return false;
    }


    if (
      pos == 0
      ||
      packet.charAt(pos - 1) == ','
    )
    {
      int valueStart =
        pos +
        token.length();


      int valueEnd =
        packet.indexOf(
          ',',
          valueStart
        );


      if (
        valueEnd < 0
      )
      {
        valueEnd =
          packet.length();
      }


      value =
        packet.substring(
          valueStart,
          valueEnd
        );


      value.trim();


      return true;
    }


    searchFrom =
      pos + 1;
  }
}


// ============================================================
//                    PACKET TYPE
// ============================================================

bool packetHasType(
  const String &packet,
  const char *type
)
{
  String value;


  if (
    !getPacketField(
      packet,
      "TYPE:",
      value
    )
  )
  {
    return false;
  }


  return value.equalsIgnoreCase(
    type
  );
}


// ============================================================
//                    CREATE SOS PACKET
// ============================================================

String createSOSPacket()
{
  String packet;

  packet.reserve(
    180
  );


  packet +=
    "SRC:";


  packet +=
    String(
      NODE_ID
    );


  packet +=
    ",DST:";


  packet +=
    String(
      BROADCAST_ID
    );


  packet +=
    ",SEQ:";


  packet +=
    String(
      activeSequence
    );


  packet +=
    ",TTL:";


  packet +=
    String(
      SOS_INITIAL_TTL
    );


  packet +=
    ",HOP:0";


  packet +=
    ",LAT:";


  if (
    pendingSOS
  )
  {
    packet +=
      String(
        persistentSOSLatitude,
        6
      );
  }
  else if (
    gpsFixAvailable
  )
  {
    packet +=
      String(
        currentLatitude,
        6
      );
  }
  else
  {
    packet +=
      "0.000000";
  }


  packet +=
    ",LON:";


  if (
    pendingSOS
  )
  {
    packet +=
      String(
        persistentSOSLongitude,
        6
      );
  }
  else if (
    gpsFixAvailable
  )
  {
    packet +=
      String(
        currentLongitude,
        6
      );
  }
  else
  {
    packet +=
      "0.000000";
  }


  packet +=
    ",TYPE:SOS";


  return packet;
}


// ============================================================
//                    SEND SOS
// ============================================================

void sendSOS()
{
  if (
    !loraInitialized
  )
  {
    Serial.println(
      "ERROR: LoRa not initialized."
    );

    return;
  }


  String packet =
    createSOSPacket();


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "TERRALINK ROUTED SOS TRANSMISSION"
  );


  Serial.print(
    "Packet: "
  );


  Serial.println(
    packet
  );


  Serial.print(
    "Sequence: "
  );


  Serial.println(
    activeSequence
  );


  Serial.println(
    "Destination: BROADCAST / MESH"
  );


  Serial.print(
    "Initial TTL: "
  );


  Serial.println(
    SOS_INITIAL_TTL
  );


  Serial.println(
    "Hop: 0 (sender)"
  );


  if (
    pendingSOS &&
    persistentSOSGPSFix
  )
  {
    Serial.print(
      "Stored GPS Latitude: "
    );


    Serial.println(
      persistentSOSLatitude,
      6
    );


    Serial.print(
      "Stored GPS Longitude: "
    );


    Serial.println(
      persistentSOSLongitude,
      6
    );
  }
  else if (
    gpsFixAvailable
  )
  {
    Serial.print(
      "GPS Latitude: "
    );


    Serial.println(
      currentLatitude,
      6
    );


    Serial.print(
      "GPS Longitude: "
    );


    Serial.println(
      currentLongitude,
      6
    );
  }
  else
  {
    Serial.println(
      "GPS: NO FIX"
    );


    Serial.println(
      "LAT/LON = 0.000000"
    );


    Serial.println(
      "SOS transmission NOT blocked."
    );
  }


  LoRa.idle();


  LoRa.beginPacket();


  LoRa.print(
    packet
  );


  int result =
    LoRa.endPacket();


  if (
    result == 1
  )
  {
    Serial.println(
      "Routed SOS transmitted successfully."
    );
  }
  else
  {
    Serial.println(
      "SOS transmission failed."
    );
  }


  LoRa.receive();


  Serial.println(
    "LoRa returned to RX mode."
  );


  Serial.println(
    "================================"
  );


  Serial.println();
}


// ============================================================
//              SEND ROUTED MESSAGE ACK
// ============================================================

void sendMessageACK(
  int messageID,
  uint32_t sequence
)
{
  String ackPacket =
    "SRC:" +
    String(NODE_ID) +

    ",DST:" +
    String(RECEIVER_NODE_ID) +

    ",SEQ:" +
    String(sequence) +

    ",TTL:" +
    String(REVERSE_PACKET_TTL) +

    ",HOP:0" +

    ",MSGID:" +
    String(messageID) +

    ",STATUS:DELIVERED" +

    ",TYPE:MSGACK";


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "SENDING ROUTED MESSAGE ACK"
  );


  Serial.println(
    ackPacket
  );


  LoRa.idle();


  LoRa.beginPacket();


  LoRa.print(
    ackPacket
  );


  int result =
    LoRa.endPacket();


  if (
    result == 1
  )
  {
    Serial.println(
      "MSGACK SENT"
    );
  }
  else
  {
    Serial.println(
      "MSGACK TRANSMISSION FAILED"
    );
  }


  LoRa.receive();


  Serial.println(
    "================================"
  );
}


// ============================================================
//              SEND CURRENT LOCATION
// ============================================================

void sendCurrentLocation()
{
  String packet =
    "SRC:" +
    String(NODE_ID) +

    ",DST:" +
    String(RECEIVER_NODE_ID) +

    ",SEQ:" +
    String(activeSequence) +

    ",TTL:" +
    String(REVERSE_PACKET_TTL) +

    ",HOP:0";


  packet +=
    ",LAT:";


  if (
    gpsFixAvailable
  )
  {
    packet +=
      String(
        currentLatitude,
        6
      );
  }
  else
  {
    packet +=
      "0.000000";
  }


  packet +=
    ",LON:";


  if (
    gpsFixAvailable
  )
  {
    packet +=
      String(
        currentLongitude,
        6
      );
  }
  else
  {
    packet +=
      "0.000000";
  }


  packet +=
    ",TYPE:LOC";


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "SENDING ROUTED LOCATION UPDATE"
  );


  Serial.println(
    packet
  );


  LoRa.idle();


  LoRa.beginPacket();


  LoRa.print(
    packet
  );


  int result =
    LoRa.endPacket();


  if (
    result == 1
  )
  {
    Serial.println(
      "LOCATION UPDATE SENT"
    );
  }
  else
  {
    Serial.println(
      "LOCATION UPDATE FAILED"
    );
  }


  LoRa.receive();


  Serial.println(
    "================================"
  );
}


// ============================================================
//              HANDLE ROUTED RESCUE MESSAGE
// ============================================================

bool handleRescueMessage(
  const String &packet
)
{
  String srcString;
  String dstString;
  String seqString;
  String msgIDString;
  String sourceString;
  String priorityString;


  // ==========================================================
  // BASIC ROUTING FIELDS
  //
  // MSGID IS OPTIONAL.
  //
  // Your current Receiver #1 packet does not contain MSGID.
  // ==========================================================

  if (
    !getPacketField(
      packet,
      "SRC:",
      srcString
    )
    ||
    !getPacketField(
      packet,
      "DST:",
      dstString
    )
    ||
    !getPacketField(
      packet,
      "SEQ:",
      seqString
    )
  )
  {
    Serial.println(
      "MSG rejected - missing routing fields."
    );


    return false;
  }


  // ==========================================================
  // CHECK TYPE
  // ==========================================================

  if (
    !packetHasType(
      packet,
      "MSG"
    )
  )
  {
    return false;
  }


  // ==========================================================
  // CHECK SOURCE / DESTINATION
  // ==========================================================

  if (
    srcString.toInt()
    !=
    RECEIVER_NODE_ID
    ||
    dstString.toInt()
    !=
    NODE_ID
  )
  {
    Serial.println(
      "MSG ignored - wrong SRC/DST."
    );


    return false;
  }


  // ==========================================================
  // MESSAGE SEQUENCE
  //
  // IMPORTANT:
  //
  // Do NOT compare messageSequence with activeSequence.
  //
  // Rescue/local/website messages can have their own sequence.
  // ==========================================================

  uint32_t messageSequence =
    strtoul(
      seqString.c_str(),
      NULL,
      10
    );


  // ==========================================================
  // MESSAGE TEXT
  // ==========================================================

  int msgStart =
    packet.indexOf(
      ",MSG:"
    );


  if (
    msgStart < 0
  )
  {
    Serial.println(
      "MSG rejected - no message text."
    );


    return false;
  }


  msgStart += 5;


  String message =
    packet.substring(
      msgStart
    );


  // ----------------------------------------------------------
  // Remove TYPE if TYPE appears after MSG.
  // ----------------------------------------------------------

  int typePosition =
    message.indexOf(
      ",TYPE:"
    );


  if (
    typePosition >= 0
  )
  {
    message =
      message.substring(
        0,
        typePosition
      );
  }


  message.trim();


  if (
    message.length() == 0
  )
  {
    Serial.println(
      "MSG rejected - empty message."
    );


    return false;
  }


  if (
    message.length()
    >
    MAX_RESCUE_MESSAGE_LENGTH
  )
  {
    message =
      message.substring(
        0,
        MAX_RESCUE_MESSAGE_LENGTH
      );
  }


  // ==========================================================
  // MESSAGE ID
  //
  // If Receiver #1 sends MSGID -> use it.
  //
  // If not -> generate fallback ID.
  // ==========================================================

  int messageID =
    0;


  if (
    getPacketField(
      packet,
      "MSGID:",
      msgIDString
    )
  )
  {
    messageID =
      msgIDString.toInt();
  }
  else
  {
    /*
       Generate deterministic fallback ID.

       This allows:

       SRC:4,DST:1,SEQ:54,...,
       TYPE:MSG,PRIORITY:0,MSG:STAY CALM

       to work immediately.
    */

    uint32_t hash =
      messageSequence;


    for (
      unsigned int i = 0;
      i < message.length();
      i++
    )
    {
      hash =
        (
          hash * 31UL
        )
        +
        (
          uint8_t
        )message[i];
    }


    messageID =
      (
        int
      )(
        hash % 30000UL
      );


    if (
      messageID <= 0
    )
    {
      messageID =
        1;
    }


    Serial.println(
      "MSGID not present - generated fallback ID."
    );
  }


  // ==========================================================
  // SOURCE
  //
  // Optional:
  //
  // SOURCE:LOCAL
  // SOURCE:WEBSITE
  //
  // If absent:
  //
  // RESCUE
  // ==========================================================

  if (
    getPacketField(
      packet,
      "SOURCE:",
      sourceString
    )
  )
  {
    sourceString.trim();

    sourceString.toUpperCase();
  }
  else
  {
    sourceString =
      "RESCUE";
  }


  if (
    sourceString.length() == 0
  )
  {
    sourceString =
      "RESCUE";
  }


  if (
    sourceString.length() > 11
  )
  {
    sourceString =
      sourceString.substring(
        0,
        11
      );
  }


  // ==========================================================
  // PRIORITY
  //
  // Optional:
  //
  // PRIORITY:0 = Normal
  // PRIORITY:1 = Important
  // PRIORITY:2 = Critical
  // ==========================================================

  int messagePriority =
    0;


  if (
    getPacketField(
      packet,
      "PRIORITY:",
      priorityString
    )
  )
  {
    messagePriority =
      priorityString.toInt();
  }


  messagePriority =
    constrain(
      messagePriority,
      0,
      2
    );


  // ==========================================================
  // DUPLICATE CHECK
  // ==========================================================

  bool duplicate =
    (
      messageID ==
      lastReceivedMessageID
    )
    &&
    (
      messageSequence ==
      lastReceivedMessageSequence
    );


  // ==========================================================
  // SERIAL INFORMATION
  // ==========================================================

  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "ROUTED RESCUE MESSAGE RECEIVED"
  );


  Serial.print(
    "SOURCE: "
  );


  Serial.println(
    sourceString
  );


  Serial.print(
    "MSG ID: "
  );


  Serial.println(
    messageID
  );


  Serial.print(
    "MESSAGE SEQ: "
  );


  Serial.println(
    messageSequence
  );


  Serial.print(
    "ACTIVE SOS SEQ: "
  );


  Serial.println(
    activeSequence
  );


  Serial.print(
    "PRIORITY: "
  );


  Serial.println(
    messagePriority
  );


  Serial.print(
    "MESSAGE: "
  );


  Serial.println(
    message
  );


  if (
    duplicate
  )
  {
    Serial.println(
      "STATUS: DUPLICATE"
    );
  }
  else
  {
    Serial.println(
      "STATUS: NEW MESSAGE"
    );
  }


  Serial.println(
    "================================"
  );


  // ==========================================================
  // ALWAYS SEND MESSAGE ACK
  // ==========================================================

  sendMessageACK(
    messageID,
    messageSequence
  );


  // ==========================================================
  // DUPLICATE
  // ==========================================================

  if (
    duplicate
  )
  {
    Serial.println(
      "Duplicate rescue message."
    );


    Serial.println(
      "MSGACK resent; display suppressed."
    );


    return true;
  }


  // ==========================================================
  // SAVE LAST MESSAGE
  // ==========================================================

  lastReceivedMessageID =
    messageID;


  lastReceivedMessageSequence =
    messageSequence;


  // ==========================================================
  // QUEUE MESSAGE
  // ==========================================================

  portENTER_CRITICAL(
    &timerMux
  );


  if (
    rescueQueueCount
    <
    RESCUE_MESSAGE_QUEUE_SIZE
  )
  {
    RescueMessageRecord &slot =
      rescueMessageQueue[
        rescueQueueTail
      ];


    slot.messageID =
      messageID;


    slot.sequence =
      messageSequence;


    slot.priority =
      messagePriority;


    sourceString.toCharArray(
      slot.source,
      sizeof(slot.source)
    );


    message.toCharArray(
      slot.message,
      sizeof(slot.message)
    );


    rescueQueueTail =
      (
        rescueQueueTail + 1
      )
      %
      RESCUE_MESSAGE_QUEUE_SIZE;


    rescueQueueCount++;
  }
  else
  {
    /*
       Queue full.

       Replace oldest message.
    */

    RescueMessageRecord &slot =
      rescueMessageQueue[
        rescueQueueHead
      ];


    slot.messageID =
      messageID;


    slot.sequence =
      messageSequence;


    slot.priority =
      messagePriority;


    sourceString.toCharArray(
      slot.source,
      sizeof(slot.source)
    );


    message.toCharArray(
      slot.message,
      sizeof(slot.message)
    );


    rescueQueueHead =
      (
        rescueQueueHead + 1
      )
      %
      RESCUE_MESSAGE_QUEUE_SIZE;


    rescueQueueTail =
      rescueQueueHead;
  }


  // ==========================================================
  // LEGACY VARIABLES
  // ==========================================================

  message.toCharArray(
    receivedRescueMessage,
    MAX_RESCUE_MESSAGE_LENGTH + 1
  );


  receivedMessageID =
    messageID;


  receivedMessageSequence =
    messageSequence;


  receivedMessagePriority =
    messagePriority;


  sourceString.toCharArray(
    currentRescueMessageSource,
    sizeof(currentRescueMessageSource)
  );


  rescueMessageReceived =
    true;


  portEXIT_CRITICAL(
    &timerMux
  );


  Serial.println(
    "NEW MESSAGE QUEUED FOR OLED."
  );


  Serial.println(
    "RESCUE MESSAGE SOUND QUEUED."
  );


  // ==========================================================
  // SPECIAL LOCATION MESSAGE
  // ==========================================================

  if (
    message.equalsIgnoreCase(
      "SEND LOCATION AGAIN"
    )
  )
  {
    Serial.println(
      "LOCATION REQUEST MESSAGE RECEIVED"
    );


    updateGPS();


    sendCurrentLocation();
  }


  return true;
}


// ============================================================
//              HANDLE ROUTED LOCATION REQUEST
// ============================================================

bool handleLocationRequest(
  const String &packet
)
{
  String srcString;
  String dstString;
  String seqString;


  if (
    !getPacketField(
      packet,
      "SRC:",
      srcString
    )
    ||
    !getPacketField(
      packet,
      "DST:",
      dstString
    )
    ||
    !getPacketField(
      packet,
      "SEQ:",
      seqString
    )
  )
  {
    return false;
  }


  if (
    !packetHasType(
      packet,
      "LOCREQ"
    )
  )
  {
    return false;
  }


  if (
    srcString.toInt()
    !=
    RECEIVER_NODE_ID
    ||
    dstString.toInt()
    !=
    NODE_ID
  )
  {
    return false;
  }


  uint32_t requestedSequence =
    strtoul(
      seqString.c_str(),
      NULL,
      10
    );


  /*
     IMPORTANT:

     LOCREQ still MUST match active SOS sequence.
  */

  if (
    requestedSequence
    !=
    activeSequence
  )
  {
    Serial.println(
      "LOCREQ ignored - sequence mismatch."
    );


    return false;
  }


  Serial.println(
    "ROUTED LOCATION REQUEST RECEIVED"
  );


  updateGPS();


  sendCurrentLocation();


  return true;
}


// ============================================================
//              CHECK ROUTED FINAL ACK
// ============================================================

bool checkForFinalACK(
  const String &received
)
{
  String srcString;
  String dstString;
  String seqString;
  String ttlString;
  String hopString;


  if (
    !getPacketField(
      received,
      "SRC:",
      srcString
    )
    ||
    !getPacketField(
      received,
      "DST:",
      dstString
    )
    ||
    !getPacketField(
      received,
      "SEQ:",
      seqString
    )
  )
  {
    return false;
  }


  if (
    !packetHasType(
      received,
      "ACK"
    )
  )
  {
    return false;
  }


  if (
    srcString.toInt()
    !=
    RECEIVER_NODE_ID
    ||
    dstString.toInt()
    !=
    NODE_ID
  )
  {
    return false;
  }


  uint32_t ackSequence =
    strtoul(
      seqString.c_str(),
      NULL,
      10
    );


  // ==========================================================
  // CRITICAL:
  //
  // Final SOS ACK MUST match persistent SOS sequence.
  // ==========================================================

  if (
    ackSequence
    !=
    persistentSOSSequence
  )
  {
    Serial.println(
      "ACK ignored - sequence mismatch."
    );


    Serial.print(
      "Expected: "
    );


    Serial.println(
      persistentSOSSequence
    );


    Serial.print(
      "Received: "
    );


    Serial.println(
      ackSequence
    );


    return false;
  }


  getPacketField(
    received,
    "TTL:",
    ttlString
  );


  getPacketField(
    received,
    "HOP:",
    hopString
  );


  receivedAckSequence =
    ackSequence;


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "FINAL ROUTED RESCUE ACK CONFIRMED"
  );


  Serial.print(
    "ACK HOP: "
  );


  Serial.println(
    hopString.length()
      ? hopString
      : "unknown"
  );


  Serial.print(
    "ACK TTL remaining: "
  );


  Serial.println(
    ttlString.length()
      ? ttlString
      : "unknown"
  );


  Serial.println(
    "================================"
  );


  return true;
}


// ============================================================
//          POP NEXT RESCUE MESSAGE
// ============================================================

bool popNextRescueMessage()
{
  bool available =
    false;


  portENTER_CRITICAL(
    &timerMux
  );


  if (
    rescueQueueCount > 0
  )
  {
    RescueMessageRecord &slot =
      rescueMessageQueue[
        rescueQueueHead
      ];


    receivedMessageID =
      slot.messageID;


    receivedMessageSequence =
      slot.sequence;


    receivedMessagePriority =
      slot.priority;


    strncpy(
      receivedRescueMessage,
      slot.message,
      MAX_RESCUE_MESSAGE_LENGTH
    );


    receivedRescueMessage[
      MAX_RESCUE_MESSAGE_LENGTH
    ] =
      '\0';


    strncpy(
      currentRescueMessageSource,
      slot.source,
      sizeof(currentRescueMessageSource) - 1
    );


    currentRescueMessageSource[
      sizeof(currentRescueMessageSource) - 1
    ] =
      '\0';


    rescueQueueHead =
      (
        rescueQueueHead + 1
      )
      %
      RESCUE_MESSAGE_QUEUE_SIZE;


    rescueQueueCount--;


    available =
      true;
  }


  portEXIT_CRITICAL(
    &timerMux
  );


  return available;
}


// ============================================================
//                    PROCESS LoRa PACKET
// ============================================================

void processLoRaPacket(
  const String &received
)
{
  // ==========================================================
  // MESSAGE
  // ==========================================================

  if (
    packetHasType(
      received,
      "MSG"
    )
  )
  {
    handleRescueMessage(
      received
    );


    return;
  }


  // ==========================================================
  // LOCATION REQUEST
  // ==========================================================

  if (
    packetHasType(
      received,
      "LOCREQ"
    )
  )
  {
    handleLocationRequest(
      received
    );


    return;
  }


  // ==========================================================
  // FINAL SOS ACK
  // ==========================================================

  if (
    packetHasType(
      received,
      "ACK"
    )
  )
  {
    if (
      checkForFinalACK(
        received
      )
    )
    {
      portENTER_CRITICAL(
        &timerMux
      );


      finalAckReceived =
        true;


      portEXIT_CRITICAL(
        &timerMux
      );
    }


    return;
  }


  // ==========================================================
  // LOCATION
  // ==========================================================

  if (
    packetHasType(
      received,
      "LOC"
    )
  )
  {
    Serial.println(
      "LOCATION packet received/looped back; ignored."
    );


    return;
  }


  // ==========================================================
  // HELLO
  // ==========================================================

  if (
    packetHasType(
      received,
      "HELLO"
    )
  )
  {
    Serial.println(
      "HELLO received; sender does not maintain routing table."
    );


    return;
  }


  Serial.println(
    "Packet not recognized."
  );
}


// ============================================================
//                    CORE 0 LoRa TASK
// ============================================================

void LoRaTask(
  void *parameter
)
{
  Serial.println(
    "LoRa task running on Core 0."
  );


  while (true)
  {
    // ========================================================
    // SOS TRANSMISSION REQUEST
    // ========================================================

    if (
      sosTransmitRequest
    )
    {
      sosTransmitRequest =
        false;


      portENTER_CRITICAL(
        &timerMux
      );


      finalAckReceived =
        false;


      portEXIT_CRITICAL(
        &timerMux
      );


      sendSOS();
    }


    // ========================================================
    // RECEIVE LoRa PACKETS
    // ========================================================

    int packetSize =
      LoRa.parsePacket();


    if (
      packetSize > 0
    )
    {
      String received =
        "";


      while (
        LoRa.available()
      )
      {
        received +=
          (
            char
          )LoRa.read();
      }


      received.trim();


      Serial.println();
      Serial.println(
        "--------------------------------"
      );


      Serial.print(
        "LoRa RX: "
      );


      Serial.println(
        received
      );


      Serial.print(
        "RSSI: "
      );


      Serial.println(
        LoRa.packetRssi()
      );


      Serial.println(
        "--------------------------------"
      );


      processLoRaPacket(
        received
      );


      LoRa.receive();
    }


    vTaskDelay(
      pdMS_TO_TICKS(
        10
      )
    );
  }
}


// ============================================================
//                    REQUEST NEW SOS
// ============================================================

void requestSOS()
{
  /*
     NEW SOS:
       Create new sequence.

     PERSISTENT SOS:
       Reuse same sequence.
  */

  if (
    !pendingSOS
  )
  {
    sequenceNumber++;


    if (
      sequenceNumber == 0
    )
    {
      sequenceNumber =
        1;
    }


    saveSequenceNumber();


    activeSequence =
      sequenceNumber;


    persistentSOSSequence =
      activeSequence;


    persistentSOSLatitude =
      gpsFixAvailable
        ? currentLatitude
        : 0.0;


    persistentSOSLongitude =
      gpsFixAvailable
        ? currentLongitude
        : 0.0;


    persistentSOSGPSFix =
      gpsFixAvailable;


    /*
       SAVE BEFORE TRANSMISSION.
    */

    savePendingSOS();


    Serial.println();
    Serial.println(
      "NEW SOS CREATED AND PERSISTED."
    );
  }
  else
  {
    activeSequence =
      persistentSOSSequence;


    Serial.println();
    Serial.println(
      "RETRANSMITTING PERSISTENT SOS."
    );


    Serial.print(
      "Sequence = "
    );


    Serial.println(
      activeSequence
    );
  }


  Serial.println();


  if (
    persistentSOSGPSFix
  )
  {
    Serial.println(
      "GPS LOCATION STORED."
    );
  }
  else
  {
    Serial.println(
      "GPS FIX NOT AVAILABLE."
    );


    Serial.println(
      "SOS WILL STILL BE TRANSMITTED."
    );
  }


  buzzerSending();


  sosTransmitRequest =
    true;
}


// ============================================================
//              START PERSISTENT SOS RETRY
// ============================================================

void startPersistentSOSRetry()
{
  if (
    !pendingSOS
  )
  {
    return;
  }


  activeSequence =
    persistentSOSSequence;


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "PERSISTENT SOS RETRY"
  );


  Serial.print(
    "Sequence: "
  );


  Serial.println(
    activeSequence
  );


  Serial.println(
    "Flash record retained until ACK."
  );


  Serial.println(
    "================================"
  );


  goToState(
    STATE_SENDING
  );


  sosTransmitRequest =
    true;
}


// ============================================================
//                    DEEP SLEEP
// ============================================================

void enterDeepSleep()
{
  Serial.println();
  Serial.println(
    "Preparing for deep sleep..."
  );


  oled.clearBuffer();


  drawHeader();


  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    29,
    31,
    "POWER SAVE"
  );


  oled.drawStr(
    58,
    45,
    "Z"
  );


  oled.drawStr(
    63,
    52,
    "Z"
  );


  oled.setFont(
    u8g2_font_5x8_tf
  );


  oled.drawStr(
    39,
    62,
    "TOUCH TO WAKE"
  );


  oled.sendBuffer();


  delay(1000);


  buzzerOff();


  oled.setPowerSave(
    1
  );


  // ----------------------------------------------------------
  // GPIO32 HIGH wakes ESP32.
  // ----------------------------------------------------------

  esp_sleep_enable_ext0_wakeup(
    (gpio_num_t)TOUCH_PIN,
    1
  );


  // ----------------------------------------------------------
  // TIMER WAKE
  // ----------------------------------------------------------

  uint64_t sleepSeconds;


  if (
    pendingSOS
  )
  {
    sleepSeconds =
      SOS_RETRY_INTERVAL_SEC;
  }
  else
  {
    sleepSeconds =
      NORMAL_SLEEP_INTERVAL_SEC;
  }


  esp_sleep_enable_timer_wakeup(
    sleepSeconds *
    1000000ULL
  );


  Serial.println();


  if (
    pendingSOS
  )
  {
    Serial.print(
      "Pending SOS detected."
    );


    Serial.println();


    Serial.print(
      "Sleeping for "
    );


    Serial.print(
      SOS_RETRY_INTERVAL_SEC
    );


    Serial.println(
      " seconds before retry."
    );
  }
  else
  {
    Serial.print(
      "No pending SOS."
    );


    Serial.println();


    Serial.print(
      "Sleeping for "
    );


    Serial.print(
      NORMAL_SLEEP_INTERVAL_SEC
    );


    Serial.println(
      " seconds."
    );
  }


  Serial.println(
    "Touch or timer will wake ESP32."
  );


  Serial.flush();


  delay(100);


  esp_deep_sleep_start();
}


// ============================================================
//                    TOUCH HANDLING
// ============================================================

void handleTouch()
{
  bool pressed =
    (
      digitalRead(
        TOUCH_PIN
      ) == HIGH
    );


  // ==========================================================
  // READY -> HOLDING SOS
  // ==========================================================

  if (
    currentState ==
    STATE_READY
  )
  {
    if (
      pressed &&
      !touchWasPressed
    )
    {
      touchStartTime =
        millis();


      sosHoldTriggered =
        false;


      goToState(
        STATE_HOLDING_SOS
      );


      Serial.println(
        "SOS hold started."
      );
    }
  }


  // ==========================================================
  // HOLDING SOS
  // ==========================================================

  else if (
    currentState ==
    STATE_HOLDING_SOS
  )
  {
    if (
      !pressed
    )
    {
      Serial.println(
        "SOS hold cancelled."
      );


      goToState(
        STATE_READY
      );
    }
    else
    {
      if (
        !sosHoldTriggered
        &&
        (
          millis() -
          touchStartTime
          >=
          SOS_LONG_PRESS_MS
        )
      )
      {
        sosHoldTriggered =
          true;


        goToState(
          STATE_SOS_ACTIVATED
        );


        buzzerSOSActivated();


        gpsWarningStart =
          millis();


        Serial.println(
          "SOS 2-second hold completed."
        );
      }
    }
  }


  // ==========================================================
  // POST SOS -> HOLDING RESEND
  // ==========================================================

  else if (
    currentState ==
    STATE_POST_SOS
  )
  {
    if (
      pressed &&
      !touchWasPressed
    )
    {
      touchStartTime =
        millis();


      sosHoldTriggered =
        false;


      goToState(
        STATE_HOLDING_RESEND
      );


      Serial.println(
        "Resend hold started."
      );
    }
  }


  // ==========================================================
  // HOLDING RESEND
  // ==========================================================

  else if (
    currentState ==
    STATE_HOLDING_RESEND
  )
  {
    if (
      !pressed
    )
    {
      Serial.println(
        "Resend cancelled."
      );


      goToState(
        STATE_POST_SOS
      );
    }
    else
    {
      if (
        !sosHoldTriggered
        &&
        (
          millis() -
          touchStartTime
          >=
          SOS_LONG_PRESS_MS
        )
      )
      {
        sosHoldTriggered =
          true;


        Serial.println(
          "Resend 2-second hold completed."
        );


        buzzerSOSActivated();


        goToState(
          STATE_SOS_ACTIVATED
        );


        gpsWarningStart =
          millis();
      }
    }
  }


  // ==========================================================
  // RESCUE MESSAGE
  // ==========================================================

  else if (
    currentState ==
    STATE_RESCUE_MESSAGE
  )
  {
    // No touch action.
  }


  touchWasPressed =
    pressed;
}


// ============================================================
//                    SETUP
// ============================================================

void setup()
{
  Serial.begin(
    115200
  );


  delay(500);


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "       TERRALINK SENDER"
  );


  Serial.println(
    "================================"
  );


  // ==========================================================
  // LOAD PERSISTENT SOS
  // ==========================================================

  loadPersistentData();


  // ==========================================================
  // GPIO
  // ==========================================================

  pinMode(
    TOUCH_PIN,
    INPUT
  );


  pinMode(
    LOCAL_ALARM_PIN,
    INPUT_PULLUP
  );


  pinMode(
    LED_BUTTON_PIN,
    INPUT_PULLUP
  );


  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  pinMode(
    TORCH_PIN,
    OUTPUT
  );


  buzzerOff();


  digitalWrite(
    TORCH_PIN,
    LOW
  );


  // ==========================================================
  // OLED
  // ==========================================================

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );


  oled.begin();


  oled.setPowerSave(
    0
  );


  oled.clearBuffer();


  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    31,
    30,
    "TERRALINK"
  );


  oled.drawStr(
    35,
    46,
    "STARTING"
  );


  oled.sendBuffer();


  Serial.println(
    "OLED initialized."
  );


  // ==========================================================
  // GPS
  // ==========================================================

  GPSserial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );


  Serial.println(
    "GPS initialized."
  );


  // ==========================================================
  // SPI
  // ==========================================================

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );


  // ==========================================================
  // LoRa
  // ==========================================================

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );


  Serial.println(
    "Initializing LoRa..."
  );


  if (
    LoRa.begin(
      LORA_FREQUENCY
    )
  )
  {
    loraInitialized =
      true;


    Serial.println(
      "LoRa initialized successfully."
    );


    LoRa.setSyncWord(
      LORA_SYNC_WORD
    );


    LoRa.setSpreadingFactor(
      LORA_SPREADING_FACTOR
    );


    LoRa.setSignalBandwidth(
      LORA_BANDWIDTH
    );


    LoRa.setCodingRate4(
      LORA_CODING_RATE
    );


    LoRa.enableCrc();


    LoRa.setTxPower(
      LORA_TX_POWER
    );


    LoRa.receive();


    Serial.println(
      "LoRa configuration complete."
    );
  }
  else
  {
    loraInitialized =
      false;


    Serial.println(
      "ERROR: LoRa initialization FAILED."
    );
  }


  // ==========================================================
  // WAKE REASON
  // ==========================================================

  esp_sleep_wakeup_cause_t wakeReason =
    esp_sleep_get_wakeup_cause();


  if (
    wakeReason ==
    ESP_SLEEP_WAKEUP_EXT0
  )
  {
    Serial.println(
      "Woke from touch."
    );


    if (
      pendingSOS
    )
    {
      activeSequence =
        persistentSOSSequence;


      Serial.println(
        "Pending SOS exists after touch wake."
      );


      Serial.println(
        "Waiting for release before retry."
      );
    }


    goToState(
      STATE_WAIT_RELEASE
    );


    buzzerWake();
  }
  else if (
    wakeReason ==
    ESP_SLEEP_WAKEUP_TIMER
  )
  {
    Serial.println(
      "Woke from timer."
    );


    if (
      pendingSOS
    )
    {
      activeSequence =
        persistentSOSSequence;


      Serial.println();
      Serial.println(
        "================================"
      );


      Serial.println(
        "AUTOMATIC SOS RETRY WAKE"
      );


      Serial.print(
        "Persistent sequence: "
      );


      Serial.println(
        persistentSOSSequence
      );


      Serial.println(
        "No user action required."
      );


      Serial.println(
        "================================"
      );


      goToState(
        STATE_SENDING
      );


      sosTransmitRequest =
        true;
    }
    else
    {
      goToState(
        STATE_AWAKENING
      );
    }


    buzzerWake();
  }
  else
  {
    /*
       Normal power-on.

       If flash contains an SOS,
       automatically retry.
    */

    if (
      pendingSOS
    )
    {
      activeSequence =
        persistentSOSSequence;


      Serial.println();
      Serial.println(
        "POWER-ON WITH PENDING SOS"
      );


      goToState(
        STATE_SENDING
      );


      sosTransmitRequest =
        true;
    }
    else
    {
      goToState(
        STATE_AWAKENING
      );
    }


    buzzerWake();
  }


  // ==========================================================
  // CORE 0 LoRa TASK
  // ==========================================================

  xTaskCreatePinnedToCore(
    LoRaTask,
    "LoRaTask",
    8192,
    NULL,
    2,
    NULL,
    0
  );


  Serial.println(
    "LoRa task assigned to Core 0."
  );


  Serial.println(
    "Application running on Core 1."
  );


  Serial.println(
    "Local alarm button = GPIO33."
  );


  Serial.println(
    "Buzzer control = GPIO25."
  );


  Serial.println(
    "LED button = GPIO13."
  );


  Serial.println(
    "LED / Torch = GPIO4."
  );


  Serial.println(
    "Persistent SOS storage = ESP32 NVS."
  );


  Serial.println(
    "Automatic SOS retry = ENABLED."
  );


  Serial.println(
    "Rescue message reception = ENABLED."
  );


  Serial.println(
    "OLED rescue message display = ENABLED."
  );


  Serial.println(
    "Rescue message sound indication = ENABLED."
  );


  Serial.println(
    "Optional SOURCE field = ENABLED."
  );


  Serial.println(
    "Optional PRIORITY field = ENABLED."
  );


  Serial.println(
    "Location request response = ENABLED."
  );


  Serial.println(
    "Mesh routing packet support = ENABLED."
  );


  Serial.println(
    "Sender routing role = END NODE; relays select next hop."
  );


  Serial.println(
    "================================"
  );
}


// ============================================================
//                    MAIN LOOP - CORE 1
// ============================================================

void loop()
{
  // ==========================================================
  // LOCAL ALARM
  // ==========================================================

  handleLocalAlarm();


  // ==========================================================
  // RESCUE MESSAGE EVENT
  // ==========================================================

  bool newRescueMessage =
    popNextRescueMessage();


  if (
    newRescueMessage
  )
  {
    rescueMessageDisplayStart =
      millis();


    goToState(
      STATE_RESCUE_MESSAGE
    );


    /*
       Do not override the local alarm buzzer.

       Otherwise the rescue message notification
       could turn off/interrupt the local alarm.
    */

    if (
      !localAlarmPressed
    )
    {
      buzzerPriorityMessage(
        receivedMessagePriority
      );
    }
  }


  // ==========================================================
  // LED BUTTON
  // ==========================================================

  handleLEDButton();


  // ==========================================================
  // GPS
  // ==========================================================

  updateGPS();


  // ==========================================================
  // STATE MACHINE
  // ==========================================================

  switch (
    currentState
  )
  {
    // ========================================================
    // AWAKENING
    // ========================================================

    case STATE_AWAKENING:

      if (
        millis() -
        stateStartTime
        >=
        AWAKENING_TIME_MS
      )
      {
        goToState(
          STATE_READY
        );
      }

      break;


    // ========================================================
    // WAIT RELEASE
    // ========================================================

    case STATE_WAIT_RELEASE:

      if (
        digitalRead(
          TOUCH_PIN
        ) == LOW
      )
      {
        if (
          pendingSOS
        )
        {
          startPersistentSOSRetry();
        }
        else
        {
          goToState(
            STATE_READY
          );
        }
      }

      break;


    // ========================================================
    // READY
    // ========================================================

    case STATE_READY:

      handleTouch();

      break;


    // ========================================================
    // HOLDING SOS
    // ========================================================

    case STATE_HOLDING_SOS:

      handleTouch();

      break;


    // ========================================================
    // SOS ACTIVATED
    // ========================================================

    case STATE_SOS_ACTIVATED:

      if (
        gpsFixAvailable
      )
      {
        /*
           For a NEW SOS:
           capture current GPS.

           For pending SOS:
           retain original stored GPS.
        */

        if (
          !pendingSOS
        )
        {
          persistentSOSLatitude =
            currentLatitude;


          persistentSOSLongitude =
            currentLongitude;


          persistentSOSGPSFix =
            true;
        }


        goToState(
          STATE_SENDING
        );


        requestSOS();
      }
      else if (
        millis() -
        gpsWarningStart
        >=
        GPS_MESSAGE_TIME_MS
      )
      {
        /*
           No GPS fix.

           SOS still gets created and stored.
        */

        if (
          !pendingSOS
        )
        {
          persistentSOSLatitude =
            0.0;


          persistentSOSLongitude =
            0.0;


          persistentSOSGPSFix =
            false;
        }


        goToState(
          STATE_SENDING
        );


        requestSOS();
      }

      break;


    // ========================================================
    // SENDING
    // ========================================================

    case STATE_SENDING:

      if (
        millis() -
        stateStartTime
        >=
        SENDING_DISPLAY_MS
      )
      {
        goToState(
          STATE_WAITING_ACK
        );


        Serial.println(
          "Waiting for final rescue ACK..."
        );


        if (
          !localAlarmPressed
        )
        {
          buzzerWaiting();
        }
      }

      break;


    // ========================================================
    // WAITING FOR ACK
    // ========================================================

    case STATE_WAITING_ACK:
    {
      bool ackReceivedNow =
        false;


      portENTER_CRITICAL(
        &timerMux
      );


      if (
        finalAckReceived
      )
      {
        finalAckReceived =
          false;


        ackReceivedNow =
          true;
      }


      portEXIT_CRITICAL(
        &timerMux
      );


      if (
        ackReceivedNow
      )
      {
        Serial.println();
        Serial.println(
          "FINAL RESCUE ACK RECEIVED."
        );


        /*
           Final ACK means rescue station received SOS.

           Therefore clear persistent SOS.
        */

        clearPendingSOS();


        goToState(
          STATE_SOS_SUCCESS
        );


        if (
          !localAlarmPressed
        )
        {
          buzzerSuccess();
        }
      }
      else if (
        millis() -
        stateStartTime
        >=
        ACK_WAIT_TIMEOUT_MS
      )
      {
        Serial.println();
        Serial.println(
          "RESCUE ACK TIMEOUT."
        );


        /*
           DO NOT clear flash.
        */

        Serial.println(
          "SOS REMAINS STORED IN FLASH."
        );


        Serial.println(
          "Automatic retry will occur after sleep."
        );


        goToState(
          STATE_SOS_FAILURE
        );


        if (
          !localAlarmPressed
        )
        {
          buzzerFailure();
        }
      }

      break;
    }


    // ========================================================
    // SUCCESS
    // ========================================================

    case STATE_SOS_SUCCESS:

      if (
        millis() -
        stateStartTime
        >=
        SUCCESS_DISPLAY_MS
      )
      {
        goToState(
          STATE_POST_SOS
        );
      }

      break;


    // ========================================================
    // FAILURE
    // ========================================================

    case STATE_SOS_FAILURE:

      if (
        millis() -
        stateStartTime
        >=
        FAILURE_DISPLAY_MS
      )
      {
        goToState(
          STATE_POST_SOS
        );
      }

      break;


    // ========================================================
    // POST SOS
    // ========================================================

    case STATE_POST_SOS:

      handleTouch();


      /*
         Pending SOS:
            60 sec awake
                ↓
            deep sleep
                ↓
            30 sec timer wake
                ↓
            retransmit
      */

      if (
        millis() -
        stateStartTime
        >=
        POST_SOS_AWAKE_TIME_MS
      )
      {
        enterDeepSleep();
      }

      break;


    // ========================================================
    // HOLDING RESEND
    // ========================================================

    case STATE_HOLDING_RESEND:

      handleTouch();

      break;


    // ========================================================
    // RESCUE MESSAGE
    // ========================================================

    case STATE_RESCUE_MESSAGE:

      if (
        millis() -
        rescueMessageDisplayStart
        >=
        RESCUE_MESSAGE_DISPLAY_MS
      )
      {
        /*
           Return to HELP MODE after showing
           rescue response.
        */

        goToState(
          STATE_POST_SOS
        );
      }

      break;


    // ========================================================
    // DEFAULT
    // ========================================================

    default:

      goToState(
        STATE_READY
      );

      break;
  }


  // ==========================================================
  // OLED
  // ==========================================================

  updateOLED();


  // ==========================================================
  // SMALL DELAY
  // ==========================================================

  delay(5);
}