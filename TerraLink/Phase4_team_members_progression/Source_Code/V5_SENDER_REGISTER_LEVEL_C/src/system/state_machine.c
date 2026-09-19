#include "state_machine.h"
#include "terralink_globals.h"
#include "terralink_config.h"
#include "display_manager.h"
#include "sos_manager.h"
#include "timer_driver.h"

static uint64_t state_start_ms;

void state_machine_init(void)
{
    g_state = TL_STATE_AWAKENING;
    state_start_ms = timer_millis();
}

void state_machine_set_sos(void)
{
    g_state = TL_STATE_SOS_ACTIVATED;
    state_start_ms = timer_millis();
    sos_request();
}

void state_machine_task(void)
{
    if (g_state == TL_STATE_AWAKENING)
    {
        display_ready();
        g_state = TL_STATE_READY;
        state_start_ms = timer_millis();
    }

    display_state(g_state);
}
