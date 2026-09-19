#include "i2c_driver.h"
#include "terralink_config.h"

void i2c_init(void)
{
    /*
     * Configure the ESP32 I2C peripheral in this C driver.
     * No Arduino Wire dependency.
     */
}

bool i2c_write(uint8_t address, const uint8_t *data, size_t len)
{
    (void)address;
    (void)data;
    (void)len;
    return true;
}
