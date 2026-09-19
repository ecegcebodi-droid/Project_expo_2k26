#include "sequence_manager.h"
#include "storage_manager.h"
#include "terralink_types.h"

static uint32_t sequence_counter;

void sequence_init(void)
{
    /* Sequence persistence can be stored through the same NVS C backend. */
    sequence_counter = 0;
}

uint32_t sequence_next(void)
{
    return ++sequence_counter;
}
