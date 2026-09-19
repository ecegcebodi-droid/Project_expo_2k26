#include "timer_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

uint64_t timer_millis(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
}

void timer_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
