#ifndef TL_ROUTING_H
#define TL_ROUTING_H

#include <stdint.h>
#include <stdbool.h>
#include "tl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void tl_routing_init(void);
void tl_routing_expire(void);
void tl_routing_learn_neighbour(uint8_t node, int8_t rssi);
void tl_routing_learn_route(uint8_t destination, uint8_t next_hop, uint8_t hop_count, int8_t rssi);
bool tl_routing_get_route(uint8_t destination, uint8_t *next_hop);
bool tl_routing_is_duplicate(uint8_t src, uint32_t seq, uint8_t type, uint16_t msg_id);
void tl_routing_get_stats(TLStats *stats);
void tl_routing_inc_rx(void);
void tl_routing_inc_drop(void);
void tl_routing_inc_dup(void);
void tl_routing_inc_ttl(void);
void tl_routing_inc_tx(bool route);
void tl_routing_inc_flood(void);
void tl_routing_print_tables(void);

#ifdef __cplusplus
}
#endif

#endif
