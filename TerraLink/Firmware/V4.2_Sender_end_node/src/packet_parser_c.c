#include "packet_parser_c.h"
#include <string.h>
#include <stdio.h>
bool tl_c_get_field(const char *packet,const char *key,char *out,size_t out_size){if(!packet||!key||!out||!out_size)return false;char k[64];int n=snprintf(k,sizeof(k),"%s:",key);if(n<=0||(size_t)n>=sizeof(k))return false;const char*s=strstr(packet,k);if(!s)return false;s+=strlen(k);const char*e=strchr(s,',');size_t len=e?(size_t)(e-s):strlen(s);if(len>=out_size)len=out_size-1;memcpy(out,s,len);out[len]='\0';return true;}
