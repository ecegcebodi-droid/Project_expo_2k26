#ifndef TL_DATABASE_H
#define TL_DATABASE_H

void tl_database_init(void);
void tl_database_load_all(void);
void tl_database_save_all(void);
void tl_database_clear(void);

int tl_find_node(unsigned int nodeId);
int tl_allocate_node_slot(void);
int tl_get_node_count(void);
int tl_get_active_sos_count(void);

void tl_update_node_from_sos(unsigned int nodeId, unsigned long seq,
                             const String &lat, const String &lon,
                             int hop, int rssi, bool sos);

void tl_create_incident(unsigned int nodeId, unsigned long seq,
                        const String &lat, const String &lon,
                        int hop, int rssi);

void tl_update_incident_status(unsigned int nodeId, const String &status);
void tl_add_message_log(unsigned int nodeId, unsigned long seq,
                        const String &message, const String &status);
void tl_update_message_status(unsigned int nodeId, unsigned long seq,
                              const String &status);

#endif
