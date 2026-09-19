#include <Arduino.h>

#include "tl_config.h"
#include "tl_registers.h"
#include "tl_keypad.h"
#include "tl_state.h"
#include "tl_database.h"
#include "tl_ui.h"
#include "tl_command.h"
#include "tl_incident.h"

static const uint8_t rowPins[4] = {TL_ROW1_PIN, TL_ROW2_PIN, TL_ROW3_PIN, TL_ROW4_PIN};
static const uint8_t colPins[4] = {TL_COL1_PIN, TL_COL2_PIN, TL_COL3_PIN, TL_COL4_PIN};
static const char keyMap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

static char lastRaw = 0;
static char stableKey = 0;
static unsigned long keyChangedAt = 0;

static char tl_scan_raw(void) {
    for (uint8_t r = 0; r < 4; ++r) {
        for (uint8_t i = 0; i < 4; ++i) tl_gpio_high(rowPins[i]);
        tl_gpio_low(rowPins[r]);

        for (uint8_t c = 0; c < 4; ++c) {
            if (tl_gpio_read(colPins[c]) == LOW) {
                for (uint8_t i = 0; i < 4; ++i) tl_gpio_high(rowPins[i]);
                return keyMap[r][c];
            }
        }
    }
    for (uint8_t i = 0; i < 4; ++i) tl_gpio_high(rowPins[i]);
    return 0;
}

static char tl_get_key(void) {
    char raw = tl_scan_raw();
    unsigned long now = millis();

    if (raw != lastRaw) {
        lastRaw = raw;
        keyChangedAt = now;
    }

    if ((now - keyChangedAt) >= 40 && raw != stableKey) {
        stableKey = raw;
        if (stableKey) {
            if (stableKey == '*') return 'F';
            if (stableKey == '#') return 'E';
            return stableKey;
        }
    }
    return 0;
}

static void tl_handle_key(char k) {
    if (k == '0') {
        tl_ui_state = TL_UI_HOME;
        tl_pending_action = TL_ACT_NONE;
        tl_ui_home();
        return;
    }

    switch (tl_ui_state) {
    case TL_UI_HOME:
        if (k == '1') {
            tl_ui_state = TL_UI_NODE_SELECT;
            tl_ui_node_selection();
        } else if (k == 'E' && tl_selected_node_index >= 0) {
            tl_ui_state = TL_UI_NODE_INFO;
            tl_ui_node_info();
        }
        break;

    case TL_UI_NODE_SELECT:
        if (k == '2') {
            int n = tl_get_node_count();
            if (n) {
                int start = tl_selected_node_index < 0 ? 0 : tl_selected_node_index;
                for (int i=1;i<=TL_MAX_NODES;i++) {
                    int x=(start+i)%TL_MAX_NODES;
                    if (tl_nodes[x].valid) { tl_selected_node_index=x; break; }
                }
            }
            tl_ui_node_selection();
        } else if (k == '3') {
            int n = tl_get_node_count();
            if (n) {
                int start = tl_selected_node_index < 0 ? 0 : tl_selected_node_index;
                for (int i=1;i<=TL_MAX_NODES;i++) {
                    int x=(start-i+TL_MAX_NODES)%TL_MAX_NODES;
                    if (tl_nodes[x].valid) { tl_selected_node_index=x; break; }
                }
            }
            tl_ui_node_selection();
        } else if (k == 'A' && tl_selected_node_index >= 0) {
            tl_ui_state = TL_UI_NODE_CONTROL; tl_ui_node_control();
        } else if (k == 'B' || k == 'C') {
            tl_ui_state = TL_UI_HOME; tl_ui_home();
        }
        break;

    case TL_UI_NODE_CONTROL:
        if (k >= '4' && k <= '9') {
            tl_pending_action = (TL_ActionType)(TL_ACT_STAY_CALM + (k-'4'));
            tl_ui_state = TL_UI_ACTION_CONFIRM; tl_ui_action_confirm();
        } else if (k == 'F') {
            tl_pending_action = TL_ACT_EMERGENCY;
            tl_ui_state = TL_UI_ACTION_CONFIRM; tl_ui_action_confirm();
        } else if (k == 'D') {
            tl_ui_state = TL_UI_RESOLVE_CONFIRM; tl_ui_resolve_confirm();
        } else if (k == 'E') {
            tl_ui_state = TL_UI_NODE_INFO; tl_ui_node_info();
        } else if (k == 'B') {
            tl_ui_state = TL_UI_NODE_SELECT; tl_ui_node_selection();
        }
        break;

    case TL_UI_ACTION_CONFIRM:
        if (k == 'A') tl_execute_pending_action();
        else if (k == 'B' || k == 'C') {
            tl_pending_action=TL_ACT_NONE; tl_ui_state=TL_UI_NODE_CONTROL; tl_ui_node_control();
        }
        break;

    case TL_UI_RESOLVE_CONFIRM:
        if (k == 'A') tl_resolve_selected_node();
        else if (k == 'B' || k == 'C') {
            tl_ui_state=TL_UI_NODE_CONTROL; tl_ui_node_control();
        }
        break;

    case TL_UI_NODE_INFO:
        if (k == 'B' || k == 'C') {
            tl_ui_state=TL_UI_NODE_CONTROL; tl_ui_node_control();
        }
        break;

    case TL_UI_SENT_STATUS:
        if (k == 'A' || k == 'B' || k == 'C') {
            tl_ui_state=TL_UI_NODE_CONTROL; tl_ui_node_control();
        }
        break;
    }
}

void tl_keypad_init(void) {
    for (uint8_t i=0;i<4;i++) {
        tl_gpio_output(rowPins[i]);
        tl_gpio_high(rowPins[i]);
        tl_gpio_input_pullup(colPins[i]);
    }
}

void tl_keypad_process(void) {
    char k=tl_get_key();
    if (k) {
        Serial.print(F("[KEY] ")); Serial.println(k);
        tl_handle_key(k);
    }
}
