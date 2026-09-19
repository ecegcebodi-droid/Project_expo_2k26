#ifndef TL_STATE_H
#define TL_STATE_H

#include "tl_config.h"
#include "tl_types.h"

extern TL_NodeRecord tl_nodes[TL_MAX_NODES];
extern TL_IncidentRecord tl_incidents[TL_MAX_INCIDENTS];
extern TL_MessageRecord tl_messages[TL_MAX_MESSAGES];

extern int tl_selected_node_index;
extern TL_UIState tl_ui_state;
extern TL_ActionType tl_pending_action;

extern bool tl_rx1_online;
extern unsigned long tl_rx1_last_seen;

extern unsigned long tl_rx1_lora_rx;
extern unsigned long tl_rx1_lora_tx;
extern unsigned long tl_rx1_sos;
extern unsigned long tl_rx1_ack;
extern unsigned long tl_rx1_loc;
extern unsigned long tl_rx1_msg;

extern String tl_web_message;
extern unsigned long tl_web_message_time;

void tl_state_init(void);
void tl_state_process(void);

#endif