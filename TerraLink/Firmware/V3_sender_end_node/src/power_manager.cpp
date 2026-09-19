#include <Arduino.h>
#include "esp_sleep.h"
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"

void enterDeepSleep() {
    Serial.println("\nPreparing for deep sleep...");
    oled.clearBuffer();
    drawHeader();
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawStr(29, 31, "POWER SAVE");
    oled.drawStr(58, 45, "Z");
    oled.drawStr(63, 52, "Z");
    oled.setFont(u8g2_font_5x8_tf);
    oled.drawStr(39, 62, "TOUCH TO WAKE");
    oled.sendBuffer();
    delay(1000);
    buzzerOff();
    oled.setPowerSave(1);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_PIN, 1);
    const uint64_t sleepSeconds = pendingSOS ? SOS_RETRY_INTERVAL_SEC : NORMAL_SLEEP_INTERVAL_SEC;
    esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
    Serial.print(pendingSOS ? "Pending SOS detected. Sleeping for " : "No pending SOS. Sleeping for ");
    Serial.print((unsigned long long)sleepSeconds);
    Serial.println(" seconds.");
    Serial.println("Touch or timer will wake ESP32.");
    Serial.flush();
    delay(100);
    esp_deep_sleep_start();
}
