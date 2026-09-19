#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
void enterDeepSleep()
{
  Serial.println();
  Serial.println(
    "Preparing for deep sleep..."
  );


  oled.clearBuffer();


  drawHeader();


  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    29,
    31,
    "POWER SAVE"
  );


  oled.drawStr(
    58,
    45,
    "Z"
  );


  oled.drawStr(
    63,
    52,
    "Z"
  );


  oled.setFont(
    u8g2_font_5x8_tf
  );


  oled.drawStr(
    39,
    62,
    "TOUCH TO WAKE"
  );


  oled.sendBuffer();


  delay(1000);


  buzzerOff();


  oled.setPowerSave(1);


  /*
     TOUCH wake:

     GPIO32 HIGH wakes ESP32.
  */

  esp_sleep_enable_ext0_wakeup(
    (gpio_num_t)TOUCH_PIN,
    1
  );


  /*
     TIMER wake:

     This is the important addition.

     The ESP32 can wake even if the victim does not
     touch the device again.
  */

  uint64_t sleepSeconds;


  if (pendingSOS)
  {
    sleepSeconds =
      SOS_RETRY_INTERVAL_SEC;
  }
  else
  {
    sleepSeconds =
      NORMAL_SLEEP_INTERVAL_SEC;
  }


  esp_sleep_enable_timer_wakeup(
    sleepSeconds *
    1000000ULL
  );


  Serial.println();


  if (pendingSOS)
  {
    Serial.print(
      "Pending SOS detected."
    );


    Serial.println();


    Serial.print(
      "Sleeping for "
    );


    Serial.print(
      SOS_RETRY_INTERVAL_SEC
    );


    Serial.println(
      " seconds before retry."
    );
  }
  else
  {
    Serial.print(
      "No pending SOS."
    );


    Serial.println();


    Serial.print(
      "Sleeping for "
    );


    Serial.print(
      NORMAL_SLEEP_INTERVAL_SEC
    );


    Serial.println(
      " seconds."
    );
  }


  Serial.println(
    "Touch or timer will wake ESP32."
  );


  Serial.flush();


  delay(100);


  esp_deep_sleep_start();
}
