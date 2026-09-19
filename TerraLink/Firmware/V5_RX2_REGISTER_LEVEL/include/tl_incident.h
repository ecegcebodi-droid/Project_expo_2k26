#ifndef TL_INCIDENT_H
#define TL_INCIDENT_H

void tl_process_sos(const String &line);
void tl_process_location(const String &line);
void tl_process_ack(const String &line);
void tl_process_msg_ack(const String &line);
void tl_process_resolve_ack(const String &line);
void tl_process_rx1_status(const String &line);
void tl_process_tx_status(const String &line);
void tl_resolve_selected_node(void);

#endif
