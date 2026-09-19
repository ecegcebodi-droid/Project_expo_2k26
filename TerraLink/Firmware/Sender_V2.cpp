/*
   ============================================================
                       TERRALINK SENDER
   ============================================================

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
      - ACK validation

   CORE 1:
      - GPS
      - Touch/SOS button
      - Local alarm button
      - LED button
      - OLED
      - Buzzer control
      - LED control
      - State machine

   ============================================================
                       LOCAL ALARM
   ============================================================

   GPIO33 -> Push Button -> GND

   INPUT_PULLUP is used.

   Button RELEASED:
      GPIO33 = HIGH
      Buzzer = OFF

   Button PRESSED:
      GPIO33 = LOW
      Buzzer = ON continuously

   ============================================================
                       LED / TORCH
   ============================================================

   GPIO13 -> LED Push Button -> GND

   GPIO4 -> 220/330 ohm resistor -> LED -> GND

   LED Button RELEASED:
      GPIO13 = HIGH
      LED = OFF

   LED Button PRESSED:
      GPIO13 = LOW
      LED = ON

   No MOSFET is required for a normal indicator/torch LED
   when the LED current is kept within the ESP32 GPIO limit.

   ============================================================
                       SOS BEHAVIOR
   ============================================================

   GPS FIX AVAILABLE:
      Actual latitude/longitude transmitted.

   GPS FIX NOT AVAILABLE:
      LAT = 0.000000
      LON = 0.000000

      SOS IS STILL TRANSMITTED.

   ============================================================
*/


#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
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

// NEW LED BUTTON
#define LED_BUTTON_PIN     13

// Existing buzzer
#define BUZZER_PIN         25

// Existing LED / Torch
#define TORCH_PIN          4


// ---------------- Node ID ----------------

#define NODE_ID        1


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

#define AWAKENING_TIME_MS       2000UL

#define NORMAL_AWAKE_TIME_MS    60000UL

#define POST_SOS_AWAKE_TIME_MS  60000UL

#define SOS_LONG_PRESS_MS       2000UL

#define GPS_MESSAGE_TIME_MS     4000UL

#define ACK_WAIT_TIMEOUT_MS     15000UL

#define SUCCESS_DISPLAY_MS      5000UL

#define FAILURE_DISPLAY_MS      5000UL

#define SENDING_DISPLAY_MS      1500UL

#define UI_UPDATE_MS            120UL


// ============================================================
//                    OBJECTS
// ============================================================

TinyGPSPlus gps;

HardwareSerial GPSserial(2);


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

  STATE_HOLDING_RESEND
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

uint32_t sequenceNumber = 0;


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


// ============================================================
//                    TOUCH VARIABLES
// ============================================================

bool touchWasPressed =
  false;

bool sosHoldTriggered =
  false;


// ============================================================
//                    LOCAL ALARM VARIABLES
// ============================================================

bool localAlarmPressed =
  false;


// ============================================================
//                    LED VARIABLES
// ============================================================

bool ledButtonPressed =
  false;


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


// ------------------------------------------------------------
// Normal short beep
// ------------------------------------------------------------

void beep(unsigned int duration)
{
  buzzerOn();

  delay(duration);

  buzzerOff();
}


// ------------------------------------------------------------
// Wake
// ------------------------------------------------------------

void buzzerWake()
{
  beep(100);
}


// ------------------------------------------------------------
// SOS activated
// ------------------------------------------------------------

void buzzerSOSActivated()
{
  beep(120);

  delay(120);

  beep(120);

  delay(120);

  beep(120);
}


// ------------------------------------------------------------
// Sending
// ------------------------------------------------------------

void buzzerSending()
{
  beep(80);

  delay(80);

  beep(80);

  delay(80);

  beep(80);
}


// ------------------------------------------------------------
// Waiting
// ------------------------------------------------------------

void buzzerWaiting()
{
  beep(70);

  delay(70);

  beep(70);
}


// ------------------------------------------------------------
// Success
// ------------------------------------------------------------

void buzzerSuccess()
{
  beep(120);

  delay(120);

  beep(120);

  delay(300);

  beep(300);
}


// ------------------------------------------------------------
// Failure
// ------------------------------------------------------------

void buzzerFailure()
{
  beep(350);

  delay(350);

  beep(350);
}


// ============================================================
//                    LOCAL ALARM CONTROL
// ============================================================

