#include <Arduino.h>
extern "C" {
#include "tl_config.h"
#include "tl_relay.h"
}

extern "C" void tl_console_print(const char *text){ Serial.print(text); }
extern "C" void tl_console_println(const char *text){ Serial.println(text); }
extern "C" void tl_console_u32(unsigned long value){ Serial.print(value); }
extern "C" void tl_console_i32(long value){ Serial.print(value); }
extern "C" void tl_console_line(const char *tag,const char *text){ Serial.print(tag);Serial.print(' ');Serial.println(text); }

void setup(){
    Serial.begin(TL_SERIAL_BAUD);
    delay(300);
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F(" TERRALINK REGISTER-LEVEL ROUTED RELAY"));
    Serial.println(F("========================================"));
    Serial.print(F("Relay Node ID: ")); Serial.println(TL_RELAY_NODE_ID);
    Serial.println(F("LoRa: SX1278 / 433 MHz / SF7 / BW125k / CR4/5"));
    Serial.println(F("SPI: D13 SCK, D12 MISO, D11 MOSI, D10 NSS"));
    Serial.println(F("RST: D9   DIO0: D2 (reserved)"));
    tl_relay_init();
}

void loop(){
    tl_relay_process();
    tl_relay_periodic();
}
