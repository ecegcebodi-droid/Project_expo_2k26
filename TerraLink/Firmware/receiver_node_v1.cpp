#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- LoRa ----------
#define SS 5
#define RST 16
#define DIO0 -1

// ---------- Buzzer ----------
#define BUZZER D3

// ---------- Data ----------
String msg = "";
int rssi = 0;

// ---------- SOS STATE ----------
bool sosActive = false;
unsigned long sosStart = 0;

// ---------- BUZZER CONTROL ----------
unsigned long buzzerStart = 0;
bool buzzerOn = false;

void setup() {

  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // LCD INIT
  Wire.begin(D2, D4);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("TerraLink Ready");
  lcd.setCursor(0,1);
  lcd.print("Monitoring...");

  // LoRa INIT
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {

    lcd.clear();
    lcd.print("LoRa Failed");

    Serial.println("LoRa FAILED");
    while (1);
  }

  LoRa.setSyncWord(0xF3);

  Serial.println("\n=== TERRALINK RESCUE STATION READY ===");
}

void loop() {

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    msg = "";

    while (LoRa.available()) {
      msg += (char)LoRa.read();
    }

    rssi = LoRa.packetRssi();

    String nodeID = extractValue("ID:");
    String lat    = extractValue("LAT:");
    String lon    = extractValue("LON:");
    String bat    = extractValue("BAT:");

    // ---------- GOOGLE MAPS URL ----------
    String googleMaps =
      "https://www.google.com/maps?q=" +
      lat + "," + lon;

    // ---------- SERIAL OUTPUT ----------
    Serial.println("\n==============================");
    Serial.println("🚨 TERRALINK SOS RECEIVED 🚨");
    Serial.println("==============================");

    Serial.print("RAW DATA : ");
    Serial.println(msg);

    Serial.print("RSSI     : ");
    Serial.println(rssi);

    Serial.print("NODE ID  : ");
    Serial.println(nodeID);

    Serial.print("LAT      : ");
    Serial.println(lat);

    Serial.print("LON      : ");
    Serial.println(lon);

    Serial.print("BATTERY  : ");
    Serial.println(bat);

    Serial.println("\n--- EMERGENCY LOCATION ---");
    Serial.println(googleMaps);

    Serial.println("==============================");

    // ---------- LCD ALERT ----------
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SOS RECEIVED!");
    lcd.setCursor(0,1);
    lcd.print(nodeID);

    // ---------- BUZZER ----------
    digitalWrite(BUZZER, HIGH);
    buzzerStart = millis();
    buzzerOn = true;

    // ---------- SOS STATE ----------
    sosActive = true;
    sosStart = millis();
  }

  // ---------- BUZZER OFF AFTER 2.5s ----------
  if (buzzerOn && millis() - buzzerStart >= 2500) {
    digitalWrite(BUZZER, LOW);
    buzzerOn = false;
  }

  // ---------- RESET DISPLAY ----------
  if (sosActive && millis() - sosStart >= 5000) {

    sosActive = false;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("System Ready");
    lcd.setCursor(0,1);
    lcd.print("Monitoring...");
  }

  if (!sosActive) {

    lcd.setCursor(0,0);
    lcd.print("TerraLink Ready");
    lcd.setCursor(0,1);
    lcd.print("Monitoring... ");
  }
}

// ---------- HELPER FUNCTION ----------
String extractValue(String key) {

  int start = msg.indexOf(key);
  if (start == -1) return "--";

  start += key.length();

  int end = msg.indexOf(",", start);
  if (end == -1) end = msg.length();

  return msg.substring(start, end);
}
