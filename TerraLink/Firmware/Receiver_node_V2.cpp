/*
   ============================================================
                    TERRALINK RESCUE STATION
                         STAGE 1
   ============================================================

   MCU:
      ESP8266

   LoRa:
      AI-Thinker RA-02 / SX1278
      Frequency = 433 MHz

   LCD:
      16x2 I2C LCD
      Address = 0x27

   Buzzer:
      GPIO0 / D3

   ============================================================
                       PIN CONFIGURATION
   ============================================================

   LoRa RA-02:

      SCK  -> D5 / GPIO14
      MISO -> D6 / GPIO12
      MOSI -> D7 / GPIO13
      NSS  -> D1 / GPIO5
      RST  -> D0 / GPIO16
      DIO0 -> Not connected

   LCD:

      SDA -> D2 / GPIO4
      SCL -> D4 / GPIO2

   Buzzer:

      GPIO0 / D3

   ============================================================
                       LORA CONFIGURATION
   ============================================================

      Frequency        = 433 MHz
      Sync Word        = 0xF3
      Spreading Factor = 7
      Bandwidth        = 125 kHz
      Coding Rate      = 4/5
      CRC              = ENABLED
      TX Power         = 17 dBm

   These MUST match the TerraLink sender.

   ============================================================
                       RECEIVED SOS PACKET
   ============================================================

   Example:

      ID:1,LAT:11.234567,LON:76.123456,SEQ:12,TTL:5,SOS

   GPS unavailable:

      ID:1,LAT:0.000000,LON:0.000000,SEQ:12,TTL:5,SOS

   ============================================================
                       ACK PACKET
   ============================================================

   Receiver sends:

      ID:1,SEQ:12,ACK

   Sender accepts ACK only when:

      ID matches
      SEQ matches
      ",ACK" is present

   ============================================================
                       STAGE 1 FUNCTIONS
   ============================================================

      1. Receive SOS
      2. Validate SOS packet
      3. Extract Node ID
      4. Extract Latitude
      5. Extract Longitude
      6. Extract Sequence Number
      7. Extract TTL
      8. Detect duplicate packets
      9. Send ACK
     10. Display emergency information
     11. Activate buzzer
     12. Display RSSI
     13. Display GPS status
     14. Generate Google Maps location link
     15. Return to LoRa receive mode

   ============================================================
                       NOT INCLUDED YET
   ============================================================

      - Rescue message buttons
      - PCF8574
      - Custom message selection
      - Receiver -> sender text messaging
      - MSGID / MSGACK
      - Reverse relay messaging

   These will be Stage 2 / later.
   ============================================================
*/


#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// ============================================================
//                    LCD CONFIGURATION
// ============================================================

LiquidCrystal_I2C lcd(
  0x27,
  16,
  2
);


// ============================================================
//                    LORA PIN CONFIGURATION
// ============================================================

#define LORA_SS       5       // D1 / GPIO5
#define LORA_RST      16      // D0 / GPIO16
#define LORA_DIO0     -1      // DIO0 not connected


// ============================================================
//                    BUZZER
// ============================================================

#define BUZZER_PIN    0       // D3 / GPIO0


// ============================================================
//                    LORA CONFIGURATION
// ============================================================

#define LORA_FREQUENCY          433E6

#define LORA_SYNC_WORD          0xF3

#define LORA_SPREADING_FACTOR   7

#define LORA_BANDWIDTH          125E3

// setCodingRate4(5) = 4/5
#define LORA_CODING_RATE        5

#define LORA_TX_POWER           17


// ============================================================
//                    BUZZER TIMING
// ============================================================

#define BUZZER_DURATION_MS      2500UL


// ============================================================
//                    LCD TIMING
// ============================================================

#define LCD_ALERT_TIME_MS       8000UL

#define LCD_PAGE_TIME_MS        2000UL


// ============================================================
//                    DUPLICATE CACHE
// ============================================================

