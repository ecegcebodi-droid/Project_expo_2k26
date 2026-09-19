#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "tl_config.h"
#include "tl_state.h"
#include "tl_database.h"
#include "tl_uart.h"
#include "tl_dashboard.h"
#include "tl_web.h"

static ESP8266WebServer server(80);

static void root(void){ server.send(200,"text/html",tl_dashboard_html()); }

static void api_status(void){
    String j="{";
    j+="\"receiver\":2,\"rx1_online\":"+(String)(tl_rx1_online?"true":"false");
    j+=",\"nodes\":"+String(tl_get_node_count());
    j+=",\"active_sos\":"+String(tl_get_active_sos_count());
    j+=",\"rx\":"+String(tl_rx1_lora_rx)+",\"tx\":"+String(tl_rx1_lora_tx);
    j+=",\"sos_packets\":"+String(tl_rx1_sos)+",\"ack_packets\":"+String(tl_rx1_ack);
    j+=",\"loc_packets\":"+String(tl_rx1_loc)+",\"msg_packets\":"+String(tl_rx1_msg)+"}";
    server.send(200,"application/json",j);
}
static void api_nodes(void){
    String j="[";
    bool first=true;
    for(int i=0;i<TL_MAX_NODES;i++) if(tl_nodes[i].valid){
        if(!first)j+=",";
        first=false;
        j+="{\"node\":"+String(tl_nodes[i].nodeId)+",\"seq\":"+String(tl_nodes[i].sequence);
        j+=",\"lat\":\""+tl_nodes[i].latitude+"\",\"lon\":\""+tl_nodes[i].longitude+"\"";
        j+=",\"hop\":"+String(tl_nodes[i].hop)+",\"rssi\":"+String(tl_nodes[i].rssi);
        j+=",\"sos\":"+(String)(tl_nodes[i].sosActive?"true":"false")+"}";
    }
    j+="]"; server.send(200,"application/json",j);
}
static void api_messages(void){
    String out;
    for(int i=0;i<TL_MAX_MESSAGES;i++) if(tl_messages[i].valid){
        out += "NODE "+String(tl_messages[i].nodeId)+" | SEQ "+String(tl_messages[i].sequence)+
               " | "+tl_messages[i].status+" | "+tl_messages[i].message+"\n";
    }
    server.send(200,"text/plain",out);
}
static void command(void){
    if(!server.hasArg("type")||!server.hasArg("node")){server.send(400,"text/plain","missing type/node");return;}
    String type=server.arg("type"), node=server.arg("node"), msg=server.arg("msg");
    tl_uart_send_command(type.c_str(),node.toInt(),msg.c_str());
    if(type=="MSG"||type=="EMERGENCY") tl_add_message_log(node.toInt(),millis(),msg.length()?msg:type,"QUEUED");
    tl_database_save_all();
    server.send(200,"text/plain","OK");
}
static void resolve_cmd(void){
    if(!server.hasArg("node")){server.send(400,"text/plain","missing node");return;}
    unsigned int node=server.arg("node").toInt();
    int i=tl_find_node(node);
    if(i<0){server.send(404,"text/plain","node not found");return;}
    tl_uart_send_command("RESOLVE",node,"");
    tl_nodes[i].sosActive=false; tl_nodes[i].resolved=true;
    tl_update_incident_status(node,"RESOLVED"); tl_database_save_all();
    server.send(200,"text/plain","OK");
}
static void clear_cmd(void){tl_database_clear();server.send(200,"text/plain","CLEARED");}

void tl_web_init(void){
    WiFi.mode(WIFI_AP);
    WiFi.softAP(TL_AP_SSID,TL_AP_PASSWORD);
    Serial.print(F("[AP] ")); Serial.println(TL_AP_SSID);
    Serial.print(F("[IP] ")); Serial.println(WiFi.softAPIP());

    server.on("/",HTTP_GET,root);
    server.on("/api/status",HTTP_GET,api_status);
    server.on("/api/nodes",HTTP_GET,api_nodes);
    server.on("/api/messages",HTTP_GET,api_messages);
    server.on("/cmd",HTTP_GET,command);
    server.on("/resolve",HTTP_GET,resolve_cmd);
    server.on("/clear",HTTP_GET,clear_cmd);
    server.begin();
}
void tl_web_process(void){server.handleClient();}
