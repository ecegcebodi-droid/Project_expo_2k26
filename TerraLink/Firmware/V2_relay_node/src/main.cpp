/*
 * ================================================================
 *                 TERRALINK ROUTED RELAY NODE
 * ================================================================
 *
 * Hardware:
 *   Arduino UNO
 *   AI-Thinker RA-02 / SX1278
 *
 * LoRa:
 *   NSS/CS  -> D10
 *   RST     -> D9
 *   DIO0    -> D2
 *   SCK     -> D13
 *   MISO    -> D12
 *   MOSI    -> D11
 *
 * NETWORK
 *
 *       Node 1          Node 2          Node 3          Node 4
 *       ESP32           UNO             UNO             ESP8266
 *       Sender          Relay 1         Relay 2         Rescue
 *
 *          SOS
 *   1 ------------> 2 ------------> 3 ------------> 4
 *
 *          ACK
 *   1 <------------ 2 <------------ 3 <------------ 4
 *
 * Packet:
 *
 * SRC:1,DST:255,SEQ:17,TTL:5,HOP:0,
 * LAT:11.234567,LON:76.123456,TYPE:SOS
 *
 * Relay-forwarded:
 *
 * SRC:1,DST:255,SEQ:17,TTL:4,HOP:1,FROM:2,
 * LAT:11.234567,LON:76.123456,TYPE:SOS
 *
 * ================================================================
 */

#include <SPI.h>
#include <LoRa.h>

// ================================================================
//                     HARDWARE
// ================================================================

#define LORA_SS       10
#define LORA_RST       9
#define LORA_DIO0      2

// ================================================================
//                     NODE IDs
// ================================================================
//
// Relay 1:
//     #define RELAY_NODE_ID 2
//
// Relay 2:
//     #define RELAY_NODE_ID 3
//

#define RELAY_NODE_ID       2

#define SENDER_NODE_ID      1
#define RESCUE_NODE_ID      4
#define BROADCAST_ID        255

// ================================================================
//                     LORA CONFIG
// ================================================================

#define LORA_FREQUENCY      433E6
#define LORA_SYNC_WORD      0xF3
#define LORA_SPREADING      7
#define LORA_BANDWIDTH      125E3
#define LORA_CODING_RATE    5
#define LORA_TX_POWER       17

// ================================================================
//                     ROUTING
// ================================================================

#define MAX_TTL                 5

#define ROUTE_COUNT             8
#define NEIGHBOUR_COUNT         8
#define DUPLICATE_COUNT         12

#define ROUTE_TIMEOUT_MS        60000UL
#define NEIGHBOUR_TIMEOUT_MS    30000UL
#define DUPLICATE_TIMEOUT_MS    30000UL

#define HELLO_INTERVAL_MS       10000UL

#define FORWARD_DELAY_MIN       20
#define FORWARD_DELAY_MAX       70

// ================================================================
//                     PACKET BUFFER
// ================================================================

#define PACKET_BUFFER_SIZE      190

char rxPacket[PACKET_BUFFER_SIZE];
char txPacket[PACKET_BUFFER_SIZE];

// ================================================================
//                     TYPE IDs
// ================================================================

#define TYPE_UNKNOWN    0
#define TYPE_SOS        1
#define TYPE_ACK        2
#define TYPE_MSG        3
#define TYPE_MSGACK     4
#define TYPE_LOCREQ     5
#define TYPE_LOC        6
#define TYPE_HELLO      7

// ================================================================
//                     ROUTE TABLE
// ================================================================

struct RouteEntry
{
    uint8_t destination;
    uint8_t nextHop;
    uint8_t hopCount;
    int8_t rssi;
    unsigned long lastSeen;
    bool valid;
};

RouteEntry routes[ROUTE_COUNT];

// ================================================================
//                     NEIGHBOUR TABLE
// ================================================================

struct NeighbourEntry
{
    uint8_t node;
    int8_t rssi;
    unsigned long lastSeen;
    bool valid;
};

NeighbourEntry neighbours[NEIGHBOUR_COUNT];

// ================================================================
//                     DUPLICATE TABLE
// ================================================================

struct DuplicateEntry
{
    uint8_t src;
    uint32_t seq;
    uint16_t msgID;
    uint8_t type;
    unsigned long timeSeen;
    bool valid;
};

DuplicateEntry duplicates[DUPLICATE_COUNT];

// ================================================================
//                     STATISTICS
// ================================================================

