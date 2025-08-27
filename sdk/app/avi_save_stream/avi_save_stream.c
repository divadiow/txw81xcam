#include "osal/string.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal/event.h"
#include "custom_mem/custom_mem.h"


//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc

typedef struct 
{
    const char  *video_name;
    const char  *audio_name;
    uint16_t    video_cache_num;
    uint16_t    audio_cache_num;
    uint16_t    fps;
    uint16_t    frq;

}manage_stream_common_s;


typedef struct 
{
    manage_stream_common_s msg;
    stream      *video_s;
    stream      *audio_s;
    struct      os_task     task;
    struct      os_event    evt;
    int    flag;

}manage_stream_s;


enum
{
    AVI_STREAM_STOP = BIT(0),
    AVI_STREAM_EXIT = BIT(1),
};


static int avi_opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	//_os_printf("%s:%d\topcode:%d\n",__FUNCTION__,__LINE__,opcode);
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

        case STREAM_FILTER_DATA:
        {
			struct data_structure *data = (struct data_structure *)priv;
            //os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
            //os_printf("type:%X\n",GET_DATA_TYPE1(data->type));
			if(GET_DATA_TYPE1(data->type) != JPEG)
			{
				res = 1;
				break;
			}
        }
        break;



		default:
			//默认都返回成功
		break;
	}
	return res;
}


static int audio_opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	//_os_printf("%s:%d\topcode:%d\n",__FUNCTION__,__LINE__,opcode);
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

        case STREAM_FILTER_DATA:
        {
			struct data_structure *data = (struct data_structure *)priv;
            //os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
            //os_printf("type:%X\n",GET_DATA_TYPE1(data->type));
			if(GET_DATA_TYPE1(data->type) != SOUND)
			{
				res = 1;
				break;
			}
        }
        break;



		default:
			//默认都返回成功
		break;
	}
	return res;
}

extern uint8_t *get_jpeg_buf(struct data_structure *data,uint32_t *len);
extern int parse_jpg(uint8_t *jpg_buf,uint32_t maxsize,uint32_t *w,uint32_t *h);
extern int new_sd_save_avi2(int *exit_flag,stream *s,stream *audio_s,int fps,int audiofrq,int settime,int pic_w,int pic_h);
static void avi_save_thread(void *d)
{
    int err = 0;
    uint32_t w = 0;
    uint32_t h = 0;
    struct data_structure *get_f = NULL;
    uint32_t len;
    uint8_t *buf;
    manage_stream_s *manage = (manage_stream_s *)d;
    if(manage->msg.video_name)
    {
        manage->video_s = open_stream_available(manage->msg.video_name,0,manage->msg.video_cache_num,avi_opcode_func,NULL);
    }

    if(manage->msg.audio_name)
    {
        manage->audio_s = open_stream_available(manage->msg.audio_name,0,manage->msg.audio_cache_num,audio_opcode_func,NULL);
    }

    if(manage->video_s)
    {
        enable_stream(manage->video_s,1);
    }

    if(manage->audio_s)
    {
        enable_stream(manage->audio_s,1);
    }

    while(manage->flag)
    {
        //先去尝试获取第一张图片,分析分辨率
        if(w == 0 || h == 0)
        {
            get_f = recv_real_data(manage->video_s);
            if(get_f)
            {
                buf = get_jpeg_buf(get_f,&len);
                parse_jpg(buf,len,&w,&h);
                os_printf("%s:%d\tw:%d\th:%d\n",__FUNCTION__,__LINE__,w,h);
                free_data(get_f);
            }
            else
            {
                os_sleep_ms(1);
            }
        }
        else
        {
            if(manage->video_s)
            {
                enable_stream(manage->video_s,1);
            }

            if(manage->audio_s)
            {
                enable_stream(manage->audio_s,1);
            }


            err = new_sd_save_avi2(&manage->flag,manage->video_s,manage->audio_s,manage->msg.fps,manage->msg.frq,60*1,w,h);
        }
    }
    close_stream(manage->video_s);
    close_stream(manage->audio_s);
    //设置退出
    os_event_set(&manage->evt,AVI_STREAM_EXIT,NULL);
    
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
            //创建结构体,创建一个线程
            manage_stream_s *manage = (manage_stream_s*)STREAM_LIBC_ZALLOC(sizeof(manage_stream_s));
            if(priv && manage)
            {
                s->priv = (void*)manage;
                manage_stream_common_s *common = (manage_stream_common_s*)priv;
                os_memcpy(manage,common,sizeof(manage_stream_common_s));
                manage->flag = 1;

                //初始化事件
                os_event_init(&manage->evt);
                //创建线程
                OS_TASK_INIT(s->name,&manage->task,avi_save_thread,manage,OS_TASK_PRIORITY_NORMAL,1024);
                //创建失败,设置可以退出的标志
                if(!manage->task.hdl)
                {
                    os_event_set(&manage->evt,AVI_STREAM_EXIT,NULL);
                }
            }
        }
		break;
		case STREAM_OPEN_FAIL:
		break;

		case STREAM_DATA_DIS:
		{
		}
		break;

		case STREAM_DATA_FREE:
		{
            
		}

		break;

        case STREAM_FILTER_DATA:
        {
        }
        break;

        case STREAM_DATA_FREE_END:
        break;


		//数据发送完成,可以选择唤醒对应的任务
		case STREAM_SEND_DATA_FINISH:
		break;


		case STREAM_SEND_DATA_START:
		{
		}
		break;

		case STREAM_DATA_DESTORY:
		{
		}
		break;


        case STREAM_MARK_GC_END:
        {
            
            
        }
		break;

        case STREAM_CLOSE_ENTER:
        {
            manage_stream_s *manage = (manage_stream_s *)s->priv;
            manage->flag = 0;
            os_event_wait(&manage->evt,AVI_STREAM_EXIT,NULL,OS_EVENT_WMODE_OR,-1);
            break;
        }

        case STREAM_CLOSE_EXIT:
        {
            manage_stream_s *manage = (manage_stream_s *)s->priv;
            os_event_del(&manage->evt);
            STREAM_LIBC_FREE(s->priv);
            break;
        }
        break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}




//创建一个线程,类似录像模块,管理音频和视频接收流的线程
//这个会由一个流去管理
stream *avi_save_stream(const char *manage_stream_name,const char *video_name,const char *audio_name,uint16_t video_cache_num,uint16_t audio_cache_num,uint16_t fps,uint16_t frq)
{
    manage_stream_common_s manage;
    manage.video_name       = video_name;
    manage.audio_name       = audio_name;
    manage.video_cache_num  = video_cache_num;
    manage.audio_cache_num  = audio_cache_num;
    manage.fps              = fps;
    manage.frq              = frq;

    stream *s = open_stream_available(manage_stream_name,0,0,opcode_func,&manage);
	return s;
}