#ifndef TERRALINK_GLOBALS_H
#define TERRALINK_GLOBALS_H

#include "terralink_types.h"
#include <stdbool.h>
#include <stdint.h>

extern volatile tl_state_t g_state;
extern volatile bool g_lora_ready;
extern volatile bool g_final_ack;
extern volatile uint32_t g_active_sequence;

extern volatile tl_gps_data_t g_gps;
extern tl_persistent_sos_t g_persistent_sos;

extern volatile bool g_touch_pressed;
extern volatile bool g_local_alarm_pressed;
extern volatile bool g_led_button_pressed;

#endif
