#include "spi_driver.h"
#include "gpio_driver.h"
#include "terralink_config.h"
#include "terralink_registers.h"

void spi_init(void)
{
    /*
     * Register-level SPI transaction implementation belongs here.
     * The exact SPI2/VSPI register layout varies with the ESP32 target/IDF.
     * Keep this driver isolated so the application never depends on Arduino SPI.
     */
    gpio_config_output(TL_LORA_NSS);
    gpio_set_high(TL_LORA_NSS);
}

uint8_t spi_transfer_byte(uint8_t data)
{
    /*
     * Placeholder transaction hook for the selected ESP32 SPI peripheral.
     * Replace the transaction section with the exact target register sequence
     * after locking the ESP-IDF/SoC version used for the hardware build.
     */
    (void)data;
    return 0;
}

void spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        uint8_t r = spi_transfer_byte(tx ? tx[i] : 0);
        if (rx) rx[i] = r;
    }
}

void spi_cs_low(void)
{
    gpio_set_low(TL_LORA_NSS);
}

void spi_cs_high(void)
{
    gpio_set_high(TL_LORA_NSS);
}
