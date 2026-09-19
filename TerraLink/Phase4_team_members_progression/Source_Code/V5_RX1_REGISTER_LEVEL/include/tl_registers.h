#ifndef TL_REGISTERS_H
#define TL_REGISTERS_H

#include <Arduino.h>

/* ESP8266 GPIO register-level helpers. */
#define TL_REG_GPIO_OUT_SET  (*(volatile uint32_t*)0x60000304)
#define TL_REG_GPIO_OUT_CLR  (*(volatile uint32_t*)0x60000308)
#define TL_REG_GPIO_OUT      (*(volatile uint32_t*)0x60000300)
#define TL_REG_GPIO_ENABLE   (*(volatile uint32_t*)0x6000030C)
#define TL_REG_GPIO_ENABLE_W1TS (*(volatile uint32_t*)0x60000310)
#define TL_REG_GPIO_ENABLE_W1TC (*(volatile uint32_t*)0x60000314)
#define TL_REG_GPIO_IN       (*(volatile uint32_t*)0x60000318)

inline void tl_gpio_output(uint8_t gpio) {
    TL_REG_GPIO_ENABLE_W1TS = (1UL << gpio);
}
inline void tl_gpio_input(uint8_t gpio) {
    TL_REG_GPIO_ENABLE_W1TC = (1UL << gpio);
}
inline void tl_gpio_high(uint8_t gpio) {
    TL_REG_GPIO_OUT_SET = (1UL << gpio);
}
inline void tl_gpio_low(uint8_t gpio) {
    TL_REG_GPIO_OUT_CLR = (1UL << gpio);
}
inline bool tl_gpio_read(uint8_t gpio) {
    return (TL_REG_GPIO_IN & (1UL << gpio)) != 0;
}
inline void tl_gpio_mode_output(uint8_t gpio, bool initialHigh = false) {
    tl_gpio_output(gpio);
    if (initialHigh) tl_gpio_high(gpio); else tl_gpio_low(gpio);
}

#endif
