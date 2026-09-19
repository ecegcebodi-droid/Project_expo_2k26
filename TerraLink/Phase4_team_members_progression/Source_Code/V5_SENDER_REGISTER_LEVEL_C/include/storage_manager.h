#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H
#include "terralink_types.h"
void storage_init(void);
void storage_load_sos(tl_persistent_sos_t *sos);
void storage_save_sos(const tl_persistent_sos_t *sos);
void storage_clear_sos(void);
#endif
