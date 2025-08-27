
#ifndef _OS_CSKY_STDLIB_H_
#define _OS_CSKY_STDLIB_H_
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "osal/sleep.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef os_malloc
#define os_malloc(s) malloc(s)
#endif
#ifndef os_free
#define os_free(p)   do{ free((void *)p); (p)=NULL;}while(0)
#endif
#ifndef os_zalloc
#define os_zalloc(s) zalloc(s)
#endif
#ifndef os_realloc
#define os_realloc(p,s) realloc(p,s)
#endif
#ifndef os_calloc
#define os_calloc(p,s) calloc(p, s)
#endif

#ifndef os_malloc_psram
#define os_malloc_psram(s) _os_malloc_psram(s)
#endif
#ifndef os_free_psram
#define os_free_psram(p)   do{ _os_free_psram((void *)p); (p)=NULL;}while(0)
#endif
#ifndef os_zalloc_psram
#define os_zalloc_psram(s) _os_zalloc_psram(s)
#endif
#ifndef os_realloc_psram
#define os_realloc_psram(p,s) _os_realloc_psram(p,s)
#endif
#ifndef os_calloc_psram
#define os_calloc_psram(p,s) _os_calloc_psram(p, s)
#endif


#ifdef MEM_TRACE
#define os_strcpy(d,s)     _os_strcpy((char *)(d), (const char *)(s))
#define os_strncpy(d,s,n)  _os_strncpy((char *)(d), (const char *)(s), n)
#define os_memset(s,c,n)   _os_memset((void *)(s), c, n)
#define os_memcpy(d,s,n)   _os_memcpy((void *)(d), (const void *)(s), n)
#define os_memmove(d,s,n)  _os_memmove((void *)(d), (const void *)(s), n)
#define os_sprintf(s, ...) _os_sprintf((char *)(s), __VA_ARGS__)
#define os_vsnprintf       _os_vsnprintf
#define os_snprintf        _os_snprintf
#else
#define os_strcpy(d,s)     strcpy((char *)(d), (const char *)(s))
#define os_strncpy(d,s,n)  strncpy((char *)(d), (const char *)(s), n)
#define os_memset(s,c,n)   memset((void *)(s), c, n)
#define os_memcpy(d,s,n)   memcpy((void *)(d), (const void *)(s), n)
#define os_memmove(d,s,n)  memmove((void *)(d), (const void *)(s), n)
#define os_sprintf(s, ...) sprintf((char *)(s), __VA_ARGS__)
#define os_vsnprintf       vsnprintf
#define os_snprintf        snprintf
#endif

#define os_abs             abs
#define os_atoi(c)         atoi((const char *)c)
#define os_atol(c)         atol((const char *)c)
#define os_atoll(c)        atoll((const char *)c)
#define os_atof(c)         atof((const char *)c)
#define os_strcmp(s1,s2)   strcmp((const char *)(s1), (const char *)(s2))
#define os_strstr(s1,s2)   strstr((const char *)(s1), (const char *)(s2))
#define os_strchr(s,c)     strchr((const char *)(s), c)
#define os_strlen(s)       strlen((const char *)(s))
#define os_memcmp(s1,s2,n) memcmp((const void *)(s1), (const void *)(s2), n)
#define os_strncmp(s1,s2,n) strncmp((const char *)s1, (const char *)s2, n)
#define os_strrchr(s,c)    strrchr((const char *)(s), c)
#define os_strncasecmp(s1,s2,n)  strncasecmp((const char *)(s1), (const char *)(s2), n)
#define os_strcasecmp(s1,s2)     strcasecmp((const char *)(s1), (const char *)(s2))
#define os_srand(v)        srand(v)
#define os_rand()          rand()
#define os_sscanf          sscanf
#define os_htons(x) ((((x) & (uint16)0x00ffUL) << 8) | (((x) & (uint16)0xff00UL) >> 8))
#define os_ntohs(x) os_htons(x)
#define os_htonl(x) ((((x) & (uint32)0x000000ffUL) << 24) | \
                     (((x) & (uint32)0x0000ff00UL) <<  8) | \
                     (((x) & (uint32)0x00ff0000UL) >>  8) | \
                     (((x) & (uint32)0xff000000UL) >> 24))
#define os_ntohl(x) os_htonl(x)

int strncasecmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
char *os_strdup(const char *s);
uint32 os_atoh(char *str);
uint64 os_atohl(char *str);

void *zalloc(size_t size);

#ifdef __cplusplus
}
#endif

#endif