#define DUPLICATE_CACHE_SIZE    10


String duplicateNodeID[
  DUPLICATE_CACHE_SIZE
];

String duplicateSequence[
  DUPLICATE_CACHE_SIZE
];

int duplicateCacheIndex = 0;


// ============================================================
//                    RECEIVED PACKET
// ============================================================

String receivedMessage = "";


// ============================================================
//                    RECEIVED DATA
// ============================================================

String receivedNodeID = "";

String receivedLatitude = "";

String receivedLongitude = "";

String receivedSequence = "";

String receivedTTL = "";

int receivedRSSI = 0;


// ============================================================
//                    SOS STATE
// ============================================================

bool sosActive = false;

unsigned long sosStartTime = 0;


// ============================================================
//                    BUZZER STATE
// ============================================================

bool buzzerActive = false;

unsigned long buzzerStartTime = 0;


// ============================================================
//                    ACK STATE
// ============================================================

bool ackSent = false;

unsigned long ackSentTime = 0;


// ============================================================
//                    LCD PAGE
// ============================================================

int lcdPage = 0;

unsigned long lastLCDPageChange = 0;


// ============================================================
//                    HELPER
//                    PRINT 16 CHARACTER LINE
// ============================================================

void printLCDLine(
  int row,
  String text
)
{
  lcd.setCursor(
    0,
    row
  );

  // Clear entire 16-character line
  lcd.print(
    "                "
  );

  lcd.setCursor(
    0,
    row
  );

  // Display maximum 16 characters
  if (text.length() > 16)
  {
    text = text.substring(
      0,
      16
    );
  }

  lcd.print(
    text
  );
}


// ============================================================
//                    EXTRACT FIELD
// ============================================================

String extractField(
  String packet,
  String key
)
{
  int start =
    packet.indexOf(
      key
    );


  if (start < 0)
  {
    return "";
  }


  start +=
    key.length();


  int end =
    packet.indexOf(
      ",",
      start
    );


  if (end < 0)
  {
    end =
      packet.length();
  }


  return packet.substring(
    start,
    end
  );
}


// ============================================================
//                    CHECK NUMERIC STRING
// ============================================================

bool isNumericString(
  String value
)
{
  if (value.length() == 0)
  {
    return false;
  }


  for (
    unsigned int i = 0;
    i < value.length();
    i++
  )
  {
    if (
      !isDigit(
        value[i]
      )
    )
    {
      return false;
    }
  }


  return true;
}


// ============================================================
//                    CHECK SOS FORMAT
// ============================================================

bool isValidSOSPacket(
  String packet
)
{
  packet.trim();


  // ----------------------------------------------------------
  // Packet must end with ",SOS"
  // ----------------------------------------------------------

  if (
    !packet.endsWith(
      ",SOS"
    )
  )
  {
    return false;
  }


  // ----------------------------------------------------------
  // Extract required fields
  // ----------------------------------------------------------

  String nodeID =
    extractField(
      packet,
      "ID:"
    );


  String latitude =
    extractField(
      packet,
      "LAT:"
    );


  String longitude =
    extractField(
      packet,
      "LON:"
    );


  String sequence =
    extractField(
      packet,
      "SEQ:"
    );


  String ttl =
    extractField(
      packet,
      "TTL:"
    );


  // ----------------------------------------------------------
  // Check for empty values
  // ----------------------------------------------------------

  if (
    nodeID.length() == 0 ||
    latitude.length() == 0 ||
    longitude.length() == 0 ||
    sequence.length() == 0 ||
    ttl.length() == 0
  )
  {
    return false;
  }


  // ----------------------------------------------------------
  // Sequence must be numeric
  // ----------------------------------------------------------

  if (
    !isNumericString(
      sequence
    )
  )
  {
    return false;
  }


  // ----------------------------------------------------------
  // TTL must be numeric
  // ----------------------------------------------------------

  if (
    !isNumericString(
      ttl
    )
  )
  {
    return false;
  }


  int ttlValue =
    ttl.toInt();


  // ----------------------------------------------------------
  // Sender starts with TTL = 5.
  // Relay nodes may decrement it.
  // Receiver accepts 0 through 5.
  // ----------------------------------------------------------

  if (
    ttlValue < 0 ||
    ttlValue > 5
  )
  {
    return false;
  }


  return true;
}