unsigned long packetsReceived = 0;
unsigned long packetsForwarded = 0;
unsigned long routeForwarded = 0;
unsigned long floodForwarded = 0;
unsigned long packetsDropped = 0;
unsigned long duplicateDropped = 0;
unsigned long ttlDropped = 0;

unsigned long lastHello = 0;
unsigned long lastStatus = 0;

// ================================================================
//                     FIELD PARSER
// ================================================================

bool getField(
    const char *packet,
    const char *field,
    char *output,
    uint8_t outputSize
)
{
    uint8_t fieldLength = strlen(field);

    const char *p = packet;

    while (*p)
    {
        if (
            strncmp(
                p,
                field,
                fieldLength
            ) == 0 &&
            p[fieldLength] == ':'
        )
        {
            p += fieldLength + 1;

            uint8_t i = 0;

            while (
                *p &&
                *p != ',' &&
                i < outputSize - 1
            )
            {
                output[i++] = *p++;
            }

            output[i] = '\0';

            return true;
        }

        while (
            *p &&
            *p != ','
        )
        {
            p++;
        }

        if (*p == ',')
        {
            p++;
        }
    }

    return false;
}

// ================================================================
//                     INTEGER FIELD
// ================================================================

long getFieldLong(
    const char *packet,
    const char *field,
    long defaultValue
)
{
    char value[16];

    if (
        !getField(
            packet,
            field,
            value,
            sizeof(value)
        )
    )
    {
        return defaultValue;
    }

    return atol(value);
}

// ================================================================
//                     TYPE
// ================================================================

uint8_t getType(
    const char *packet
)
{
    char value[12];

    if (
        !getField(
            packet,
            "TYPE",
            value,
            sizeof(value)
        )
    )
    {
        return TYPE_UNKNOWN;
    }

    if (strcmp(value, "SOS") == 0)
        return TYPE_SOS;

    if (strcmp(value, "ACK") == 0)
        return TYPE_ACK;

    if (strcmp(value, "MSG") == 0)
        return TYPE_MSG;

    if (strcmp(value, "MSGACK") == 0)
        return TYPE_MSGACK;

    if (strcmp(value, "LOCREQ") == 0)
        return TYPE_LOCREQ;

    if (strcmp(value, "LOC") == 0)
        return TYPE_LOC;

    if (strcmp(value, "HELLO") == 0)
        return TYPE_HELLO;

    return TYPE_UNKNOWN;
}

// ================================================================
//                     TYPE NAME
// ================================================================

const char *typeName(
    uint8_t type
)
{
    switch (type)
    {
        case TYPE_SOS:
            return "SOS";

        case TYPE_ACK:
            return "ACK";

        case TYPE_MSG:
            return "MSG";

        case TYPE_MSGACK:
            return "MSGACK";

        case TYPE_LOCREQ:
            return "LOCREQ";

        case TYPE_LOC:
            return "LOC";

        case TYPE_HELLO:
            return "HELLO";
    }

    return "UNKNOWN";
}

// ================================================================
//                     ROUTING PARSER
// ================================================================

bool parseRoutingFields(
    const char *packet,
    uint8_t &src,
    uint8_t &dst,
    uint32_t &seq,
    uint8_t &ttl,
    uint8_t &hop
)
{
    src = (uint8_t)getFieldLong(
        packet,
        "SRC",
        0
    );

    dst = (uint8_t)getFieldLong(
        packet,
        "DST",
        0
    );

    seq = (uint32_t)getFieldLong(
        packet,
        "SEQ",
        0
    );

    ttl = (uint8_t)getFieldLong(
        packet,
        "TTL",
        0
    );

    hop = (uint8_t)getFieldLong(
        packet,
        "HOP",
        255
    );

    if (src == 0)
        return false;

    if (dst == 0)
        return false;

    if (ttl == 0)
        return false;

    if (hop > MAX_TTL)
        return false;

    return true;
}

// ================================================================
//                     FROM FIELD
// ================================================================
//
// This tells the next relay which node physically forwarded
// the packet.
//
// Original sender/receiver packets don't have FROM.
// Relay-generated packets do.
//

uint8_t getFromField(
    const char *packet
)
{
    return (uint8_t)getFieldLong(
        packet,
        "FROM",
        0
    );
}

// ================================================================
//                     NEXT FIELD
// ================================================================

uint8_t getNextField(
    const char *packet
)
{
    return (uint8_t)getFieldLong(
        packet,
        "NEXT",
        BROADCAST_ID
    );
}

