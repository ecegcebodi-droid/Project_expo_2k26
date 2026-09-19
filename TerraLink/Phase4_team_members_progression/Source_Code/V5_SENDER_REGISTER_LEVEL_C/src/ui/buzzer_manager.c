#include "buzzer_manager.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUZZER_PIN 25

void buzzer_init(void)
{
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    buzzer_off();
}

void buzzer_on(void)
{
    gpio_set_level(BUZZER_PIN, 1);
}

void buzzer_off(void)
{
    gpio_set_level(BUZZER_PIN, 0);
}

void buzzer_beep(uint32_t duration_ms)
{
    buzzer_on();

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    buzzer_off();
}

void buzzer_sos_activated(void)
{
    buzzer_beep(300);
    vTaskDelay(pdMS_TO_TICKS(100));
    buzzer_beep(300);
}

void buzzer_rescue_message(void)
{
    buzzer_beep(100);
    vTaskDelay(pdMS_TO_TICKS(100));
    buzzer_beep(100);
}