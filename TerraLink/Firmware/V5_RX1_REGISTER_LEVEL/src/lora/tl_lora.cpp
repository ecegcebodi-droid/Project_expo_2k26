#include "tl_lora.h"

#include "tl_config.h"
#include "tl_registers.h"
#include "tl_spi.h"
#include "tl_packet.h"


/*
 * ============================================================
 *                 TERRALINK SX1278 DRIVER
 * ============================================================
 *
 * Direct register-level SX1278 / RA-02 driver.
 *
 * No LoRa library is used.
 *
 * SPI:
 *   SCK  = D5 / GPIO14
 *   MISO = D6 / GPIO12
 *   MOSI = D7 / GPIO13
 *   NSS  = D1 / GPIO5
 *
 * RESET:
 *   D0 / GPIO16
 *
 * DIO0:
 *   Not connected.
 *   IRQ flags are polled directly.
 * ============================================================
 */


/* ------------------------------------------------------------
 * SX1278 Registers
 * ------------------------------------------------------------ */

#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01

#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08

#define REG_PA_CONFIG            0x09
#define REG_LNA                  0x0C

#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE         0x0E
#define REG_FIFO_RX_BASE         0x0F
#define REG_FIFO_RX_CURRENT      0x10

#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13

#define REG_PKT_SNR              0x19
#define REG_PKT_RSSI             0x1A

#define REG_MODEM_CONFIG1        0x1D
#define REG_MODEM_CONFIG2        0x1E
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21

#define REG_MODEM_CONFIG3        0x26

#define REG_SYNC_WORD            0x39
#define REG_VERSION              0x42


/* ------------------------------------------------------------
 * SX1278 Modes
 * ------------------------------------------------------------ */

#define MODE_LONG_RANGE          0x80

#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05


/* ------------------------------------------------------------
 * IRQ flags
 * ------------------------------------------------------------ */

#define IRQ_RX_DONE              0x40
#define IRQ_TX_DONE              0x08
#define IRQ_PAYLOAD_CRC_ERROR    0x20


/* ------------------------------------------------------------
 * Local helpers
 * ------------------------------------------------------------ */

static uint8_t regRead(uint8_t address)
{
    uint8_t value;

    tl_spi_begin();

    tl_spi_transfer(address & 0x7F);

    value = tl_spi_transfer(0x00);

    tl_spi_end();

    return value;
}


static void regWrite(
    uint8_t address,
    uint8_t value
)
{
    tl_spi_begin();

    tl_spi_transfer(address | 0x80);

    tl_spi_transfer(value);

    tl_spi_end();
}


/* ------------------------------------------------------------
 * Reset SX1278
 * ------------------------------------------------------------ */

static void loraReset()
{
    tl_gpio_output(TL_PIN_LORA_RST);

    tl_gpio_low(TL_PIN_LORA_RST);

    delay(10);

    tl_gpio_high(TL_PIN_LORA_RST);

    delay(10);
}


/* ------------------------------------------------------------
 * Set frequency
 *
 * For 433 MHz:
 *
 * FRF = 433000000 / 61.03515625
 *     ≈ 7094272
 * ------------------------------------------------------------ */

static void setFrequency433MHz()
{
    uint32_t frf = 7094272UL;

    regWrite(
        REG_FRF_MSB,
        (uint8_t)(frf >> 16)
    );

    regWrite(
        REG_FRF_MID,
        (uint8_t)(frf >> 8)
    );

    regWrite(
        REG_FRF_LSB,
        (uint8_t)(frf)
    );
}


/* ------------------------------------------------------------
 * Enter RX continuous mode
 * ------------------------------------------------------------ */

static void startReceive()
{
    regWrite(
        REG_FIFO_ADDR_PTR,
        regRead(REG_FIFO_RX_BASE)
    );

    regWrite(
        REG_OP_MODE,
        MODE_LONG_RANGE | MODE_RX_CONTINUOUS
    );
}


/* ------------------------------------------------------------
 * Initialize SX1278
 * ------------------------------------------------------------ */

