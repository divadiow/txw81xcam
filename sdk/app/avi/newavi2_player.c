#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "custom_mem/custom_mem.h"
#include "osal/string.h"
#include "osal/task.h"
#include "osal/event.h"
#include "stream_define.h"
#include "avidemux.h"
#include "osal_file.h"

/*****************************************************************************
 * AVI2.0
*****************************************************************************/

//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc


enum
{
    PLAYER2_STOP = BIT(0),
    PLAYER2_EXIT = BIT(1),
};





struct player2_msg_s
{
    void *task;
    struct      os_event    evt;
    void *fp;
    struct aviinfo *info;
    stream *s;
    uint32_t magic;
    struct data_structure *video_data_tmp;
    struct data_structure *audio_data_tmp;
    int32_t locate_frame_number;
    uint32_t fps;
    uint32_t frq; 
    //用于音频视频同步计算的时间
    uint32_t video_time;
    uint32_t audio_time;
    uint8_t thread_stop : 1, rev: 7;
};


//暂时不支持快进快退
static uint32_t newavi_player2_cmd(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
    struct player2_msg_s *msg  = (struct player2_msg_s *)s->priv;
    switch(opcode)
    {
        //跳转位置,快进
        case NEWAVI_PLAYER_LOCATE_FORWARD_INDEX:
        {
            res = msg->locate_frame_number*1000/msg->fps;
        }
        break;

        //快退
        case NEWAVI_PLAYER_LOCATE_REWIND_INDEX:
        {
            res = msg->locate_frame_number*1000/msg->fps;
        }
        break;
        //修改data的magic,用于过滤某些数据情况
        case NEWAVI_PLAYER_MAGIC:
            msg->magic = arg;
        break;
        //返回是否正在播放的状态
        case NEWAVI_PLAYER_STATUS:
            res = msg->thread_stop;
        break;
        default:
        break;
    }
    return res;

}


