#include "message_manager.h"
#include "packet_parser.h"
#include "packet_protocol.h"
#include "sx1278_driver.h"
#include "buzzer_manager.h"
#include "display_manager.h"
#include "terralink_config.h"
#include <string.h>

static uint8_t last_message_id;
static uint32_t last_message_sequence;

void message_manager_init(void)
{
    last_message_id = 0;
    last_message_sequence = 0;
}

void message_manager_process_packet(const char *packet)
{
    if (!packet || !packet_has_type(packet, "MSG"))
        return;

    tl_packet_t p;
    if (!packet_parse(packet, &p))
        return;

    /* Receiver -> sender message; sequence is not tied to active SOS sequence. */
    if (p.dst != TERRALINK_NODE_ID)
        return;

    if (p.msg_id == last_message_id && p.sequence == last_message_sequence)
        return;

    last_message_id = p.msg_id;
    last_message_sequence = p.sequence;

    buzzer_rescue_message();
    display_rescue_message(p.message);

    tl_packet_t ack = p;
    ack.src = TERRALINK_NODE_ID;
    ack.dst = TERRALINK_RECEIVER_NODE_ID;

    char ack_text[192];
    if (packet_create_msgack(ack_text, sizeof(ack_text), &ack))
        sx1278_send((const uint8_t *)ack_text, (uint8_t)strlen(ack_text));
}

void message_manager_task(void)
{
    uint8_t buffer[255];
    int16_t rssi;

    int len = sx1278_receive(buffer, sizeof(buffer) - 1, &rssi);
    if (len > 0)
    {
        buffer[len] = 0;
        message_manager_process_packet((const char *)buffer);
    }
}
