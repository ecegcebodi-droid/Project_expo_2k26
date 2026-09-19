#include <Arduino.h>
#include <SoftwareSerial.h>
#include "tl_config.h"
#include "tl_uart.h"
#include "tl_incident.h"
#include "tl_state.h"

static SoftwareSerial tl_rescue_serial(TL_UART_RX_PIN, TL_UART_TX_PIN);
static String tl_uart_buffer;

static void tl_process_uart_line(const String &line) {
    if (line.startsWith("SOS,"))             tl_process_sos(line);
    else if (line.startsWith("LOCATION,"))  tl_process_location(line);
    else if (line.startsWith("ACK,"))       tl_process_ack(line);
    else if (line.startsWith("MSG_ACK,"))   tl_process_msg_ack(line);
    else if (line.startsWith("RESOLVE_ACK,")) tl_process_resolve_ack(line);
    else if (line.startsWith("RX1_STATUS,"))  tl_process_rx1_status(line);
    else if (line.startsWith("TX_STATUS,"))   tl_process_tx_status(line);
    else if (line == "RX1,ONLINE") {
        tl_rx1_online = true;
        tl_rx1_last_seen = millis();
    }
}

void tl_uart_init(void) {
    tl_rescue_serial.begin(TL_UART_BAUD);
    tl_uart_buffer.reserve(256);
}

void tl_uart_process(void) {
    while (tl_rescue_serial.available()) {
        char c = (char)tl_rescue_serial.read();

        if (c == '\n') {
            tl_uart_buffer.trim();
            if (tl_uart_buffer.length()) tl_process_uart_line(tl_uart_buffer);
            tl_uart_buffer = "";
        } else if (c != '\r') {
            if (tl_uart_buffer.length() < 250) {
                tl_uart_buffer += c;
            } else {
                tl_uart_buffer = "";
            }
        }
    }
}

void tl_uart_send_command(const char *type, unsigned int node, const char *payload) {
    String out = "TL,";
    out += type;
    out += ",";
    out += String(node);

    if (payload && payload[0]) {
        out += ",";
        for (const char *p = payload; *p; ++p) {
            if (*p != '\r' && *p != '\n' && *p != ',') out += *p;
        }
    }

    tl_rescue_serial.println(out);
    tl_rescue_serial.flush();

    Serial.print(F("[UART TX] "));
    Serial.println(out);
}
