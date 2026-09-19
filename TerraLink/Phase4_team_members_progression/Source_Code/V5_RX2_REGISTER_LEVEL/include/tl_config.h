#ifndef TL_CONFIG_H
#define TL_CONFIG_H

/* TerraLink RX2 — hardware configuration */
#define TL_UART_RX_PIN  15   /* D8 / GPIO15 */
#define TL_UART_TX_PIN  16   /* D0 / GPIO16 */
#define TL_UART_BAUD    9600

#define TL_COL1_PIN 5       /* D1 */
#define TL_COL2_PIN 4       /* D2 */
#define TL_COL3_PIN 14      /* D5 */
#define TL_COL4_PIN 12      /* D6 */
#define TL_ROW1_PIN 13      /* D7 */
#define TL_ROW2_PIN 3       /* RX / GPIO3 */
#define TL_ROW3_PIN 2       /* D4 */
#define TL_ROW4_PIN 0       /* D3 */

#define TL_DEBUG_BAUD 115200

#define TL_MAX_NODES     15
#define TL_MAX_INCIDENTS 20
#define TL_MAX_MESSAGES  25

#define TL_AP_SSID     "TerraLink-RX2"
#define TL_AP_PASSWORD "terralink123"
#define TL_RX1_TIMEOUT_MS 10000UL

#endif