// ============================================================
//                    DUPLICATE CHECK
// ============================================================

bool isDuplicate(
  String nodeID,
  String sequence
)
{
  for (
    int i = 0;
    i < DUPLICATE_CACHE_SIZE;
    i++
  )
  {
    if (
      duplicateNodeID[i] ==
        nodeID
      &&
      duplicateSequence[i] ==
        sequence
    )
    {
      return true;
    }
  }


  return false;
}


// ============================================================
//                    ADD TO DUPLICATE CACHE
// ============================================================

void rememberSOS(
  String nodeID,
  String sequence
)
{
  duplicateNodeID[
    duplicateCacheIndex
  ] =
    nodeID;


  duplicateSequence[
    duplicateCacheIndex
  ] =
    sequence;


  duplicateCacheIndex++;


  if (
    duplicateCacheIndex >=
    DUPLICATE_CACHE_SIZE
  )
  {
    duplicateCacheIndex = 0;
  }
}


// ============================================================
//                    SEND ACK
// ============================================================

void sendACK(
  String nodeID,
  String sequence
)
{
  String ackPacket =
    "ID:" +
    nodeID +
    ",SEQ:" +
    sequence +
    ",ACK";


  Serial.println();
  Serial.println(
    "--------------------------------"
  );

  Serial.println(
    "SENDING RESCUE ACK"
  );


  Serial.print(
    "ACK PACKET : "
  );

  Serial.println(
    ackPacket
  );


  // ----------------------------------------------------------
  // Change from RX to TX
  // ----------------------------------------------------------

  LoRa.idle();


  // ----------------------------------------------------------
  // Start packet
  // ----------------------------------------------------------

  LoRa.beginPacket();


  LoRa.print(
    ackPacket
  );


  // ----------------------------------------------------------
  // Transmit
  // ----------------------------------------------------------

  int result =
    LoRa.endPacket();


  // ----------------------------------------------------------
  // Check transmission result
  // ----------------------------------------------------------

  if (
    result == 1
  )
  {
    ackSent =
      true;


    ackSentTime =
      millis();


    Serial.println(
      "ACK TRANSMITTED SUCCESSFULLY"
    );
  }
  else
  {
    ackSent =
      false;


    Serial.println(
      "ACK TRANSMISSION FAILED"
    );
  }


  // ----------------------------------------------------------
  // Return to RX
  // ----------------------------------------------------------

  LoRa.receive();


  Serial.println(
    "LoRa returned to RX mode."
  );

  Serial.println(
    "--------------------------------"
  );

  Serial.println();
}


// ============================================================
//                    START BUZZER
// ============================================================

void startBuzzer()
{
  digitalWrite(
    BUZZER_PIN,
    HIGH
  );


  buzzerActive =
    true;


  buzzerStartTime =
    millis();


  Serial.println(
    "Rescue station buzzer ON."
  );
}


// ============================================================
//                    STOP BUZZER
// ============================================================

void stopBuzzer()
{
  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  buzzerActive =
    false;


  Serial.println(
    "Rescue station buzzer OFF."
  );
}


// ============================================================
//                    GOOGLE MAPS LINK
// ============================================================