static void newplayer2_thread(void *d)
{
    struct player2_msg_s *msg  = (struct player2_msg_s *)d;
    stream *s = msg->s;
    uint32_t base = 0,size;
    uint32_t audio_base;
    uint32_t base_tmp = 0;
    int res = 0;
    int read_len;
    uint8_t *data = NULL;
    struct aviinfo *info = msg->info;
    uint32_t audio_read_len = 0;

    while(!msg->thread_stop)
    {
        if(!s->enable)
        {
            goto newplayer2_thread_wait;
        }

        if(!msg->video_data_tmp)
        {
            msg->video_data_tmp = get_src_data_f(msg->s);
        }

        if(msg->frq && !msg->audio_data_tmp)
        {
            msg->audio_data_tmp = get_src_data_f(msg->s);
        }

        if(!msg->video_data_tmp && !msg->audio_data_tmp)
        {
            goto newplayer2_thread_wait;
        }

        if(msg->video_data_tmp)
        {
            //这里应该要去计算一下音频视频是否同步,以音频为主(因为音频比较敏感)
            //播放的时间小,就需要播放下一帧视频
            //os_printf("video_time:%d\taudio_time:%d\n",msg->video_time,msg->audio_time);
            if(msg->video_time > msg->audio_time)
            {
                goto newplayer2_thread_video_end;
            }
            //代表已经发送了,可以尝试读取下一帧
            if(base == base_tmp)
            {
                res = read_avi_offset(&info->str[0],&base,&size);
            }
            
            if(base == 0 || res)
            {
                force_del_data(msg->video_data_tmp);
                msg->video_data_tmp = NULL;

                //需要退出
                os_printf("@@@@@@@@@@@@@@@@@@@@@@%s:%d\n",__FUNCTION__,__LINE__);
                goto newplayer2_thread_exit;
            }

            if(base_tmp != base)
            {
                
                //读取data数据发送出去
                data = (uint8_t*)STREAM_MALLOC(size);
                if(!data)
                {
                    goto newplayer2_thread_wait;
                }
                read_len = read_avi_data(&info->str[0],(char*)data,size);
                if(read_len <= 0)
                {
                    //要退出,结束或者读取异常了
                    os_printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%s:%d\n",__FUNCTION__,__LINE__);
                    goto newplayer2_thread_exit;
                }

                msg->video_data_tmp->data = data;
                msg->video_data_tmp->ref = 0;
                msg->video_data_tmp->type = SET_DATA_TYPE(JPEG,JPEG_FULL);
                //暂时使用msg->fps帧去播放
                msg->video_data_tmp->magic = msg->magic;
                set_stream_real_data_len(msg->video_data_tmp,read_len);
                msg->locate_frame_number = get_avi_cur(&info->str[0])-1;
                msg->video_time = get_avi_cur(&info->str[0])*1000/msg->fps;
                set_stream_data_time(msg->video_data_tmp,(get_avi_cur(&info->str[0])-1)*1000/msg->fps); 
                //_os_printf("(V:%X\ttype:%X\tread_len:%d)\r\n",msg->video_data_tmp,msg->video_data_tmp->type,read_len);
                send_data_to_stream(msg->video_data_tmp);
                msg->video_data_tmp = NULL;
                data = NULL;

                base_tmp = base;
                _os_printf("(V)");
            }
            else
            {
                msg->locate_frame_number = get_avi_cur(&info->str[0])-1;
                msg->video_time = get_avi_cur(&info->str[0])*1000/msg->fps;
            }
        }

        newplayer2_thread_video_end:
        if(msg->frq && msg->audio_data_tmp)
        {
            if(msg->video_time < msg->audio_time)
            {
                goto newplayer2_thread_wait;
            }

            res = read_avi_offset(&info->str[1],&audio_base,&size);
            if(audio_base == 0 || res)
            {
                force_del_data(msg->audio_data_tmp);
                msg->audio_data_tmp = NULL;
                goto newplayer2_thread_exit;
            }


            data = (uint8_t*)STREAM_MALLOC(size);
            if(!data)
            {
                goto newplayer2_thread_wait;
            }
            read_len = read_avi_data(&info->str[1],(char*)data,size);
            if(read_len <= 0)
            {
                //要退出,结束或者读取异常了
                os_printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%s:%d\n",__FUNCTION__,__LINE__);
                goto newplayer2_thread_exit;
            }
                msg->audio_data_tmp->data = data;
                msg->audio_data_tmp->ref = 0;
                msg->audio_data_tmp->type = SET_DATA_TYPE(SOUND,SOUND_FILE);
                //暂时使用msg->fps帧去播放
                msg->audio_data_tmp->magic = msg->magic;
                set_stream_real_data_len(msg->audio_data_tmp,read_len);
                audio_read_len += read_len;
                set_stream_data_time(msg->audio_data_tmp,msg->audio_time); 
                msg->audio_time = audio_read_len*1000/msg->frq;
                //_os_printf("(A:%X\ttype:%X\tread_len:%d)\r\n",msg->audio_data_tmp,msg->audio_data_tmp->type,read_len);
                send_data_to_stream(msg->audio_data_tmp);
                
                msg->audio_data_tmp = NULL;
                data = NULL;
                _os_printf("(A)");
                
        }




        newplayer2_thread_wait:
        os_sleep_ms(1);
    }
newplayer2_thread_exit:
    os_printf("%s:%d exit\n",__FUNCTION__,__LINE__);
    if(data)
    {
        STREAM_FREE(data);
        data = NULL;
    }
    if(msg->video_data_tmp)
    {
        force_del_data(msg->video_data_tmp);
        msg->video_data_tmp = NULL;
    }

    if(msg->audio_data_tmp)
    {
        force_del_data(msg->audio_data_tmp);
        msg->audio_data_tmp = NULL;
    }

    msg->thread_stop = 1;
    os_event_wait(&msg->evt,PLAYER2_STOP,NULL,OS_EVENT_WMODE_OR,-1);
    os_event_set(&msg->evt,PLAYER2_EXIT,NULL);
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

            struct player2_msg_s *msg = (struct player2_msg_s*)priv;
            s->priv = (void*)priv;
            msg->s = s;
            msg->magic = 0;
            msg->thread_stop = 0;
            msg->fps = msg->info->str[0].dwRate;
            if(msg->info->strmsk&BIT(1))
            {
                msg->frq = msg->info->str[1].dwRate;
            }
            else
            {
                msg->audio_time = ~0;   //没有音频,不需要考虑音频同步
            }
            os_printf("fps:%d\tfrq:%d\n",msg->fps,msg->frq);
            os_event_init(&msg->evt);
            register_stream_self_cmd_func(s,newavi_player2_cmd);
            msg->task = os_task_create(s->name,newplayer2_thread,(void*)msg,OS_TASK_PRIORITY_NORMAL,0,NULL,1024);
            if(!msg->task)
            {
                os_event_set(&msg->evt,PLAYER2_EXIT,NULL);
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
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                STREAM_FREE(data->data);
                data->data = NULL;
            }
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

        case STREAM_CLOSE_ENTER:
        {
            struct player2_msg_s *msg = (struct player2_msg_s*)s->priv;
            msg->thread_stop = 1;
            os_event_set(&msg->evt,PLAYER2_STOP,NULL);
            os_event_wait(&msg->evt,PLAYER2_EXIT,NULL,OS_EVENT_WMODE_OR,-1);
            os_printf("newavi2_player close\n");
        }
        break;

        case STREAM_CLOSE_EXIT:
        {
            struct player2_msg_s *msg = (struct player2_msg_s*)s->priv;
            if(msg->info)
            {
                free_aviinfo_point(msg->info);
                STREAM_LIBC_FREE(msg->info);
            }

            if(msg->fp)
            {
                osal_fclose(msg->fp);
            }
            STREAM_LIBC_FREE(msg);
            os_printf("newavi2_player success\n");
        }
        break;

        case STREAM_MARK_GC_END:
        {
        }
        break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}


static struct player2_msg_s *avi2_read_init(const char *filename)
{
    struct player2_msg_s *msg = NULL;
	struct aviinfo *info = NULL;
    struct aviinfo *info_tmp = NULL;
    void *fp = NULL;

    fp = osal_fopen(filename,"rb");
    info = (struct aviinfo *)STREAM_LIBC_ZALLOC(sizeof(struct aviinfo));
    if(fp && info)
    {
        info_tmp = avidemux_parse(fp,info);
    }

    if(fp && info_tmp)
    {
        msg = (struct player2_msg_s *)STREAM_LIBC_ZALLOC(sizeof(struct player2_msg_s));
        msg->fp = fp;
        msg->info = info_tmp;
    }
    return msg;
}

stream *newavi_player2_init(const char *stream_name,const char *filename)
{
    
    struct player2_msg_s *msg = NULL;
    stream *s = NULL;
    msg = avi2_read_init(filename);
    //创建workqueue去不停读取视频数据,然后发送出去
    if(msg)
    {
       s = open_stream_available(stream_name,8,0,opcode_func,(void*)msg);
    }
    

    return s;
}