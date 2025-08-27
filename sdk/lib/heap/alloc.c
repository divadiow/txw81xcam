
#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"
#include "lib/heap/sysheap_psram.h"
#include "lib/heap/mtrace.h"

#if MPOOL_ALLOC

__bobj struct sys_heap sram_heap;


void *_os_malloc(int size)
{
    return sysheap_alloc(&sram_heap, size, RETURN_ADDR(), 0);
}

void _os_free(void *ptr)
{
    if (ptr) {
        sysheap_free(&sram_heap, ptr);
    }
}

void *_os_zalloc(int size)
{
    void *ptr = sysheap_alloc(&sram_heap, size, RETURN_ADDR(), 0);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *_os_calloc(size_t nmemb, size_t size)
{
    void *ptr = sysheap_alloc(&sram_heap, nmemb * size, RETURN_ADDR(), 0);
    if (ptr) {
        os_memset(ptr, 0, nmemb * size);
    }
    return ptr;
}

void *_os_realloc(void *ptr, int size)
{
    void *nptr = sysheap_alloc(&sram_heap, size, RETURN_ADDR(), 0);
    if (nptr) {
        os_memcpy(nptr, ptr, size);
    }
    if(ptr){
        _os_free(ptr);
    }
    return nptr;
}

void *_os_malloc_t(int size, const char *func, int line)
{
    return sysheap_alloc(&sram_heap, size, func, line);
}

void _os_free_t(void *ptr)
{
    if (ptr) {
        sysheap_free(&sram_heap, ptr);
    }
}

void *_os_zalloc_t(int size, const char *func, int line)
{
    void *ptr = _os_malloc_t(size, func, line);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *_os_calloc_t(size_t nmemb, size_t size, const char *func, int line)
{
    return _os_zalloc_t(nmemb * size, func, line);
}

void *_os_realloc_t(void *ptr, int size, const char *func, int line)
{
    void *nptr = _os_malloc_t(size, func, line);
    if (nptr && ptr) {
        os_memcpy(nptr, ptr, size);
    }
    if(ptr){
        _os_free_t(ptr);
    }
    return nptr;
}

#ifdef MALLOC_IN_PSRAM
void *malloc(uint32 size) __alias(_os_malloc_psram);
void  free(void *ptr)     __alias(_os_free_psram);
void *zalloc(size_t size)   __alias(_os_zalloc_psram);
void *calloc(size_t nmemb, size_t size) __alias(_os_calloc_psram);
void *realloc(void *ptr, size_t size) __alias(_os_realloc_psram);
#else
void *malloc(uint32 size) __alias(_os_malloc);
void  free(void *ptr)     __alias(_os_free);
void *zalloc(size_t size)   __alias(_os_zalloc);
void *calloc(size_t nmemb, size_t size) __alias(_os_calloc);
void *realloc(void *ptr, size_t size) __alias(_os_realloc);
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#ifdef PSRAM_HEAP
__bobj struct sys_heap psram_heap;

void *_os_malloc_psram(int size)
{
    void *ret = sysheap_alloc(&psram_heap, size, RETURN_ADDR(), 0);
    if(ret)
    {
        sys_dcache_clean_invalid_range(ret,size);
    }
    return ret;
}

void _os_free_psram(void *ptr)
{
    if (ptr) {
        sysheap_free(&psram_heap, ptr);
    }
}

void *_os_zalloc_psram(size_t size)
{
    void *ptr = sysheap_alloc(&psram_heap, size, RETURN_ADDR(), 0);
    if(ptr)
    {
        sys_dcache_clean_invalid_range(ptr,size);
    }
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *_os_calloc_psram(size_t nmemb, size_t size)
{
    void *ptr = sysheap_alloc(&psram_heap, nmemb * size, RETURN_ADDR(), 0);
    if(ptr)
    {
        sys_dcache_clean_invalid_range(ptr,nmemb * size);
    }
    if (ptr) {
        os_memset(ptr, 0, nmemb * size);
    }
    return ptr;
}

void *_os_realloc_psram(void *ptr, int size)
{
    void *nptr = sysheap_alloc(&psram_heap, size, RETURN_ADDR(), 0);
    if(nptr){
        sys_dcache_clean_invalid_range(nptr,size);
        if (ptr) {
            os_memcpy(nptr, ptr, size);
        }
    }
    if(ptr){
        _os_free_psram(ptr);
    }
    return nptr;
}

void *_os_malloc_psram_t(int size, const char *func, int line)
{
    void *ret = sysheap_alloc(&psram_heap, size, func, line);
    if(ret)
    {
        sys_dcache_clean_invalid_range(ret,size);
    }
    return ret;
}

void _os_free_psram_t(void *ptr)
{
    if (ptr) {
        sysheap_free(&psram_heap, ptr);
    }
}

void *_os_zalloc_psram_t(int size, const char *func, int line)
{
    void *ptr = _os_malloc_psram_t(size, func, line);
    if(ptr)
    {
        sys_dcache_clean_invalid_range(ptr,size);
    }
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *_os_calloc_psram_t(size_t nmemb, size_t size, const char *func, int line)
{
    return _os_zalloc_psram_t(nmemb * size, func, line);
}

void *_os_realloc_psram_t(void *ptr, int size, const char *func, int line)
{
    void *nptr = _os_malloc_psram_t(size, func, line);
    if (nptr && ptr) {
        os_memcpy(nptr, ptr, size);
    }
    if(ptr){
        _os_free_psram_t(ptr);
    }    
    return nptr;
}

void *malloc_psram(uint32 size) __alias(_os_malloc_psram);
void  free_psram(void *ptr)     __alias(_os_free_psram);
void *zalloc_psram(size_t size)   __alias(_os_zalloc_psram);
void *calloc_psram(size_t nmemb, size_t size) __alias(_os_calloc_psram);
void *realloc_psram(void *ptr, size_t size) __alias(_os_realloc_psram);

#else
void *_os_malloc_psram(int size) __alias(_os_malloc);
void  _os_free_psram(void *ptr)     __alias(_os_free);
void *_os_zalloc_psram(size_t size)   __alias(_os_zalloc);
void *_os_calloc_psram(size_t nmemb, size_t size) __alias(_os_calloc);
void *_os_realloc_psram(void *ptr, int size) __alias(_os_realloc);

#endif

#endif

