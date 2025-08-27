/*
    User defined heap pool
*/

#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/mpool.h"
#include "lib/heap/mmpool.h"
#include "k_api.h"

enum SYSHEAP_FLAGS {
    SYSHEAP_FLAGS_MEM_LEAK_TRACE     = (1u << 0),
    SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK = (1u << 1),
    SYSHEAP_FLAGS_MEM_ALIGN_16       = (1u << 2),
};

/////////////////////////////////////////////////////
/*         选择使用 哪个分配算法模块                           */
#define MMPOOL3 //使用 mmpool3
//#define MMPOOL2 //使用 mmpool2
//默认就是使用 mmpool1
/////////////////////////////////////////////////////

#ifdef MMPOOL3
#define MMPOOL mmpool3
#define MPOOL_INIT(p,b,a,s)   mmpool3_init(p,a,s,b)
#define MPOOL_MALLOC(p,s,f,l) mmpool3_alloc(p,s,f,l)
#define MPOOL_FREE(mp, p)     mmpool3_free(mp, p)
#define MPOOL_ADD(mp,a,s,b)   mmpool3_add_region(mp,a,s,b)
#define MPOOL_FREE_STATE(mp,b,s,t) mmpool3_free_state(mp,b,s,t)
#define MPOOL_USED_STATE(mp,b,s,t,ms) mmpool3_used_state(mp,b,s,t,ms)
#define MPOOL_FREE_SIZE(mp,s) mmpool3_free_size(mp, s)
#define MPOOL_OF_CHECK(mp,a,s) mmpool3_of_check(mp,a,s)
#define MPOOL_TOTAL_SIZE(mp) (mp)->size
#define MPOOL_VALID_ADDR(mp, addr) mmpool3_valid_addr(mp, addr)
#define MPOOL_ALLOC_PERF(mp, values, count) mmpool3_perfermance(mp, 1, values, count)
#define MPOOL_FREE_PERF(mp, values, count) mmpool3_perfermance(mp, 0, values, count)
#define MPOOL_TIME(mp, tot_alloc, tot_free) mmpool3_time(mp, tot_alloc, tot_free)

#elif defined(MMPOOL2)
#define MMPOOL mmpool2
#define MPOOL_INIT(p,b,a,s)   mmpool2_init(p,a,s)
#define MPOOL_MALLOC(p,s,f,l) mmpool2_alloc(p,s,f,l)
#define MPOOL_FREE(mp, p)     mmpool2_free(mp, p)
#define MPOOL_ADD(mp,a,s,b)   mmpool2_add_region(mp,a,s)
#define MPOOL_FREE_STATE(mp,b,s,t) mmpool2_free_state(mp,b,s,t)
#define MPOOL_USED_STATE(mp,b,s,t,ms) mmpool2_used_state(mp,b,s,t,ms)
#define MPOOL_FREE_SIZE(mp,s) mmpool2_free_size(mp, s)
#define MPOOL_OF_CHECK(mp,a,s) mmpool2_of_check(mp,a,s)
#define MPOOL_TOTAL_SIZE(mp) (mp)->size
#define MPOOL_VALID_ADDR(mp, addr) mmpool2_valid_addr(mp, addr)
#define MPOOL_ALLOC_PERF(mp, values, count) mmpool2_perfermance(mp, 1, values, count)
#define MPOOL_FREE_PERF(mp, values, count) mmpool2_perfermance(mp, 0, values, count)
#define MPOOL_TIME(mp, tot_alloc, tot_free) mmpool2_time(mp, tot_alloc, tot_free)

#else
#define MMPOOL mmpool
#define MPOOL_INIT(p,b,a,s)   mmpool_init(p,a,s)
#define MPOOL_MALLOC(p,s,f,l) mmpool_alloc(p,s,f,l)
#define MPOOL_FREE(mp, p)     mmpool_free(mp, p)
#define MPOOL_ADD(mp,a,s,b)   mmpool_add_region(mp, a, s)
#define MPOOL_FREE_STATE(mp, b, s, t) mmpool_free_state(mp, b, s, t)
#define MPOOL_USED_STATE(mp, b, s, t, ms) mmpool_used_state(mp, b, s, t, ms)
#define MPOOL_FREE_SIZE(mp,  s) mmpool_free_size(mp, s)
#define MPOOL_OF_CHECK(mp,a,s) mmpool_of_check(mp,a,s)
#define MPOOL_TOTAL_SIZE(mp) (mp)->size
#define MPOOL_VALID_ADDR(mp, addr) mmpool_valid_addr(mp, (uint32)addr)
#define MPOOL_ALLOC_PERF(mp, values, count) mmpool_perfermance(mp, 1, values, count)
#define MPOOL_FREE_PERF(mp, values, count) mmpool_perfermance(mp, 0, values, count)
#define MPOOL_TIME(mp, tot_alloc, tot_free) mmpool_time(mp, tot_alloc, tot_free)
#endif

