#include "tl_spi.h"

#include "tl_config.h"
#include "tl_registers.h"


/*
 * ============================================================
 *              TERRALINK SOFTWARE SPI
 * ============================================================
 */


/* ------------------------------------------------------------
 * SPI pin definitions
 * ------------------------------------------------------------ */

#define TL_SPI_SCK   TL_PIN_LORA_SCK
#define TL_SPI_MISO  TL_PIN_LORA_MISO
#define TL_SPI_MOSI  TL_PIN_LORA_MOSI
#define TL_SPI_CS    TL_PIN_LORA_SS


/* ------------------------------------------------------------
 * Small timing delay
 * ------------------------------------------------------------ */

static inline void spiDelay()
{
    delayMicroseconds(1);
}


/* ------------------------------------------------------------
 * Initialize SPI GPIO
 * ------------------------------------------------------------ */

void tl_spi_init()
{
    /* SCK */

    tl_gpio_output(TL_SPI_SCK);
    tl_gpio_low(TL_SPI_SCK);


    /* MOSI */

    tl_gpio_output(TL_SPI_MOSI);
    tl_gpio_low(TL_SPI_MOSI);


    /* MISO */

    tl_gpio_input(TL_SPI_MISO);


    /* Chip select */

    tl_gpio_output(TL_SPI_CS);

    tl_gpio_high(TL_SPI_CS);
}


/* ------------------------------------------------------------
 * Begin SPI transaction
 * ------------------------------------------------------------ */

void tl_spi_begin()
{
    /*
     * SX1278 chip select is active LOW.
     */

    tl_gpio_low(TL_SPI_CS);

    spiDelay();
}


/* ------------------------------------------------------------
 * End SPI transaction
 * ------------------------------------------------------------ */

void tl_spi_end()
{
    spiDelay();

    tl_gpio_high(TL_SPI_CS);
}


/* ------------------------------------------------------------
 * Software SPI transfer
 *
 * MSB first
 * SPI Mode 0
 *
 * CPOL = 0
 * CPHA = 0
 * ------------------------------------------------------------ */

uint8_t tl_spi_transfer(uint8_t data)
{
    uint8_t received = 0;


    for (int8_t bit = 7; bit >= 0; bit--)
    {
        /* ----------------------------------------------------
         * Set MOSI
         * ---------------------------------------------------- */

        if (data & (1U << bit))
        {
            tl_gpio_high(TL_SPI_MOSI);
        }
        else
        {
            tl_gpio_low(TL_SPI_MOSI);
        }


        spiDelay();


        /* ----------------------------------------------------
         * Clock HIGH
         * ---------------------------------------------------- */

        tl_gpio_high(TL_SPI_SCK);

        spiDelay();


        /* ----------------------------------------------------
         * Sample MISO
         * ---------------------------------------------------- */

        if (tl_gpio_read(TL_SPI_MISO))
        {
            received |= (1U << bit);
        }


        /* ----------------------------------------------------
         * Clock LOW
         * ---------------------------------------------------- */

        tl_gpio_low(TL_SPI_SCK);

        spiDelay();
    }


    return received;
}