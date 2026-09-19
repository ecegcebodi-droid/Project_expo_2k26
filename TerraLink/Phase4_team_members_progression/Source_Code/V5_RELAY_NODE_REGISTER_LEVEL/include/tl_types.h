#ifndef TL_TYPES_H
#define TL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "tl_config.h"

typedef enum {
    TL_TYPE_UNKNOWN = 0,
    TL_TYPE_SOS,
    TL_TYPE_ACK,
    TL_TYPE_MSG,
    TL_TYPE_MSGACK,
    TL_TYPE_LOCREQ,
    TL_TYPE_LOC,
    TL_TYPE_HELLO
} TLPacketType;

typedef struct {
    uint8_t src;
    uint8_t dst;
    uint32_t seq;
    uint8_t ttl;
    uint8_t hop;
    uint8_t from;
    uint8_t next;
    uint16_t msg_id;
    bool has_msg_id;
    char lat[20];
    char lon[20];
    char msg[81];
    TLPacketType type;
    int8_t rssi;
    int8_t snr;
    bool valid;
    char raw[TL_PACKET_BUFFER_SIZE];
} TLPacket;

typedef struct {
    uint8_t destination;
    uint8_t next_hop;
    uint8_t hop_count;
    int8_t rssi;
    unsigned long last_seen;
    bool valid;
} TLRouteEntry;

typedef struct {
    uint8_t node;
    int8_t rssi;
    unsigned long last_seen;
    bool valid;
} TLNeighbourEntry;

typedef struct {
    uint8_t src;
    uint32_t seq;
    uint16_t msg_id;
    uint8_t type;
    unsigned long time_seen;
    bool valid;
} TLDuplicateEntry;

typedef struct {
    unsigned long packets_received;
    unsigned long packets_forwarded;
    unsigned long route_forwarded;
    unsigned long flood_forwarded;
    unsigned long packets_dropped;
    unsigned long duplicate_dropped;
    unsigned long ttl_dropped;
} TLStats;

#endif