struct MMPOOL user_pool;
__init int32 uheap_init(void *heap_start, uint32 heap_size, uint32 flags)
{
    memset(&user_pool, 0, sizeof(struct MMPOOL));
    user_pool.headsize = 4;

    /* trace_off, ofchk_off 最大值为15，小心溢出*/
    if (flags & SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK) {
        user_pool.ofchk_off = user_pool.headsize;
        user_pool.headsize += 4;
        user_pool.tailsize  = 4;
        //os_printf("HEAP: OFCHK enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_LEAK_TRACE) {
        user_pool.trace_off = user_pool.headsize;
        user_pool.headsize += 20;
        //os_printf("HEAP: TRACE enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_ALIGN_16) {
        user_pool.align = 16;
        //os_printf("HEAP: ALIGN 16\r\n");
    }
    return MPOOL_INIT(&user_pool, 0, (uint32)heap_start, heap_size);
}

uint32 uheap_time()
{
    uint32 alloc = 0, free = 0;
    MPOOL_TIME(&user_pool, &alloc, &free);
    return (alloc + free) / 1000;
}

void *uheap_alloc(int size, const char *func, int line)
{
    void *ptr = MPOOL_MALLOC(&user_pool, size, func, line);
    if (ptr == NULL) {
        os_printf(KERN_ERR"User POOL alloc fail, size=%d, (lr:0x%x,%s:%d)\r\n", size, line?0:func, line?func:"", line);
    }else{
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, user_pool.align));
    }
    return ptr;
}

void uheap_free(void *ptr)
{
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, user_pool.align));
        MPOOL_FREE(&user_pool, ptr);
    }
}

int32 uheap_add(void *heap_start, uint32 heap_size)
{
    return MPOOL_ADD(&user_pool, (uint32)heap_start, heap_size, 0);
}

void uheap_collect_init(uint32 start, uint32 end)
{
    uheap_add((void *)start, end - start);
}

uint32 uheap_freesize()
{
    return MPOOL_FREE_SIZE(&user_pool, 0);
}

uint32 uheap_totalsize()
{
    return MPOOL_TOTAL_SIZE(&user_pool);
}

int32 uheap_of_check(void *ptr, uint32 size)
{
    return MPOOL_OF_CHECK(&user_pool, (uint32)ptr, size);
}

int32 uheap_valid_addr(void *ptr)
{
    return MPOOL_VALID_ADDR(&user_pool, (uint32)ptr);
}

void uheap_status(uint32 *status_buf, int32 buf_size, uint32 mini_size)
{
    int32 i = 0 ;
    int32 count = 0;
    int32 log = 0;
    uint32 total_size = 0;
    uint32 tmps;

    if(status_buf == NULL){
        os_printf("User POOL: total:%d free:%d\r\n", user_pool.size, MPOOL_FREE_SIZE(&user_pool, 0));
        return;
    }

    os_printf("user_heap(%s): regions:%d, align:%d, headsize:%d, tailsize:%d, trace:%d, ofchk:%d\r\n", 
            user_pool.name, user_pool.region_cnt, user_pool.align, 
            user_pool.headsize, user_pool.tailsize, user_pool.trace_off, user_pool.ofchk_off);

    count = MPOOL_FREE_STATE(&user_pool, status_buf, buf_size, &total_size);
    os_printf("-----------------------------------------------\r\n");
    os_printf("free regions: %d\r\n", count / 2);
    for (i = 0; i < count; i += 2) {
        tmps = status_buf[i + 1];
        log  = 0;
        while ((tmps /= 2) != 0) log++;
        os_printf("%02d: 0x%x ~ 0x%x, size:%d \t[%d]\r\n", (i / 2) + 1, status_buf[i],
                  status_buf[i] + status_buf[i + 1], status_buf[i + 1], log);
    }
    os_printf("total free size:%d\r\n", total_size);
    os_printf("-----------------------------------------------\r\n");

    if (user_pool.trace_off) {
        total_size = 0;
        count = MPOOL_USED_STATE(&user_pool, status_buf, buf_size, &total_size, mini_size);
        os_printf("User POOL used regions: %d\r\n", count / 5);
        for (i = 0; i < count; i += 5) {
            if (status_buf[i + 1]) {
                os_printf("%s:%d, size:%d, tick:%d, addr:%x\r\n", (char *)status_buf[i], status_buf[i + 1], status_buf[i + 2], status_buf[i + 3], status_buf[i + 4]);
            } else {
                os_printf("lr:0x%x, size:%d, tick:%d, addr:%x\r\n", status_buf[i], status_buf[i + 2], status_buf[i + 3], status_buf[i + 4]);
            }
        }
        os_printf("total used size:%d\r\n", total_size);
        os_printf("-----------------------------------------------\r\n");
    }
    os_printf("total size:%d\r\n", user_pool.size);

    count = MPOOL_ALLOC_PERF(&user_pool, status_buf, buf_size);
    if(count){
        os_printf("alloc perfermace:");
        for(i=0;i<count;i++) _os_printf(" %-03d", status_buf[i]);
        _os_printf("\r\n");
    }

    count = MPOOL_FREE_PERF(&user_pool, status_buf, buf_size);
    if(count){
        os_printf("free  perfermace:");
        for(i=0;i<count;i++) _os_printf(" %-03d", status_buf[i]);
        _os_printf("\r\n");
    }
}

