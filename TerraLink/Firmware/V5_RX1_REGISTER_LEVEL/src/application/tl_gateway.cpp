#include "tl_gateway.h"

#include "tl_lora.h"
#include "tl_packet.h"
#include "tl_state.h"
#include "tl_uart.h"
#include "tl_i2c_lcd.h"
#include "tl_buzzer.h"
#include "tl_config.h"


/*
 * ============================================================
 *                 TERRALINK RECEIVER #1
 *                    GATEWAY APPLICATION
 * ============================================================
 *
 * RX1 responsibilities:
 *
 * 1. Receive LoRa packets
 * 2. Validate packets
 * 3. Detect duplicates
 * 4. Update node state
 * 5. Handle SOS
 * 6. Handle MSG
 * 7. Generate ACK / MSGACK
 * 8. Send events to RX2
 * 9. Display gateway status
 *
 * RX1 DOES NOT ROUTE PACKETS.
 *
 * Relay nodes perform routing.
 * RX1 is the final rescue station.
 * ============================================================
 */


static unsigned long lastStatusTime = 0;


/* ------------------------------------------------------------
 * Initialization
 * ------------------------------------------------------------ */

void tl_gateway_init()
{
    tl_state_init();

    tl_lcd_init();

    tl_buzzer_init();

    tl_uart_init();


    tl_lcd_message(
        "TERRALINK RX1",
        "INITIALIZING"
    );


    delay(300);


    if (tl_lora_init())
    {
        tl_lcd_message(
            "TERRALINK RX1",
            "NETWORK READY"
        );

        tl_uart_send(
            "RX1,STATUS,READY"
        );
    }
    else
    {
        tl_lcd_message(
            "TERRALINK RX1",
            "LORA ERROR"
        );

        tl_uart_send(
            "RX1,STATUS,LORA_ERROR"
        );
    }
}


/* ------------------------------------------------------------
 * Packet handler
 * ------------------------------------------------------------ */

void tl_gateway_on_packet(const TLPacket &packet)
{
    if (!packet.valid)
    {
        return;
    }


    /* Duplicate protection */

    if (tl_state_is_duplicate(packet))
    {
        return;
    }


    /* Update node database */

    tl_state_update(packet);

    tl_state_note_rx();


    /* --------------------------------------------------------
     * SOS
     * -------------------------------------------------------- */

    if (packet.type == "SOS")
    {
        tl_lcd_message(
            "!!! SOS !!!",
            "NODE " + String(packet.src)
        );


        tl_buzzer_sos();


        String event =
            "EVENT,SOS," +
            String(packet.src) + "," +
            String(packet.seq) + "," +
            String(packet.lat, 6) + "," +
            String(packet.lon, 6) + "," +
            String(packet.rssi) + "," +
            String(packet.ttl) + "," +
            String(packet.hop);


        tl_uart_send(event);


        /* Send ACK to originating node */

        String ack = tl_packet_ack(packet);

        if (tl_lora_send(ack))
        {
            tl_state_note_tx();
            tl_state_note_ack();
        }


        return;
    }


    /* --------------------------------------------------------
     * MSG
     * -------------------------------------------------------- */

    if (packet.type == "MSG")
    {
        tl_lcd_message(
            "MESSAGE",
            "NODE " + String(packet.src)
        );


        String event =
            "EVENT,MSG," +
            String(packet.src) + "," +
            String(packet.seq) + "," +
            String(packet.msgId) + "," +
            packet.msg;


        tl_uart_send(event);


        /* Message acknowledgement */

        if (packet.hasMsgId)
        {
            String msgAck = tl_packet_msgack(packet);

            if (tl_lora_send(msgAck))
            {
                tl_state_note_tx();
                tl_state_note_ack();
            }
        }


        return;
    }


    /* --------------------------------------------------------
     * ACK
     * -------------------------------------------------------- */

    if (packet.type == "ACK")
    {
        tl_uart_send(
            "EVENT,ACK," +
            String(packet.src) + "," +
            String(packet.seq)
        );

        return;
    }


    /* --------------------------------------------------------
     * MSGACK
     * -------------------------------------------------------- */

    if (packet.type == "MSGACK")
    {
        tl_uart_send(
            "EVENT,MSGACK," +
            String(packet.src) + "," +
            String(packet.msgId)
        );

        return;
    }


    /* --------------------------------------------------------
     * LOC
     * -------------------------------------------------------- */

    if (packet.type == "LOC")
    {
        tl_uart_send(
            "EVENT,LOC," +
            String(packet.src) + "," +
            String(packet.lat, 6) + "," +
            String(packet.lon, 6)
        );

        return;
    }


    /* --------------------------------------------------------
     * Any other valid packet
     * -------------------------------------------------------- */

    tl_uart_send(
        "EVENT,PACKET," +
        String(packet.src) + "," +
        packet.type +
        ",RSSI:" +
        String(packet.rssi)
    );
}


/* ------------------------------------------------------------
 * Main gateway loop
 * ------------------------------------------------------------ */

void tl_gateway_loop()
{
    TLPacket packet;


    /* Receive LoRa packet */

    if (tl_lora_receive(packet))
    {
        tl_gateway_on_packet(packet);
    }


    /* Periodic gateway status */

    if (millis() - lastStatusTime >= 10000UL)
    {
        lastStatusTime = millis();


        String status = tl_packet_status();

        tl_uart_send(status);


        tl_lcd_message(
            "RX1 ONLINE",
            "RX:" +
            String(tl_state_rx_count()) +
            " TX:" +
            String(tl_state_tx_count())
        );
    }
}