void printGoogleMapsLink()
{
  // ----------------------------------------------------------
  // Check for GPS unavailable
  // ----------------------------------------------------------

  if (
    receivedLatitude ==
      "0.000000"
    &&
    receivedLongitude ==
      "0.000000"
  )
  {
    Serial.println();

    Serial.println(
      "GPS STATUS : NO FIX"
    );

    Serial.println(
      "Google Maps link not generated."
    );

    return;
  }


  // ----------------------------------------------------------
  // Generate easy Google Maps search URL
  // ----------------------------------------------------------

  String googleMapsLink =
    "https://www.google.com/maps/search/?api=1&query=" +
    receivedLatitude +
    "," +
    receivedLongitude;


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "      EMERGENCY LOCATION"
  );

  Serial.println(
    "================================"
  );


  Serial.print(
    "Google Maps:"
  );

  Serial.println();


  Serial.println(
    googleMapsLink
  );


  Serial.println(
    "================================"
  );

  Serial.println();
}


// ============================================================
//                    DISPLAY SOS PAGE
// ============================================================

void displaySOSPage()
{
  if (
    !sosActive
  )
  {
    return;
  }


  // ----------------------------------------------------------
  // Change LCD page every 2 seconds
  // ----------------------------------------------------------

  if (
    millis() -
    lastLCDPageChange >=
    LCD_PAGE_TIME_MS
  )
  {
    lastLCDPageChange =
      millis();


    lcdPage++;


    if (
      lcdPage > 3
    )
    {
      lcdPage = 0;
    }
  }


  // ==========================================================
  // PAGE 0
  // ==========================================================

  if (
    lcdPage == 0
  )
  {
    printLCDLine(
      0,
      "!!! SOS !!!"
    );


    String line =
      "NODE:" +
      receivedNodeID +
      " S:" +
      receivedSequence;


    printLCDLine(
      1,
      line
    );
  }


  // ==========================================================
  // PAGE 1
  // ==========================================================

  else if (
    lcdPage == 1
  )
  {
    if (
      receivedLatitude ==
        "0.000000"
      &&
      receivedLongitude ==
        "0.000000"
    )
    {
      printLCDLine(
        0,
        "GPS: NO FIX"
      );


      printLCDLine(
        1,
        "SOS STILL VALID"
      );
    }
    else
    {
      String latDisplay =
        "LAT:" +
        receivedLatitude;


      String lonDisplay =
        "LON:" +
        receivedLongitude;


      printLCDLine(
        0,
        latDisplay
      );


      printLCDLine(
        1,
        lonDisplay
      );
    }
  }


  // ==========================================================
  // PAGE 2
  // ==========================================================

  else if (
    lcdPage == 2
  )
  {
    String rssiLine =
      "RSSI:" +
      String(
        receivedRSSI
      ) +
      "dBm";


    String ttlLine =
      "TTL:" +
      receivedTTL;


    printLCDLine(
      0,
      rssiLine
    );


    printLCDLine(
      1,
      ttlLine
    );
  }


  // ==========================================================
  // PAGE 3
  // ==========================================================

  else if (
    lcdPage == 3
  )
  {
    printLCDLine(
      0,
      "ACK STATUS"
    );


    if (
      ackSent
    )
    {
      printLCDLine(
        1,
        "RESCUE ACK SENT"
      );
    }
    else
    {
      printLCDLine(
        1,
        "ACK FAILED"
      );
    }
  }
}


// ============================================================
//                    DISPLAY READY
// ============================================================

void displayReady()
{
  lcd.clear();


  printLCDLine(
    0,
    "TerraLink Ready"
  );


  printLCDLine(
    1,
    "Monitoring..."
  );
}


// ============================================================
//                    HANDLE RECEIVED SOS
// ============================================================