#ifdef PSRAM_HEAP
struct MMPOOL psram_pool;

void uheap_psram_status(uint32 *status_buf, int32 buf_size, uint32 mini_size)
{
    int32 i = 0 ;
    int32 count = 0;
    int32 log = 0;
    uint32 total_size = 0;
    uint32 tmps;

    if(status_buf == NULL){
        os_printf("User POOL: total:%d free:%d\r\n", psram_pool.size, MPOOL_FREE_SIZE(&psram_pool, 0));
        return;
    }

    os_printf("user_heap(%s): regions:%d, align:%d, headsize:%d, tailsize:%d, trace:%d, ofchk:%d\r\n", 
            psram_pool.name, psram_pool.region_cnt, psram_pool.align, 
            psram_pool.headsize, psram_pool.tailsize, psram_pool.trace_off, psram_pool.ofchk_off);

    count = MPOOL_FREE_STATE(&psram_pool, status_buf, buf_size, &total_size);
    os_printf("-----------------------------------------------\r\n");
    os_printf("free regions: %d\r\n", count / 2);
    for (i = 0; i < count; i += 2) {
        tmps = status_buf[i + 1];
        log  = 0;
        while ((tmps /= 2) != 0) log++;
        os_printf("%02d: 0x%x ~ 0x%x, size:%d \t[%d]\r\n", (i / 2) + 1, status_buf[i],
                  status_buf[i] + status_buf[i + 1], status_buf[i + 1], log);
    }
    os_printf("total free size:%d\r\n", total_size);
    os_printf("-----------------------------------------------\r\n");

    if (psram_pool.trace_off) {
        total_size = 0;
        count = MPOOL_USED_STATE(&psram_pool, status_buf, buf_size, &total_size, mini_size);
        os_printf("User POOL used regions: %d\r\n", count / 5);
        for (i = 0; i < count; i += 5) {
            if (status_buf[i + 1]) {
                os_printf("%s:%d, size:%d, tick:%d, addr:%x\r\n", (char *)status_buf[i], status_buf[i + 1], status_buf[i + 2], status_buf[i + 3], status_buf[i + 4]);
            } else {
                os_printf("lr:0x%x, size:%d, tick:%d, addr:%x\r\n", status_buf[i], status_buf[i + 2], status_buf[i + 3], status_buf[i + 4]);
            }
        }
        os_printf("total used size:%d\r\n", total_size);
        os_printf("-----------------------------------------------\r\n");
    }
    os_printf("total size:%d\r\n", psram_pool.size);

    count = MPOOL_ALLOC_PERF(&psram_pool, status_buf, buf_size);
    if(count){
        os_printf("alloc perfermace:");
        for(i=0;i<count;i++) _os_printf(" %-03d", status_buf[i]);
        _os_printf("\r\n");
    }

    count = MPOOL_FREE_PERF(&psram_pool, status_buf, buf_size);
    if(count){
        os_printf("free  perfermace:");
        for(i=0;i<count;i++) _os_printf(" %-03d", status_buf[i]);
        _os_printf("\r\n");
    }
}

