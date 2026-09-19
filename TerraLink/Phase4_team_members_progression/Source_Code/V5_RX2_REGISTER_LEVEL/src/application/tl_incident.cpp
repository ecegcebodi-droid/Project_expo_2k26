#include <Arduino.h>

#include "tl_config.h"
#include "tl_state.h"
#include "tl_database.h"
#include "tl_incident.h"
#include "tl_ui.h"
#include "tl_uart.h"

static String part(const String &s,int n){
    int start=0; for(int i=0;i<n;i++){start=s.indexOf(',',start);if(start<0)return "";start++;}
    int end=s.indexOf(',',start); if(end<0)end=s.length(); return s.substring(start,end);
}

void tl_process_sos(const String &line) {
    /* SOS,node,seq,lat,lon,hop,rssi,ttl */
    unsigned int node=part(line,1).toInt(); unsigned long seq=part(line,2).toInt();
    String lat=part(line,3), lon=part(line,4); int hop=part(line,5).toInt(), rssi=part(line,6).toInt();
    tl_update_node_from_sos(node,seq,lat,lon,hop,rssi,true);
    tl_rx1_sos++; tl_rx1_online=true; tl_rx1_last_seen=millis();
    tl_ui_state=TL_UI_HOME; tl_ui_home();
}

void tl_process_location(const String &line) {
    /* LOCATION,node,seq,lat,lon,hop,rssi */
    unsigned int node=part(line,1).toInt(); unsigned long seq=part(line,2).toInt();
    String lat=part(line,3), lon=part(line,4); int hop=part(line,5).toInt(), rssi=part(line,6).toInt();
    tl_update_node_from_sos(node,seq,lat,lon,hop,rssi,false);
    tl_rx1_loc++; tl_rx1_online=true; tl_rx1_last_seen=millis();
}

void tl_process_ack(const String &line) {
    unsigned int node=part(line,1).toInt(); unsigned long seq=part(line,2).toInt();
    tl_rx1_ack++; tl_rx1_online=true; tl_rx1_last_seen=millis();
    tl_update_message_status(node,seq,"ACKNOWLEDGED");
    tl_update_incident_status(node,"ACKNOWLEDGED");
}

void tl_process_msg_ack(const String &line) {
    unsigned int node=part(line,1).toInt(); unsigned long seq=part(line,2).toInt();
    String status=part(line,3); if(!status.length()) status="ACKNOWLEDGED";
    tl_update_message_status(node,seq,status);
    tl_rx1_online=true; tl_rx1_last_seen=millis();
}

void tl_process_resolve_ack(const String &line) {
    unsigned int node=part(line,1).toInt(); unsigned long seq=part(line,2).toInt();
    int i=tl_find_node(node);
    if(i>=0){tl_nodes[i].sosActive=false;tl_nodes[i].resolved=true;tl_nodes[i].sequence=seq;}
    tl_update_incident_status(node,"RESOLVED");
    tl_rx1_online=true; tl_rx1_last_seen=millis();
}

void tl_process_rx1_status(const String &line) {
    tl_rx1_lora_rx=part(line,1).toInt(); tl_rx1_lora_tx=part(line,2).toInt();
    tl_rx1_sos=part(line,3).toInt(); tl_rx1_ack=part(line,4).toInt();
    tl_rx1_loc=part(line,5).toInt(); tl_rx1_msg=part(line,6).toInt();
    tl_rx1_online=true; tl_rx1_last_seen=millis();
}

void tl_process_tx_status(const String &line) {
    /* TX_STATUS,SENT,node,seq,message */
    String state=part(line,1); unsigned int node=part(line,2).toInt();
    unsigned long seq=part(line,3).toInt(); String msg=part(line,4);
    tl_add_message_log(node,seq,msg,state.length()?state:"SENT");
    tl_rx1_lora_tx++; tl_rx1_online=true; tl_rx1_last_seen=millis();
}

void tl_resolve_selected_node(void) {
    if(tl_selected_node_index<0) return;
    TL_NodeRecord &n=tl_nodes[tl_selected_node_index];
    tl_uart_send_command("RESOLVE",n.nodeId,"");
    n.sosActive=false; n.resolved=true;
    tl_update_incident_status(n.nodeId,"RESOLVED");
    tl_database_save_all();
    tl_pending_action=TL_ACT_NONE; tl_ui_state=TL_UI_HOME; tl_ui_home();
}