// ================================================================
//                     MESSAGE ID
// ================================================================

uint16_t getMessageID(
    const char *packet
)
{
    return (uint16_t)getFieldLong(
        packet,
        "MSGID",
        0
    );
}

// ================================================================
//                     ROUTE TABLE
// ================================================================

void clearRoutes()
{
    for (
        uint8_t i = 0;
        i < ROUTE_COUNT;
        i++
    )
    {
        routes[i].valid = false;
    }
}

// ------------------------------------------------

int8_t findRoute(
    uint8_t destination
)
{
    for (
        uint8_t i = 0;
        i < ROUTE_COUNT;
        i++
    )
    {
        if (
            routes[i].valid &&
            routes[i].destination == destination
        )
        {
            return i;
        }
    }

    return -1;
}

// ------------------------------------------------

int8_t findFreeRoute()
{
    for (
        uint8_t i = 0;
        i < ROUTE_COUNT;
        i++
    )
    {
        if (!routes[i].valid)
        {
            return i;
        }
    }

    uint8_t oldest = 0;

    for (
        uint8_t i = 1;
        i < ROUTE_COUNT;
        i++
    )
    {
        if (
            routes[i].lastSeen <
            routes[oldest].lastSeen
        )
        {
            oldest = i;
        }
    }

    return oldest;
}

// ------------------------------------------------

void learnRoute(
    uint8_t destination,
    uint8_t nextHop,
    uint8_t hopCount,
    int8_t rssi
)
{
    if (
        destination == RELAY_NODE_ID ||
        nextHop == RELAY_NODE_ID
    )
    {
        return;
    }

    if (hopCount == 0)
        return;

    int8_t index =
        findRoute(destination);

    if (index < 0)
    {
        index = findFreeRoute();
    }

    routes[index].valid = true;

    routes[index].destination =
        destination;

    routes[index].nextHop =
        nextHop;

    routes[index].hopCount =
        hopCount;

    routes[index].rssi =
        rssi;

    routes[index].lastSeen =
        millis();

    Serial.print(F("[ROUTE] D"));
    Serial.print(destination);

    Serial.print(F(" via N"));
    Serial.print(nextHop);

    Serial.print(F(" hops="));
    Serial.println(hopCount);
}

// ------------------------------------------------

bool getRoute(
    uint8_t destination,
    uint8_t &nextHop
)
{
    int8_t index =
        findRoute(destination);

    if (index < 0)
    {
        return false;
    }

    if (
        millis() -
        routes[index].lastSeen >
        ROUTE_TIMEOUT_MS
    )
    {
        routes[index].valid = false;

        return false;
    }

    nextHop =
        routes[index].nextHop;

    return true;
}

// ------------------------------------------------

void expireRoutes()
{
    unsigned long now =
        millis();

    for (
        uint8_t i = 0;
        i < ROUTE_COUNT;
        i++
    )
    {
        if (
            routes[i].valid &&
            now - routes[i].lastSeen >
            ROUTE_TIMEOUT_MS
        )
        {
            routes[i].valid = false;
        }
    }
}

// ================================================================
//                     NEIGHBOURS
// ================================================================

int8_t findNeighbour(
    uint8_t node
)
{
    for (
        uint8_t i = 0;
        i < NEIGHBOUR_COUNT;
        i++
    )
    {
        if (
            neighbours[i].valid &&
            neighbours[i].node == node
        )
        {
            return i;
        }
    }

    return -1;
}

// ------------------------------------------------

void learnNeighbour(
    uint8_t node,
    int8_t rssi
)
{
    if (
        node == RELAY_NODE_ID ||
        node == 0
    )
    {
        return;
    }

    int8_t index =
        findNeighbour(node);

    if (index < 0)
    {
        for (
            uint8_t i = 0;
            i < NEIGHBOUR_COUNT;
            i++
        )
        {
            if (!neighbours[i].valid)
            {
                index = i;
                break;
            }
        }
    }

    if (index < 0)
        return;

    neighbours[index].valid = true;
    neighbours[index].node = node;
    neighbours[index].rssi = rssi;
    neighbours[index].lastSeen = millis();
}

// ------------------------------------------------

void expireNeighbours()
{
    unsigned long now =
        millis();

    for (
        uint8_t i = 0;
        i < NEIGHBOUR_COUNT;
        i++
    )
    {
        if (
            neighbours[i].valid &&
            now - neighbours[i].lastSeen >
            NEIGHBOUR_TIMEOUT_MS
        )
        {
            neighbours[i].valid = false;
        }
    }
}

