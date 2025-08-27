/*
    用户自定义的内存管理,主要为了根据客户自己的需求,用独立的,可以避免内存在heap中碎片化
*/

#include "custom_mem.h"
#include "typesdef.h"

#if 1
extern __init int32 uheap_init(void *heap_start, uint32 heap_size, uint32 flags);
extern uint32 uheap_time();
extern void *uheap_alloc(int size, const char *func, int line);
extern void uheap_free(void *ptr);
extern uint32 uheap_freesize();

void custom_mem_init(void *buf, int custom_heap_size)
{
    uheap_init(buf, custom_heap_size, SYSHEAP_FLAGS_MEM_LEAK_TRACE | 
                                      SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK);
}

void custom_mem_deinit()
{
}

void print_custom_sram()
{
    os_printf("custom mem sram:%d\n", uheap_freesize());
}

void *custom_malloc(int size)
{
    return uheap_alloc(size, RETURN_ADDR(), 0);
}

void custom_free(void *ptr)
{
    if (ptr) {
        uheap_free(ptr);
    }
}

void *custom_zalloc(int size)
{
    void *ptr = uheap_alloc(size, RETURN_ADDR(), 0);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *custom_calloc(int nmemb, int size)
{
    void *ptr = uheap_alloc(nmemb * size, RETURN_ADDR(), 0);
    if (ptr) {
        os_memset(ptr, 0, nmemb * size);
    }
    return ptr;
}

void *custom_realloc(void *ptr, int size)
{
    void *nptr = uheap_alloc(size, RETURN_ADDR(), 0);
    if (nptr) {
        os_memcpy(nptr, ptr, size);
        uheap_free(ptr);
    }
    return nptr;
}

int ml_byte_alignment(uint32 size, uint32 align_len) {
    uint32 new_length = size / align_len * align_len;
	if(new_length < size) new_length += align_len;
	return new_length;
}

void *_custom_malloc(int size, void *call_addr) {
    return uheap_alloc(size, call_addr, 0);
}

void _custom_free(void *ptr, void *call_addr) {
    if (ptr) {
        uheap_free(ptr);
    }
}

void *_custom_zalloc(int size, void *call_addr) {
    void *ptr = uheap_alloc(size, call_addr, 0);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *_custom_calloc(size_t nmemb, size_t size, void *call_addr) {
    void *ptr = uheap_alloc(nmemb * size, call_addr, 0);
    if (ptr) {
        os_memset(ptr, 0, nmemb * size);
    }
    return ptr;
}

void *_custom_realloc(void *ptr, int size, void *call_addr) {
    void *nptr = uheap_alloc(size, call_addr, 0);
    if (nptr) {
        os_memcpy(nptr, ptr, size);
        uheap_free(ptr);
    }
    return nptr;
}

#ifdef PSRAM_HEAP

    extern __init int32 psram_heap_init(void *heap_start, uint32 heap_size, uint32 flags);
    extern void *psram_heap_alloc(int size, const char *func, int line);
    extern void psram_heap_free(void *ptr);
    extern uint32 psram_heap_freesize();
    void custom_mem_psram_init(void *buf,int custom_heap_size) {
        psram_heap_init(buf, custom_heap_size, SYSHEAP_FLAGS_MEM_ALIGN_16|SYSHEAP_FLAGS_MEM_LEAK_TRACE | 
                                      SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK);
    }

    void custom_mem_psram_deinit() {
        
    }

    void print_custom_psram() {
        os_printf("custom mem psram:%d\n", psram_heap_freesize());
    }

    void *_custom_malloc_psram(int size, void *call_addr) {
        return psram_heap_alloc(size, call_addr, 0);
    }
    
    void _custom_free_psram(void *ptr, void *call_addr) {
        if (ptr) {
            psram_heap_free(ptr);
        }
    }
    
    void *_custom_zalloc_psram(int size, void *call_addr) {
        void *ptr = psram_heap_alloc(size, call_addr, 0);
        if (ptr) {
            os_memset(ptr, 0, size);
        }
        return ptr;    
    }
    
    void *_custom_calloc_psram(size_t nmemb, size_t size, void *call_addr) {
        void *ptr = psram_heap_alloc(nmemb * size, call_addr, 0);
        if (ptr) {
            os_memset(ptr, 0, nmemb * size);
        }
        return ptr;
    }
    
    void *_custom_realloc_psram(void *ptr, int size, void *call_addr) {
        void *nptr = psram_heap_alloc(size, call_addr, 0);
        if (nptr) {
            os_memcpy(nptr, ptr, size);
            psram_heap_free(ptr);
        }
        return nptr;    
    }

    void *custom_malloc_psram(int size) {
        return _custom_malloc_psram(size, RETURN_ADDR());
    }

    void custom_free_psram(void *ptr) {
        _custom_free_psram(ptr, RETURN_ADDR());
    }

    void *custom_zalloc_psram(int size) {
        return _custom_zalloc_psram(size, RETURN_ADDR());
    }

    void *custom_calloc_psram(int nmemb, int size) {
        return _custom_calloc_psram(nmemb, size, RETURN_ADDR());
    }

    void *custom_realloc_psram(void *ptr, int size) {
        return _custom_realloc_psram(ptr, size, RETURN_ADDR());
    }

    void psram_check_cache(void *ptr) {
        
    }
#endif

#ifdef LV_PSRAM_HEAP
    void lv_psram_init(void *buf, uint32_t custom_heap_size) {
        lvgl_heap_init(buf, custom_heap_size, SYSHEAP_FLAGS_MEM_LEAK_TRACE | 
                                      SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK);
    }

    void lv_psram_deinit() {
        
    }

    void print_lv_psram() {
        os_printf("lvgl mem psram:%d\n", lvgl_heap_freesize());
    }

    void *lv_malloc_psram(int size) {
        return lvgl_heap_alloc(size, RETURN_ADDR(), 0);
    }

    void lv_free_psram(void *ptr) {
        if (ptr) {
            lvgl_heap_free(ptr);
        }
    }

    void _lv_free_psram(void *ptr, void *call_addr) {
         if (ptr) {
            lvgl_heap_free(ptr);
        }
    }

    void *lv_zalloc_psram(int size) {
        void *ptr = lvgl_heap_alloc(size, RETURN_ADDR(), 0);
        if (ptr) {
            os_memset(ptr, 0, size);
        }
        return ptr;    
    }

    void *lv_calloc_psram(size_t nmemb, size_t size) {
        void *ptr = lvgl_heap_alloc(nmemb * size, RETURN_ADDR(), 0);
        if (ptr) {
            os_memset(ptr, 0, nmemb * size);
        }
        return ptr;
    }

    void *lv_realloc_psram(void *ptr, int size) {
        void *nptr = lvgl_heap_alloc(size, RETURN_ADDR(), 0);
        if (nptr) {
            os_memcpy(nptr, ptr, size);
            lvgl_heap_free(ptr);
        }
        return nptr;      
    }
#endif

#ifdef LWIP_PSRAM_HEAP
    void lwip_psram_init(void *buf, uint32_t custom_heap_size) {
        lwip_heap_init(buf, custom_heap_size, SYSHEAP_FLAGS_MEM_LEAK_TRACE | 
                                      SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK);
    }

    void lwip_psram_deinit() {
        
    }

    void print_lwip_psram() {
        os_printf("lwip mem psram:%d\n", lwip_heap_freesize());
    }

    void *lwip_malloc_psram(int size) {
        return lwip_heap_alloc(size, RETURN_ADDR(), 0);
    }

    void lwip_free_psram(void *ptr) {
        if (ptr) {
            lwip_heap_free(ptr);
        }
    }

    void _lwip_free_psram(void *ptr, void *call_addr) {
         if (ptr) {
            lwip_heap_free(ptr);
        }
    }

    void *lwip_zalloc_psram(int size) {
        void *ptr = lwip_heap_alloc(size, RETURN_ADDR(), 0);
        if (ptr) {
            os_memset(ptr, 0, size);
        }
        return ptr;    
    }

    void *lwip_calloc_psram(size_t nmemb, size_t size) {
        void *ptr = lwip_heap_alloc(nmemb * size, RETURN_ADDR(), 0);
        if (ptr) {
            os_memset(ptr, 0, nmemb * size);
        }
        return ptr;
    }

    void *lwip_realloc_psram(void *ptr, int size) {
        void *nptr = lwip_heap_alloc(size, RETURN_ADDR(), 0);
        if (nptr) {
            os_memcpy(nptr, ptr, size);
            lwip_heap_free(ptr);
        }
        return nptr;      
    }
    
    void *_lwip_malloc_psram(int size, void *call_addr) {
        return lwip_heap_alloc(size, call_addr, 0);
    }
    
    
    void *_lwip_calloc_psram(size_t nmemb, size_t size, void *call_addr) {
        void *ptr = lwip_heap_alloc(nmemb * size, call_addr, 0);
        if (ptr) {
            os_memset(ptr, 0, nmemb * size);
        }
        return ptr;
    }
#endif

#ifdef SYS_TASK_PSRAM_HEAP
    void ml_sys_psram_init(void *buf, uint32_t custom_heap_size) {
        mlsys_heap_init(buf, custom_heap_size, SYSHEAP_FLAGS_MEM_LEAK_TRACE | 
                                      SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK);
    }

    void ml_sys_psram_deinit() {
        
    }

    void print_mlsys_psram() {
        os_printf("mlsys mem psram:%d\n", mlsys_heap_freesize());
    }

    void *ml_sys_malloc_psram(int size) {
        return mlsys_heap_alloc(size, RETURN_ADDR(), 0);
    }

    void ml_sys_free_psram(void *ptr) {
        if (ptr) {
            mlsys_heap_free(ptr);
        }
    }

    void _ml_sys_free_psram(void *ptr, void *call_addr) {
         if (ptr) {
            mlsys_heap_free(ptr);
        }
    }

    void *ml_sys_zalloc_psram(int size) {
        void *ptr = mlsys_heap_alloc(size, RETURN_ADDR(), 0);
        if (ptr) {
            os_memset(ptr, 0, size);
        }
        return ptr;    
    }

    void *ml_sys_calloc_psram(size_t nmemb, size_t size) {
        void *ptr = mlsys_heap_alloc(nmemb * size, RETURN_ADDR(), 0);
        if (ptr) {
            os_memset(ptr, 0, nmemb * size);
        }
        return ptr;
    }

    void *ml_sys_realloc_psram(void *ptr, int size) {
        void *nptr = mlsys_heap_alloc(size, RETURN_ADDR(), 0);
        if (nptr) {
            os_memcpy(nptr, ptr, size);
            mlsys_heap_free(ptr);
        }
        return nptr;      
    }
#endif 

#ifdef FREE_TYPE_PSRAM_HEAP
    void free_type_psram_init(void *buf, uint32_t custom_heap_size) {
        ft_heap_init(buf, custom_heap_size, SYSHEAP_FLAGS_MEM_LEAK_TRACE | 
                                      SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK);
    }

    void free_type_psram_deinit() {
        
    }

    void print_free_type_psram() {
        os_printf("free type mem psram:%d\n", ft_heap_freesize());
    }

    void *free_type_malloc_psram(int size) {
        return ft_heap_alloc(size, RETURN_ADDR(), 0);
    }

    void free_type_free_psram(void *ptr) {
        if (ptr) {
            ft_heap_free(ptr);
        }
    }

    void *free_type_zalloc_psram(int size) {
        void *ptr = ft_heap_alloc(size, RETURN_ADDR(), 0);
        if (ptr) {
            os_memset(ptr, 0, size);
        }
        return ptr;    
    }

    void *free_type_calloc_psram(size_t nmemb, size_t size) {
        void *ptr = ft_heap_alloc(nmemb * size, RETURN_ADDR(), 0);
        if (ptr) {
            os_memset(ptr, 0, nmemb * size);
        }
        return ptr;
    }

    void *free_type_realloc_psram(void *ptr, int size) {
        void *nptr = ft_heap_alloc(size, RETURN_ADDR(), 0);
        if (nptr) {
            os_memcpy(nptr, ptr, size);
            ft_heap_free(ptr);
        }
        return nptr;      
    }
#endif 

/**打印custom psram heap 使用情况 */
void ml_print_psram_heap_list() {
    
}

/**打印lvgl psram heap 使用情况 */
void ml_print_lvgl_heap_list() {
    
}

/**打印lwip psram heap 使用情况 */
void ml_print_lwip_heap_list() {
    
}

/**打印sys psram heap 使用情况 */
void ml_print_sys_heap_list() {
    
}

#endif 

