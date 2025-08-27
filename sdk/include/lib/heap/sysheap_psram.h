
#ifndef _SYS_HEAP_PSRAM_H_
#define _SYS_HEAP_PSRAM_H_
#include "sysheap.h"    //主要为了共用SYSHEAP_FLAGS


int sysheap_psram_init(void *heap_start, unsigned int heap_size, unsigned int flags);
void *sysheap_psram_alloc(int size, const char *func, int line);
void sysheap_psram_free(void *ptr);
void sysheap_psram_collect_init(void);
int32 sysheap_psram_of_check(void *ptr, uint32 size);
void psram_mm_init(void *start,uint32 size);
#ifdef MPOOL_ALLOC
void sysheap_psram_status(void);
#else
#define sysheap_status()
#endif

#endif

