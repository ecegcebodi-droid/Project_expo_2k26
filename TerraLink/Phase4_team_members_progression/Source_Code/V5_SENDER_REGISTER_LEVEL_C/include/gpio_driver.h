#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Initialize GPIO-related hardware.
 */
void gpio_init(void);

/*
 * TerraLink-specific GPIO output control.
 *
 * These names intentionally use the "tl_" prefix
 * to avoid conflicts with ESP-IDF GPIO functions.
 */
void tl_gpio_output_enable(uint8_t pin);
void tl_gpio_output_disable(uint8_t pin);

/*
 * Set GPIO output level.
 */
void gpio_set_high(uint8_t pin);
void gpio_set_low(uint8_t pin);

/*
 * Read GPIO input level.
 */
uint8_t gpio_read(uint8_t pin);

/*
 * Configure GPIO pins as inputs/outputs.
 */
void gpio_config_input(uint8_t pin);
void gpio_config_input_pullup(uint8_t pin);
void gpio_config_output(uint8_t pin);

#endif /* GPIO_DRIVER_H */