#include "packet_protocol.h"
#include <stdio.h>
#include <string.h>

bool packet_create_sos(char *out, size_t out_size, const tl_packet_t *p)
{
    if (!out || !p) return false;

    int n = snprintf(
        out, out_size,
        "SRC:%u,DST:%u,SEQ:%lu,TTL:%u,HOP:0,LAT:%.6f,LON:%.6f,TYPE:SOS",
        p->src, p->dst, (unsigned long)p->sequence, p->ttl,
        p->latitude, p->longitude
    );

    return n > 0 && (size_t)n < out_size;
}

bool packet_create_msgack(char *out, size_t out_size, const tl_packet_t *p)
{
    if (!out || !p) return false;

    int n = snprintf(
        out, out_size,
        "SRC:%u,DST:%u,SEQ:%lu,TTL:%u,HOP:0,MSGID:%u,STATUS:DELIVERED,TYPE:MSGACK",
        p->src, p->dst, (unsigned long)p->sequence, p->ttl, p->msg_id
    );

    return n > 0 && (size_t)n < out_size;
}

bool packet_has_type(const char *packet, const char *type)
{
    if (!packet || !type) return false;

    char token[24];
    snprintf(token, sizeof(token), "TYPE:%s", type);
    return strstr(packet, token) != NULL;
}
