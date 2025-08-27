#ifndef _WEBRTC_TYPEDEF_H_
#define _WEBRTC_TYPEDEF_H_

#include "custom_mem/custom_mem.h"

#define bool               _Bool 

#ifndef INT8_MIN
#define INT8_MIN (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN (-32768)
#endif
#ifndef INT32_MIN
#define INT32_MIN (-2147483647 - 1)
#endif
#ifndef INT64_MIN
#define INT64_MIN  (-9223372036854775807LL - 1)
#endif

#ifndef INT8_MAX
#define INT8_MAX 127
#endif
#ifndef INT16_MAX
#define INT16_MAX 32767
#endif
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

#ifndef UINT8_MAX
#define UINT8_MAX 255
#endif
#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffffU  /* 4294967295U */
#endif
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL /* 18446744073709551615ULL */
#endif

#define true	1
#define false	0

typedef unsigned int         size_t ;
typedef signed int           int32_t;
typedef signed short         int16_t;
typedef signed char          int8_t;
typedef unsigned int         uint32_t;
typedef unsigned short       uint16_t;
typedef unsigned char        uint8_t;
typedef long long            int64_t;
typedef unsigned long long   uint64_t;

#ifdef PSRAM_HEAP
#define webrtc_malloc custom_malloc_psram
#define webrtc_calloc custom_calloc_psram
#define webrtc_free   custom_free_psram
#define webrtc_zalloc custom_zalloc_psram
#else
#define webrtc_malloc custom_malloc
#define webrtc_calloc custom_calloc
#define webrtc_free   custom_free
#endif

#define webrtc_memset os_memset
#define webrtc_memcpy os_memcpy

#endif