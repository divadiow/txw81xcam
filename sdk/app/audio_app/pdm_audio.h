#ifndef __PDM_AUDIO_H
#define __PDM_AUDIO_H
#include "hal/pdm.h"



/******************************************************************************
逻辑:priv_el结构体注册可以是一个函数,然后通过该函数和el_point得到buf
******************************************************************************/
//返回值是一个buf,录音的buf,priv_el则是应用层的一个结构,el_point则是一个指针地址,audio_set_buf在返回buf前同时要配置el_point的值(最后在audio_get_buf的时候会需要调用)
typedef void *(*set_buf)(void *priv_el,void *el_point);
//priv_el则是应用层的一个结构,el_point则是可以寻找到buf的一个结构体,el_point的值是audio_set_buf赋值的
typedef void (*get_buf)(void *priv_el,void *el_point);




void 	pdm_audio_close(void *audio_hdl);
int 	pdm_audio_start(void *audio_hdl);
void 	pdm_audio_register(void *audio_hdl,void *priv_el,int play_size,set_buf set_buf,get_buf get_buf);
void  	*pdm_audio_open(enum pdm_sample_freq freq,enum pdm_channel channel);
void *audio_task(const char *name);




#endif
