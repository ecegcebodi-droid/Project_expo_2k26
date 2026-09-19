#ifndef TL_RELAY_H
#define TL_RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

void tl_relay_init(void);
void tl_relay_process(void);
void tl_relay_periodic(void);

#ifdef __cplusplus
}
#endif

#endif
