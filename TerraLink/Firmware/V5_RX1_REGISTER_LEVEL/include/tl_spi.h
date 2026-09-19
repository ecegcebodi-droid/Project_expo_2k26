#ifndef TL_SPI_H
#define TL_SPI_H

#include <Arduino.h>

/*
 * ============================================================
 *              TERRALINK SOFTWARE SPI DRIVER
 * ============================================================
 *
 * ESP8266 NodeMCU
 *
 * SCK  -> D5 / GPIO14
 * MISO -> D6 / GPIO12
 * MOSI -> D7 / GPIO13
 * NSS  -> D1 / GPIO5
 *
 * Direct GPIO/register-level implementation.
 * ============================================================
 */

void tl_spi_init();

void tl_spi_begin();
void tl_spi_end();

uint8_t tl_spi_transfer(uint8_t data);

#endif