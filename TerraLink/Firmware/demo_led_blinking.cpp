#include <Arduino.h>

#define LED_PIN 13

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(9600);
}

void loop()
{
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    Serial.println("led on");

    digitalWrite(LED_PIN, LOW);
    delay(1000);
      Serial.println("led off");
}