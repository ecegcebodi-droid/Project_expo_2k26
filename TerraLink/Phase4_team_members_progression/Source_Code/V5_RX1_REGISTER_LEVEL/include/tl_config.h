#ifndef TL_CONFIG_H
#define TL_CONFIG_H

#include <Arduino.h>

#define TL_NODE_ID             4
#define TL_BROADCAST_ID        255
#define TL_LORA_FREQUENCY      433000000UL
#define TL_LORA_SYNC_WORD      0xF3
#define TL_LORA_SF             7
#define TL_LORA_BW_HZ          125000UL
#define TL_LORA_CR_DEN         5
#define TL_LORA_TX_POWER       17
#define TL_DEFAULT_TTL         5
#define TL_MAX_HOP             15


/* Physical mapping: RX1 -> SX1278 RA-02 */
#define TL_PIN_LORA_SS         5   // D1 / GPIO5
#define TL_PIN_LORA_RST        16  // D0 / GPIO16
#define TL_PIN_LORA_SCK        14
#define TL_PIN_LORA_MISO       12
#define TL_PIN_LORA_MOSI       13
/* DIO0 is intentionally not connected; RX1 polls RegIrqFlags. */

/* 16x2 I2C LCD through PCF8574 */
#define TL_PIN_I2C_SDA         4   // D2 / GPIO4
#define TL_PIN_I2C_SCL         2   // D4 / GPIO2
#define TL_LCD_ADDR             0x27

/* Buzzer */
#define TL_PIN_BUZZER          15  // D8 / GPIO15

/* RX1 -> RX2 UART. TX only is required for gateway telemetry. */
#define TL_PIN_RX2_TX          0   // D3 / GPIO0
#define TL_UART_BAUD           9600

#define TL_MAX_PACKET_LEN      220
#define TL_UART_LINE_LEN       220
#define TL_DEDUP_SIZE          24
#define TL_MAX_NODES           32
#define TL_MAX_SEEN_PACKETS    64

#define TL_LCD_HOME_MS         2500UL
#define TL_SOS_BEEP_ON_MS      180UL
#define TL_SOS_BEEP_OFF_MS     120UL

#endif