void handleSOS(
  String packet
)
{
  // ----------------------------------------------------------
  // Extract all fields
  // ----------------------------------------------------------

  receivedNodeID =
    extractField(
      packet,
      "ID:"
    );


  receivedLatitude =
    extractField(
      packet,
      "LAT:"
    );


  receivedLongitude =
    extractField(
      packet,
      "LON:"
    );


  receivedSequence =
    extractField(
      packet,
      "SEQ:"
    );


  receivedTTL =
    extractField(
      packet,
      "TTL:"
    );


  receivedRSSI =
    LoRa.packetRssi();

    // ---------- GOOGLE MAPS URL ----------
    String googleMaps =
      "https://www.google.com/maps?q=" +
      receivedLatitude + "," + receivedLongitude;

  // ==========================================================
  // SERIAL INFORMATION
  // ==========================================================

  Serial.println();

  Serial.println(
    "========================================"
  );

    Serial.println("🚨 TERRALINK SOS RECEIVED 🚨");

  Serial.println(
    "========================================"
  );


  Serial.print(
    "RAW PACKET : "
  );

  Serial.println(
    packet
  );


  Serial.print(
    "NODE ID    : "
  );

  Serial.println(
    receivedNodeID
  );


  Serial.print(
    "LATITUDE   : "
  );

  Serial.println(
    receivedLatitude
  );


  Serial.print(
    "LONGITUDE  : "
  );

  Serial.println(
    receivedLongitude
  );


  Serial.print(
    "SEQUENCE   : "
  );

  Serial.println(
    receivedSequence
  );


  Serial.print(
    "TTL        : "
  );

  Serial.println(
    receivedTTL
  );


  Serial.print(
    "RSSI       : "
  );

  Serial.print(
    receivedRSSI
  );

  Serial.println(
    " dBm"
  );

 Serial.println("\n--- EMERGENCY LOCATION ---");
    Serial.println(googleMaps);


  // ==========================================================
  // GPS STATUS
  // ==========================================================

  if (
    receivedLatitude ==
      "0.000000"
    &&
    receivedLongitude ==
      "0.000000"
  )
  {
    Serial.println();

    Serial.println(
      "GPS STATUS : NO FIX"
    );

    Serial.println(
      "SOS remains valid."
    );
  }
  else
  {
    Serial.println();

    Serial.println(
      "GPS STATUS : FIX AVAILABLE"
    );


    // --------------------------------------------------------
    // Generate Google Maps link
    // --------------------------------------------------------

    printGoogleMapsLink();
  }


  // ==========================================================
  // DUPLICATE CHECK
  // ==========================================================

  if (
    isDuplicate(
      receivedNodeID,
      receivedSequence
    )
  )
  {
    Serial.println();

    Serial.println(
      "DUPLICATE SOS DETECTED."
    );


    Serial.println(
      "Emergency alarm will NOT restart."
    );


    Serial.println(
      "Resending ACK."
    );


    // --------------------------------------------------------
    // Important:
    //
    // If sender did not receive the previous ACK,
    // it may retransmit the same SOS.
    //
    // We therefore resend the ACK but do NOT
    // trigger the buzzer again.
    // --------------------------------------------------------

    sendACK(
      receivedNodeID,
      receivedSequence
    );


    return;
  }


  // ==========================================================
  // NEW SOS
  // ==========================================================

  rememberSOS(
    receivedNodeID,
    receivedSequence
  );


  Serial.println();

  Serial.println(
    "NEW SOS EVENT CONFIRMED."
  );


  // ==========================================================
  // SEND ACK IMMEDIATELY
  // ==========================================================

  sendACK(
    receivedNodeID,
    receivedSequence
  );


  // ==========================================================
  // START LOCAL EMERGENCY ALARM
  // ==========================================================

  startBuzzer();


  // ==========================================================
  // START LCD ALERT
  // ==========================================================

  sosActive =
    true;


  sosStartTime =
    millis();


  lcdPage =
    0;


  lastLCDPageChange =
    millis();


  displaySOSPage();


  Serial.println();

  Serial.println(
    "RESCUE STATION ALERT ACTIVE."
  );

  Serial.println(
    "========================================"
  );

  Serial.println();
}


// ============================================================
//                    SETUP
// ============================================================

