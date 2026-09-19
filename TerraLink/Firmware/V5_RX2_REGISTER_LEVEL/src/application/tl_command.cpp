#include <Arduino.h>
#include "tl_state.h"
#include "tl_database.h"
#include "tl_uart.h"
#include "tl_command.h"
#include "tl_ui.h"

const char *tl_action_name(int a) {
    switch(a) {
        case TL_ACT_STAY_CALM:return "STAY CALM";
        case TL_ACT_RESCUE:return "RESCUE TEAM";
        case TL_ACT_DO_NOT_MOVE:return "DO NOT MOVE";
        case TL_ACT_LOCATION_AGAIN:return "LOCATION AGAIN";
        case TL_ACT_HELP_APPROACHING:return "HELP APPROACHING";
        case TL_ACT_GPS_REQUEST:return "GPS REQUEST";
        case TL_ACT_EMERGENCY:return "EMERGENCY";
        case TL_ACT_RESOLVE:return "RESOLVE";
        default:return "NONE";
    }
}

void tl_execute_pending_action(void) {
    if(tl_selected_node_index<0) return;
    TL_NodeRecord &n=tl_nodes[tl_selected_node_index];
    unsigned long seq=n.sequence?n.sequence:millis();

    switch(tl_pending_action) {
        case TL_ACT_STAY_CALM: tl_uart_send_command("MSG",n.nodeId,"STAY CALM"); tl_add_message_log(n.nodeId,seq,"STAY CALM","QUEUED"); break;
        case TL_ACT_RESCUE: tl_uart_send_command("MSG",n.nodeId,"RESCUE TEAM IS ON THE WAY"); tl_add_message_log(n.nodeId,seq,"RESCUE TEAM IS ON THE WAY","QUEUED"); break;
        case TL_ACT_DO_NOT_MOVE: tl_uart_send_command("MSG",n.nodeId,"DO NOT MOVE"); tl_add_message_log(n.nodeId,seq,"DO NOT MOVE","QUEUED"); break;
        case TL_ACT_LOCATION_AGAIN: tl_uart_send_command("LOC_AGAIN",n.nodeId,""); break;
        case TL_ACT_HELP_APPROACHING: tl_uart_send_command("MSG",n.nodeId,"HELP IS APPROACHING"); tl_add_message_log(n.nodeId,seq,"HELP IS APPROACHING","QUEUED"); break;
        case TL_ACT_GPS_REQUEST: tl_uart_send_command("GPS_REQUEST",n.nodeId,""); break;
        case TL_ACT_EMERGENCY: tl_uart_send_command("EMERGENCY",n.nodeId,""); tl_add_message_log(n.nodeId,seq,"EMERGENCY","QUEUED"); break;
        default:return;
    }
    tl_database_save_all();
    tl_ui_state=TL_UI_SENT_STATUS; tl_ui_sent();
}
