#ifndef TL_LORA_H
#define TL_LORA_H

#include <Arduino.h>
#include "tl_types.h"

bool tl_lora_init();
bool tl_lora_receive(TLPacket &packet);
bool tl_lora_send(const String &raw);

#endif