// ================================================================
//                     DUPLICATE FILTER
// ================================================================

bool checkDuplicate(
    uint8_t src,
    uint32_t seq,
    uint8_t type,
    uint16_t msgID
)
{
    unsigned long now =
        millis();

    // Remove expired entries.
    for (
        uint8_t i = 0;
        i < DUPLICATE_COUNT;
        i++
    )
    {
        if (
            duplicates[i].valid &&
            now - duplicates[i].timeSeen >
            DUPLICATE_TIMEOUT_MS
        )
        {
            duplicates[i].valid = false;
        }
    }

    // Search existing packet.
    for (
        uint8_t i = 0;
        i < DUPLICATE_COUNT;
        i++
    )
    {
        if (!duplicates[i].valid)
            continue;

        if (
            duplicates[i].src == src &&
            duplicates[i].seq == seq &&
            duplicates[i].type == type &&
            duplicates[i].msgID == msgID
        )
        {
            return true;
        }
    }

    // Find free slot.
    int8_t slot = -1;

    for (
        uint8_t i = 0;
        i < DUPLICATE_COUNT;
        i++
    )
    {
        if (!duplicates[i].valid)
        {
            slot = i;
            break;
        }
    }

    // Replace oldest.
    if (slot < 0)
    {
        slot = 0;

        for (
            uint8_t i = 1;
            i < DUPLICATE_COUNT;
            i++
        )
        {
            if (
                duplicates[i].timeSeen <
                duplicates[slot].timeSeen
            )
            {
                slot = i;
            }
        }
    }

    duplicates[slot].valid = true;

    duplicates[slot].src =
        src;

    duplicates[slot].seq =
        seq;

    duplicates[slot].type =
        type;

    duplicates[slot].msgID =
        msgID;

    duplicates[slot].timeSeen =
        now;

    return false;
}

// ================================================================
//                     BUILD FORWARDED PACKET
// ================================================================

bool buildForwardPacket(
    const char *original,
    uint8_t newTTL,
    uint8_t newHop,
    uint8_t nextHop
)
{
    char src[8];
    char dst[8];
    char seq[16];
    char type[12];

    if (
        !getField(
            original,
            "SRC",
            src,
            sizeof(src)
        )
    )
        return false;

    if (
        !getField(
            original,
            "DST",
            dst,
            sizeof(dst)
        )
    )
        return false;

    if (
        !getField(
            original,
            "SEQ",
            seq,
            sizeof(seq)
        )
    )
        return false;

    if (
        !getField(
            original,
            "TYPE",
            type,
            sizeof(type)
        )
    )
        return false;

    // ------------------------------------------------
    // Basic routing header
    // ------------------------------------------------

    snprintf(
        txPacket,
        PACKET_BUFFER_SIZE,

        "SRC:%s,DST:%s,SEQ:%s,"
        "TTL:%u,HOP:%u,NEXT:%u,"
        "FROM:%u,TYPE:%s",

        src,
        dst,
        seq,
        newTTL,
        newHop,
        nextHop,
        RELAY_NODE_ID,
        type
    );

    // ------------------------------------------------
    // SOS
    // ------------------------------------------------

    if (
        strcmp(type, "SOS") == 0
    )
    {
        char lat[20];
        char lon[20];

        if (
            !getField(
                original,
                "LAT",
                lat,
                sizeof(lat)
            )
        )
        {
            return false;
        }

        if (
            !getField(
                original,
                "LON",
                lon,
                sizeof(lon)
            )
        )
        {
            return false;
        }

        snprintf(
            txPacket + strlen(txPacket),

            PACKET_BUFFER_SIZE -
            strlen(txPacket),

            ",LAT:%s,LON:%s",

            lat,
            lon
        );
    }

    // ------------------------------------------------
    // MESSAGE
    // ------------------------------------------------

    else if (
        strcmp(type, "MSG") == 0
    )
    {
        char msgID[10];
        char message[80];

        getField(
            original,
            "MSGID",
            msgID,
            sizeof(msgID)
        );

        getField(
            original,
            "MSG",
            message,
            sizeof(message)
        );

        snprintf(
            txPacket + strlen(txPacket),

            PACKET_BUFFER_SIZE -
            strlen(txPacket),

            ",MSGID:%s,MSG:%s",

            msgID,
            message
        );
    }

    // ------------------------------------------------
    // MSGACK / LOCREQ
    // ------------------------------------------------

    else if (
        strcmp(type, "MSGACK") == 0 ||
        strcmp(type, "LOCREQ") == 0
    )
    {
        char msgID[10];

        if (
            getField(
                original,
                "MSGID",
                msgID,
                sizeof(msgID)
            )
        )
        {
            snprintf(
                txPacket + strlen(txPacket),

                PACKET_BUFFER_SIZE -
                strlen(txPacket),

                ",MSGID:%s",

                msgID
            );
        }
    }

    // ------------------------------------------------
    // LOCATION
    // ------------------------------------------------

    else if (
        strcmp(type, "LOC") == 0
    )
    {
        char lat[20];
        char lon[20];

        getField(
            original,
            "LAT",
            lat,
            sizeof(lat)
        );

        getField(
            original,
            "LON",
            lon,
            sizeof(lon)
        );

        snprintf(
            txPacket + strlen(txPacket),

            PACKET_BUFFER_SIZE -
            strlen(txPacket),

            ",LAT:%s,LON:%s",

            lat,
            lon
        );
    }

    return true;
}

