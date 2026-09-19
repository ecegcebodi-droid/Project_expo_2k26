#include <Arduino.h>
#include <string.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"

static void transmitPacket(const String &packet, const char *label) {
    if (!loraInitialized) {
        Serial.println("ERROR: LoRa not initialized.");
        return;
    }
    LoRa.idle();
    LoRa.beginPacket();
    LoRa.print(packet);
    const int result = LoRa.endPacket();
    Serial.println(result == 1 ? label : "Packet transmission failed.");
    LoRa.receive();
}

void sendMessageACK(int messageID, uint32_t sequence) {
    String ack = "SRC:" + String(NODE_ID) + ",DST:" + String(RECEIVER_NODE_ID) + ",SEQ:" + String(sequence) +
                 ",TTL:" + String(REVERSE_PACKET_TTL) + ",HOP:0,MSGID:" + String(messageID) +
                 ",STATUS:DELIVERED,TYPE:MSGACK";
    Serial.println("\n================================");
    Serial.println("SENDING ROUTED MESSAGE ACK");
    Serial.println(ack);
    transmitPacket(ack, "MSGACK SENT");
    Serial.println("================================");
}

void sendCurrentLocation() {
    String packet = "SRC:" + String(NODE_ID) + ",DST:" + String(RECEIVER_NODE_ID) + ",SEQ:" + String((uint32_t)activeSequence) +
                    ",TTL:" + String(REVERSE_PACKET_TTL) + ",HOP:0,LAT:";
    packet += gpsFixAvailable ? String((double)currentLatitude, 6) : "0.000000";
    packet += ",LON:";
    packet += gpsFixAvailable ? String((double)currentLongitude, 6) : "0.000000";
    packet += ",TYPE:LOC";
    Serial.println("\n================================");
    Serial.println("SENDING ROUTED LOCATION UPDATE");
    Serial.println(packet);
    transmitPacket(packet, "LOCATION UPDATE SENT");
    Serial.println("================================");
}

bool handleRescueMessage(const String &packet) {
    String src, dst, seq, msgIDString, sourceString, priorityString;
    if (!getPacketField(packet, "SRC:", src) || !getPacketField(packet, "DST:", dst) || !getPacketField(packet, "SEQ:", seq)) {
        Serial.println("MSG rejected - missing routing fields."); return false;
    }
    if (!packetHasType(packet, "MSG")) return false;
    if (src.toInt() != RECEIVER_NODE_ID || dst.toInt() != NODE_ID) {
        Serial.println("MSG ignored - wrong SRC/DST."); return false;
    }

    const uint32_t messageSequence = strtoul(seq.c_str(), nullptr, 10);
    int msgStart = packet.indexOf(",MSG:");
    if (msgStart < 0) { Serial.println("MSG rejected - no message text."); return false; }
    msgStart += 5;
    String message = packet.substring(msgStart);
    const int typePosition = message.indexOf(",TYPE:");
    if (typePosition >= 0) message = message.substring(0, typePosition);
    message.trim();
    if (message.isEmpty()) { Serial.println("MSG rejected - empty message."); return false; }
    if (message.length() > MAX_RESCUE_MESSAGE_LENGTH) message = message.substring(0, MAX_RESCUE_MESSAGE_LENGTH);

    int messageID = 0;
    if (getPacketField(packet, "MSGID:", msgIDString)) messageID = msgIDString.toInt();
    else {
        uint32_t hash = messageSequence;
        for (unsigned int i = 0; i < message.length(); ++i) hash = hash * 31UL + (uint8_t)message[i];
        messageID = (int)(hash % 30000UL); if (messageID <= 0) messageID = 1;
        Serial.println("MSGID not present - generated fallback ID.");
    }

    if (getPacketField(packet, "SOURCE:", sourceString)) {
        sourceString.trim(); sourceString.toUpperCase();
    } else sourceString = "RESCUE";
    if (sourceString.isEmpty()) sourceString = "RESCUE";
    if (sourceString.length() > 11) sourceString = sourceString.substring(0, 11);

    int priority = 0;
    if (getPacketField(packet, "PRIORITY:", priorityString)) priority = priorityString.toInt();
    priority = constrain(priority, 0, 2);

    const bool duplicate = messageID == lastReceivedMessageID && messageSequence == lastReceivedMessageSequence;
    Serial.println("\n================================");
    Serial.println("ROUTED RESCUE MESSAGE RECEIVED");
    Serial.print("SOURCE: "); Serial.println(sourceString);
    Serial.print("MSG ID: "); Serial.println(messageID);
    Serial.print("MESSAGE SEQ: "); Serial.println(messageSequence);
    Serial.print("ACTIVE SOS SEQ: "); Serial.println((uint32_t)activeSequence);
    Serial.print("PRIORITY: "); Serial.println(priority);
    Serial.print("MESSAGE: "); Serial.println(message);
    Serial.println(duplicate ? "STATUS: DUPLICATE" : "STATUS: NEW MESSAGE");
    Serial.println("================================");

    sendMessageACK(messageID, messageSequence);
    if (duplicate) { Serial.println("Duplicate rescue message. MSGACK resent; display suppressed."); return true; }

    lastReceivedMessageID = messageID;
    lastReceivedMessageSequence = messageSequence;

    portENTER_CRITICAL(&timerMux);
    if (rescueQueueCount < RESCUE_MESSAGE_QUEUE_SIZE) {
        RescueMessageRecord &slot = rescueMessageQueue[rescueQueueTail];
        slot.messageID = messageID; slot.sequence = messageSequence; slot.priority = priority;
        sourceString.toCharArray(slot.source, sizeof(slot.source));
        message.toCharArray(slot.message, sizeof(slot.message));
        rescueQueueTail = (rescueQueueTail + 1) % RESCUE_MESSAGE_QUEUE_SIZE;
        rescueQueueCount++;
    } else {
        RescueMessageRecord &slot = rescueMessageQueue[rescueQueueHead];
        slot.messageID = messageID; slot.sequence = messageSequence; slot.priority = priority;
        sourceString.toCharArray(slot.source, sizeof(slot.source));
        message.toCharArray(slot.message, sizeof(slot.message));
        rescueQueueHead = (rescueQueueHead + 1) % RESCUE_MESSAGE_QUEUE_SIZE;
        rescueQueueTail = rescueQueueHead;
    }
    message.toCharArray(receivedRescueMessage, sizeof(receivedRescueMessage));
    receivedMessageID = messageID;
    receivedMessageSequence = messageSequence;
    receivedMessagePriority = priority;
    sourceString.toCharArray(currentRescueMessageSource, sizeof(currentRescueMessageSource));
    rescueMessageReceived = true;
    portEXIT_CRITICAL(&timerMux);

    Serial.println("NEW MESSAGE QUEUED FOR OLED.");
    Serial.println("RESCUE MESSAGE SOUND QUEUED.");

    if (message.equalsIgnoreCase("SEND LOCATION AGAIN")) {
        Serial.println("LOCATION REQUEST MESSAGE RECEIVED");
        updateGPS();
        sendCurrentLocation();
    }
    return true;
}

