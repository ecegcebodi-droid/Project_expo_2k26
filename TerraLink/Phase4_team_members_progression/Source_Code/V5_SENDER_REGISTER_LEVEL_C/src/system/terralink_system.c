#include "terralink_system.h"
#include "terralink_globals.h"
#include "gpio_driver.h"
#include "spi_driver.h"
#include "uart_driver.h"
#include "i2c_driver.h"
#include "timer_driver.h"
#include "interrupt_driver.h"
#include "power_driver.h"
#include "sx1278_driver.h"
#include "gps_driver.h"
#include "sh1106_driver.h"
#include "storage_manager.h"
#include "sequence_manager.h"
#include "routing_manager.h"
#include "sos_manager.h"
#include "message_manager.h"
#include "input_manager.h"
#include "buzzer_manager.h"
#include "display_manager.h"
#include "state_machine.h"

void terralink_system_init(void)
{
    gpio_init();
    spi_init();
    uart2_init(9600);
    i2c_init();
    interrupt_init();
    power_init();

    buzzer_init();
    display_init();
    gps_init();

    storage_init();
    sequence_init();
    routing_init();
    sos_init();
    message_manager_init();
    input_init();
    state_machine_init();

    g_lora_ready = sx1278_init();
    display_starting();
}

void terralink_system_start(void)
{
    /* Application scheduler loop. RTOS tasks can be introduced here later. */
    while (1)
    {
        gps_task();
        input_task();
        message_manager_task();
        sos_task();
        state_machine_task();

        timer_delay_ms(10);
    }
}
