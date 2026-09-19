#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

/*
============================================================
                  TERRALINK RECEIVER #1
                    LoRa GATEWAY
============================================================

ESP8266 NodeMCU
SX1278 LoRa
16x2 I2C LCD
Buzzer
UART -> Receiver #2

LoRa:
SS   = D1
RST  = D0
DIO0 = polling mode
FREQ = 433 MHz

LCD:
SDA = D2
SCL = D4

BUZZER:
D8

UART:
RX1 -> RX2 : D3 -> D8
RX2 -> RX1 : D0 -> GPIO3/RX
GND -> GND

============================================================
*/

/* =========================================================
                         LoRa
========================================================= */

#define LORA_SS       D1
#define LORA_RST      D0
#define LORA_DIO0     -1
#define LORA_FREQ     433E6

#define RECEIVER_NODE_ID 4

/* =========================================================
                         LCD
========================================================= */

#define LCD_SDA D2
#define LCD_SCL D4

LiquidCrystal_I2C lcd(0x27, 16, 2);

/* =========================================================
                         BUZZER
========================================================= */

#define BUZZER_PIN D8

/* =========================================================
                         UART
========================================================= */

#define R2_TX_PIN D3

SoftwareSerial receiver2TX(-1, R2_TX_PIN);

#define UART_BAUD 9600

String uartCommandBuffer = "";

/* =========================================================
                       SOS DATA
========================================================= */

struct SOSData
{
  int src;
  unsigned long seq;

  float lat;
  float lon;

  int ttl;
  int hop;
  int rssi;

  bool valid;

  unsigned long receivedAt;
};

SOSData latestSOS;

int lastSOSSource = -1;
unsigned long lastSOSSeq = 0;

/* =========================================================
                    COMMAND SEQUENCE
========================================================= */

unsigned long commandSequence = 1000;

/* =========================================================
                    NETWORK STATISTICS
========================================================= */

unsigned long totalLoRaRx = 0;
unsigned long totalLoRaTx = 0;

unsigned long totalSOS = 0;
unsigned long totalACK = 0;
unsigned long totalLOC = 0;
unsigned long totalMSG = 0;

unsigned long totalCommands = 0;

unsigned long lastLoRaActivity = 0;

/* =========================================================
                       PROTOTYPES
========================================================= */

String getField(const String &packet, const String &key);

void lcdHome();
void lcdSOS();
void lcdCommandSent(const String &name, int node);

void buzzerSOS();
void buzzerTest();

void sendToReceiver2(const String &msg);
void processReceiver2UART();
void processOperatorCommand(String command);

void setupLoRa();
void sendLoRa(const String &packet);
void checkLoRa();

void processSOS(const String &packet);
void processLOC(const String &packet);
void processACK(const String &packet);
void processMSGACK(const String &packet);
void processRESOLVEACK(const String &packet);

void sendACK(int source, unsigned long seq);

void sendCommand(
  int destination,
  unsigned long seq,
  String message,
  bool priority
);

void requestLocation(
  int destination,
  unsigned long seq
);

void requestLocationAgain(
  int destination,
  unsigned long seq
);

void resolveSOS(
  int destination,
  unsigned long seq
);

void sendPriority(
  int destination,
  unsigned long seq
);

unsigned long getCommandSequence(int node);

void sendGatewayStatus();

/* =========================================================
                         GET FIELD
========================================================= */

String getField(
  const String &packet,
  const String &key
)
{
  String search = key + ":";

  int start = packet.indexOf(search);

  if (start < 0)
    return "";

  start += search.length();

  int end = packet.indexOf(',', start);

  if (end < 0)
    end = packet.length();

  return packet.substring(start, end);
}

/* =========================================================
                         LCD HOME
========================================================= */

void lcdHome()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("TERRALINK RX1");

  lcd.setCursor(0, 1);

  if (latestSOS.valid)
    lcd.print("SOS NODE ");
  else
    lcd.print("NETWORK READY");

  if (latestSOS.valid)
    lcd.print(latestSOS.src);
}

/* =========================================================
                         LCD SOS
========================================================= */

void lcdSOS()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SOS NODE:");
  lcd.print(latestSOS.src);

  lcd.setCursor(0, 1);
  lcd.print("SEQ:");
  lcd.print(latestSOS.seq);
}

