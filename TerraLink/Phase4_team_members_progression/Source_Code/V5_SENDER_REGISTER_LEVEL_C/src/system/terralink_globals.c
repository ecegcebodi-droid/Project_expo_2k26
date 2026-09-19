#include "terralink_globals.h"

volatile tl_state_t g_state = TL_STATE_AWAKENING;
volatile bool g_lora_ready = false;
volatile bool g_final_ack = false;
volatile uint32_t g_active_sequence = 0;

volatile tl_gps_data_t g_gps = {0};
tl_persistent_sos_t g_persistent_sos = {0};

volatile bool g_touch_pressed = false;
volatile bool g_local_alarm_pressed = false;
volatile bool g_led_button_pressed = false;
