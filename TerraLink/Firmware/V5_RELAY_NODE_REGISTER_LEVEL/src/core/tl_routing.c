#include <stdio.h>
#include <string.h>
#include "tl_routing.h"
#include "tl_config.h"
#include "tl_time.h"
#include "tl_console.h"
static TLRouteEntry routes[TL_ROUTE_COUNT];
static TLNeighbourEntry neighbours[TL_NEIGHBOUR_COUNT];
static TLDuplicateEntry duplicates[TL_DUPLICATE_COUNT];
static TLStats stats;
void tl_routing_init(void){memset(routes,0,sizeof(routes));memset(neighbours,0,sizeof(neighbours));memset(duplicates,0,sizeof(duplicates));memset(&stats,0,sizeof(stats));}
static int8_t route_find(uint8_t d){for(uint8_t i=0;i<TL_ROUTE_COUNT;i++)if(routes[i].valid&&routes[i].destination==d)return(int8_t)i;return-1;}
void tl_routing_learn_route(uint8_t d,uint8_t nh,uint8_t hops,int8_t rssi){if(d==TL_RELAY_NODE_ID||nh==TL_RELAY_NODE_ID||hops==0)return;int8_t i=route_find(d);if(i<0){for(uint8_t k=0;k<TL_ROUTE_COUNT;k++)if(!routes[k].valid){i=(int8_t)k;break;}}if(i<0){i=0;for(uint8_t k=1;k<TL_ROUTE_COUNT;k++)if(routes[k].last_seen<routes[(uint8_t)i].last_seen)i=(int8_t)k;}routes[(uint8_t)i]=(TLRouteEntry){d,nh,hops,rssi,millis(),true};}
bool tl_routing_get_route(uint8_t d,uint8_t*nh){int8_t i=route_find(d);if(i<0)return false;if(millis()-routes[(uint8_t)i].last_seen>TL_ROUTE_TIMEOUT_MS){routes[(uint8_t)i].valid=false;return false;}*nh=routes[(uint8_t)i].next_hop;return true;}
static int8_t neigh_find(uint8_t node){for(uint8_t i=0;i<TL_NEIGHBOUR_COUNT;i++)if(neighbours[i].valid&&neighbours[i].node==node)return(int8_t)i;return-1;}
void tl_routing_learn_neighbour(uint8_t node,int8_t rssi){if(node==0||node==TL_RELAY_NODE_ID)return;int8_t i=neigh_find(node);if(i<0)for(uint8_t k=0;k<TL_NEIGHBOUR_COUNT;k++)if(!neighbours[k].valid){i=(int8_t)k;break;}if(i<0)return;neighbours[(uint8_t)i]=(TLNeighbourEntry){node,rssi,millis(),true};}
void tl_routing_expire(void){unsigned long now=millis();for(uint8_t i=0;i<TL_ROUTE_COUNT;i++)if(routes[i].valid&&now-routes[i].last_seen>TL_ROUTE_TIMEOUT_MS)routes[i].valid=false;for(uint8_t i=0;i<TL_NEIGHBOUR_COUNT;i++)if(neighbours[i].valid&&now-neighbours[i].last_seen>TL_NEIGHBOUR_TIMEOUT_MS)neighbours[i].valid=false;for(uint8_t i=0;i<TL_DUPLICATE_COUNT;i++)if(duplicates[i].valid&&now-duplicates[i].time_seen>TL_DUPLICATE_TIMEOUT_MS)duplicates[i].valid=false;}
bool tl_routing_is_duplicate(uint8_t src,uint32_t seq,uint8_t type,uint16_t id){tl_routing_expire();for(uint8_t i=0;i<TL_DUPLICATE_COUNT;i++)if(duplicates[i].valid&&duplicates[i].src==src&&duplicates[i].seq==seq&&duplicates[i].type==type&&duplicates[i].msg_id==id)return true;int8_t slot=-1;for(uint8_t i=0;i<TL_DUPLICATE_COUNT;i++)if(!duplicates[i].valid){slot=(int8_t)i;break;}if(slot<0){slot=0;for(uint8_t i=1;i<TL_DUPLICATE_COUNT;i++)if(duplicates[i].time_seen<duplicates[(uint8_t)slot].time_seen)slot=(int8_t)i;}duplicates[(uint8_t)slot]=(TLDuplicateEntry){src,seq,id,type,millis(),true};return false;}
void tl_routing_get_stats(TLStats*s){*s=stats;}
void tl_routing_inc_rx(void){stats.packets_received++;}
void tl_routing_inc_drop(void){stats.packets_dropped++;}
void tl_routing_inc_dup(void){stats.duplicate_dropped++;stats.packets_dropped++;}
void tl_routing_inc_ttl(void){stats.ttl_dropped++;stats.packets_dropped++;}
void tl_routing_inc_tx(bool route){stats.packets_forwarded++;if(route)stats.route_forwarded++;else stats.flood_forwarded++;}
void tl_routing_inc_flood(void){stats.flood_forwarded++;}
void tl_routing_print_tables(void){char b[80];tl_console_println("--- ROUTES ---");bool any=false;for(uint8_t i=0;i<TL_ROUTE_COUNT;i++)if(routes[i].valid){any=true;snprintf(b,sizeof(b),"D%u -> N%u H=%u RSSI=%d",routes[i].destination,routes[i].next_hop,routes[i].hop_count,routes[i].rssi);tl_console_println(b);}if(!any)tl_console_println("No routes");tl_console_println("--- NEIGHBOURS ---");any=false;for(uint8_t i=0;i<TL_NEIGHBOUR_COUNT;i++)if(neighbours[i].valid){any=true;snprintf(b,sizeof(b),"N%u RSSI=%d",neighbours[i].node,neighbours[i].rssi);tl_console_println(b);}if(!any)tl_console_println("No neighbours");}