bool handleLocationRequest(const String &packet) {
    String src, dst, seq;
    if (!getPacketField(packet, "SRC:", src) || !getPacketField(packet, "DST:", dst) || !getPacketField(packet, "SEQ:", seq)) return false;
    if (!packetHasType(packet, "LOCREQ")) return false;
    if (src.toInt() != RECEIVER_NODE_ID || dst.toInt() != NODE_ID) return false;
    const uint32_t requested = strtoul(seq.c_str(), nullptr, 10);
    if (requested != activeSequence) { Serial.println("LOCREQ ignored - sequence mismatch."); return false; }
    Serial.println("ROUTED LOCATION REQUEST RECEIVED");
    updateGPS();
    sendCurrentLocation();
    return true;
}

bool checkForFinalACK(const String &received) {
    String src, dst, seq, ttl, hop;
    if (!getPacketField(received, "SRC:", src) || !getPacketField(received, "DST:", dst) || !getPacketField(received, "SEQ:", seq)) return false;
    if (!packetHasType(received, "ACK")) return false;
    if (src.toInt() != RECEIVER_NODE_ID || dst.toInt() != NODE_ID) return false;
    const uint32_t ackSequence = strtoul(seq.c_str(), nullptr, 10);
    if (ackSequence != persistentSOSSequence) {
        Serial.println("ACK ignored - sequence mismatch.");
        Serial.print("Expected: "); Serial.println(persistentSOSSequence);
        Serial.print("Received: "); Serial.println(ackSequence);
        return false;
    }
    getPacketField(received, "TTL:", ttl); getPacketField(received, "HOP:", hop);
    receivedAckSequence = ackSequence;
    Serial.println("\n================================");
    Serial.println("FINAL ROUTED RESCUE ACK CONFIRMED");
    Serial.print("ACK HOP: "); Serial.println(hop.length() ? hop : "unknown");
    Serial.print("ACK TTL remaining: "); Serial.println(ttl.length() ? ttl : "unknown");
    Serial.println("================================");
    return true;
}

bool popNextRescueMessage() {
    bool available = false;
    portENTER_CRITICAL(&timerMux);
    if (rescueQueueCount > 0) {
        RescueMessageRecord &slot = rescueMessageQueue[rescueQueueHead];
        receivedMessageID = slot.messageID;
        receivedMessageSequence = slot.sequence;
        receivedMessagePriority = slot.priority;
        strncpy(receivedRescueMessage, slot.message, MAX_RESCUE_MESSAGE_LENGTH);
        receivedRescueMessage[MAX_RESCUE_MESSAGE_LENGTH] = '\0';
        strncpy(currentRescueMessageSource, slot.source, sizeof(currentRescueMessageSource) - 1);
        currentRescueMessageSource[sizeof(currentRescueMessageSource) - 1] = '\0';
        rescueQueueHead = (rescueQueueHead + 1) % RESCUE_MESSAGE_QUEUE_SIZE;
        rescueQueueCount--;
        available = true;
    }
    portEXIT_CRITICAL(&timerMux);
    return available;
}
