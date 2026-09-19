#include "display_manager.h"
#include "sh1106_driver.h"
#include "terralink_config.h"

void display_init(void)
{
    sh1106_init();
}

void display_starting(void)
{
    sh1106_clear();
    sh1106_draw_text(30, 30, "TERRALINK");
    sh1106_draw_text(34, 46, "STARTING");
    sh1106_update();
}

void display_ready(void)
{
    sh1106_clear();
    sh1106_draw_text(35, 30, "READY");
    sh1106_update();
}

void display_state(tl_state_t state)
{
    (void)state;
    /* Existing detailed OLED state screens should be ported here. */
}

void display_rescue_message(const char *message)
{
    sh1106_clear();
    sh1106_draw_text(0, 12, "RESCUE MESSAGE");
    sh1106_draw_text(0, 30, message);
    sh1106_update();
}
