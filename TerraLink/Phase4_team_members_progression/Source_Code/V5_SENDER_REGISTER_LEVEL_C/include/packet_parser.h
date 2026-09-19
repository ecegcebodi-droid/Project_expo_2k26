#ifndef PACKET_PARSER_H
#define PACKET_PARSER_H
#include "terralink_types.h"
#include <stdbool.h>
bool packet_parse(const char *text, tl_packet_t *packet);
#endif
