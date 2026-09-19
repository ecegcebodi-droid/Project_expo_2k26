#include "tl_state.h"

/*
 * ============================================================
 *                 TERRALINK STATE MANAGER
 * ============================================================
 */

static TLNodeState nodes[TL_MAX_NODES];


/* ------------------------------------------------------------
 * Duplicate packet record
 * ------------------------------------------------------------ */

struct TLSeenPacket
{
    bool used;
    uint16_t src;
    uint16_t seq;
    String type;
};

static TLSeenPacket seen[TL_MAX_SEEN_PACKETS];


/* ------------------------------------------------------------
 * Statistics
 * ------------------------------------------------------------ */

static uint32_t rxCount = 0;
static uint32_t txCount = 0;
static uint32_t ackCount = 0;


/* ------------------------------------------------------------
 * Initialize state
 * ------------------------------------------------------------ */

void tl_state_init()
{
    for (uint8_t i = 0; i < TL_MAX_NODES; i++)
    {
        nodes[i] = TLNodeState();
    }

    for (uint8_t i = 0; i < TL_MAX_SEEN_PACKETS; i++)
    {
        seen[i].used = false;
        seen[i].src = 0;
        seen[i].seq = 0;
        seen[i].type = "";
    }

    rxCount = 0;
    txCount = 0;
    ackCount = 0;
}


/* ------------------------------------------------------------
 * Find existing node
 * ------------------------------------------------------------ */

static TLNodeState *findNode(uint16_t nodeId)
{
    for (uint8_t i = 0; i < TL_MAX_NODES; i++)
    {
        if (nodes[i].active && nodes[i].nodeId == nodeId)
        {
            return &nodes[i];
        }
    }

    return nullptr;
}


/* ------------------------------------------------------------
 * Allocate new node
 * ------------------------------------------------------------ */

static TLNodeState *allocateNode(uint16_t nodeId)
{
    for (uint8_t i = 0; i < TL_MAX_NODES; i++)
    {
        if (!nodes[i].active)
        {
            nodes[i] = TLNodeState();

            nodes[i].nodeId = nodeId;
            nodes[i].active = true;

            return &nodes[i];
        }
    }

    return nullptr;
}


/* ------------------------------------------------------------
 * Update node state
 * ------------------------------------------------------------ */

TLNodeState *tl_state_update(const TLPacket &packet)
{
    if (packet.src == 0)
    {
        return nullptr;
    }

    TLNodeState *node = findNode(packet.src);

    if (node == nullptr)
    {
        node = allocateNode(packet.src);
    }

    if (node == nullptr)
    {
        return nullptr;
    }


    /* General information */

    node->active = true;
    node->lastSeen = millis();

    node->packetCount++;
    node->lastRSSI = packet.rssi;


    /* GPS */

    if (packet.hasLat && packet.hasLon)
    {
        node->lastLat = packet.lat;
        node->lastLon = packet.lon;
        node->gpsValid = true;
    }


    /* Packet-specific state */

    if (packet.type == "SOS")
    {
        node->sosCount++;
        node->sosActive = true;
    }


    if (packet.type == "MSG")
    {
        node->msgCount++;
    }


    if (packet.type == "MSGACK")
    {
        node->msgCount++;
        node->ackCount++;
    }


    if (packet.type == "LOC")
    {
        node->locCount++;
    }


    if (packet.type == "ACK")
    {
        node->ackCount++;
    }


    return node;
}


/* ------------------------------------------------------------
 * Duplicate detection
 *
 * Duplicate identity:
 *
 *     SRC + SEQ + TYPE
 *
 * This prevents the gateway from repeatedly processing the
 * same packet.
 * ------------------------------------------------------------ */

bool tl_state_is_duplicate(const TLPacket &packet)
{
    if (packet.src == 0)
    {
        return false;
    }

    for (uint8_t i = 0; i < TL_MAX_SEEN_PACKETS; i++)
    {
        if (!seen[i].used)
        {
            continue;
        }

        if (seen[i].src == packet.src &&
            seen[i].seq == packet.seq &&
            seen[i].type == packet.type)
        {
            return true;
        }
    }


    /* Find free entry */

    for (uint8_t i = 0; i < TL_MAX_SEEN_PACKETS; i++)
    {
        if (!seen[i].used)
        {
            seen[i].used = true;
            seen[i].src = packet.src;
            seen[i].seq = packet.seq;
            seen[i].type = packet.type;

            return false;
        }
    }


    /* Simple replacement when table is full */

    seen[0].used = true;
    seen[0].src = packet.src;
    seen[0].seq = packet.seq;
    seen[0].type = packet.type;

    return false;
}


/* ------------------------------------------------------------
 * Statistics
 * ------------------------------------------------------------ */

void tl_state_note_rx()
{
    rxCount++;
}


void tl_state_note_tx()
{
    txCount++;
}


void tl_state_note_ack()
{
    ackCount++;
}


uint32_t tl_state_rx_count()
{
    return rxCount;
}


uint32_t tl_state_tx_count()
{
    return txCount;
}


uint32_t tl_state_ack_count()
{
    return ackCount;
}


/* ------------------------------------------------------------
 * Get node state
 * ------------------------------------------------------------ */

const TLNodeState *tl_state_get_node(uint16_t nodeId)
{
    return findNode(nodeId);
}