#ifndef TL_I2C_LCD_H
#define TL_I2C_LCD_H

#include <Arduino.h>

void tl_lcd_init();
void tl_lcd_clear();
void tl_lcd_home();
void tl_lcd_line(uint8_t row, const String &text);
void tl_lcd_message(const String &line1, const String &line2);

#endif
