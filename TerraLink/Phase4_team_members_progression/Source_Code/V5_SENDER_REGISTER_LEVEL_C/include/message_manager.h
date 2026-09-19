#ifndef MESSAGE_MANAGER_H
#define MESSAGE_MANAGER_H
void message_manager_init(void);
void message_manager_task(void);
void message_manager_process_packet(const char *packet);
#endif
