#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"

void initializeLoRa() {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    Serial.println("Initializing LoRa...");
    if (!LoRa.begin(LORA_FREQUENCY)) {
        loraInitialized = false;
        Serial.println("ERROR: LoRa initialization FAILED.");
        return;
    }
    loraInitialized = true;
    LoRa.setSyncWord(LORA_SYNC_WORD);
    LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(LORA_BANDWIDTH);
    LoRa.setCodingRate4(LORA_CODING_RATE);
    LoRa.enableCrc();
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.receive();
    Serial.println("LoRa initialized successfully.");
    Serial.println("LoRa configuration complete.");
}

void sendSOS() {
    if (!loraInitialized) { Serial.println("ERROR: LoRa not initialized."); return; }
    const String packet = createSOSPacket();
    Serial.println("\n================================");
    Serial.println("TERRALINK ROUTED SOS TRANSMISSION");
    Serial.print("Packet: "); Serial.println(packet);
    Serial.print("Sequence: "); Serial.println((uint32_t)activeSequence);
    Serial.println("Destination: BROADCAST / MESH");
    Serial.print("Initial TTL: "); Serial.println(SOS_INITIAL_TTL);
    Serial.println("Hop: 0 (sender)");
    if (pendingSOS && persistentSOSGPSFix) {
        Serial.print("Stored GPS Latitude: "); Serial.println(persistentSOSLatitude, 6);
        Serial.print("Stored GPS Longitude: "); Serial.println(persistentSOSLongitude, 6);
    } else if (gpsFixAvailable) {
        Serial.print("GPS Latitude: "); Serial.println((double)currentLatitude, 6);
        Serial.print("GPS Longitude: "); Serial.println((double)currentLongitude, 6);
    } else {
        Serial.println("GPS: NO FIX"); Serial.println("LAT/LON = 0.000000"); Serial.println("SOS transmission NOT blocked.");
    }
    LoRa.idle();
    LoRa.beginPacket();
    LoRa.print(packet);
    const int result = LoRa.endPacket();
    Serial.println(result == 1 ? "Routed SOS transmitted successfully." : "SOS transmission failed.");
    LoRa.receive();
    Serial.println("LoRa returned to RX mode.");
    Serial.println("================================\n");
}

void processLoRaPacket(const String &received) {
    if (packetHasType(received, "MSG")) { handleRescueMessage(received); return; }
    if (packetHasType(received, "LOCREQ")) { handleLocationRequest(received); return; }
    if (packetHasType(received, "ACK")) {
        if (checkForFinalACK(received)) {
            portENTER_CRITICAL(&timerMux); finalAckReceived = true; portEXIT_CRITICAL(&timerMux);
        }
        return;
    }
    if (packetHasType(received, "LOC")) { Serial.println("LOCATION packet received/looped back; ignored."); return; }
    if (packetHasType(received, "HELLO")) { Serial.println("HELLO received; sender does not maintain routing table."); return; }
    Serial.println("Packet not recognized.");
}

void LoRaTask(void *parameter) {
    (void)parameter;
    Serial.println("LoRa task running on Core 0.");
    while (true) {
        if (sosTransmitRequest) {
            sosTransmitRequest = false;
            portENTER_CRITICAL(&timerMux); finalAckReceived = false; portEXIT_CRITICAL(&timerMux);
            sendSOS();
        }
        if (loraInitialized) {
            const int packetSize = LoRa.parsePacket();
            if (packetSize > 0) {
                String received;
                received.reserve(packetSize + 1);
                while (LoRa.available()) received += (char)LoRa.read();
                received.trim();
                Serial.println("\n--------------------------------");
                Serial.print("LoRa RX: "); Serial.println(received);
                Serial.print("RSSI: "); Serial.println(LoRa.packetRssi());
                Serial.println("--------------------------------");
                processLoRaPacket(received);
                LoRa.receive();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
