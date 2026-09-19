#include "sx1278_driver.h"
#include "spi_driver.h"
#include "gpio_driver.h"
#include "terralink_config.h"
#include "terralink_types.h"

#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_MODEM_CONFIG1        0x1D
#define REG_MODEM_CONFIG2        0x1E
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_MODEM_CONFIG3        0x26
#define REG_SYNC_WORD            0x39
#define REG_DIO_MAPPING1         0x40
#define REG_VERSION              0x42

#define MODE_LONG_RANGE          0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05

#define IRQ_TX_DONE              0x08
#define IRQ_RX_DONE              0x40

static void reset_radio(void)
{
    gpio_config_output(TL_LORA_RST);
    gpio_set_low(TL_LORA_RST);
    for (volatile uint32_t i = 0; i < 50000; ++i) {}
    gpio_set_high(TL_LORA_RST);
    for (volatile uint32_t i = 0; i < 50000; ++i) {}
}

uint8_t sx1278_read_reg(uint8_t address)
{
    uint8_t value;
    spi_cs_low();
    spi_transfer_byte(address & 0x7F);
    value = spi_transfer_byte(0);
    spi_cs_high();
    return value;
}

void sx1278_write_reg(uint8_t address, uint8_t value)
{
    spi_cs_low();
    spi_transfer_byte(address | 0x80);
    spi_transfer_byte(value);
    spi_cs_high();
}

static void set_frequency_433mhz(void)
{
    /* FRF = frequency / FSTEP; FSTEP = 32MHz / 2^19 */
    uint64_t frf = ((uint64_t)TL_LORA_FREQUENCY_HZ << 19) / 32000000ULL;

    sx1278_write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
    sx1278_write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
    sx1278_write_reg(REG_FRF_LSB, (uint8_t)frf);
}

bool sx1278_init(void)
{
    reset_radio();

    uint8_t version = sx1278_read_reg(REG_VERSION);
    if (version == 0x00 || version == 0xFF)
        return false;

    sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_SLEEP);
    sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_STDBY);

    set_frequency_433mhz();

    /* BW=125kHz, CR=4/5, explicit header */
    sx1278_write_reg(REG_MODEM_CONFIG1, 0x72);

    /* SF7, CRC on */
    sx1278_write_reg(REG_MODEM_CONFIG2, 0x74);

    /* Low data-rate optimization off, AGC on */
    sx1278_write_reg(REG_MODEM_CONFIG3, 0x04);

    sx1278_write_reg(REG_SYNC_WORD, TL_LORA_SYNC_WORD);
    sx1278_write_reg(REG_FIFO_TX_BASE_ADDR, 0x00);
    sx1278_write_reg(REG_FIFO_RX_BASE_ADDR, 0x00);
    sx1278_write_reg(REG_IRQ_FLAGS, 0xFF);

    sx1278_receive_mode();
    return true;
}

bool sx1278_send(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0 || len > 255)
        return false;

    sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_STDBY);
    sx1278_write_reg(REG_FIFO_ADDR_PTR, 0);

    spi_cs_low();
    spi_transfer_byte(REG_FIFO | 0x80);

    for (uint16_t i = 0; i < len; ++i)
        spi_transfer_byte(data[i]);

    spi_cs_high();

    sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_TX);

    uint32_t timeout = 1000000;

    while (timeout--)
    {
        if (sx1278_read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE)
        {
            sx1278_write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE);
            sx1278_receive_mode();
            return true;
        }
    }

    sx1278_receive_mode();
    return false;
}
int sx1278_receive(uint8_t *buffer, uint8_t max_len, int16_t *rssi)
{
    uint8_t flags = sx1278_read_reg(REG_IRQ_FLAGS);

    if (!(flags & IRQ_RX_DONE))
        return 0;

    sx1278_write_reg(REG_IRQ_FLAGS, 0xFF);

    uint8_t len = sx1278_read_reg(REG_RX_NB_BYTES);
    uint8_t current = sx1278_read_reg(REG_FIFO_RX_CURRENT_ADDR);
    sx1278_write_reg(REG_FIFO_ADDR_PTR, current);

    if (len > max_len)
        len = max_len;

    spi_cs_low();
    spi_transfer_byte(REG_FIFO & 0x7F);
    for (uint8_t i = 0; i < len; ++i)
        buffer[i] = spi_transfer_byte(0);
    spi_cs_high();

    if (rssi)
        *rssi = -137 + sx1278_read_reg(0x1A);

    return len;
}

void sx1278_receive_mode(void)
{
    sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_RX_CONTINUOUS);
}