bool tl_lora_init()
{
    tl_spi_init();

    loraReset();


    /* Check SX1278 version */

    uint8_t version = regRead(REG_VERSION);

    if (version != 0x12)
    {
        return false;
    }


    /* Sleep */

    regWrite(
        REG_OP_MODE,
        MODE_LONG_RANGE | MODE_SLEEP
    );

    delay(5);


    /* Standby */

    regWrite(
        REG_OP_MODE,
        MODE_LONG_RANGE | MODE_STDBY
    );


    /* Frequency */

    setFrequency433MHz();


    /* FIFO */

    regWrite(
        REG_FIFO_TX_BASE,
        0x00
    );

    regWrite(
        REG_FIFO_RX_BASE,
        0x00
    );


    /* LNA boost */

    regWrite(
        REG_LNA,
        0x23
    );


    /*
     * Modem:
     *
     * BW      = 125 kHz
     * CR      = 4/5
     */

    regWrite(
        REG_MODEM_CONFIG1,
        0x72
    );


    /*
     * SF7
     */

    regWrite(
        REG_MODEM_CONFIG2,
        0x74
    );


    /*
     * Low data rate optimization OFF
     */

    regWrite(
        REG_MODEM_CONFIG3,
        0x04
    );


    /* Preamble = 8 */

    regWrite(
        REG_PREAMBLE_MSB,
        0x00
    );

    regWrite(
        REG_PREAMBLE_LSB,
        0x08
    );


    /* TerraLink network sync word */

    regWrite(
        REG_SYNC_WORD,
        0xF3
    );


    /*
     * PA configuration
     *
     * PA_BOOST
     */

    regWrite(
        REG_PA_CONFIG,
        0x80 | 0x0D
    );


    /* Clear IRQ */

    regWrite(
        REG_IRQ_FLAGS,
        0xFF
    );


    /* Start RX */

    startReceive();


    return true;
}


/* ------------------------------------------------------------
 * Receive one LoRa packet
 * ------------------------------------------------------------ */

bool tl_lora_receive(TLPacket &packet)
{
    packet = TLPacket();


    uint8_t irq = regRead(REG_IRQ_FLAGS);


    /* No packet */

    if ((irq & IRQ_RX_DONE) == 0)
    {
        return false;
    }


    /* Clear RX_DONE */

    regWrite(
        REG_IRQ_FLAGS,
        IRQ_RX_DONE
    );


    /* CRC error */

    if (irq & IRQ_PAYLOAD_CRC_ERROR)
    {
        regWrite(
            REG_IRQ_FLAGS,
            IRQ_PAYLOAD_CRC_ERROR
        );

        startReceive();

        return false;
    }


    /* Number of received bytes */

    uint8_t length =
        regRead(REG_RX_NB_BYTES);


    /* FIFO current address */

    uint8_t currentAddress =
        regRead(REG_FIFO_RX_CURRENT);


    regWrite(
        REG_FIFO_ADDR_PTR,
        currentAddress
    );


    /* Limit packet size */

    if (length == 0 || length >= 250)
    {
        startReceive();

        return false;
    }


    char buffer[256];

    uint8_t i = 0;


    for (i = 0; i < length; i++)
    {
        buffer[i] =
            (char)regRead(REG_FIFO);
    }


    buffer[length] = '\0';


    /* Radio information */

    packet.rssi =
        (int)regRead(REG_PKT_RSSI) - 164;


    int8_t rawSnr =
        (int8_t)regRead(REG_PKT_SNR);

    packet.snr =
        rawSnr / 4;


    /* Parse packet */

    bool valid =
        tl_packet_parse(
            String(buffer),
            packet
        );


    /* Restore RX mode */

    startReceive();


    return valid;
}


/* ------------------------------------------------------------
 * Transmit LoRa packet
 * ------------------------------------------------------------ */

bool tl_lora_send(const String &raw)
{
    if (raw.length() == 0)
    {
        return false;
    }


    if (raw.length() >= 256)
    {
        return false;
    }


    /* Standby */

    regWrite(
        REG_OP_MODE,
        MODE_LONG_RANGE | MODE_STDBY
    );


    /* FIFO pointer */

    regWrite(
        REG_FIFO_ADDR_PTR,
        0x00
    );


    /* Write payload */

    for (uint16_t i = 0;
         i < raw.length();
         i++)
    {
        regWrite(
            REG_FIFO,
            (uint8_t)raw[i]
        );
    }


    /* Payload length */

    regWrite(
        0x22,
        (uint8_t)raw.length()
    );


    /* Clear TX IRQ */

    regWrite(
        REG_IRQ_FLAGS,
        IRQ_TX_DONE
    );


    /* TX */

    regWrite(
        REG_OP_MODE,
        MODE_LONG_RANGE | MODE_TX
    );


    /* Wait for TX_DONE */

    unsigned long start =
        millis();


    while (true)
    {
        uint8_t irq =
            regRead(REG_IRQ_FLAGS);


        if (irq & IRQ_TX_DONE)
        {
            regWrite(
                REG_IRQ_FLAGS,
                IRQ_TX_DONE
            );

            startReceive();

            return true;
        }


        if (millis() - start > 3000UL)
        {
            startReceive();

            return false;
        }


        yield();
    }
}