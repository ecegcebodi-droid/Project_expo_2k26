#include "packet_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void copy_field(const char *src, const char *key,
                       char *dst, size_t dst_size)
{
    const char *p = strstr(src, key);
    if (!p) { if (dst_size) dst[0] = 0; return; }

    p += strlen(key);
    size_t i = 0;
    while (*p && *p != ',' && i + 1 < dst_size)
        dst[i++] = *p++;

    dst[i] = 0;
}

bool packet_parse(const char *text, tl_packet_t *packet)
{
    if (!text || !packet) return false;

    memset(packet, 0, sizeof(*packet));

    char field[128];

    copy_field(text, "SRC:", field, sizeof(field));
    packet->src = (uint8_t)atoi(field);

    copy_field(text, "DST:", field, sizeof(field));
    packet->dst = (uint8_t)atoi(field);

    copy_field(text, "SEQ:", field, sizeof(field));
    packet->sequence = (uint32_t)strtoul(field, NULL, 10);

    copy_field(text, "TTL:", field, sizeof(field));
    packet->ttl = (uint8_t)atoi(field);

    copy_field(text, "HOP:", field, sizeof(field));
    packet->hop = (uint8_t)atoi(field);

    copy_field(text, "TYPE:", packet->type, sizeof(packet->type));
    copy_field(text, "MSGID:", field, sizeof(field));
    packet->msg_id = (uint8_t)atoi(field);

    copy_field(text, "MSG:", packet->message, sizeof(packet->message));

    copy_field(text, "LAT:", field, sizeof(field));
    packet->latitude = strtod(field, NULL);

    copy_field(text, "LON:", field, sizeof(field));
    packet->longitude = strtod(field, NULL);

    return true;
}
