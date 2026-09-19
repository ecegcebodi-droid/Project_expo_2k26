#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <U8g2lib.h>

// ---------- OLED ----------
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ---------- LoRa ----------
#define SS 5          // D1
#define RST 16        // D0
#define DIO0 -1

// ---------- Touch ----------
#define TOUCH 4       // D2

// ---------- GPS ----------
SoftwareSerial gpsSerial(D3, -1);
TinyGPSPlus gps;

// ---------- Node ----------
int nodeID = 101;
int battery = 85;

// ---------- Packet control (NEW) ----------
unsigned long packetCounter = 0;   // PKT generator

// ---------- SOS Display ----------
bool sosActive = false;
unsigned long sosStartTime = 0;

// ---------- Touch Detection ----------
bool lastTouchState = LOW;

void setup() {

  Serial.begin(9600);

  // OLED
  Wire.begin(D4, 3);
  u8g2.begin();

  // GPS
  gpsSerial.begin(9600);

  // Touch
  pinMode(TOUCH, INPUT);

  // LoRa
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LORA FAILED");
    while (1);
  }

  LoRa.setSyncWord(0xF3);

  Serial.println("SYSTEM READY");
}

void loop() {

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  String latitude = "0.000000";
  String longitude = "0.000000";

  if (gps.location.isValid()) {
    latitude = String(gps.location.lat(), 6);
    longitude = String(gps.location.lng(), 6);
  }

  bool currentTouch = digitalRead(TOUCH);

  if (currentTouch == HIGH && lastTouchState == LOW) {

    sosActive = true;
    sosStartTime = millis();

    // ================= NEW PACKET STRUCTURE =================
    packetCounter++;

    String message =
      "ID:" + String(nodeID) +
      ",LAT:" + latitude +
      ",LON:" + longitude +
      ",BAT:" + String(battery) +
      ",PKT:" + String(packetCounter) +
      ",TTL:3" +
      ",HOP:0" +
      ",SOS";

    Serial.println(message);

    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();

    Serial.println("SOS SENT");
    Serial.println("----------------");
  }

  lastTouchState = currentTouch;

  // ---------- OLED ----------
  if (sosActive) {

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(15,30,"SOS ALERT SENT");
    u8g2.sendBuffer();

    if (millis() - sosStartTime >= 2000) {
      sosActive = false;
    }

    delay(50);
    return;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  u8g2.drawStr(0,10,"ID:");
  u8g2.setCursor(25,10);
  u8g2.print(nodeID);

  u8g2.drawStr(0,22,"STATUS:");
  u8g2.setCursor(55,22);
  u8g2.print("ACTIVE");

  u8g2.drawStr(0,34,"LAT:");
  u8g2.setCursor(25,34);
  u8g2.print(latitude);

  u8g2.drawStr(0,46,"LON:");
  u8g2.setCursor(25,46);
  u8g2.print(longitude);

  u8g2.drawStr(0,58,"BAT:");
  u8g2.setCursor(25,58);
  u8g2.print(battery);
  u8g2.print("%");

  u8g2.sendBuffer();

  delay(100);
}