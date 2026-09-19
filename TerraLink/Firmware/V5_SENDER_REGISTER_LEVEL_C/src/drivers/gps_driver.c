#include "gps_driver.h"
#include "uart_driver.h"
#include "terralink_config.h"
#include <string.h>
#include <stdlib.h>

static tl_gps_data_t gps_data;

static void nmea_process_byte(uint8_t c)
{
    /*
     * C NMEA parser hook.
     * Implement GGA/RMC field parsing here without TinyGPSPlus.
     */
    (void)c;
}

void gps_init(void)
{
    memset(&gps_data, 0, sizeof(gps_data));
    uart2_init(TL_GPS_BAUD);
}

void gps_task(void)
{
    while (uart2_available())
        nmea_process_byte(uart2_read_byte());
}

const tl_gps_data_t *gps_get_data(void)
{
    return &gps_data;
}
