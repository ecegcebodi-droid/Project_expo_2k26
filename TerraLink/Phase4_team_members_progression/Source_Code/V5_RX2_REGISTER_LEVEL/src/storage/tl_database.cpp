#include <Arduino.h>
#include <LittleFS.h>
#include "tl_config.h"
#include "tl_state.h"
#include "tl_database.h"

static String incident_id(unsigned int nodeId, unsigned long seq) {
    return "TL-" + String(nodeId) + "-" + String(seq);
}

void tl_database_init(void) {
    for (int i=0;i<TL_MAX_NODES;i++) {
        tl_nodes[i] = TL_NodeRecord{};
    }
    for (int i=0;i<TL_MAX_INCIDENTS;i++) {
        tl_incidents[i] = TL_IncidentRecord{};
    }
    for (int i=0;i<TL_MAX_MESSAGES;i++) {
        tl_messages[i] = TL_MessageRecord{};
    }
}

int tl_find_node(unsigned int nodeId) {
    for (int i=0;i<TL_MAX_NODES;i++)
        if (tl_nodes[i].valid && tl_nodes[i].nodeId == nodeId) return i;
    return -1;
}

int tl_allocate_node_slot(void) {
    for (int i=0;i<TL_MAX_NODES;i++) if (!tl_nodes[i].valid) return i;
    unsigned long oldest=0xFFFFFFFFUL; int slot=0;
    for (int i=0;i<TL_MAX_NODES;i++) if (tl_nodes[i].lastSeen < oldest) { oldest=tl_nodes[i].lastSeen; slot=i; }
    return slot;
}

int tl_get_node_count(void) {
    int n=0; for (int i=0;i<TL_MAX_NODES;i++) if (tl_nodes[i].valid) n++; return n;
}

int tl_get_active_sos_count(void) {
    int n=0; for (int i=0;i<TL_MAX_NODES;i++) if (tl_nodes[i].valid && tl_nodes[i].sosActive && !tl_nodes[i].resolved) n++; return n;
}

void tl_create_incident(unsigned int nodeId, unsigned long seq,
                        const String &lat, const String &lon, int hop, int rssi) {
    for (int i=0;i<TL_MAX_INCIDENTS;i++) {
        if (tl_incidents[i].valid && tl_incidents[i].nodeId==nodeId && tl_incidents[i].sequence==seq) {
            tl_incidents[i].updatedAt=millis(); return;
        }
    }
    int slot=-1;
    for (int i=0;i<TL_MAX_INCIDENTS;i++) if (!tl_incidents[i].valid) {slot=i;break;}
    if (slot<0) {
        unsigned long oldest=0xFFFFFFFFUL; slot=0;
        for (int i=0;i<TL_MAX_INCIDENTS;i++) if (tl_incidents[i].createdAt<oldest){oldest=tl_incidents[i].createdAt;slot=i;}
    }
    TL_IncidentRecord &x=tl_incidents[slot];
    x.valid=true; x.incidentId=incident_id(nodeId,seq); x.nodeId=nodeId; x.sequence=seq;
    x.latitude=lat; x.longitude=lon; x.hop=hop; x.rssi=rssi; x.status="ACTIVE";
    x.createdAt=millis(); x.updatedAt=x.createdAt;
}

void tl_update_node_from_sos(unsigned int nodeId, unsigned long seq,
                             const String &lat, const String &lon,
                             int hop, int rssi, bool sos) {
    int i=tl_find_node(nodeId);
    if(i<0) i=tl_allocate_node_slot();
    TL_NodeRecord &x=tl_nodes[i];
    if(!x.valid) x.firstSeen=millis();
    x.valid=true; x.nodeId=nodeId; x.sequence=seq; x.latitude=lat; x.longitude=lon;
    x.hop=hop; x.rssi=rssi; x.sosActive=sos; x.resolved=!sos; x.lastSeen=millis();
    tl_selected_node_index=i;
    if(sos) tl_create_incident(nodeId,seq,lat,lon,hop,rssi);
}

void tl_update_incident_status(unsigned int nodeId, const String &status) {
    for(int i=0;i<TL_MAX_INCIDENTS;i++)
        if(tl_incidents[i].valid && tl_incidents[i].nodeId==nodeId && tl_incidents[i].status!="RESOLVED") {
            tl_incidents[i].status=status; tl_incidents[i].updatedAt=millis();
        }
}

