#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H
#include <stdint.h>
#include <stddef.h>
void spi_init(void);
uint8_t spi_transfer_byte(uint8_t data);
void spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len);
void spi_cs_low(void);
void spi_cs_high(void);
#endif
