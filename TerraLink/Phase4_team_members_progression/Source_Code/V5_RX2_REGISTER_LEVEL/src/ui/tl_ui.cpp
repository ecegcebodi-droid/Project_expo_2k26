#include <Arduino.h>
#include "tl_state.h"
#include "tl_database.h"
#include "tl_command.h"
#include "tl_ui.h"

void tl_ui_home(void) {
    Serial.println(F("\n=== TERRALINK RX2 HOME ==="));
    Serial.printf("RX1: %s | Nodes: %d | Active SOS: %d\n",
                  tl_rx1_online?"ONLINE":"OFFLINE",tl_get_node_count(),tl_get_active_sos_count());
    Serial.println(F("[1] Nodes   [E] Selected info   [0] Home"));
}

void tl_ui_node_selection(void) {
    Serial.println(F("\n=== NODE SELECTION ==="));
    if(tl_selected_node_index<0 || !tl_nodes[tl_selected_node_index].valid) {
        for(int i=0;i<TL_MAX_NODES;i++) if(tl_nodes[i].valid){tl_selected_node_index=i;break;}
    }
    if(tl_selected_node_index>=0) {
        TL_NodeRecord &n=tl_nodes[tl_selected_node_index];
        Serial.printf("Selected: NODE %u | SOS=%s | RSSI=%d | HOP=%d\n",
                      n.nodeId,n.sosActive?"ACTIVE":"NO",n.rssi,n.hop);
    } else Serial.println(F("No nodes registered."));
    Serial.println(F("[2] Next [3] Previous [A] Control [B/C] Home"));
}

void tl_ui_node_control(void) {
    Serial.println(F("\n=== NODE CONTROL ==="));
    if(tl_selected_node_index>=0) {
        TL_NodeRecord &n=tl_nodes[tl_selected_node_index];
        Serial.printf("NODE %u\n",n.nodeId);
    }
    Serial.println(F("4 Stay Calm  5 Rescue  6 Do Not Move"));
    Serial.println(F("7 Location Again  8 Help Approaching  9 GPS Request"));
    Serial.println(F("F Emergency   D Resolve   E Info   B Back"));
}

void tl_ui_action_confirm(void) {
    Serial.print(F("\nCONFIRM ACTION: ")); Serial.println(tl_action_name(tl_pending_action));
    Serial.println(F("[A] SEND   [B/C] CANCEL"));
}

void tl_ui_node_info(void) {
    Serial.println(F("\n=== NODE INFO ==="));
    if(tl_selected_node_index>=0) {
        TL_NodeRecord &n=tl_nodes[tl_selected_node_index];
       Serial.printf(
    "NODE=%u SEQ=%u\n"
    "LAT=%s\n"
    "LON=%s\n"
    "HOP=%d RSSI=%d\n"
    "SOS=%d RESOLVED=%d\n",
    n.nodeId,
    (unsigned int)n.sequence,
    n.latitude.c_str(),
    n.longitude.c_str(),
    n.hop,
    n.rssi,
    n.sosActive,
    n.resolved
);
    }
    Serial.println(F("[B/C] Control"));
}

void tl_ui_resolve_confirm(void) {
    Serial.println(F("\nRESOLVE SELECTED INCIDENT? [A] YES [B/C] CANCEL"));
}

void tl_ui_sent(void) {
    Serial.println(F("\n=== COMMAND QUEUED ==="));
    Serial.println(F("Command sent to RX1. [A/B/C] Control"));
}