void setup()
{
  // ==========================================================
  // SERIAL
  // ==========================================================

  Serial.begin(
    115200
  );


  delay(500);


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "       TERRALINK RESCUE STATION"
  );

  Serial.println(
    "              STAGE 1"
  );

  Serial.println(
    "========================================"
  );


  // ==========================================================
  // BUZZER
  // ==========================================================

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  // ==========================================================
  // LCD
  // ==========================================================

  // Existing receiver I2C pins
  Wire.begin(
    D2,
    D4
  );


  lcd.init();


  lcd.backlight();


  lcd.clear();


  printLCDLine(
    0,
    "TerraLink Ready"
  );


  printLCDLine(
    1,
    "Starting..."
  );


  delay(1000);


  // ==========================================================
  // LORA PIN CONFIGURATION
  // ==========================================================

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );


  Serial.println(
    "Initializing LoRa..."
  );


  // ==========================================================
  // LORA INITIALIZATION
  // ==========================================================

  if (
    !LoRa.begin(
      LORA_FREQUENCY
    )
  )
  {
    Serial.println(
      "ERROR: LoRa initialization FAILED."
    );


    lcd.clear();


    printLCDLine(
      0,
      "LoRa FAILED"
    );


    printLCDLine(
      1,
      "Check RA-02"
    );


    while (true)
    {
      delay(1000);
    }
  }


  Serial.println(
    "LoRa initialized."
  );


  // ==========================================================
  // MATCH SENDER SETTINGS
  // ==========================================================

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


  // ==========================================================
  // START RECEIVE MODE
  // ==========================================================

  LoRa.receive();


  Serial.println(
    "LoRa configuration complete."
  );


  // ==========================================================
  // READY DISPLAY
  // ==========================================================

  lcd.clear();


  printLCDLine(
    0,
    "TerraLink Ready"
  );


  printLCDLine(
    1,
    "Monitoring..."
  );


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "RESCUE STATION READY"
  );

  Serial.println(
    "Waiting for TerraLink SOS..."
  );

  Serial.println(
    "========================================"
  );

  Serial.println();
}


// ============================================================
//                    MAIN LOOP
// ============================================================

void loop()
{
  // ==========================================================
  // CHECK FOR LORA PACKET
  // ==========================================================

  int packetSize =
    LoRa.parsePacket();


  if (
    packetSize > 0
  )
  {
    receivedMessage =
      "";


    // --------------------------------------------------------
    // Read complete packet
    // --------------------------------------------------------

    while (
      LoRa.available()
    )
    {
      receivedMessage +=
        (char)LoRa.read();
    }


    receivedMessage.trim();


    // ========================================================
    // SERIAL RAW PACKET
    // ========================================================

    Serial.println();

    Serial.println(
      "LoRa packet received."
    );


    Serial.print(
      "Packet: "
    );


    Serial.println(
      receivedMessage
    );


    // ========================================================
    // CHECK VALID SOS
    // ========================================================

    if (
      isValidSOSPacket(
        receivedMessage
      )
    )
    {
      handleSOS(
        receivedMessage
      );
    }
    else
    {
      Serial.println(
        "Packet rejected: "
        "not a valid TerraLink SOS."
      );
    }


    // --------------------------------------------------------
    // Always return to receive mode
    // --------------------------------------------------------

    LoRa.receive();
  }


  // ==========================================================
  // BUZZER TIMEOUT
  // ==========================================================

  if (
    buzzerActive
    &&
    millis() -
      buzzerStartTime >=
    BUZZER_DURATION_MS
  )
  {
    stopBuzzer();
  }


  // ==========================================================
  // LCD ALERT
  // ==========================================================

  if (
    sosActive
  )
  {
    displaySOSPage();


    // --------------------------------------------------------
    // Return to monitoring after alert period
    // --------------------------------------------------------

    if (
      millis() -
        sosStartTime >=
      LCD_ALERT_TIME_MS
    )
    {
      sosActive =
        false;


      displayReady();


      Serial.println(
        "Rescue station returned "
        "to monitoring mode."
      );
    }
  }


  // ==========================================================
  // ESP8266 COOPERATIVE DELAY
  // ==========================================================

  yield();
}