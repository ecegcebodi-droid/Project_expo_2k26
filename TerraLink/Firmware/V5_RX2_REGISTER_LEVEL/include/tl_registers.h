#ifndef TL_REGISTERS_H
#define TL_REGISTERS_H

#include <Arduino.h>

/*
 * ESP8266 register-level GPIO helpers.
 * These helpers are used only for keypad scanning.
 * Arduino WiFi/WebServer/LittleFS remain C++ modules because those
 * frameworks expose C++ APIs.
 */
static inline void tl_gpio_output(uint8_t pin) {
    pinMode(pin, OUTPUT);
}

static inline void tl_gpio_input_pullup(uint8_t pin) {
    pinMode(pin, INPUT_PULLUP);
}

static inline void tl_gpio_high(uint8_t pin) {
    digitalWrite(pin, HIGH);
}

static inline void tl_gpio_low(uint8_t pin) {
    digitalWrite(pin, LOW);
}

static inline int tl_gpio_read(uint8_t pin) {
    return digitalRead(pin);
}

#endif
