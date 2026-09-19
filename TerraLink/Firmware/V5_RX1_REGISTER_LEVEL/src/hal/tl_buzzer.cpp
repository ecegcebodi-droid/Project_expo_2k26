#include "tl_buzzer.h"
#include "tl_registers.h"
#include "tl_config.h"

void tl_buzzer_init(){tl_gpio_mode_output(TL_PIN_BUZZER,false);}
void tl_buzzer_sos(){for(int i=0;i<3;i++){tl_gpio_high(TL_PIN_BUZZER);delay(TL_SOS_BEEP_ON_MS);tl_gpio_low(TL_PIN_BUZZER);delay(TL_SOS_BEEP_OFF_MS);}}
void tl_buzzer_test(){tl_gpio_high(TL_PIN_BUZZER);delay(250);tl_gpio_low(TL_PIN_BUZZER);}
