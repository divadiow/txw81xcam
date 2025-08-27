#ifndef __CUSTOM_MEM_H
#define __CUSTOM_MEM_H
#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"
#include "lib/heap/sysheap_psram.h"
#include "lib/heap/mtrace.h"
void custom_mem_init(void *buf,int custom_heap_size);
void custom_mem_deinit();
void *custom_malloc(int size);
void custom_free(void *ptr);
void *custom_zalloc(int size);
void *custom_calloc(int nmemb, int size);
void *custom_realloc(void *ptr, int size);  
void print_custom_sram();

#ifdef PSRAM_HEAP
void custom_mem_psram_init(void *buf,int custom_heap_size);
void custom_mem_psram_deinit();
void print_custom_psram();
void print_custom_psram_heap_status(uint32_t *buf,int size);
void *custom_malloc_psram(int size);
void custom_free_psram(void *ptr);
void *custom_zalloc_psram(int size);
void *custom_calloc_psram(int nmemb, int size);
void *custom_realloc_psram(void *ptr, int size);
#else
#define custom_malloc_psram custom_malloc
#define custom_free_psram   custom_free
#define custom_zalloc_psram custom_zalloc
#define custom_calloc_psram custom_calloc
#define custom_realloc_psram custom_realloc
#endif

#endif