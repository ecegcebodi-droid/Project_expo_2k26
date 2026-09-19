#include "tl_packet.h"

#include "tl_state.h"
#include "tl_config.h"


/*
 * ============================================================
 *                 TERRALINK PACKET PARSER
 * ============================================================
 *
 * Packet format:
 *
 * SRC:1,DST:255,SEQ:12,TTL:5,HOP:0,
 * LAT:11.234567,LON:76.123456,TYPE:SOS
 *
 * MSG:
 *
 * SRC:4,DST:1,SEQ:12,TTL:5,HOP:0,
 * MSGID:1,TYPE:MSG,MSG:STAY CALM
 * ============================================================
 */


/* ------------------------------------------------------------
 * Extract one field from comma-separated packet
 * ------------------------------------------------------------ */

static String field(
    const String &packet,
    const String &key
)
{
    String target = key + ":";

    int start = 0;
    int packetLength = packet.length();

    while (start < packetLength)
    {
        int comma = packet.indexOf(',', start);

        if (comma < 0)
        {
            comma = packetLength;
        }

        String token = packet.substring(
            start,
            comma
        );

        token.trim();

        if (token.startsWith(target))
        {
            return token.substring(
                target.length()
            );
        }

        start = comma + 1;
    }

    return "";
}


/* ------------------------------------------------------------
 * Parse LoRa packet
 * ------------------------------------------------------------ */

bool tl_packet_parse(
    const String &raw,
    TLPacket &p
)
{
    p = TLPacket();

    p.raw = raw;


    if (raw.length() == 0)
    {
        return false;
    }


    /* SRC */

    String value = field(raw, "SRC");

    if (value.length() > 0)
    {
        p.src = (uint16_t)value.toInt();
    }


    /* DST */

    value = field(raw, "DST");

    if (value.length() > 0)
    {
        p.dst = (uint16_t)value.toInt();
    }


    /* SEQ */

    value = field(raw, "SEQ");

    if (value.length() > 0)
    {
        p.seq = (uint16_t)value.toInt();
    }


    /* TTL */

    value = field(raw, "TTL");

    if (value.length() > 0)
    {
        p.ttl = (uint8_t)value.toInt();
    }


    /* HOP */

    value = field(raw, "HOP");

    if (value.length() > 0)
    {
        p.hop = (uint8_t)value.toInt();
    }


    /* TYPE */

    p.type = field(raw, "TYPE");


    /* LAT */

    value = field(raw, "LAT");

    if (value.length() > 0)
    {
        p.lat = value.toFloat();
        p.hasLat = true;
    }


    /* LON */

    value = field(raw, "LON");

    if (value.length() > 0)
    {
        p.lon = value.toFloat();
        p.hasLon = true;
    }


    /* MSGID */

    value = field(raw, "MSGID");

    if (value.length() > 0)
    {
        p.msgId = (uint16_t)value.toInt();
        p.hasMsgId = true;
    }


    /* MSG */

    p.msg = field(raw, "MSG");


    /* --------------------------------------------------------
     * Validate
     * -------------------------------------------------------- */

    p.valid =
        (p.src != 0) &&
        (p.type.length() > 0);


    return p.valid;
}


/* ------------------------------------------------------------
 * Build ACK
 * ------------------------------------------------------------ */

String tl_packet_ack(
    const TLPacket &p
)
{
    String packet;

    packet += "SRC:";
    packet += String(TL_NODE_ID);

    packet += ",DST:";
    packet += String(p.src);

    packet += ",SEQ:";
    packet += String(p.seq);

    packet += ",TTL:";
    packet += String(TL_DEFAULT_TTL);

    packet += ",HOP:0";

    packet += ",TYPE:ACK";


    return packet;
}


/* ------------------------------------------------------------
 * Build MSGACK
 * ------------------------------------------------------------ */

String tl_packet_msgack(
    const TLPacket &p
)
{
    String packet;

    packet += "SRC:";
    packet += String(TL_NODE_ID);

    packet += ",DST:";
    packet += String(p.src);

    packet += ",SEQ:";
    packet += String(p.seq);

    packet += ",TTL:";
    packet += String(TL_DEFAULT_TTL);

    packet += ",HOP:0";

    if (p.hasMsgId)
    {
        packet += ",MSGID:";
        packet += String(p.msgId);
    }

    packet += ",TYPE:MSGACK";


    return packet;
}


/* ------------------------------------------------------------
 * Gateway status
 * ------------------------------------------------------------ */

String tl_packet_status()
{
    String packet;

    packet += "RX1,STATUS";

    packet += ",NODE:";
    packet += String(TL_NODE_ID);

    packet += ",RX:";
    packet += String(tl_state_rx_count());

    packet += ",TX:";
    packet += String(tl_state_tx_count());

    packet += ",ACK:";
    packet += String(tl_state_ack_count());


    return packet;
}