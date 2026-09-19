#ifndef TERRALINK_CONFIG_H
#define TERRALINK_CONFIG_H

/* =========================================================
                      RX2 CONFIGURATION
   ========================================================= */

#define RX2_NODE_ID             5
#define RX1_NODE_ID             4

/* =========================================================
                         UART
   ========================================================= */

#define UART_BAUD               9600

/*
   RX2 receives from RX1:
       RX1 TX D3 -> RX2 RX D8

   RX2 sends to RX1:
       RX2 D0 -> RX1 GPIO3/RX
*/

#define RX1_RX_PIN              D8
#define RX1_TX_PIN              D0

/* =========================================================
                       KEYPAD
   ========================================================= */

/*
        PmodKYPD 4x4

        Columns:
        D1 D2 D5 D6

        Rows:
        D7 D0 D4 D3
*/

#define KEY_COL_1               D1
#define KEY_COL_2               D2
#define KEY_COL_3               D5
#define KEY_COL_4               D6

#define KEY_ROW_1               D7
#define KEY_ROW_2               D0
#define KEY_ROW_3               D4
#define KEY_ROW_4               D3

#define KEYPAD_ROWS             4
#define KEYPAD_COLS             4

/* =========================================================
                       LCD / DISPLAY
   ========================================================= */

/*
   If your RX2 does not currently use an LCD,
   keep this disabled.
*/

#define RX2_LCD_ENABLED         0

/* =========================================================
                       WEB SERVER
   ========================================================= */

#define WEB_SERVER_ENABLED      1

#define WIFI_AP_NAME            "TerraLink_Rescue"
#define WIFI_AP_PASSWORD        "terralink123"

#define WEB_SERVER_PORT         80

/* =========================================================
                     STORAGE
   ========================================================= */

#define MAX_NODES               20
#define MAX_INCIDENTS           20
#define MAX_MESSAGES            50
#define MAX_NETWORK_ENTRIES     30

/* =========================================================
                     PROTOCOL
   ========================================================= */

#define DEFAULT_TTL             5

#define BROADCAST_ID            255

#endif