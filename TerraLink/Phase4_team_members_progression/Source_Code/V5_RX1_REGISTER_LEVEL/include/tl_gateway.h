#ifndef TL_GATEWAY_H
#define TL_GATEWAY_H

#include <Arduino.h>
#include "tl_types.h"

void tl_gateway_init();
void tl_gateway_loop();
void tl_gateway_on_packet(const TLPacket &packet);

#endif