#include <avr/io.h>
#include "tl_spi.h"
void tl_spi_init(void){DDRB|=_BV(DDB5)|_BV(DDB3)|_BV(DDB2);DDRB&=(uint8_t)~_BV(DDB4);SPCR=_BV(SPE)|_BV(MSTR);SPSR=_BV(SPI2X);}
uint8_t tl_spi_transfer(uint8_t d){SPDR=d;while(!(SPSR&_BV(SPIF))){}return SPDR;}
