/*
 * Copyright (C) 2004 Nathan Lutchansky <lutchann@litech.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
//#include <test_util.h>
#include <csi_config.h>
 
#include <event.h>
#include <log.h>
#include <frame.h>
#include <list.h>
//#include <linux/linux_mutex.h>
#include "spook_config.h"
#include "rtp.h"

#include "lwip/api.h"
#include <csi_kernel.h>

#include "csi_core.h"
#include "list.h"

#include "jpgdef.h"


#include "osal/sleep.h"

#ifdef USB_EN
#include "dev/usb/uvc_host.h"
#endif

#include "osal/string.h"


#include "stream_frame.h"
//默认先申请一个图传空间
unsigned int live_buf_cache[SPOOK_CACHE_BUF_LEN/4];

struct frame *new_frame(void)
{
	struct frame *f;

	//新帧
	f = (struct frame*)malloc(sizeof(struct frame));

	return f;
}

struct frame *new_audio_frame(void)
{
	struct frame *f;

	//新帧
	f = (struct frame*)malloc(sizeof(struct frame));
	return f;
}


struct frame_exchanger *new_exchanger_audio( int slots,
					frame_deliver_func func, void *d )
{
	struct frame_exchanger *ex;
	ex = (struct frame_exchanger *)malloc( sizeof( struct frame_exchanger ) );

	ex->f = func;
	ex->d = d;
	return ex;
}


struct frame_exchanger *new_exchanger( int slots,
					frame_deliver_func func, void *d )
{
	struct frame_exchanger *ex;

	ex = (struct frame_exchanger *)
			malloc( sizeof( struct frame_exchanger ) );
	ex->f = func;
	ex->d = d;



	/* read_mutex is used to avoid  closing the tcp when frame is sending through sendmsg */




	
	return ex;
}

int exchange_frame( struct frame_exchanger *ex, struct frame *frame )
{
	ex->jf= frame;
	return 0;
}



void unref_frame( struct frame *f )
{
//	int r;
	//del_frame
	if(!f)
	{
		return;
	}
	DEL_JPG_FRAME(f->get_f);
}





static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_RECV_DATA_FINISH:
		break;

		case STREAM_CLOSE_EXIT:
			os_printf("%s:%d close\n",__FUNCTION__,__LINE__);
		break;
		


		default:
			//默认都返回成功
		break;
	}
	return res;
}


uint32_t creat_stream(struct rtsp_priv *r,const char *video_name,const char *audio_name)
{
	if(video_name)
	{
		r->video_s = open_stream(video_name,0,2,opcode_func,NULL);

		if(!r->video_s)
		{
			goto creat_stream_err;
		}
	}
	else
	{
		r->video_s = NULL;
	}

	if(audio_name)
	{
		r->audio_s = open_stream(audio_name,0,2,opcode_func,NULL);
		if(!r->audio_s)
		{
			goto creat_stream_err;
		}
	}
	else
	{
		r->audio_s = NULL;
	}


	return 0;
creat_stream_err:
	if(!r->video_s || !r->audio_s)
	{
		if(r->video_s)
		{
			close_stream(r->video_s);
		}

		if(r->audio_s)
		{
			close_stream(r->audio_s);
		}
		return 1;
	}
	return 1;
}


uint32_t destory_stream(struct rtsp_priv *r)
{
	if(r->video_s)
	{
		close_stream(r->video_s);
	}

	if(r->audio_s)
	{
		close_stream(r->audio_s);
	}
	return 0;
}

#define SCAN_BUF_LEN	1024
#if SCAN_BUF_LEN > SPOOK_CACHE_BUF_LEN
	#err "please check SCAN_BUF_LEN and SPOOK_CACHE_BUF_LEN"
#endif
extern unsigned int live_buf_cache[SPOOK_CACHE_BUF_LEN/4];
unsigned char *scan_buf = (unsigned char*)live_buf_cache;

void spook_send_thread_stream(struct rtsp_priv *r)
{

	struct rtp_node *node = r->live_node;	
	struct frame_exchanger *ex = node->video_ex;
	struct frame_exchanger *audio_ex = node->audio_ex;
	stream *s = r->video_s;
	stream *audio_s = r->audio_s;
	static struct frame *jpeg;
	static struct frame *audio;
	//s = open_stream(R_RTP_JPEG,0,2,opcode_func,NULL);

	//audio_s = open_stream(R_RTP_AUDIO,0,2,opcode_func,NULL);

	
	struct data_structure  *get_f = NULL;
	struct data_structure  *audio_f = NULL;


	int count_times = 0;
	int get_times_out = 0;
	int cnt_num = 0;
	uint32_t time_for_count;
	uint32_t time = 0;

	time_for_count = os_jiffies();
	uint8_t *scan_data;
	//这里while用是否图传标志去判断
	while(1) 
	{
		if(audio_s)
		{
			audio_f = 	GET_FRAME(audio_s);
			if(audio_f)
			{
				_os_printf("A");
				audio = audio_ex->jf;//ex->slave_cur->f;
				audio->get_f = audio_f;
				audio->length = GET_DATA_LEN(audio_f);
				audio->timestamp = GET_DATA_TIMESTAMP(audio_f);
				audio_ex->f(audio,audio_ex->d);
			}
		}

		get_f 	= 	GET_FRAME(s);
		if (get_f)
		{
				jpeg = ex->jf;//ex->slave_cur->f;
			    //指针赋值
			    jpeg->get_f = get_f;
				jpeg->node_len = GET_NODE_LEN(get_f);
				//主要是扫描用scan_buf
				scan_data = (uint8_t*)GET_JPG_BUF(get_f);
				//这里copy一次主要为了兼容带psram的情况
				if(scan_data)
				{
					hw_memcpy0(scan_buf,scan_data,SCAN_BUF_LEN);
				}
				else
				{
					_os_printf("%s err:%d\t%X\n",__FUNCTION__,__LINE__,(int)scan_data);
					continue;
				}
				jpeg->d = (void*)scan_buf;
				jpeg->first_length = SCAN_BUF_LEN;//GET_NODE_LEN(get_f);//JPG_BUF_LEN;


				cnt_num++;

				if((os_jiffies() - time_for_count) > 1000){
						time_for_count = os_jiffies();
						_os_printf("cnt_num:%d\r\n",cnt_num);
						cnt_num = 0;
				}
				jpeg->length = GET_DATA_LEN(get_f);;

				

				jpeg->format = FORMAT_JPEG;

				jpeg->timestamp = GET_DATA_TIMESTAMP(get_f);
				//_os_printf("jpeg time:%d\n",jpeg->timestamp);
				/*
				* callback: get_back_frame( struct frame *f, void *d )
				* d: (jpeg_encoder *)en
				*/
				_os_printf("#");
				//_os_printf("ex:%X\tf:%X\n",ex,ex->f);
				ex->f( jpeg, ex->d );
		}

		if(get_f)
		{
			ex->ready = 0;
			count_times++;
			if(count_times>25)
			{
				_os_printf("time:%lld\n",csi_kernel_get_ticks()-time);
				count_times = 0;
				time = csi_kernel_get_ticks();
			}
		}
		if(!audio_f && !get_f)
		{
			os_sleep_ms(1);
			get_times_out++;
			if(get_times_out>1000)
			{
				ex->f( NULL, ex->d );
				get_times_out = 0;
			}
			continue;
		}
		else
		{
			get_times_out = 0;
		}


	}

}