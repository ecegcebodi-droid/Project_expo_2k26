#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"

bool getPacketField(const String &packet, const char *field, String &value) {
    const String token(field);
    int searchFrom = 0;
    while (true) {
        const int pos = packet.indexOf(token, searchFrom);
        if (pos < 0) return false;
        if (pos == 0 || packet.charAt(pos - 1) == ',') {
            const int valueStart = pos + token.length();
            int valueEnd = packet.indexOf(',', valueStart);
            if (valueEnd < 0) valueEnd = packet.length();
            value = packet.substring(valueStart, valueEnd);
            value.trim();
            return true;
        }
        searchFrom = pos + 1;
    }
}

bool packetHasType(const String &packet, const char *type) {
    String value;
    return getPacketField(packet, "TYPE:", value) && value.equalsIgnoreCase(type);
}

String createSOSPacket() {
    String packet;
    packet.reserve(180);
    packet += "SRC:" + String(NODE_ID);
    packet += ",DST:" + String(BROADCAST_ID);
    packet += ",SEQ:" + String((uint32_t)activeSequence);
    packet += ",TTL:" + String(SOS_INITIAL_TTL);
    packet += ",HOP:0,LAT:";
    if (pendingSOS) packet += String(persistentSOSLatitude, 6);
    else if (gpsFixAvailable) packet += String((double)currentLatitude, 6);
    else packet += "0.000000";
    packet += ",LON:";
    if (pendingSOS) packet += String(persistentSOSLongitude, 6);
    else if (gpsFixAvailable) packet += String((double)currentLongitude, 6);
    else packet += "0.000000";
    packet += ",TYPE:SOS";
    return packet;
}
