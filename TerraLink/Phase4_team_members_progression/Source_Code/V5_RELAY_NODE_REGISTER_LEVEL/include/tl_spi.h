#ifndef TL_SPI_H
#define TL_SPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void tl_spi_init(void);
uint8_t tl_spi_transfer(uint8_t data);

#ifdef __cplusplus
}
#endif

#endif
