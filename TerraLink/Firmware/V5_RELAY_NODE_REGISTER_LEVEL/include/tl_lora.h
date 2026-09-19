#ifndef TL_LORA_H
#define TL_LORA_H

#include <stdint.h>
#include <stdbool.h>
#include "tl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool tl_lora_init(void);
bool tl_lora_send(const char *packet);
bool tl_lora_receive(TLPacket *packet);

#ifdef __cplusplus
}
#endif

#endif
