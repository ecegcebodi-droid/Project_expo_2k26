#include "power_driver.h"
#include "esp_sleep.h"

void power_init(void)
{
}

void power_deep_sleep_seconds(uint32_t seconds)
{
    esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
    esp_deep_sleep_start();
}
