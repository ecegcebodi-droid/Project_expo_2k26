#ifndef TL_PACKET_H
#define TL_PACKET_H

#include <Arduino.h>

#include "tl_types.h"

/*
 * ============================================================
 *                  TERRALINK PACKET API
 * ============================================================
 */

bool tl_packet_parse(
    const String &raw,
    TLPacket &packet
);

String tl_packet_ack(
    const TLPacket &packet
);

String tl_packet_msgack(
    const TLPacket &packet
);

String tl_packet_status();

#endif