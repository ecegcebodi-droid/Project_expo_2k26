#ifndef TERRALINK_CONFIG_H
#define TERRALINK_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* Node / protocol */
#define TERRALINK_NODE_ID             1
#define TERRALINK_RECEIVER_NODE_ID    4
#define TERRALINK_BROADCAST_ID        255
#define TERRALINK_SOS_TTL             5
#define TERRALINK_REVERSE_TTL         5

/* Timing */
#define TL_SOS_LONG_PRESS_MS          2000
#define TL_ACK_TIMEOUT_MS             15000
#define TL_SOS_RETRY_INTERVAL_SEC     30
#define TL_NORMAL_SLEEP_INTERVAL_SEC  60
#define TL_MAX_RESCUE_MESSAGE_LENGTH  120
#define TL_RESCUE_QUEUE_SIZE          5

/* GPIO */
#define TL_PIN_SOS_TOUCH              32
#define TL_PIN_LOCAL_ALARM            33
#define TL_PIN_LED_BUTTON             13
#define TL_PIN_BUZZER                 25
#define TL_PIN_TORCH                  4

/* SPI / SX1278 */
#define TL_LORA_SCK                   18
#define TL_LORA_MISO                  19
#define TL_LORA_MOSI                  23
#define TL_LORA_NSS                   5
#define TL_LORA_RST                   27
#define TL_LORA_DIO0                  26
#define TL_LORA_FREQUENCY_HZ          433000000UL
#define TL_LORA_SYNC_WORD             0xF3
#define TL_LORA_SF                    7
#define TL_LORA_BW_HZ                 125000UL
#define TL_LORA_CR_DENOM              5
#define TL_LORA_TX_POWER_DBM          17

/* UART2 / GPS */
#define TL_GPS_UART_PORT              2
#define TL_GPS_RX                     16
#define TL_GPS_TX                     17
#define TL_GPS_BAUD                   9600

/* I2C / SH1106 */
#define TL_I2C_PORT                   0
#define TL_OLED_SDA                   21
#define TL_OLED_SCL                   22
#define TL_OLED_ADDRESS               0x3C
#define TL_OLED_WIDTH                 128
#define TL_OLED_HEIGHT                64

#endif
