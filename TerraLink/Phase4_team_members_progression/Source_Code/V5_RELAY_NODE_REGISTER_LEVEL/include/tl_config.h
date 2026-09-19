#ifndef TL_CONFIG_H
#define TL_CONFIG_H

#include <stdint.h>

#ifndef TL_RELAY_NODE_ID
#define TL_RELAY_NODE_ID 2U
#endif

#define TL_SENDER_NODE_ID      1U
#define TL_RESCUE_NODE_ID      4U
#define TL_BROADCAST_ID        255U

#define TL_MAX_TTL              5U
#define TL_ROUTE_COUNT          8U
#define TL_NEIGHBOUR_COUNT      8U
#define TL_DUPLICATE_COUNT     12U

#define TL_ROUTE_TIMEOUT_MS     60000UL
#define TL_NEIGHBOUR_TIMEOUT_MS 30000UL
#define TL_DUPLICATE_TIMEOUT_MS 30000UL
#define TL_HELLO_INTERVAL_MS    10000UL
#define TL_STATUS_INTERVAL_MS   15000UL

#define TL_FORWARD_DELAY_MIN    20U
#define TL_FORWARD_DELAY_MAX    70U

#define TL_PACKET_BUFFER_SIZE   190U

/* Arduino UNO / RA-02 */
#define TL_PIN_LORA_NSS         10U  /* PB2 */
#define TL_PIN_LORA_RST          9U  /* PB1 */
#define TL_PIN_LORA_DIO0         2U  /* PD2, retained for hardware compatibility */

#define TL_LORA_FREQUENCY_HZ 433000000UL
#define TL_LORA_SYNC_WORD       0xF3U
#define TL_LORA_SF              7U
#define TL_LORA_BW              125000UL
#define TL_LORA_CR              5U
#define TL_LORA_TX_POWER        17U

#define TL_LORA_FRF_MSB         0x6CU
#define TL_LORA_FRF_MID         0x40U
#define TL_LORA_FRF_LSB         0x00U

#define TL_SERIAL_BAUD          9600UL

#endif