/* =========================================================
                    LCD COMMAND SENT
========================================================= */

void lcdCommandSent(
  const String &name,
  int node
)
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("NODE:");
  lcd.print(node);

  lcd.setCursor(0, 1);

  if (name.length() <= 16)
    lcd.print(name);
  else
    lcd.print(name.substring(0, 16));
}

/* =========================================================
                         BUZZER
========================================================= */

void buzzerSOS()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(180);

    digitalWrite(BUZZER_PIN, LOW);
    delay(120);
  }
}

void buzzerTest()
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(700);
  digitalWrite(BUZZER_PIN, LOW);
}

/* =========================================================
                     UART -> RX2
========================================================= */

void sendToReceiver2(
  const String &msg
)
{
  receiver2TX.println(msg);
  receiver2TX.flush();

  Serial.print("[UART TX -> RX2] ");
  Serial.println(msg);
}

/* =========================================================
                  GATEWAY STATUS
========================================================= */

void sendGatewayStatus()
{
  String msg =
    "RX1_STATUS," +
    String(totalLoRaRx) + "," +
    String(totalLoRaTx) + "," +
    String(totalSOS) + "," +
    String(totalACK) + "," +
    String(totalLOC) + "," +
    String(totalMSG);

  sendToReceiver2(msg);
}

/* =========================================================
                       SEND LoRa
========================================================= */

void sendLoRa(
  const String &packet
)
{
  LoRa.idle();

  delay(5);

  LoRa.beginPacket();
  LoRa.print(packet);

  int result = LoRa.endPacket();

  if (result == 1)
  {
    totalLoRaTx++;
    lastLoRaActivity = millis();

    Serial.print("[LoRa TX] ");
    Serial.println(packet);
  }
  else
  {
    Serial.println("[LoRa TX] FAILED");
  }

  delay(10);

  LoRa.receive();
}

/* =========================================================
                           ACK
========================================================= */

void sendACK(
  int source,
  unsigned long seq
)
{
  String ack =
    "SRC:4,"
    "DST:" + String(source) +
    ",SEQ:" + String(seq) +
    ",TTL:5,"
    "HOP:0,"
    "TYPE:ACK";

  sendLoRa(ack);
}

/* =========================================================
                    COMMAND SEQUENCE
========================================================= */

unsigned long getCommandSequence(
  int node
)
{
  if (
    latestSOS.valid &&
    latestSOS.src == node
  )
  {
    return latestSOS.seq;
  }

  commandSequence++;

  if (commandSequence < 1000)
    commandSequence = 1000;

  return commandSequence;
}

/* =========================================================
                    NORMAL MESSAGE
========================================================= */

void sendCommand(
  int destination,
  unsigned long seq,
  String message,
  bool priority
)
{
  /*
     Commas can interfere with the simple packet parser.
     Replace them for prototype stability.
  */

  message.replace(",", " ");

  String packet =
    "SRC:4,"
    "DST:" + String(destination) +
    ",SEQ:" + String(seq) +
    ",TTL:5,"
    "HOP:0,"
    "TYPE:MSG,"
    "PRIORITY:" +
    String(priority ? 1 : 0) +
    ",MSG:" +
    message;

  sendLoRa(packet);

  totalMSG++;

  sendToReceiver2(
    "TX_STATUS,SENT," +
    String(destination) +
    "," +
    String(seq) +
    "," +
    message
  );

  lcdCommandSent(
    priority ? "PRIORITY" : "MESSAGE",
    destination
  );
}

/* =========================================================
                    LOCATION REQUEST
========================================================= */

void requestLocation(
  int destination,
  unsigned long seq
)
{
  String packet =
    "SRC:4,"
    "DST:" + String(destination) +
    ",SEQ:" + String(seq) +
    ",TTL:5,"
    "HOP:0,"
    "TYPE:LOCREQ";

  sendLoRa(packet);

  sendToReceiver2(
    "TX_STATUS,LOCATION_REQUEST," +
    String(destination) +
    "," +
    String(seq)
  );

  lcdCommandSent("GPS REQUEST", destination);
}

/* =========================================================
                  LOCATION AGAIN
========================================================= */