// ================================================================
//                     FORWARD JITTER
// ================================================================

void forwardingDelay()
{
    randomSeed(
        micros() ^
        analogRead(A0)
    );

    delay(
        random(
            FORWARD_DELAY_MIN,
            FORWARD_DELAY_MAX + 1
        )
    );
}

// ================================================================
//                     TRANSMIT
// ================================================================

bool transmit(
    const char *packet
)
{
    LoRa.idle();

    if (
        LoRa.beginPacket() != 1
    )
    {
        Serial.println(
            F("[TX] BEGIN FAILED")
        );

        LoRa.receive();

        return false;
    }

    LoRa.print(packet);

    int result =
        LoRa.endPacket();

    LoRa.receive();

    if (result == 0)
    {
        Serial.println(
            F("[TX] FAILED")
        );

        return false;
    }

    Serial.print(
        F("[TX] ")
    );

    Serial.println(packet);

    return true;
}

// ================================================================
//                     FORWARD PACKET
// ================================================================

void forwardPacket(
    const char *packet,
    uint8_t destination,
    uint8_t ttl,
    uint8_t hop,
    uint8_t incomingNode
)
{
    // ------------------------------------------------
    // TTL
    // ------------------------------------------------

    if (ttl <= 1)
    {
        Serial.println(
            F("[DROP] TTL EXHAUSTED")
        );

        ttlDropped++;
        packetsDropped++;

        return;
    }

    uint8_t newTTL =
        ttl - 1;

    uint8_t newHop =
        hop + 1;

    // ------------------------------------------------
    // Find route
    // ------------------------------------------------

    uint8_t nextHop = 0;

    bool routeKnown =
        getRoute(
            destination,
            nextHop
        );

    // ------------------------------------------------
    // Never send back to incoming node.
    // ------------------------------------------------

    if (
        routeKnown &&
        nextHop == incomingNode
    )
    {
        Serial.println(
            F("[ROUTE] Next hop is incoming node")
        );

        routeKnown = false;
    }

    // ------------------------------------------------
    // KNOWN ROUTE
    // ------------------------------------------------

    if (routeKnown)
    {
        Serial.print(
            F("[ROUTE] D")
        );

        Serial.print(destination);

        Serial.print(
            F(" via N")
        );

        Serial.println(nextHop);

        if (
            buildForwardPacket(
                packet,
                newTTL,
                newHop,
                nextHop
            )
        )
        {
            forwardingDelay();

            if (
                transmit(txPacket)
            )
            {
                packetsForwarded++;
                routeForwarded++;
            }
        }

        return;
    }

    // ------------------------------------------------
    // NO ROUTE
    //
    // Broadcast controlled flooding.
    // ------------------------------------------------

    Serial.print(
        F("[FLOOD] Destination ")
    );

    Serial.print(destination);

    Serial.println(
        F(" unknown")
    );

    if (
        buildForwardPacket(
            packet,
            newTTL,
            newHop,
            BROADCAST_ID
        )
    )
    {
        forwardingDelay();

        if (
            transmit(txPacket)
        )
        {
            packetsForwarded++;
            floodForwarded++;
        }
    }
}

// ================================================================
//                     HELLO
// ================================================================

