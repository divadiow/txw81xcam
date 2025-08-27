
#ifndef _SYS_HEAP_H_
#define _SYS_HEAP_H_

#include "lib/heap/mmpool.h"

enum SYSHEAP_FLAGS {
    SYSHEAP_FLAGS_MEM_LEAK_TRACE     = (1u << 0),
    SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK = (1u << 1),
    SYSHEAP_FLAGS_MEM_ALIGN_16       = (1u << 2),
};

//#define MMPOOL3

#ifdef MMPOOL3
#define MMPOOL mmpool3
#elif defined(MMPOOL2)
#define MMPOOL mmpool2
#else
#define MMPOOL mmpool
#endif

struct sys_heap {
    const char *name;
    uint32 flags;
    struct MMPOOL pool;
};

int32 sysheap_init(struct sys_heap *heap, void *heap_start, unsigned int heap_size, unsigned int flags);
uint32 sysheap_time(struct sys_heap *heap);
void *sysheap_alloc(struct sys_heap *heap, int size, const char *func, int line);
void sysheap_free(struct sys_heap *heap, void *ptr);
uint32 sysheap_freesize(struct sys_heap *heap);
uint32 sysheap_totalsize(struct sys_heap *heap);
void sysheap_collect_init(struct sys_heap *heap, uint32 start, uint32 end);
int32 sysheap_add(struct sys_heap *heap, void *heap_start, uint32 heap_size);
int32 sysheap_of_check(struct sys_heap *heap, void *ptr, uint32 size);
void sysheap_status(struct sys_heap *heap, uint32 *status_buf, int32 buf_size, uint32 mini_size);
int32 sysheap_valid_addr(struct sys_heap *heap, void *ptr);
uint32 sysheap_time(struct sys_heap *heap);
int sysheap_use_list(struct sys_heap *heap, uint32_t *list_buf, int32 buf_size);
void sysheap_dump(struct sys_heap *heap);

extern struct sys_heap sram_heap;
extern struct sys_heap psram_heap;

#endif

