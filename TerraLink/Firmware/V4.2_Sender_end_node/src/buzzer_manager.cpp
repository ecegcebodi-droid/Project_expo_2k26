#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
void buzzerOn()
{
  digitalWrite(
    BUZZER_PIN,
    HIGH
  );
}
void buzzerOff()
{
  digitalWrite(
    BUZZER_PIN,
    LOW
  );
}
void beep(
  unsigned int duration
)
{
  buzzerOn();

  delay(duration);

  buzzerOff();
}
void buzzerWake()
{
  beep(100);
}
void buzzerSOSActivated()
{
  beep(120);

  delay(120);

  beep(120);

  delay(120);

  beep(120);
}
void buzzerSending()
{
  beep(80);

  delay(80);

  beep(80);

  delay(80);

  beep(80);
}
void buzzerWaiting()
{
  beep(70);

  delay(70);

  beep(70);
}
void buzzerSuccess()
{
  beep(120);

  delay(120);

  beep(120);

  delay(300);

  beep(300);
}
void buzzerFailure()
{
  beep(350);

  delay(350);

  beep(350);
}
void buzzerRescueMessage()
{
  beep(120);

  delay(100);

  beep(120);
}
