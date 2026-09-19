#include "input_manager.h"
#include "gpio_driver.h"
#include "terralink_globals.h"
#include "terralink_config.h"
#include "timer_driver.h"
#include "state_machine.h"

static uint64_t touch_start;

void input_init(void)
{
    touch_start = 0;
}

void input_task(void)
{
    bool pressed = gpio_read(TL_PIN_SOS_TOUCH) != 0;

    g_touch_pressed = pressed;
    g_local_alarm_pressed = gpio_read(TL_PIN_LOCAL_ALARM) == 0;
    g_led_button_pressed = gpio_read(TL_PIN_LED_BUTTON) == 0;

    if (pressed && touch_start == 0)
        touch_start = timer_millis();

    if (!pressed)
        touch_start = 0;

    if (pressed && touch_start &&
        (timer_millis() - touch_start) >= TL_SOS_LONG_PRESS_MS)
    {
        state_machine_set_sos();
        touch_start = 0;
    }
}
