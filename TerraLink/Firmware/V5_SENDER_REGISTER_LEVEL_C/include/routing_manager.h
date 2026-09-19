#ifndef ROUTING_MANAGER_H
#define ROUTING_MANAGER_H
#include <stdint.h>
void routing_init(void);
uint8_t routing_get_next_hop(uint8_t destination);
#endif
