#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H
#include "terralink_types.h"
void gps_init(void);
void gps_task(void);
const tl_gps_data_t *gps_get_data(void);
#endif
