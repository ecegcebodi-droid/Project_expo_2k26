#ifndef BUZZER_MANAGER_H
#define BUZZER_MANAGER_H
#include <stdint.h>

void buzzer_init(void);
void buzzer_off(void);
void buzzer_on(void);
void buzzer_beep(uint32_t duration_ms);
void buzzer_sos_activated(void);
void buzzer_success(void);
void buzzer_failure(void);
void buzzer_rescue_message(void);
#endif
