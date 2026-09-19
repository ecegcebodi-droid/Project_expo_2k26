#ifndef PACKET_PROTOCOL_H
#define PACKET_PROTOCOL_H
#include "terralink_types.h"
#include <stddef.h>
bool packet_create_sos(char *out, size_t out_size, const tl_packet_t *p);
bool packet_create_msgack(char *out, size_t out_size, const tl_packet_t *p);
bool packet_has_type(const char *packet, const char *type);
#endif