void requestLocationAgain(
  int destination,
  unsigned long seq
)
{
  String packet =
    "SRC:4,"
    "DST:" + String(destination) +
    ",SEQ:" + String(seq) +
    ",TTL:5,"
    "HOP:0,"
    "TYPE:LOC_AGAIN";

  sendLoRa(packet);

  sendToReceiver2(
    "TX_STATUS,LOCATION_AGAIN," +
    String(destination) +
    "," +
    String(seq)
  );

  lcdCommandSent("LOC AGAIN", destination);
}

/* =========================================================
                        RESOLVE
========================================================= */

void resolveSOS(
  int destination,
  unsigned long seq
)
{
  String packet =
    "SRC:4,"
    "DST:" + String(destination) +
    ",SEQ:" + String(seq) +
    ",TTL:5,"
    "HOP:0,"
    "TYPE:RESOLVE";

  sendLoRa(packet);

  sendToReceiver2(
    "TX_STATUS,RESOLVE_SENT," +
    String(destination) +
    "," +
    String(seq)
  );

  lcdCommandSent("RESOLVED", destination);
}

/* =========================================================
                       PRIORITY
========================================================= */

void sendPriority(
  int destination,
  unsigned long seq
)
{
  sendCommand(
    destination,
    seq,
    "EMERGENCY PRIORITY",
    true
  );
}

/* =========================================================
                         PROCESS SOS
========================================================= */

void processSOS(
  const String &packet
)
{
  int src =
    getField(packet, "SRC").toInt();

  unsigned long seq =
    strtoul(
      getField(packet, "SEQ").c_str(),
      NULL,
      10
    );

  float lat =
    getField(packet, "LAT").toFloat();

  float lon =
    getField(packet, "LON").toFloat();

  int ttl =
    getField(packet, "TTL").toInt();

  int hop =
    getField(packet, "HOP").toInt();

  int rssi =
    LoRa.packetRssi();

  bool duplicate =
    (
      src == lastSOSSource &&
      seq == lastSOSSeq
    );

  /*
     ALWAYS ACK.
  */

  sendACK(src, seq);
  totalACK++;

  if (duplicate)
  {
    Serial.println("[SOS] Duplicate -> ACK only");

    sendToReceiver2(
      "SOS_DUPLICATE," +
      String(src) +
      "," +
      String(seq)
    );

    return;
  }

  latestSOS.src = src;
  latestSOS.seq = seq;
  latestSOS.lat = lat;
  latestSOS.lon = lon;
  latestSOS.ttl = ttl;
  latestSOS.hop = hop;
  latestSOS.rssi = rssi;
  latestSOS.valid = true;
  latestSOS.receivedAt = millis();

  lastSOSSource = src;
  lastSOSSeq = seq;

  totalSOS++;
  totalLoRaRx++;
  lastLoRaActivity = millis();

  Serial.println();
  Serial.println("======================================");
  Serial.println("           NEW TERRALINK SOS");
  Serial.println("======================================");

  Serial.print("NODE : ");
  Serial.println(src);

  Serial.print("SEQ  : ");
  Serial.println(seq);

  Serial.print("LAT  : ");
  Serial.println(lat, 6);

  Serial.print("LON  : ");
  Serial.println(lon, 6);

  Serial.print("HOP  : ");
  Serial.println(hop);

  Serial.print("RSSI : ");
  Serial.println(rssi);

  Serial.println("======================================");

  lcdSOS();

  buzzerSOS();

  /*
     Forward complete SOS information.
  */

  String uartMessage =
    "SOS," +
    String(src) +
    "," +
    String(seq) +
    "," +
    String(lat, 6) +
    "," +
    String(lon, 6) +
    "," +
    String(hop) +
    "," +
    String(rssi) +
    "," +
    String(ttl);

  sendToReceiver2(uartMessage);
}

/* =========================================================
                     PROCESS LOCATION
========================================================= */

void processLOC(
  const String &packet
)
{
  int src =
    getField(packet, "SRC").toInt();

  unsigned long seq =
    strtoul(
      getField(packet, "SEQ").c_str(),
      NULL,
      10
    );

  float lat =
    getField(packet, "LAT").toFloat();

  float lon =
    getField(packet, "LON").toFloat();

  int hop =
    getField(packet, "HOP").toInt();

  int rssi =
    LoRa.packetRssi();

  totalLOC++;
  totalLoRaRx++;

  String msg =
    "LOCATION," +
    String(src) +
    "," +
    String(seq) +
    "," +
    String(lat, 6) +
    "," +
    String(lon, 6) +
    "," +
    String(hop) +
    "," +
    String(rssi);

  sendToReceiver2(msg);

  Serial.println("[LoRa] LOCATION forwarded");
}

