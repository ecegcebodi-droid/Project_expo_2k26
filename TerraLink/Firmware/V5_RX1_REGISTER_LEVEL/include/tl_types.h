#ifndef TL_TYPES_H
#define TL_TYPES_H

#include <Arduino.h>

/*
 * ============================================================
 *                    TERRALINK DATA TYPES
 * ============================================================
 *
 * This file contains shared data structures only.
 *
 * TLPacket:
 *   Represents one LoRa packet.
 *
 * TLNodeState:
 *   Represents the state/statistics of one remote node.
 *
 * No hardware functions should be placed here.
 * ============================================================
 */


/* ------------------------------------------------------------
 * TerraLink LoRa Packet
 * ------------------------------------------------------------ */
struct TLPacket
{
    /* Basic addressing */
    uint16_t src;
    uint16_t dst;
    uint16_t seq;

    /* Routing */
    uint8_t ttl;
    uint8_t hop;

    /* Packet type */
    String type;

    /* GPS */
    float lat;
    float lon;

    bool hasLat;
    bool hasLon;

    /* Message */
    uint16_t msgId;
    bool hasMsgId;

    String msg;

    /* Radio information */
    int rssi;
    int snr;

    /* Packet validity */
    bool valid;

    /* Original raw packet */
    String raw;

    TLPacket()
    {
        src = 0;
        dst = 0;
        seq = 0;

        ttl = 0;
        hop = 0;

        type = "";

        lat = 0.0f;
        lon = 0.0f;

        hasLat = false;
        hasLon = false;

        msgId = 0;
        hasMsgId = false;

        msg = "";

        rssi = 0;
        snr = 0;

        valid = false;

        raw = "";
    }
};


/* ------------------------------------------------------------
 * TerraLink Node State
 * ------------------------------------------------------------ */
struct TLNodeState
{
    uint16_t nodeId;

    bool active;
    bool sosActive;

    unsigned long lastSeen;

    uint32_t packetCount;
    uint32_t sosCount;
    uint32_t msgCount;
    uint32_t locCount;
    uint32_t ackCount;

    int lastRSSI;

    float lastLat;
    float lastLon;

    bool gpsValid;

    TLNodeState()
    {
        nodeId = 0;

        active = false;
        sosActive = false;

        lastSeen = 0;

        packetCount = 0;
        sosCount = 0;
        msgCount = 0;
        locCount = 0;
        ackCount = 0;

        lastRSSI = 0;

        lastLat = 0.0f;
        lastLon = 0.0f;

        gpsValid = false;
    }
};

#endif