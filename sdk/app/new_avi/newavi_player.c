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
#include "avi.h"
#include "stream_define.h"
#include "newavi_player.h"
#include "osal_file.h"
/*****************************************************************************
 * AVI1.0
*****************************************************************************/

//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc

#define AUDIO_TMP_BUF_SIZE   (1024)



struct player_msg_s
{
    void *task;
    stream *s;
    uint32_t magic;
    struct avi_msg_s *avi_msg;
    struct fast_index_list *g_fast_index_list;

    uint32_t audio_frame_number;            //读取索引的长度
    uint32_t audio_frame_number_send;       //已经播放的长度
    uint32_t frame_number;  //当前读取的帧
    uint32_t not_read_size; //剩余未读取的空间长度
    uint32_t cache_addr;        //读取下一次的偏移地址

    struct data_structure *data_tmp;
    struct data_structure *audio_data_tmp;

    int32_t locate_frame_number;      //跳转视频的偏移
    uint8_t running: 1, thread_stop : 1, speed_flag:1, rev: 5;
};



static uint32_t newavi_player_cmd(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
    struct player_msg_s *msg  = (struct player_msg_s *)s->priv;
    switch(opcode)
    {
        //跳转位置,快进
        case NEWAVI_PLAYER_LOCATE_FORWARD_INDEX:
        {
            //快进3s的时间
            uint32_t play_time = arg+3000;
            msg->locate_frame_number = play_time*msg->avi_msg->fps/1000;
            //要判断是否已经到最后
            if(msg->locate_frame_number > msg->avi_msg->avi.dwTotalFrame )
            {
                msg->locate_frame_number = msg->avi_msg->avi.dwTotalFrame;
            }
            //os_printf("msg->locate_frame_number:%d\targ:%d\n",msg->locate_frame_number,arg);
            res = msg->locate_frame_number*1000/msg->avi_msg->fps;
        }
        break;

        //快退
        case NEWAVI_PLAYER_LOCATE_REWIND_INDEX:
        {
            uint32_t play_time = 0;
            if(arg>3000)
            {
                play_time = arg-3000;
            }

            msg->locate_frame_number = play_time*msg->avi_msg->fps/1000;

            //os_printf("msg->locate_frame_number:%d\targ:%d\n",msg->locate_frame_number,arg);
            res = msg->locate_frame_number*1000/msg->avi_msg->fps;
        }
        break;
        //修改data的magic,用于过滤某些数据情况
        case NEWAVI_PLAYER_MAGIC:
            msg->magic = arg;
            msg->speed_flag = 1;
        break;
        default:
        break;
    }
    return res;

}
static void newplayer_thread(void *d)
{
    struct player_msg_s *msg  = (struct player_msg_s *)d;
    stream *s = msg->s;
    struct avi_msg_s *avi_msg = msg->avi_msg;
    uint32_t magic = 0;
    uint8_t *audio_tmp_buf = (uint8_t *)STREAM_LIBC_MALLOC(AUDIO_TMP_BUF_SIZE);
    uint32_t now_audio_offset = 0;
    uint32_t video_time = 0;
    uint32_t audio_time = 0;
    if(!avi_msg->sample)
    {
        audio_time = ~0;        //没有音频
    }
    while(!msg->thread_stop)
    {
        if(!s->enable)
        {
            //这里是否可以考虑先去建立部分快速索引?
            goto newplayer_thread_end;
        }
        //正常这里去建立快速索引
        gen_fast_index_list(avi_msg);

       //判断一下是否要快进或者快退?
        //所以先设置locate_frame_number,再配置speed_flag
        if(msg->speed_flag)
        {
            msg->speed_flag = 0;
            locate_avi_index(avi_msg,msg->locate_frame_number);
            msg->frame_number = msg->locate_frame_number;
            magic = msg->magic;
            video_time = msg->frame_number*1000/avi_msg->fps;
            
            //os_printf("msg->frame_number:%d\n",msg->frame_number);

            if(avi_msg->sample)
            {
                //重置音频,音频是跟着视频走的,表示音频和video同步
                msg->audio_frame_number_send = video_time*avi_msg->sample*2/1000;;
                audio_time = msg->audio_frame_number_send*500/avi_msg->sample;
                now_audio_offset = 0;

                //实际音频索引的位置应该在前面,意思是快速索引在前面,实际播放时间可能在后面,后面需要慢慢去轮询读取到对应的音频
                msg->audio_frame_number = avi_msg->audio_frame_num;
            }

        }

        //如果视频比音频播放前,就需要去读取音频了
        if(video_time > audio_time)
        {
            goto newplayer_audio_thread_start;
        }


        if(!msg->data_tmp)
        {
            msg->data_tmp = get_src_data_f(msg->s);
            if(!msg->data_tmp)
            {
                goto newplayer_audio_thread_start;
            }
        }

 
        
        //理论要判断当前播放的视频帧是否已经最大,最大就直接退出
        uint32_t offset;
        uint32_t size = 0;
        avi_read_next_index(avi_msg,&offset,&size,&msg->frame_number);
        if(!size)
        {
            goto newplayer_audio_thread_start;
        }
        uint8_t *buf = (uint8_t *)STREAM_MALLOC(size);
        if(buf)
        {
            osal_fseek(avi_msg->fp,offset);
            osal_fread(buf,1,size,avi_msg->fp);
            sys_dcache_clean_range((uint32_t*)buf,size);
            msg->data_tmp->data = (uint8_t *)buf;
            msg->data_tmp->type = SET_DATA_TYPE(JPEG,JPEG_FULL);
            //转换成ms
            //os_printf("msg->avi_msg->fps:%d\t%d\n",msg->avi_msg->fps,msg->frame_number*1000/msg->avi_msg->fps);
            set_stream_data_time(msg->data_tmp,msg->frame_number*1000/msg->avi_msg->fps);
            set_stream_real_data_len(msg->data_tmp,size);
            video_time = msg->frame_number*1000/msg->avi_msg->fps;

            msg->data_tmp->magic = magic;
            send_data_to_stream(msg->data_tmp);
            msg->data_tmp = NULL;
            //_os_printf("(V:%d)",size);
            
        }

newplayer_audio_thread_start:

        if(!avi_msg->sample)
        {
            goto newplayer_thread_end;
        }
        if(video_time < audio_time)
        {
            //os_printf("audio_time:%d\tvideo_time:%d\n",audio_time,video_time);
            goto newplayer_thread_end;
        }


        if(!msg->audio_data_tmp)
        {
            msg->audio_data_tmp = get_src_data_f(msg->s);
            if(!msg->audio_data_tmp)
            {
                goto newplayer_thread_end;
            }
        }
        //先去获取一个音频节点
        if(msg->audio_data_tmp)
        {

            //os_printf("audio_time:%d\tvideo_time:%d\n",audio_time,video_time);
            //为stream申请音频的空间,然后读取音频发送
            uint8_t *audio_data_buf = (uint8_t *)STREAM_LIBC_MALLOC(AUDIO_TMP_BUF_SIZE);
            //申请到空间,就将音频先读取到缓冲区,没有空间,则下次重新尝试
            if(audio_data_buf)
            {
                uint32_t max_read_len;
                uint32_t read_len = 0;
                max_read_len = AUDIO_TMP_BUF_SIZE - now_audio_offset;
                while(max_read_len)
                {
                    
                    //读取够音频数据长度,然后通过流发送,然后再将剩下的拷贝到缓冲区,由于
                    avi_read_next_audio_index(avi_msg,&offset,&size,&msg->audio_frame_number);
                    //音频已经读取完毕了
                    if(!size)
                    {
                        break;
                    }
                    osal_fseek(avi_msg->fp,offset);
                    read_len = max_read_len>size?size:max_read_len;
                    osal_fread(audio_tmp_buf+now_audio_offset,1,read_len,avi_msg->fp);
                    now_audio_offset += read_len;
                    max_read_len -= read_len;
                }

                //流发送,先拷贝数据
                hw_memcpy(audio_data_buf,audio_tmp_buf,now_audio_offset);
                msg->audio_data_tmp->data = (uint8_t *)audio_data_buf;
                msg->audio_data_tmp->type = SET_DATA_TYPE(SOUND,SOUND_FILE);
                //转换成ms
                //os_printf("msg->avi_msg->fps:%d\t%d\n",msg->avi_msg->fps,msg->frame_number*1000/msg->avi_msg->fps);
                //设置音频的时间
                set_stream_data_time(msg->audio_data_tmp,msg->audio_frame_number_send*500/avi_msg->sample);//msg->audio_frame_number_send*1000/(avi_msg->sample*2)
                set_stream_real_data_len(msg->audio_data_tmp,now_audio_offset);
                msg->audio_data_tmp->magic = magic;
                //_os_printf("(A:%d\t%d)",now_audio_offset,avi_msg->sample);
                send_data_to_stream(msg->audio_data_tmp);
                msg->audio_data_tmp = NULL;
                msg->audio_frame_number_send += now_audio_offset;
                audio_time = msg->audio_frame_number_send*500/avi_msg->sample;

                //将剩余的数据拷贝
                now_audio_offset = size-read_len;
                //os_printf("remain:%d\n",now_audio_offset);
                //如果有剩余,紧接着读取数据就可以了
                if(now_audio_offset)
                {
                    osal_fread(audio_tmp_buf,1,now_audio_offset,avi_msg->fp);
                }
            }
        }

        newplayer_thread_end:
        os_sleep_ms(1);
    }
    msg->running = 0;

    if(audio_tmp_buf)
    {
        STREAM_LIBC_FREE(audio_tmp_buf);
    }
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

            struct player_msg_s *msg = (struct player_msg_s*)STREAM_LIBC_ZALLOC(sizeof(struct player_msg_s));
            if(msg)
            {
                s->priv = (void*)msg;
                msg->s = s;
                msg->avi_msg = (struct avi_msg_s*)priv;
                msg->magic = 0;
                //初始化快速索引
                
                msg->not_read_size = msg->avi_msg->idx1_size;
                msg->cache_addr = msg->avi_msg->idx1_addr;
                register_stream_self_cmd_func(s,newavi_player_cmd);
                //enable_stream(s,1);
                //OS_WORK_INIT(&msg->work, newplayer_work, 0);
                msg->running = 1;
                msg->task = os_task_create(s->name,newplayer_thread,(void*)msg,OS_TASK_PRIORITY_NORMAL,0,NULL,1024);
                //os_task_init(s->name,&msg->task,newplayer_thread,(uint32_t)msg);
                //os_run_work_delay(&msg->work, 1);
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
                if(GET_DATA_TYPE1(data->type) == SOUND) 
                {
                    STREAM_LIBC_FREE(data->data);
                    
                }
                else if(GET_DATA_TYPE1(data->type) == JPEG)
                {
                    STREAM_FREE(data->data);
                }
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

        case STREAM_CLOSE_EXIT:
        {
            os_printf("newavi player close\n");
            struct player_msg_s *msg = (struct player_msg_s *)s->priv;
           //os_work_cancle2(&msg->work, 1);
            msg->thread_stop = 1;
            while(msg->running)
            {
                os_sleep_ms(1);
            }
            if(msg->data_tmp)
            {
                force_del_data(msg->data_tmp);
            }

            if(msg->audio_data_tmp)
            {
                force_del_data(msg->audio_data_tmp);
            }

            if(msg->avi_msg)
            {
                avi_deinit(msg->avi_msg);
            }

            STREAM_LIBC_FREE(msg);
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


stream *newavi_player_init(const char *stream_name,const char *filename)
{
    struct avi_msg_s *avi_msg = NULL;
    stream *s = NULL;
    avi_msg = avi_read_init(filename);
    os_printf("avi_msg:%X\n",avi_msg);
    //创建workqueue去不停读取视频数据,然后发送出去
    s = open_stream_available(stream_name,8,0,opcode_func,avi_msg);

    return s;
}