/* =========================================================
                       PROCESS ACK
========================================================= */

void processACK(
  const String &packet
)
{
  int src =
    getField(packet, "SRC").toInt();

  unsigned long seq =
    strtoul(
      getField(packet, "SEQ").c_str(),
      NULL,
      10
    );

  totalACK++;
  totalLoRaRx++;

  String msg =
    "ACK," +
    String(src) +
    "," +
    String(seq);

  sendToReceiver2(msg);

  Serial.println("[LoRa] ACK forwarded");
}

/* =========================================================
                    MESSAGE ACK
========================================================= */

void processMSGACK(
  const String &packet
)
{
  int src =
    getField(packet, "SRC").toInt();

  unsigned long seq =
    strtoul(
      getField(packet, "SEQ").c_str(),
      NULL,
      10
    );

  String status =
    getField(packet, "STATUS");

  totalLoRaRx++;

  sendToReceiver2(
    "MSG_ACK," +
    String(src) +
    "," +
    String(seq) +
    "," +
    status
  );
}

/* =========================================================
                   RESOLVE ACK
========================================================= */

void processRESOLVEACK(
  const String &packet
)
{
  int src =
    getField(packet, "SRC").toInt();

  unsigned long seq =
    strtoul(
      getField(packet, "SEQ").c_str(),
      NULL,
      10
    );

  totalLoRaRx++;

  sendToReceiver2(
    "RESOLVE_ACK," +
    String(src) +
    "," +
    String(seq)
  );
}

/* =========================================================
                  PROCESS RX2 COMMAND
========================================================= */

void processOperatorCommand(
  String command
)
{
  command.trim();

  if (command.length() == 0)
    return;

  Serial.print("[RX2 COMMAND] ");
  Serial.println(command);

  totalCommands++;

  /*
     --------------------------------------------------------
                       TL PROTOCOL
     --------------------------------------------------------

     TL,MSG,12,STAY CALM

     TL,LOC_AGAIN,12

     TL,GPS_REQUEST,12

     TL,RESOLVE,12

     TL,EMERGENCY,12
  */

  if (command.startsWith("TL,"))
  {
    int p1 = command.indexOf(',');
    int p2 = command.indexOf(',', p1 + 1);
    int p3 = command.indexOf(',', p2 + 1);

    if (p1 < 0 || p2 < 0)
    {
      Serial.println("[ERROR] Invalid TL command");
      return;
    }

    String type =
      command.substring(p1 + 1, p2);

    String nodeString;

    if (p3 >= 0)
      nodeString =
        command.substring(p2 + 1, p3);
    else
      nodeString =
        command.substring(p2 + 1);

    nodeString.trim();

    int node =
      nodeString.toInt();

    String payload = "";

    if (p3 >= 0)
    {
      payload =
        command.substring(p3 + 1);

      payload.trim();
    }

    unsigned long seq =
      getCommandSequence(node);

    /* -----------------------------------------------------
                           MESSAGE
    ----------------------------------------------------- */

    if (type == "MSG")
    {
      if (payload.length() == 0)
      {
        Serial.println("[ERROR] Empty message");
        return;
      }

      sendCommand(
        node,
        seq,
        payload,
        false
      );

      return;
    }

    /* -----------------------------------------------------
                        LOCATION AGAIN
    ----------------------------------------------------- */

    if (type == "LOC_AGAIN")
    {
      requestLocationAgain(
        node,
        seq
      );

      return;
    }

    /* -----------------------------------------------------
                       GPS REQUEST
    ----------------------------------------------------- */

    if (type == "GPS_REQUEST")
    {
      requestLocation(
        node,
        seq
      );

      return;
    }

    /* -----------------------------------------------------
                          RESOLVE
    ----------------------------------------------------- */

    if (type == "RESOLVE")
    {
      resolveSOS(
        node,
        seq
      );

      return;
    }

    /* -----------------------------------------------------
                        EMERGENCY
    ----------------------------------------------------- */

    if (type == "EMERGENCY")
    {
      sendPriority(
        node,
        seq
      );

      return;
    }

    Serial.print("[ERROR] Unknown TL command: ");
    Serial.println(type);

    return;
  }

  /* -------------------------------------------------------
                       STATUS REQUEST
  ------------------------------------------------------- */

  if (command == "GET_STATUS")
  {
    sendGatewayStatus();
    return;
  }

  /* -------------------------------------------------------
                       BUZZER TEST
  ------------------------------------------------------- */

  if (command == "BUZZER_TEST")
  {
    buzzerTest();

    sendToReceiver2(
      "CMD_STATUS,BUZZER_TEST,OK"
    );

    return;
  }

  Serial.println("[UART] Unknown command");
}