void handleLocalAlarm()
{
  /*
     GPIO33 uses INPUT_PULLUP.

     PRESSED  = LOW
     RELEASED = HIGH
  */

  localAlarmPressed =
    (digitalRead(LOCAL_ALARM_PIN) == LOW);


  if (localAlarmPressed)
  {
    /*
       Continuous alarm.
    */

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
  /*
     GPIO13 uses INPUT_PULLUP.

     PRESSED  = LOW
     RELEASED = HIGH
  */

  ledButtonPressed =
    (digitalRead(LED_BUTTON_PIN) == LOW);


  if (ledButtonPressed)
  {
    // LED ON
    digitalWrite(
      TORCH_PIN,
      HIGH
    );
  }
  else
  {
    // LED OFF
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
//                    NAKED CHECK
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
//                    NAKED CROSS
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
    ((width - 2) * percent) / 100;


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
//                    UPDATE OLED
// ============================================================

void updateOLED()
{
  if (
    millis() - lastUIUpdate <
    UI_UPDATE_MS
  )
  {
    return;
  }


  lastUIUpdate =
    millis();


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
      (millis() / 250) % 4;


    for (int i = 0; i < 4; i++)
    {
      if (i == phase)
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
      (float)(
        SOS_LONG_PRESS_MS - held
      ) / 1000.0;


    if (remaining < 0)
      remaining = 0;


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

    oled.print("s");


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


    if (gpsFixAvailable)
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


    if (!gpsFixAvailable)
    {
      unsigned long elapsed =
        millis() - gpsWarningStart;


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


    if (gpsFixAvailable)
      oled.print(
        currentLatitude,
        6
      );
    else
      oled.print(
        "0.000000"
      );


    oled.drawStr(
      0,
      47,
      "LON:"
    );


    oled.setCursor(
      22,
      47
    );


    if (gpsFixAvailable)
      oled.print(
        currentLongitude,
        6
      );
    else
      oled.print(
        "0.000000"
      );


    if (gpsFixAvailable)
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
      (millis() / 180) % 4;


    oled.drawDisc(
      87,
      54,
      2
    );


    if (phase >= 1)
      oled.drawCircle(
        87,
        54,
        5
      );


    if (phase >= 2)
      oled.drawCircle(
        87,
        54,
        9
      );


    if (phase >= 3)
      oled.drawCircle(
        87,
        54,
        13
      );


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
      millis() - stateStartTime;


    long remaining =
      (
        (long)ACK_WAIT_TIMEOUT_MS -
        (long)elapsed
      ) / 1000;


    if (remaining < 0)
      remaining = 0;


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
      (millis() / 220) % 4;


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
  // FAILURE / NO REPLY
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
  // POST SOS / HELP MODE
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
      (millis() / 400) % 2;


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
      (float)(
        SOS_LONG_PRESS_MS - held
      ) / 1000.0;


    if (remaining < 0)
      remaining = 0;


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
//                    CREATE SOS PACKET
// ============================================================

String createSOSPacket()
{
  String packet;

  packet.reserve(120);


  packet +=
    "ID:";


  packet +=
    String(NODE_ID);


  packet +=
    ",LAT:";


  if (gpsFixAvailable)
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


  if (gpsFixAvailable)
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
    ",SEQ:";


  packet +=
    String(
      activeSequence
    );


  packet +=
    ",TTL:5";


  packet +=
    ",SOS";


  return packet;
}


// ============================================================
//                    SEND SOS
// ============================================================

void sendSOS()
{
  if (!loraInitialized)
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
    "TERRALINK SOS TRANSMISSION"
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


  if (gpsFixAvailable)
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
      "LAT/LON set to 0.000000"
    );


    Serial.println(
      "SOS transmission NOT blocked."
    );
  }


  // ----------------------------------------------------------
  // Transmit
  // ----------------------------------------------------------

  LoRa.idle();


  LoRa.beginPacket();


  LoRa.print(
    packet
  );


  int result =
    LoRa.endPacket();


  if (result == 1)
  {
    Serial.println(
      "SOS transmitted successfully."
    );
  }
  else
  {
    Serial.println(
      "SOS transmission failed."
    );
  }


  // Return to receive mode

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
//                    CHECK FINAL ACK
// ============================================================

bool checkForFinalACK()
{
  int packetSize =
    LoRa.parsePacket();


  if (packetSize <= 0)
  {
    return false;
  }


  String received = "";


  while (
    LoRa.available()
  )
  {
    received +=
      (char)LoRa.read();
  }


  received.trim();


  Serial.print(
    "LoRa RX: "
  );


  Serial.println(
    received
  );


  String expectedID =
    "ID:" +
    String(NODE_ID);


  String expectedSEQ =
    "SEQ:" +
    String(activeSequence);


  bool idMatch =
    received.indexOf(
      expectedID
    ) >= 0;


  bool sequenceMatch =
    received.indexOf(
      expectedSEQ
    ) >= 0;


  bool ackMatch =
    received.indexOf(
      ",ACK"
    ) >= 0;


  if (
    idMatch &&
    sequenceMatch &&
    ackMatch
  )
  {
    receivedAckSequence =
      activeSequence;


    Serial.println(
      "FINAL RESCUE ACK CONFIRMED"
    );


    return true;
  }


  Serial.println(
    "ACK rejected - not matching current SOS."
  );


  return false;
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


      sendSOS();


      finalAckReceived =
        false;
    }


    // ========================================================
    // ACK MONITORING
    // ========================================================

    if (
      currentState ==
      STATE_WAITING_ACK
    )
    {
      if (
        checkForFinalACK()
      )
      {
        finalAckReceived =
          true;
      }
    }


    vTaskDelay(
      pdMS_TO_TICKS(10)
    );
  }
}


// ============================================================
//                    REQUEST SOS
// ============================================================

void requestSOS()
{
  sequenceNumber++;


  activeSequence =
    sequenceNumber;


  Serial.println();


  Serial.println(
    "SOS REQUEST CREATED"
  );


  Serial.print(
    "Sequence = "
  );


  Serial.println(
    activeSequence
  );


  if (gpsFixAvailable)
  {
    Serial.println(
      "GPS FIX AVAILABLE"
    );
  }
  else
  {
    Serial.println(
      "GPS FIX NOT AVAILABLE"
    );


    Serial.println(
      "SOS WILL STILL BE SENT"
    );
  }


  buzzerSending();


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


  delay(1500);


  Serial.println(
    "Waiting for touch release..."
  );


  while (
    digitalRead(TOUCH_PIN) == HIGH
  )
  {
    delay(20);
  }


  delay(100);


  // Make sure buzzer is OFF before sleep

  buzzerOff();


  // Turn OLED off

  oled.setPowerSave(1);


  // GPIO32 wakes ESP32

  esp_sleep_enable_ext0_wakeup(
    (gpio_num_t)TOUCH_PIN,
    1
  );


  Serial.println(
    "Entering deep sleep..."
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
    digitalRead(
      TOUCH_PIN
    ) == HIGH;


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
    if (!pressed)
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
        !sosHoldTriggered &&
        (
          millis() -
          touchStartTime >=
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
    if (!pressed)
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
        !sosHoldTriggered &&
        (
          millis() -
          touchStartTime >=
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
  // GPIO
  // ==========================================================

  // ----------------------------------------------------------
  // Existing SOS touch input
  // ----------------------------------------------------------

  pinMode(
    TOUCH_PIN,
    INPUT
  );


  // ----------------------------------------------------------
  // Existing local alarm button
  // ----------------------------------------------------------

  pinMode(
    LOCAL_ALARM_PIN,
    INPUT_PULLUP
  );


  // ----------------------------------------------------------
  // NEW LED PUSH BUTTON
  //
  // GPIO13 ---- Push Button ---- GND
  //
  // Released = HIGH
  // Pressed  = LOW
  // ----------------------------------------------------------

  pinMode(
    LED_BUTTON_PIN,
    INPUT_PULLUP
  );


  // ----------------------------------------------------------
  // Buzzer
  // ----------------------------------------------------------

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  // ----------------------------------------------------------
  // LED / Torch
  // ----------------------------------------------------------

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


  // Make sure OLED is active after wake

  oled.setPowerSave(0);


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


    goToState(
      STATE_WAIT_RELEASE
    );


    buzzerWake();
  }
  else
  {
    goToState(
      STATE_AWAKENING
    );


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
  //
  // Handle first for immediate response.
  // ==========================================================

  handleLocalAlarm();


  // ==========================================================
  // LED BUTTON
  //
  // Independent of SOS and LoRa.
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
        stateStartTime >=
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
        goToState(
          STATE_READY
        );
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
        goToState(
          STATE_SENDING
        );


        requestSOS();
      }


      else if (
        millis() -
        gpsWarningStart >=
        GPS_MESSAGE_TIME_MS
      )
      {
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
        stateStartTime >=
        SENDING_DISPLAY_MS
      )
      {
        goToState(
          STATE_WAITING_ACK
        );


        Serial.println(
          "Waiting for final rescue ACK..."
        );


        if (!localAlarmPressed)
        {
          buzzerWaiting();
        }
      }

      break;


    // ========================================================
    // WAITING FOR ACK
    // ========================================================

    case STATE_WAITING_ACK:

      if (
        finalAckReceived
      )
      {
        finalAckReceived =
          false;


        Serial.println(
          "FINAL RESCUE ACK RECEIVED."
        );


        goToState(
          STATE_SOS_SUCCESS
        );


        if (!localAlarmPressed)
        {
          buzzerSuccess();
        }
      }


      else if (
        millis() -
        stateStartTime >=
        ACK_WAIT_TIMEOUT_MS
      )
      {
        Serial.println(
          "RESCUE ACK TIMEOUT."
        );


        goToState(
          STATE_SOS_FAILURE
        );


        if (!localAlarmPressed)
        {
          buzzerFailure();
        }
      }

      break;


    // ========================================================
    // SUCCESS
    // ========================================================

    case STATE_SOS_SUCCESS:

      if (
        millis() -
        stateStartTime >=
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
        stateStartTime >=
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


      if (
        millis() -
        stateStartTime >=
        POST_SOS_AWAKE_TIME_MS
      )
      {
        if (
          digitalRead(
            TOUCH_PIN
          ) == LOW
        )
        {
          enterDeepSleep();
        }
      }

      break;


    // ========================================================
    // HOLDING RESEND
    // ========================================================

    case STATE_HOLDING_RESEND:

      handleTouch();

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