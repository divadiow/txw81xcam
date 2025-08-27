#ifndef _MP3_DECODE_MSI_H_
#define _MP3_DECODE_MSI_H_

#include "basic_include.h"
#include "custom_mem/custom_mem.h"

#ifdef PSRAM_HEAP
#define MP3_DECODE_MALLOC    custom_malloc_psram
#define MP3_DECODE_ZALLOC    custom_zalloc_psram
#define MP3_DECODE_FREE      custom_free_psram
#else
#define MP3_DECODE_MALLOC    custom_malloc
#define MP3_DECODE_ZALLOC    custom_zalloc
#define MP3_DECODE_FREE      custom_free
#endif

typedef int32 (*mp3_read_func)(uint8_t *buf, uint32_t size);
enum
{
    MP3_NOT_RUN, //代表可能创建了线程,但没有实际运行(可能是创建线程失败或者被其他高优先级任务一直卡住)
    MP3_PLAY,
    MP3_PAUSE,
    MP3_STOP,
};

void mp3_decode_init(void *filename, void *read_func);
void set_mp3_decode_status(uint8_t status);
uint8_t get_mp3_decode_status(void);

#endif