void sendHello()
{
    snprintf(
        txPacket,
        PACKET_BUFFER_SIZE,

        "SRC:%u,DST:%u,SEQ:0,"
        "TTL:1,HOP:0,NEXT:%u,"
        "FROM:%u,TYPE:HELLO",

        RELAY_NODE_ID,
        BROADCAST_ID,
        BROADCAST_ID,
        RELAY_NODE_ID
    );

    Serial.println(
        F("[HELLO] TRANSMIT")
    );

    transmit(txPacket);
}

// ================================================================
//                     PROCESS HELLO
// ================================================================

void processHello(
    uint8_t src,
    int8_t rssi
)
{
    learnNeighbour(
        src,
        rssi
    );

    // Direct neighbour.
    learnRoute(
        src,
        src,
        1,
        rssi
    );

    Serial.print(
        F("[HELLO] Node ")
    );

    Serial.print(src);

    Serial.println(
        F(" detected")
    );
}

// ================================================================
//                     MAIN PACKET PROCESSOR
// ================================================================

void processPacket()
{
    int packetSize =
        LoRa.parsePacket();

    if (packetSize <= 0)
    {
        return;
    }

    // ------------------------------------------------
    // Read packet
    // ------------------------------------------------

    uint16_t i = 0;

    while (
        LoRa.available() &&
        i < PACKET_BUFFER_SIZE - 1
    )
    {
        rxPacket[i++] =
            (char)LoRa.read();
    }

    while (LoRa.available())
    {
        LoRa.read();
    }

    rxPacket[i] = '\0';

    packetsReceived++;

    int8_t rssi =
        LoRa.packetRssi();

    float snr =
        LoRa.packetSnr();

    Serial.println();
    Serial.println(
        F("================================")
    );

    Serial.println(
        F("[RX] PACKET")
    );

    Serial.println(rxPacket);

    Serial.print(
        F("RSSI: ")
    );

    Serial.println(rssi);

    Serial.print(
        F("SNR : ")
    );

    Serial.println(snr);

    Serial.println(
        F("================================")
    );

    // ------------------------------------------------
    // Parse routing fields
    // ------------------------------------------------

    uint8_t src;
    uint8_t dst;

    uint32_t seq;

    uint8_t ttl;
    uint8_t hop;

    if (
        !parseRoutingFields(
            rxPacket,
            src,
            dst,
            seq,
            ttl,
            hop
        )
    )
    {
        Serial.println(
            F("[DROP] INVALID ROUTING")
        );

        packetsDropped++;

        return;
    }

    // ------------------------------------------------
    // Type
    // ------------------------------------------------

    uint8_t type =
        getType(rxPacket);

    if (
        type == TYPE_UNKNOWN
    )
    {
        Serial.println(
            F("[DROP] UNKNOWN TYPE")
        );

        packetsDropped++;

        return;
    }

    // ------------------------------------------------
    // Ignore own packets
    // ------------------------------------------------

    if (
        src == RELAY_NODE_ID
    )
    {
        return;
    }

    // ------------------------------------------------
    // Determine incoming node
    //
    // If FROM exists, it tells us which relay sent
    // this packet.
    //
    // Otherwise the source itself is assumed to be
    // the immediate neighbour.
    // ------------------------------------------------

    uint8_t from =
        getFromField(rxPacket);

    uint8_t incomingNode;

    if (from != 0)
    {
        incomingNode = from;
    }
    else
    {
        incomingNode = src;
    }

    // ------------------------------------------------
    // NEXT
    // ------------------------------------------------

    uint8_t nextField =
        getNextField(rxPacket);

    // If packet specifically targets another node,
    // don't process it.
    //
    // Broadcast = 255.
    // ------------------------------------------------

    if (
        nextField != BROADCAST_ID &&
        nextField != RELAY_NODE_ID
    )
    {
        return;
    }

    // ------------------------------------------------
    // Learn physical neighbour
    // ------------------------------------------------

    learnNeighbour(
        incomingNode,
        rssi
    );

    // ------------------------------------------------
    // Reverse route learning
    //
    // Example:
    //
    // Node 1 -> Relay 2 -> Relay 3
    //
    // Relay 3 receives:
    //
    // SRC:1
    // FROM:2
    //
    // Therefore:
    //
    // Node 1 is reachable through Node 2.
    //
    // ------------------------------------------------

    if (
        incomingNode != src
    )
    {
        learnRoute(
            src,
            incomingNode,
            hop + 1,
            rssi
        );
    }
    else
    {
        // Direct neighbour.
        learnRoute(
            src,
            incomingNode,
            1,
            rssi
        );
    }

    // ------------------------------------------------
    // Duplicate check
    // ------------------------------------------------

    uint16_t msgID =
        getMessageID(rxPacket);

    if (
        checkDuplicate(
            src,
            seq,
            type,
            msgID
        )
    )
    {
        Serial.print(
            F("[DROP] DUPLICATE ")
        );

        Serial.println(
            typeName(type)
        );

        duplicateDropped++;
        packetsDropped++;

        return;
    }

    // =================================================
    // HELLO
    // =================================================

    if (
        type == TYPE_HELLO
    )
    {
        processHello(
            src,
            rssi
        );

        return;
    }

    // =================================================
    // SOS
    // =================================================

    if (
        type == TYPE_SOS
    )
    {
        Serial.println();
        Serial.println(
            F("******** SOS ********")
        );

        Serial.print(
            F("SRC : ")
        );

        Serial.println(src);

        Serial.print(
            F("DST : ")
        );

        Serial.println(dst);

        Serial.print(
            F("SEQ : ")
        );

        Serial.println(seq);

        Serial.print(
            F("TTL : ")
        );

        Serial.println(ttl);

        Serial.print(
            F("HOP : ")
        );

        Serial.println(hop);

        Serial.print(
            F("FROM: ")
        );

        Serial.println(incomingNode);

        Serial.println(
            F("*********************")
        );

        // ------------------------------------------------
        // SOS destination
        //
        // Sender initially uses DST 255.
        // Relay network routes it toward Node 4.
        // ------------------------------------------------

        uint8_t routingDestination =
            RESCUE_NODE_ID;

        forwardPacket(
            rxPacket,
            routingDestination,
            ttl,
            hop,
            incomingNode
        );

        return;
    }

    // =================================================
    // ACK / MSG / LOCREQ / LOC / MSGACK
    // =================================================

    if (
        type == TYPE_ACK ||
        type == TYPE_MSG ||
        type == TYPE_MSGACK ||
        type == TYPE_LOCREQ ||
        type == TYPE_LOC
    )
    {
        Serial.print(
            F("[ROUTED] ")
        );

        Serial.println(
            typeName(type)
        );

        Serial.print(
            F("SRC=")
        );

        Serial.print(src);

        Serial.print(
            F(" DST=")
        );

        Serial.print(dst);

        Serial.print(
            F(" SEQ=")
        );

        Serial.println(seq);

        // ------------------------------------------------
        // Packet is for this relay.
        // ------------------------------------------------

        if (
            dst == RELAY_NODE_ID
        )
        {
            Serial.println(
                F("[LOCAL] Packet belongs to relay")
            );

            return;
        }

        // ------------------------------------------------
        // Forward toward destination.
        // ------------------------------------------------

        forwardPacket(
            rxPacket,
            dst,
            ttl,
            hop,
            incomingNode
        );

        return;
    }

    // =================================================
    // UNSUPPORTED
    // =================================================

    Serial.println(
        F("[DROP] UNSUPPORTED PACKET")
    );

    packetsDropped++;
}

