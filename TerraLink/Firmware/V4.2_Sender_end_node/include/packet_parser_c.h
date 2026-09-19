#pragma once
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
bool tl_c_get_field(const char *packet,const char *key,char *out,size_t out_size);
#ifdef __cplusplus
}
#endif
