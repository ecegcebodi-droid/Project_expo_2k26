#include "tl_config.h"
#include "tl_state.h"
TL_NodeRecord tl_nodes[TL_MAX_NODES];
TL_IncidentRecord tl_incidents[TL_MAX_INCIDENTS];
TL_MessageRecord tl_messages[TL_MAX_MESSAGES];

int tl_selected_node_index = -1;
TL_UIState tl_ui_state = TL_UI_HOME;
TL_ActionType tl_pending_action = TL_ACT_NONE;

bool tl_rx1_online = false;
unsigned long tl_rx1_last_seen = 0;
unsigned long tl_rx1_lora_rx = 0;
unsigned long tl_rx1_lora_tx = 0;
unsigned long tl_rx1_sos = 0;
unsigned long tl_rx1_ack = 0;
unsigned long tl_rx1_loc = 0;
unsigned long tl_rx1_msg = 0;

String tl_web_message = "System Ready";
unsigned long tl_web_message_time = 0;

void tl_state_init(void) {
    tl_selected_node_index = -1;
    tl_ui_state = TL_UI_HOME;
    tl_pending_action = TL_ACT_NONE;
    tl_rx1_online = false;
    tl_rx1_last_seen = 0;
}

void tl_state_process(void) {
    if (tl_rx1_online &&
        (millis() - tl_rx1_last_seen > TL_RX1_TIMEOUT_MS)) {
        tl_rx1_online = false;
    }
}
