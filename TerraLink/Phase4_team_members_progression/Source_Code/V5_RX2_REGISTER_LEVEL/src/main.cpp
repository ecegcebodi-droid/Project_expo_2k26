#include <Arduino.h>
#include "tl_config.h"
#include "tl_state.h"
#include "tl_uart.h"
#include "tl_keypad.h"
#include "tl_database.h"
#include "tl_web.h"
#include "tl_ui.h"

void setup(void) {
    Serial.begin(TL_DEBUG_BAUD);
    delay(300);

    tl_state_init();
    tl_uart_init();
    tl_keypad_init();
    tl_database_init();
    tl_database_load_all();
    tl_web_init();

    Serial.println();
    Serial.println(F("============================================"));
    Serial.println(F(" TERRALINK RECEIVER #2"));
    Serial.println(F(" RESCUE COMMAND CENTER"));
    Serial.println(F(" ESP8266 / modular register-oriented build"));
    Serial.println(F("============================================"));

    tl_ui_home();
}

void loop(void) {
    tl_uart_process();
    tl_keypad_process();
    tl_web_process();
    tl_state_process();
    yield();
}
