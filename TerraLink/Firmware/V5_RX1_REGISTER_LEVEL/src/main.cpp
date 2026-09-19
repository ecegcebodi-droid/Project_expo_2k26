#include <Arduino.h>
#include "tl_gateway.h"

void setup(){
    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println("========================================");
    Serial.println(" TERRALINK RX1 - REGISTER LEVEL GATEWAY");
    Serial.println("========================================");
    tl_gateway_init();
}

void loop(){
    tl_gateway_loop();
    yield();
}
