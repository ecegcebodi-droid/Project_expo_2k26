#ifndef SX1278_DRIVER_H
#define SX1278_DRIVER_H
#include <stdint.h>
#include <stdbool.h>
bool sx1278_init(void);
uint8_t sx1278_read_reg(uint8_t address);
void sx1278_write_reg(uint8_t address, uint8_t value);
bool sx1278_send(const uint8_t *data, uint16_t len);
int sx1278_receive(uint8_t *buffer, uint8_t max_len, int16_t *rssi);
void sx1278_receive_mode(void);
#endif
