#include "tl_i2c_lcd.h"
#include "tl_registers.h"
#include "tl_config.h"

static void i2cDelay(){delayMicroseconds(5);} static void sda(bool h){if(h)tl_gpio_input(TL_PIN_I2C_SDA);else{tl_gpio_output(TL_PIN_I2C_SDA);tl_gpio_low(TL_PIN_I2C_SDA);}} static void scl(bool h){if(h)tl_gpio_input(TL_PIN_I2C_SCL);else{tl_gpio_output(TL_PIN_I2C_SCL);tl_gpio_low(TL_PIN_I2C_SCL);}}
static void start(){sda(true);scl(true);i2cDelay();sda(false);i2cDelay();scl(false);} static void stop(){sda(false);scl(true);i2cDelay();sda(true);i2cDelay();}
static bool writeByte(uint8_t b){for(int i=0;i<8;i++){sda(b&0x80);b<<=1;scl(true);i2cDelay();scl(false);}sda(true);scl(true);bool ack=!tl_gpio_read(TL_PIN_I2C_SDA);i2cDelay();scl(false);return ack;}
static void expWrite(uint8_t v){start();writeByte(TL_LCD_ADDR<<1);writeByte(v);stop();}
static void pulse(uint8_t x){expWrite(x|0x04);delayMicroseconds(1);expWrite(x&~0x04);delayMicroseconds(40);} static void nibble(uint8_t n,bool rs){uint8_t x=(n&0xF0)|(rs?1:0);pulse(x);} static void cmd(uint8_t c){nibble(c,false);nibble(c<<4,false);}
static void data(uint8_t c){nibble(c,true);nibble(c<<4,true);} static void put(const String&s){for(size_t i=0;i<s.length();i++)data((uint8_t)s[i]);}
void tl_lcd_init(){tl_gpio_input(TL_PIN_I2C_SDA);tl_gpio_input(TL_PIN_I2C_SCL);delay(50);for(int i=0;i<3;i++){nibble(0x30,false);delay(5);}nibble(0x20,false);cmd(0x28);cmd(0x0C);cmd(0x06);cmd(0x01);delay(3);}
void tl_lcd_clear(){cmd(0x01);delay(3);} void tl_lcd_home(){cmd(0x02);delay(2);} void tl_lcd_line(uint8_t row,const String&t){cmd(row?0xC0:0x80);String s=t; if(s.length()>16)s=s.substring(0,16); while(s.length()<16)s+=' '; put(s);} void tl_lcd_message(const String&a,const String&b){tl_lcd_clear();tl_lcd_line(0,a);tl_lcd_line(1,b);}
