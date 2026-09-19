#ifndef SH1106_DRIVER_H
#define SH1106_DRIVER_H
#include <stdint.h>
void sh1106_init(void);
void sh1106_clear(void);
void sh1106_set_pixel(uint8_t x, uint8_t y, uint8_t on);
void sh1106_draw_text(uint8_t x, uint8_t y, const char *text);
void sh1106_update(void);
#endif