__init int32 psram_heap_init(void *heap_start, uint32 heap_size, uint32 flags) {
    memset(&psram_pool, 0, sizeof(struct MMPOOL));
    psram_pool.headsize = 4;

    /* trace_off, ofchk_off 最大值为15，小心溢出*/
    if (flags & SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK) {
        psram_pool.ofchk_off = psram_pool.headsize;
        psram_pool.headsize += 4;
        psram_pool.tailsize  = 4;
        //os_printf("HEAP: OFCHK enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_LEAK_TRACE) {
        psram_pool.trace_off = psram_pool.headsize;
        psram_pool.headsize += 20;
        //os_printf("HEAP: TRACE enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_ALIGN_16) {
        psram_pool.align = 16;
        //os_printf("HEAP: ALIGN 16\r\n");
    }
    return MPOOL_INIT(&psram_pool, 0, (uint32)heap_start, heap_size);
}

void *psram_heap_alloc(int size, const char *func, int line)
{
    void *ptr = MPOOL_MALLOC(&psram_pool, size, func, line);
    if (ptr == NULL) {
        os_printf(KERN_ERR"User POOL alloc fail, size=%d, (lr:0x%x,%s:%d)\r\n", size, line?0:func, line?func:"", line);
    }
    sys_dcache_clean_invalid_range(ptr,size);
    return ptr;
}

void psram_heap_free(void *ptr)
{
    if (ptr) {
        MPOOL_FREE(&psram_pool, ptr);
    }
}

uint32 psram_heap_freesize()
{
    return MPOOL_FREE_SIZE(&psram_pool, 0);
}

uint32 psram_heap_totalsize()
{
    return MPOOL_TOTAL_SIZE(&psram_pool);
}
#endif

#ifdef LV_PSRAM_HEAP
struct MMPOOL lvgl_pool;

__init int32 lvgl_heap_init(void *heap_start, uint32 heap_size, uint32 flags) {
    memset(&lvgl_pool, 0, sizeof(struct MMPOOL));
    lvgl_pool.headsize = 4;

    /* trace_off, ofchk_off 最大值为15，小心溢出*/
    if (flags & SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK) {
        lvgl_pool.ofchk_off = lvgl_pool.headsize;
        lvgl_pool.headsize += 4;
        lvgl_pool.tailsize  = 4;
        //os_printf("HEAP: OFCHK enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_LEAK_TRACE) {
        lvgl_pool.trace_off = lvgl_pool.headsize;
        lvgl_pool.headsize += 20;
        //os_printf("HEAP: TRACE enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_ALIGN_16) {
        lvgl_pool.align = 16;
        //os_printf("HEAP: ALIGN 16\r\n");
    }
    return MPOOL_INIT(&lvgl_pool, 0, (uint32)heap_start, heap_size);
}

void *lvgl_heap_alloc(int size, const char *func, int line)
{
    void *ptr = MPOOL_MALLOC(&lvgl_pool, size, func, line);
    if (ptr == NULL) {
        os_printf(KERN_ERR"User POOL alloc fail, size=%d, (lr:0x%x,%s:%d)\r\n", size, line?0:func, line?func:"", line);
    }
    return ptr;
}

void lvgl_heap_free(void *ptr)
{
    if (ptr) {
        MPOOL_FREE(&lvgl_pool, ptr);
    }
}

uint32 lvgl_heap_freesize()
{
    return MPOOL_FREE_SIZE(&lvgl_pool, 0);
}

uint32 lvgl_heap_totalsize()
{
    return MPOOL_TOTAL_SIZE(&lvgl_pool);
}
#endif 

#ifdef LWIP_PSRAM_HEAP
struct MMPOOL lwip_pool;

__init int32 lwip_heap_init(void *heap_start, uint32 heap_size, uint32 flags) {
    memset(&lwip_pool, 0, sizeof(struct MMPOOL));
    lwip_pool.headsize = 4;

    /* trace_off, ofchk_off 最大值为15，小心溢出*/
    if (flags & SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK) {
        lwip_pool.ofchk_off = lwip_pool.headsize;
        lwip_pool.headsize += 4;
        lwip_pool.tailsize  = 4;
        //os_printf("HEAP: OFCHK enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_LEAK_TRACE) {
        lwip_pool.trace_off = lwip_pool.headsize;
        lwip_pool.headsize += 20;
        //os_printf("HEAP: TRACE enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_ALIGN_16) {
        lwip_pool.align = 16;
        //os_printf("HEAP: ALIGN 16\r\n");
    }
    return MPOOL_INIT(&lwip_pool, 0, (uint32)heap_start, heap_size);
}