// ================================================================
//                     ROUTE TABLE DISPLAY
// ================================================================

void printRoutes()
{
    Serial.println(
        F("--- ROUTES ---")
    );

    bool any = false;

    for (
        uint8_t i = 0;
        i < ROUTE_COUNT;
        i++
    )
    {
        if (!routes[i].valid)
            continue;

        any = true;

        Serial.print(
            F("D")
        );

        Serial.print(
            routes[i].destination
        );

        Serial.print(
            F(" -> N")
        );

        Serial.print(
            routes[i].nextHop
        );

        Serial.print(
            F(" H=")
        );

        Serial.print(
            routes[i].hopCount
        );

        Serial.print(
            F(" RSSI=")
        );

        Serial.println(
            routes[i].rssi
        );
    }

    if (!any)
    {
        Serial.println(
            F("No routes")
        );
    }
}

// ================================================================
//                     NEIGHBOUR DISPLAY
// ================================================================

void printNeighbours()
{
    Serial.println(
        F("--- NEIGHBOURS ---")
    );

    bool any = false;

    for (
        uint8_t i = 0;
        i < NEIGHBOUR_COUNT;
        i++
    )
    {
        if (!neighbours[i].valid)
            continue;

        any = true;

        Serial.print(
            F("N")
        );

        Serial.print(
            neighbours[i].node
        );

        Serial.print(
            F(" RSSI=")
        );

        Serial.println(
            neighbours[i].rssi
        );
    }

    if (!any)
    {
        Serial.println(
            F("No neighbours")
        );
    }
}

