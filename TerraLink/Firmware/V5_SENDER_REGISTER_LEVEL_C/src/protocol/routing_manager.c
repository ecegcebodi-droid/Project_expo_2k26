#include "routing_manager.h"
#include "terralink_config.h"

void routing_init(void)
{
    /* Sender does not maintain a routing table. */
}

uint8_t routing_get_next_hop(uint8_t destination)
{
    (void)destination;
    return TERRALINK_BROADCAST_ID;
}