/* =========================================================
                  PROCESS RX2 UART
========================================================= */

void processReceiver2UART()
{
  while (Serial.available())
  {
    char c = Serial.read();

    if (c == '\r')
      continue;

    if (c == '\n')
    {
      uartCommandBuffer.trim();

      if (uartCommandBuffer.length() > 0)
      {
        processOperatorCommand(
          uartCommandBuffer
        );
      }

      uartCommandBuffer = "";

      continue;
    }

    uartCommandBuffer += c;

    if (uartCommandBuffer.length() > 220)
    {
      uartCommandBuffer = "";

      Serial.println(
        "[ERROR] UART buffer overflow"
      );
    }
  }
}

/* =========================================================
                        CHECK LoRa
========================================================= */

void checkLoRa()
{
  int packetSize =
    LoRa.parsePacket();

  if (packetSize <= 0)
    return;

  String packet = "";

  while (LoRa.available())
  {
    packet += (char)LoRa.read();
  }

  packet.trim();

  Serial.println();
  Serial.println("[LoRa RX]");
  Serial.println(packet);

  String type =
    getField(packet, "TYPE");

  if (type == "SOS")
  {
    processSOS(packet);
    return;
  }

  if (type == "LOC")
  {
    processLOC(packet);
    return;
  }

  if (type == "ACK")
  {
    processACK(packet);
    return;
  }

  if (type == "MSGACK")
  {
    processMSGACK(packet);
    return;
  }

  if (type == "RESOLVEACK")
  {
    processRESOLVEACK(packet);
    return;
  }

  Serial.print("[LoRa] Unknown TYPE: ");
  Serial.println(type);
}

/* =========================================================
                       SETUP LoRa
========================================================= */

void setupLoRa()
{
  SPI.begin();

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );

  Serial.println("[LoRa] Initializing...");

  if (!LoRa.begin(LORA_FREQ))
  {
    Serial.println("[LoRa] INITIALIZATION FAILED");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LORA INIT FAIL");

    while (true)
    {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);

      digitalWrite(BUZZER_PIN, LOW);
      delay(900);

      yield();
    }
  }

  LoRa.setSyncWord(0xF3);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  LoRa.setTxPower(17);

  LoRa.receive();

  Serial.println("[LoRa] READY");
}

/* =========================================================
                          SETUP
========================================================= */

void setup()
{
  Serial.begin(UART_BAUD);

  receiver2TX.begin(UART_BAUD);

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  Wire.begin(
    LCD_SDA,
    LCD_SCL
  );

  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("TERRALINK RX1");

  lcd.setCursor(0, 1);
  lcd.print("STARTING...");

  latestSOS.valid = false;

  latestSOS.src = 0;
  latestSOS.seq = 0;
  latestSOS.lat = 0;
  latestSOS.lon = 0;
  latestSOS.ttl = 0;
  latestSOS.hop = 0;
  latestSOS.rssi = 0;
  latestSOS.receivedAt = 0;

  delay(500);

  setupLoRa();

  lcdHome();

  sendToReceiver2(
    "RX1,ONLINE"
  );

  sendGatewayStatus();

  Serial.println();
  Serial.println("========================================");
  Serial.println("       TERRALINK RECEIVER #1 READY");
  Serial.println("========================================");
}

/* =========================================================
                          LOOP
========================================================= */

void loop()
{
  checkLoRa();

  processReceiver2UART();

  yield();
}