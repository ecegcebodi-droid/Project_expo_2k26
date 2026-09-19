#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H
#include <stdint.h>
#include <stddef.h>
void i2c_init(void);
bool i2c_write(uint8_t address, const uint8_t *data, size_t len);
#endif
