#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/string.h"
#include "osal/irq.h"
#include "hal/dma.h"
#include "hal/crc.h"
#include "lib/common/common.h"
#include "lib/heap/sysheap.h"

extern void *krhino_mm_alloc(size_t size, void *caller);
extern void  krhino_mm_free(void *ptr);

__bobj uint64 cpu_loading_tick;
#ifdef M2M_DMA
__bobj struct dma_device *m2mdma;
#endif

void cpu_loading_print(uint8 all, struct os_task_info *tsk_info, uint32 size)
{
    uint32 i = 0;
    uint32 diff_tick = 0;
    uint32 irq_time = 0;
    uint32 count;
    uint64 jiff = os_jiffies();

    if(tsk_info == NULL) return;
    diff_tick = DIFF_JIFFIES(cpu_loading_tick, jiff);
    cpu_loading_tick = jiff;

    irq_time = irq_status();
    os_printf("--------------------------------------------------------------------\r\n");
    os_printf("Task Runtime Statistic, interval:%dms\r\n", os_jiffies_to_msecs(diff_tick));
    os_printf("PID     Name            %%CPU(Time)    Stack  Prio              Status\r\n");
    os_printf("--------------------------------------------------------------------\r\n");
    if(irq_time > 0){
        count = 100 * os_msecs_to_jiffies(irq_time/1000);
        os_printf("SYS IRQ: %dms, %d%%\r\n", (irq_time/1000), (count/diff_tick));
        os_printf("--------------------------------------------------------------------\r\n");
    }

    count = os_task_runtime(tsk_info, size);
    for (i = 0; i < count; i++) {
        if (tsk_info[i].time > 0 || all) {
            os_printf("%2d     %-12s\t%2d%%(%6d)   %4d  %2d (%08x)  %s\r\n",
                      tsk_info[i].id,
                      tsk_info[i].name ? tsk_info[i].name : "----",
#ifdef CSKY_OS
                      (tsk_info[i].time * 100) / diff_tick,
#elif defined(OHOS)
                      tsk_info[i].time,
#endif

                      tsk_info[i].time,
                      tsk_info[i].stack * 4,
                      tsk_info[i].prio,
                      tsk_info[i].arg,
                      tsk_info[i].status);
        }
    }
    os_printf("--------------------------------------------------------------------\r\n");
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    size_t i = 0;

    for (i = 0; i < n && s1[i] && s2[i]; i++) {
        if (s1[i] == s2[i] || s1[i] + 32 == s2[i] || s1[i] - 32 == s2[i]) {
        } else {
            break;
        }
    }
    return (i != n);
}

int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 || *s2) {
        if (*s1 == *s2 || *s1 + 32 == *s2 || *s1 - 32 == *s2) {
            s1++; s2++;
        } else {
            return -1;
        }
    }
    return 0;
}

#ifdef M2M_DMA
void hw_memcpy(void *dest, const void *src, uint32 size)
{
    if (dest && src) {
        if (m2mdma && size > 45) {
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
            struct sys_heap *heap = sysheap_valid_addr(&psram_heap, dest) ? &psram_heap : &sram_heap;
#else
            struct sys_heap *heap = &sram_heap;
#endif
            int32 ret = sysheap_of_check(heap, dest, size);
            if (ret == -1) {
                //os_printf("%s: WARING: OF CHECK 0x%x\r\n", dest, __FUNCTION__);
            } else {
                if (!ret) {
                    os_printf("check addr fail: %x, size:%d \r\n", dest, size);
                }
                ASSERT(ret == 1);
            }
#endif
            dma_memcpy(m2mdma, dest, src, size);
        } else {
            os_memcpy(dest, src, size);
        }
    }
}

void hw_memcpy0(void *dest, const void *src, uint32 size)
{
    if (m2mdma && size > 45) {
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
        struct sys_heap *heap = sysheap_valid_addr(&psram_heap, dest) ? &psram_heap : &sram_heap;
#else
        struct sys_heap *heap = &sram_heap;
#endif
        int32 ret = sysheap_of_check(heap, dest, size);
        if (ret == -1) {
            //os_printf("%s: WARING: OF CHECK 0x%x\r\n", dest, __FUNCTION__);
        } else {
            if (!ret) {
                os_printf("check addr fail: %x, size:%d \r\n", dest, size);
            }
            ASSERT(ret == 1);
        }
#endif
        dma_memcpy(m2mdma, dest, src, size);
    } else {
        os_memcpy(dest, src, size);
    }
}

void hw_memset(void *dest, uint8 val, uint32 n)
{
    if (dest) {
        if (m2mdma && n > 12) {
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
            struct sys_heap *heap = sysheap_valid_addr(&psram_heap, dest) ? &psram_heap : &sram_heap;
#else
            struct sys_heap *heap = &sram_heap;
#endif
            int32 ret = sysheap_of_check(heap, dest, n);
            if (ret == -1) {
                //os_printf("%s: WARING: OF CHECK 0x%x\r\n", dest, __FUNCTION__);
            } else {
                if (!ret) {
                    os_printf("check addr fail: %x, size:%d \r\n", dest, n);
                }
                ASSERT(ret == 1);
            }
#endif
            dma_memset(m2mdma, dest, val, n);
        } else {
            os_memset(dest, val, n);
        }
    }
}
#endif

void *os_memdup(const void *ptr, uint32 len)
{
    void *p;
    if (!ptr || len == 0) {
        return NULL;
    }
    p = os_malloc(len);
    if (p) {
        hw_memcpy(p, ptr, len);
    }
    return p;
}


int32 os_random_bytes(uint8 *data, int32 len)
{
    int32 i = 0;
    int32 seed;
#ifdef TXW4002ACK803
    seed = csi_coret_get_value() ^ (csi_coret_get_value() << 8) ^ (csi_coret_get_value() >> 8);
#else
    seed = csi_coret_get_value() ^ sysctrl_get_trng() ^ (sysctrl_get_trng() >> 8);
#endif
    for (i = 0; i < len; i++) {
        seed = seed * 214013L + 2531011L;
        data[i] = (uint8)(((seed >> 16) & 0x7fff) & 0xff);
    }
    return 0;
}

uint32 hw_crc(enum CRC_DEV_TYPE type, uint8 *data, uint32 len)
{
    uint32 crc = 0xffff;
    struct crc_dev_req req;
    struct crc_dev *crcdev = (struct crc_dev *)dev_get(HG_CRC_DEVID);
    if (crcdev) {
        req.type = type;
        req.data = data;
        req.len  = len;
        //ASSERT((uint32)data % 4 == 0); // crc模块数据地址必须4字节对齐检查
        crc_dev_calc(crcdev, &req, &crc, 0);
    } else {
        os_printf("no crc dev\r\n");
        crc = (uint32)os_jiffies();
    }
    return crc;
}

int ffs(int x)
{
    int r = 1;

    if (!x) {
        return 0;
    }

    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

int fls(int x)
{
    int r = 32;

    if (!x) {
        return 0;
    }

    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

uint32 scatter_data_size(scatter_data *data, uint32 count)
{
    uint32 size = 0;
    uint32 i = 0;
    for (i = 0; i < count; i++) {
        size += data[i].size;
    }
    return size;
}

