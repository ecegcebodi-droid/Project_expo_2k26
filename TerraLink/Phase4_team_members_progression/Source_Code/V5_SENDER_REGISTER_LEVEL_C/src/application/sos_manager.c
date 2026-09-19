#include "sos_manager.h"
#include "terralink_config.h"
#include "terralink_globals.h"
#include "terralink_types.h"
#include "sequence_manager.h"
#include "storage_manager.h"
#include "sx1278_driver.h"
#include "packet_protocol.h"
#include "buzzer_manager.h"
#include "timer_driver.h"
#include <string.h>

void sos_init(void)
{
    storage_load_sos(&g_persistent_sos);

    if (g_persistent_sos.pending)
        g_active_sequence = g_persistent_sos.sequence;
}

void sos_request(void)
{
    tl_packet_t p;
    char packet[256];

    memset(&p, 0, sizeof(p));
    p.src = TERRALINK_NODE_ID;
    p.dst = TERRALINK_BROADCAST_ID;
    p.sequence = g_persistent_sos.pending
                    ? g_persistent_sos.sequence
                    : sequence_next();
    p.ttl = TERRALINK_SOS_TTL;

    p.latitude = g_gps.latitude;
    p.longitude = g_gps.longitude;

    g_active_sequence = p.sequence;

    if (!g_persistent_sos.pending)
    {
        g_persistent_sos.pending = true;
        g_persistent_sos.sequence = p.sequence;
        g_persistent_sos.latitude = p.latitude;
        g_persistent_sos.longitude = p.longitude;
        g_persistent_sos.gps_fix = g_gps.fix;
        storage_save_sos(&g_persistent_sos);
    }

    if (packet_create_sos(packet, sizeof(packet), &p))
    {
        buzzer_sos_activated();
        sx1278_send((const uint8_t *)packet, (uint8_t)strlen(packet));
    }
}

void sos_retry(void)
{
    if (g_persistent_sos.pending)
        sos_request();
}

void sos_task(void)
{
    /* ACK timeout/retry state machine is implemented here. */
}
