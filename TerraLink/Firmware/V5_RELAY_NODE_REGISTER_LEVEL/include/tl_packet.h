#ifndef TL_PACKET_H
#define TL_PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include "tl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool tl_packet_parse(const char *raw, TLPacket *packet);
const char *tl_packet_type_name(TLPacketType type);
uint8_t tl_packet_field_u8(const char *raw, const char *name, uint8_t fallback);
uint16_t tl_packet_field_u16(const char *raw, const char *name, uint16_t fallback);
uint32_t tl_packet_field_u32(const char *raw, const char *name, uint32_t fallback);
uint8_t tl_packet_get_from(const char *raw);
uint8_t tl_packet_get_next(const char *raw);

bool tl_packet_build_forward(const char *original,
                             char *output,
                             uint16_t output_size,
                             uint8_t new_ttl,
                             uint8_t new_hop,
                             uint8_t next_hop);

#ifdef __cplusplus
}
#endif

#endif
