#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H
#include "terralink_types.h"
void display_init(void);
void display_starting(void);
void display_ready(void);
void display_state(tl_state_t state);
void display_rescue_message(const char *message);
#endif
