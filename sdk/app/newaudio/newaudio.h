#ifndef __NEWAUDIO_H
#define __NEWAUDIO_H
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "stream_frame.h"
#define AUDIONUM	(4)
#define AUDIOLEN	(1024)
typedef struct audio_el audio_el;
typedef struct NewAudio NewAudio;
typedef void (*free_func)(void*node);

struct audio_el {
    unsigned char *audio_data;   //音频数据
    struct audio_el *next;
    int len;
    unsigned int timestamp;
    NewAudio *parent_priv;
    free_func free;         //释放函数
} ;

struct NewAudio
{
    stream *s;
    void *audio_hardware_hdl;
};


struct audio_pdm_s
{
	void *pdm_msgq;
	void *audio_hardware_hdl;
	struct os_task     thread_hdl;
};



void *newaudio_task();
audio_el *global_ready_el();
unsigned char *get_el_buf(audio_el* el_node);
unsigned int  get_el_timestamp(audio_el* el_node);
int get_el_buf_len(audio_el* el_node);
#endif