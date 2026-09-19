#ifndef TL_STATE_H
#define TL_STATE_H

#include <Arduino.h>

#include "tl_types.h"
#include "tl_config.h"

/*
 * ============================================================
 *                 TERRALINK STATE MANAGER
 * ============================================================
 */

void tl_state_init();

TLNodeState *tl_state_update(const TLPacket &packet);

bool tl_state_is_duplicate(const TLPacket &packet);

void tl_state_note_rx();
void tl_state_note_tx();
void tl_state_note_ack();

uint32_t tl_state_rx_count();
uint32_t tl_state_tx_count();
uint32_t tl_state_ack_count();

const TLNodeState *tl_state_get_node(uint16_t nodeId);

#endif