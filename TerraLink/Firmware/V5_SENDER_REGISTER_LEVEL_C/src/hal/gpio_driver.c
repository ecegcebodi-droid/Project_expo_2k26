#include "gpio_driver.h"

#include "driver/gpio.h"


/*
 * ============================================================
 * GPIO INITIALIZATION
 * ============================================================
 */

void gpio_init(void)
{
    /*
     * GPIO initialization is performed individually
     * by the corresponding configuration functions.
     *
     * Keep this function available so the application
     * can call gpio_init() during system startup.
     */
}


/*
 * ============================================================
 * TERRA-LINK GPIO OUTPUT ENABLE
 * ============================================================
 *
 * IMPORTANT:
 * Do NOT name this function gpio_output_enable().
 *
 * ESP-IDF already provides a function with that name.
 * The "tl_" prefix prevents linker conflicts.
 */

void tl_gpio_output_enable(uint8_t pin)
{
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
}


/*
 * ============================================================
 * TERRA-LINK GPIO OUTPUT DISABLE
 * ============================================================
 *
 * IMPORTANT:
 * Do NOT name this function gpio_output_disable().
 *
 * ESP-IDF already provides a function with that name.
 */

void tl_gpio_output_disable(uint8_t pin)
{
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
}


/*
 * ============================================================
 * SET GPIO HIGH
 * ============================================================
 */

void gpio_set_high(uint8_t pin)
{
    gpio_set_level((gpio_num_t)pin, 1);
}


/*
 * ============================================================
 * SET GPIO LOW
 * ============================================================
 */

void gpio_set_low(uint8_t pin)
{
    gpio_set_level((gpio_num_t)pin, 0);
}


/*
 * ============================================================
 * READ GPIO
 * ============================================================
 */

uint8_t gpio_read(uint8_t pin)
{
    return (uint8_t)gpio_get_level((gpio_num_t)pin);
}


/*
 * ============================================================
 * CONFIGURE GPIO AS INPUT
 * ============================================================
 */

void gpio_config_input(uint8_t pin)
{
    tl_gpio_output_disable(pin);
}


/*
 * ============================================================
 * CONFIGURE GPIO AS INPUT WITH PULL-UP
 * ============================================================
 */

void gpio_config_input_pullup(uint8_t pin)
{
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
    gpio_pullup_en((gpio_num_t)pin);
    gpio_pulldown_dis((gpio_num_t)pin);
}


/*
 * ============================================================
 * CONFIGURE GPIO AS OUTPUT
 * ============================================================
 */

void gpio_config_output(uint8_t pin)
{
    tl_gpio_output_enable(pin);
    gpio_set_low(pin);
}