void *lwip_heap_alloc(int size, const char *func, int line)
{
    void *ptr = MPOOL_MALLOC(&lwip_pool, size, func, line);
    if (ptr == NULL) {
        os_printf(KERN_ERR"User POOL alloc fail, size=%d, (lr:0x%x,%s:%d)\r\n", size, line?0:func, line?func:"", line);
    }
    return ptr;
}

void lwip_heap_free(void *ptr)
{
    if (ptr) {
        MPOOL_FREE(&lwip_pool, ptr);
    }
}

uint32 lwip_heap_freesize()
{
    return MPOOL_FREE_SIZE(&lwip_pool, 0);
}

uint32 lwip_heap_totalsize()
{
    return MPOOL_TOTAL_SIZE(&lwip_pool);
}
#endif

#ifdef SYS_TASK_PSRAM_HEAP
struct MMPOOL mlsys_pool;

__init int32 mlsys_heap_init(void *heap_start, uint32 heap_size, uint32 flags) {
    memset(&mlsys_pool, 0, sizeof(struct MMPOOL));
    mlsys_pool.headsize = 4;

    /* trace_off, ofchk_off 最大值为15，小心溢出*/
    if (flags & SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK) {
        mlsys_pool.ofchk_off = mlsys_pool.headsize;
        mlsys_pool.headsize += 4;
        mlsys_pool.tailsize  = 4;
        //os_printf("HEAP: OFCHK enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_LEAK_TRACE) {
        mlsys_pool.trace_off = mlsys_pool.headsize;
        mlsys_pool.headsize += 20;
        //os_printf("HEAP: TRACE enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_ALIGN_16) {
        mlsys_pool.align = 16;
        //os_printf("HEAP: ALIGN 16\r\n");
    }
    return MPOOL_INIT(&mlsys_pool, 0, (uint32)heap_start, heap_size);
}

void *mlsys_heap_alloc(int size, const char *func, int line)
{
    void *ptr = MPOOL_MALLOC(&mlsys_pool, size, func, line);
    if (ptr == NULL) {
        os_printf(KERN_ERR"User POOL alloc fail, size=%d, (lr:0x%x,%s:%d)\r\n", size, line?0:func, line?func:"", line);
    }
    return ptr;
}

void mlsys_heap_free(void *ptr)
{
    if (ptr) {
        MPOOL_FREE(&mlsys_pool, ptr);
    }
}

uint32 mlsys_heap_freesize()
{
    return MPOOL_FREE_SIZE(&mlsys_pool, 0);
}

uint32 mlsys_heap_totalsize()
{
    return MPOOL_TOTAL_SIZE(&mlsys_pool);
}
#endif

#ifdef FREE_TYPE_PSRAM_HEAP
struct MMPOOL ft_pool;

__init int32 ft_heap_init(void *heap_start, uint32 heap_size, uint32 flags) {
    memset(&ft_pool, 0, sizeof(struct MMPOOL));
    ft_pool.headsize = 4;

    /* trace_off, ofchk_off 最大值为15，小心溢出*/
    if (flags & SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK) {
        ft_pool.ofchk_off = ft_pool.headsize;
        ft_pool.headsize += 4;
        ft_pool.tailsize  = 4;
        //os_printf("HEAP: OFCHK enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_LEAK_TRACE) {
        ft_pool.trace_off = ft_pool.headsize;
        ft_pool.headsize += 20;
        //os_printf("HEAP: TRACE enabled\r\n");
    }

    if (flags & SYSHEAP_FLAGS_MEM_ALIGN_16) {
        ft_pool.align = 16;
        //os_printf("HEAP: ALIGN 16\r\n");
    }
    return MPOOL_INIT(&ft_pool, 0, (uint32)heap_start, heap_size);
}

void *ft_heap_alloc(int size, const char *func, int line)
{
    void *ptr = MPOOL_MALLOC(&ft_pool, size, func, line);
    if (ptr == NULL) {
        os_printf(KERN_ERR"User POOL alloc fail, size=%d, (lr:0x%x,%s:%d)\r\n", size, line?0:func, line?func:"", line);
    }
    return ptr;
}

void ft_heap_free(void *ptr)
{
    if (ptr) {
        MPOOL_FREE(&ft_pool, ptr);
    }
}

uint32 ft_heap_freesize()
{
    return MPOOL_FREE_SIZE(&ft_pool, 0);
}

uint32 ft_heap_totalsize()
{
    return MPOOL_TOTAL_SIZE(&ft_pool);
} 
#endif 