void tl_add_message_log(unsigned int nodeId,unsigned long seq,const String &msg,const String &status) {
    int slot=-1;
    for(int i=0;i<TL_MAX_MESSAGES;i++) if(!tl_messages[i].valid){slot=i;break;}
    if(slot<0){
        for(int i=0;i<TL_MAX_MESSAGES-1;i++) tl_messages[i]=tl_messages[i+1];
        slot=TL_MAX_MESSAGES-1;
    }
    tl_messages[slot].valid=true; tl_messages[slot].nodeId=nodeId; tl_messages[slot].sequence=seq;
    tl_messages[slot].message=msg; tl_messages[slot].status=status; tl_messages[slot].timestamp=millis();
}

void tl_update_message_status(unsigned int nodeId,unsigned long seq,const String &status) {
    for(int i=TL_MAX_MESSAGES-1;i>=0;i--)
        if(tl_messages[i].valid && tl_messages[i].nodeId==nodeId && tl_messages[i].sequence==seq){
            tl_messages[i].status=status; return;
        }
}

void tl_database_save_all(void) {
    if(!LittleFS.begin()) return;
    File f=LittleFS.open("/nodes.db","w");
    if(f){for(int i=0;i<TL_MAX_NODES;i++) if(tl_nodes[i].valid)
       f.printf(
        "%u|%u|%s|%s|%d|%d|%d|%d\n",
        tl_nodes[i].nodeId,
        (unsigned int)tl_nodes[i].sequence,
        tl_nodes[i].latitude.c_str(),
        tl_nodes[i].longitude.c_str(),
        tl_nodes[i].hop,
        tl_nodes[i].rssi,
        tl_nodes[i].sosActive,
        tl_nodes[i].resolved
        );
    f.close();}
    f=LittleFS.open("/incidents.db","w");
    if(f){for(int i=0;i<TL_MAX_INCIDENTS;i++) if(tl_incidents[i].valid)
        f.printf(
            "%s|%u|%u|%s|%s|%d|%d|%s\n",
            tl_incidents[i].incidentId.c_str(),
            tl_incidents[i].nodeId,
            (unsigned int)tl_incidents[i].sequence,
            tl_incidents[i].latitude.c_str(),
            tl_incidents[i].longitude.c_str(),
            tl_incidents[i].hop,
            tl_incidents[i].rssi,
            tl_incidents[i].status.c_str()
        );
    f.close();}
    f=LittleFS.open("/messages.db","w");
    if(f){for(int i=0;i<TL_MAX_MESSAGES;i++) if(tl_messages[i].valid)
        f.printf(
            "%u|%u|%s|%s\n",
            tl_messages[i].nodeId,
            (unsigned int)tl_messages[i].sequence,
            tl_messages[i].message.c_str(),
            tl_messages[i].status.c_str()
        );
}
}

void tl_database_load_all(void) {
    if(!LittleFS.begin()) { Serial.println(F("[FS] mount failed")); return; }
    /* Load is intentionally conservative; malformed records are skipped. */
    File f=LittleFS.open("/nodes.db","r"); int i=0;
    while(f && f.available() && i<TL_MAX_NODES) {
        String s=f.readStringUntil('\n'); s.trim(); if(!s.length()) continue;
        int p[8]; int q=-1; for(int k=0;k<8;k++){p[k]=s.indexOf('|',q+1);q=p[k];}
        if(p[7]<0) continue;
        tl_nodes[i].valid=true; tl_nodes[i].nodeId=s.substring(0,p[0]).toInt();
        tl_nodes[i].sequence=s.substring(p[0]+1,p[1]).toInt();
        tl_nodes[i].latitude=s.substring(p[1]+1,p[2]); tl_nodes[i].longitude=s.substring(p[2]+1,p[3]);
        tl_nodes[i].hop=s.substring(p[3]+1,p[4]).toInt(); tl_nodes[i].rssi=s.substring(p[4]+1,p[5]).toInt();
        tl_nodes[i].sosActive=s.substring(p[5]+1,p[6]).toInt(); tl_nodes[i].resolved=s.substring(p[6]+1,p[7]).toInt();
        tl_nodes[i].lastSeen=millis(); i++;
    } if(f)f.close();
}

void tl_database_clear(void) {
    tl_database_init();
    LittleFS.remove("/nodes.db"); LittleFS.remove("/incidents.db"); LittleFS.remove("/messages.db");
}