// ================================================================
//                     STATUS
// ================================================================

void printStatus()
{
    Serial.println();
    Serial.println(
        F("================================")
    );

    Serial.println(
        F("       TERRALINK RELAY STATUS")
    );

    Serial.println(
        F("================================")
    );

    Serial.print(
        F("Relay ID       : ")
    );

    Serial.println(
        RELAY_NODE_ID
    );

    Serial.print(
        F("Packets RX     : ")
    );

    Serial.println(
        packetsReceived
    );

    Serial.print(
        F("Packets TX     : ")
    );

    Serial.println(
        packetsForwarded
    );

    Serial.print(
        F("Route TX       : ")
    );

    Serial.println(
        routeForwarded
    );

    Serial.print(
        F("Flood TX       : ")
    );

    Serial.println(
        floodForwarded
    );

    Serial.print(
        F("Dropped        : ")
    );

    Serial.println(
        packetsDropped
    );

    Serial.print(
        F("Duplicates     : ")
    );

    Serial.println(
        duplicateDropped
    );

    Serial.print(
        F("TTL drops      : ")
    );

    Serial.println(
        ttlDropped
    );

    printRoutes();

    printNeighbours();

    Serial.println(
        F("================================")
    );
}

// ================================================================
//                     SETUP
// ================================================================

void setup()
{
    Serial.begin(9600);

    delay(500);

    Serial.println();
    Serial.println();
    Serial.println(
        F("================================")
    );

    Serial.println(
        F("    TERRALINK ROUTED RELAY")
    );

    Serial.println(
        F("================================")
    );

    Serial.print(
        F("RELAY NODE ID: ")
    );

    Serial.println(
        RELAY_NODE_ID
    );

    // ------------------------------------------------
    // Clear tables
    // ------------------------------------------------

    clearRoutes();

    for (
        uint8_t i = 0;
        i < NEIGHBOUR_COUNT;
        i++
    )
    {
        neighbours[i].valid = false;
    }

    for (
        uint8_t i = 0;
        i < DUPLICATE_COUNT;
        i++
    )
    {
        duplicates[i].valid = false;
    }

    // ------------------------------------------------
    // LoRa
    // ------------------------------------------------

    LoRa.setPins(
        LORA_SS,
        LORA_RST,
        LORA_DIO0
    );

    Serial.println(
        F("[LORA] INITIALIZING...")
    );

    if (
        !LoRa.begin(
            LORA_FREQUENCY
        )
    )
    {
        Serial.println(
            F("[LORA] INIT FAILED")
        );

        while (true)
        {
            delay(1000);
        }
    }

    // ------------------------------------------------
    // Match ESP32 sender / ESP8266 receiver
    // ------------------------------------------------

    LoRa.setSyncWord(
        LORA_SYNC_WORD
    );

    LoRa.setSpreadingFactor(
        LORA_SPREADING
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
        F("[LORA] READY")
    );

    Serial.println(
        F("Frequency : 433 MHz")
    );

    Serial.println(
        F("SF        : 7")
    );

    Serial.println(
        F("BW        : 125 kHz")
    );

    Serial.println(
        F("CR        : 4/5")
    );

    Serial.println(
        F("Sync      : 0xF3")
    );

    Serial.println(
        F("CRC       : ON")
    );

    Serial.println(
        F("TX Power  : 17 dBm")
    );

    Serial.println();

    Serial.println(
        F("TERRALINK RELAY READY")
    );

    Serial.println(
        F("Waiting for packets...")
    );

    lastHello =
        millis();

    lastStatus =
        millis();
}

// ================================================================
//                     LOOP
// ================================================================

void loop()
{
    // ------------------------------------------------
    // Receive/process LoRa
    // ------------------------------------------------

    processPacket();

    // ------------------------------------------------
    // HELLO
    // ------------------------------------------------

    if (
        millis() - lastHello >=
        HELLO_INTERVAL_MS
    )
    {
        lastHello =
            millis();

        sendHello();
    }

    // ------------------------------------------------
    // Expiry
    // ------------------------------------------------

    expireRoutes();

    expireNeighbours();

    // ------------------------------------------------
    // Status every 15 sec
    // ------------------------------------------------

    if (
        millis() - lastStatus >=
        15000UL
    )
    {
        lastStatus =
            millis();

        printStatus();
    }
}