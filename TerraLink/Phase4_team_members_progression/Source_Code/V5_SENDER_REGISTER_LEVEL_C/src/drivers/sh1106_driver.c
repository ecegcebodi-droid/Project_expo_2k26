#include "sh1106_driver.h"
#include "i2c_driver.h"
#include "terralink_config.h"
#include <string.h>

static uint8_t framebuffer[TL_OLED_WIDTH * TL_OLED_HEIGHT / 8];

void sh1106_init(void)
{
    i2c_init();
    memset(framebuffer, 0, sizeof(framebuffer));
}

void sh1106_clear(void)
{
    memset(framebuffer, 0, sizeof(framebuffer));
}

void sh1106_set_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= TL_OLED_WIDTH || y >= TL_OLED_HEIGHT)
        return;

    uint16_t index = x + (y / 8U) * TL_OLED_WIDTH;
    uint8_t mask = (uint8_t)(1U << (y & 7U));

    if (on) framebuffer[index] |= mask;
    else    framebuffer[index] &= (uint8_t)~mask;
}

void sh1106_draw_text(uint8_t x, uint8_t y, const char *text)
{
    /* Font rasterizer belongs here. */
    (void)x; (void)y; (void)text;
}

void sh1106_update(void)
{
    /* Page addressing and I2C transfer belong here. */
}
