#include <SPI.h>
#include <LoRa.h>

#define SS 10
#define RST 9
#define DIO0 2

String msg;

unsigned long lastPrint = 0;

void setup() {
  Serial.begin(9600);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa FAIL");
    while (1);
  }

  LoRa.setSyncWord(0xF3);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();

  Serial.println("RELAY READY - WAITING FOR SOS");
}

void loop() {

  if (millis() - lastPrint > 5000) {
    Serial.println("[STATUS] Waiting for SOS packets...");
    lastPrint = millis();
  }

  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  msg = "";

  while (LoRa.available()) {
    msg += (char)LoRa.read();
  }

  Serial.println("\n--- PACKET RECEIVED ---");
  Serial.println(msg);

  // BASIC VALIDATION ONLY
  if (msg.indexOf("ID:") == -1) {
    Serial.println("IGNORED (not SOS format)");
    return;
  }

  Serial.println("🚨 SOS ALERT RECEIVED");

  // SIMULATED FORWARD
  Serial.println("HOPPED TO RESCUE (SIMULATED)");
}