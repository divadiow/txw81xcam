
#ifndef __CSKY_TYPESDEF_H_
#define __CSKY_TYPESDEF_H_
#include <stdint.h>
#include <stdbool.h>
#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

//typedef enum { FALSE=0, TRUE=1, false=0, true=1} bool;
typedef signed char          int8;
typedef signed short         int16;
typedef signed int           int32;
typedef signed long long     int64;

typedef unsigned char        uint8;
typedef unsigned short       uint16;
typedef unsigned int         uint32;
typedef unsigned long long   uint64;

#ifndef _SIZE_T_DECLARED
typedef	unsigned int	size_t;
#define	_SIZE_T_DECLARED
#endif

#ifndef _SSIZE_T_DECLARED
#if defined(__INT_MAX__) && __INT_MAX__ == 2147483647
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define	_SSIZE_T_DECLARED
#endif

typedef unsigned long ulong;

#ifdef __cplusplus
}
#endif

#include "byteshift.h"
#endif


