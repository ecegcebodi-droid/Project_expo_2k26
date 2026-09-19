#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tl_relay.h"
#include "tl_config.h"
#include "tl_lora.h"
#include "tl_packet.h"
#include "tl_routing.h"
#include "tl_time.h"
#include "tl_console.h"
static char tx[TL_PACKET_BUFFER_SIZE];
static unsigned long last_hello,last_status;
static uint8_t msg_type(const TLPacket*p){return(uint8_t)p->type;}
static void forwarding_delay(void)
{
    unsigned long seed = millis();
    uint16_t span = (uint16_t)(TL_FORWARD_DELAY_MAX - TL_FORWARD_DELAY_MIN + 1U);
    uint16_t wait_ms;

    seed ^= ((unsigned long)rand() << 8);
    srand((unsigned int)seed);

    wait_ms = (uint16_t)(TL_FORWARD_DELAY_MIN + (uint16_t)(rand() % span));
    delay((unsigned long)wait_ms);
}
static bool transmit(const char*p){bool ok=tl_lora_send(p);if(ok)tl_console_line("[TX]",p);else tl_console_println("[TX] FAILED");return ok;}
static void forward_packet(const char*raw,uint8_t destination,uint8_t ttl,uint8_t hop,uint8_t incoming){if(ttl<=1U){tl_console_println("[DROP] TTL EXHAUSTED");tl_routing_inc_ttl();return;}uint8_t next=TL_BROADCAST_ID;bool known=tl_routing_get_route(destination,&next);if(known&&next==incoming)known=false;if(!known)next=TL_BROADCAST_ID;if(!tl_packet_build_forward(raw,tx,sizeof(tx),(uint8_t)(ttl-1U),(uint8_t)(hop+1U),next)){tl_routing_inc_drop();return;}forwarding_delay();if(transmit(tx))tl_routing_inc_tx(known);}
static void send_hello(void){snprintf(tx,sizeof(tx),"SRC:%u,DST:%u,SEQ:0,TTL:1,HOP:0,NEXT:%u,FROM:%u,TYPE:HELLO",TL_RELAY_NODE_ID,TL_BROADCAST_ID,TL_BROADCAST_ID,TL_RELAY_NODE_ID);(void)transmit(tx);}
static void process_packet(void){TLPacket p;if(!tl_lora_receive(&p))return;tl_routing_inc_rx();{char b[96];snprintf(b,sizeof(b),"RX SRC=%u DST=%u TYPE=%s RSSI=%d",p.src,p.dst,tl_packet_type_name(p.type),p.rssi);tl_console_println(b);}if(!p.valid){tl_routing_inc_drop();return;}if(p.src==TL_RELAY_NODE_ID)return;uint8_t incoming=p.from?p.from:p.src;uint8_t next=p.next;if(next!=TL_BROADCAST_ID&&next!=TL_RELAY_NODE_ID)return;tl_routing_learn_neighbour(incoming,p.rssi);tl_routing_learn_route(p.src,incoming,(p.hop==0U)?1U:(uint8_t)(p.hop+1U),p.rssi);if(tl_routing_is_duplicate(p.src,p.seq,msg_type(&p),p.has_msg_id?p.msg_id:0U)){tl_routing_inc_dup();tl_console_println("[DROP] DUPLICATE");return;}if(p.type==TL_TYPE_HELLO)return;if(p.type==TL_TYPE_SOS){forward_packet(p.raw,TL_RESCUE_NODE_ID,p.ttl,p.hop,incoming);return;}if(p.type==TL_TYPE_ACK||p.type==TL_TYPE_MSG||p.type==TL_TYPE_MSGACK||p.type==TL_TYPE_LOCREQ||p.type==TL_TYPE_LOC){if(p.dst==TL_RELAY_NODE_ID)return;forward_packet(p.raw,p.dst,p.ttl,p.hop,incoming);return;}tl_routing_inc_drop();}
void tl_relay_init(void){tl_routing_init();if(tl_lora_init())tl_console_println("[LORA] READY");else{tl_console_println("[LORA] INIT FAILED");while(1)delay(1000);}last_hello=millis();last_status=millis();tl_console_println("TERRALINK REGISTER-LEVEL RELAY READY");}
void tl_relay_process(void){process_packet();}
void tl_relay_periodic(void){unsigned long now=millis();if(now-last_hello>=TL_HELLO_INTERVAL_MS){last_hello=now;send_hello();}tl_routing_expire();if(now-last_status>=TL_STATUS_INTERVAL_MS){TLStats s;last_status=now;tl_routing_get_stats(&s);char b[100];snprintf(b,sizeof(b),"STATUS RX=%lu TX=%lu ROUTE=%lu FLOOD=%lu DROP=%lu DUP=%lu TTL=%lu",s.packets_received,s.packets_forwarded,s.route_forwarded,s.flood_forwarded,s.packets_dropped,s.duplicate_dropped,s.ttl_dropped);tl_console_println(b);tl_routing_print_tables();}}
