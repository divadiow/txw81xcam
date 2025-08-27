//主要作用是独立usb模块创建流,这个主要是转发用
//因为usb流的根据usb创建成功自动创建的,所以需要创建一个流专门接收转发,流名字也是固定的

#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/task.h"
#include "decode/jpg_decode_stream.h"
#include "osal/string.h"
#include "osal/work.h"
#include "osal_file.h"
#include "jpgdef.h"
#include "utlist.h"
#include "custom_mem/custom_mem.h"

//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc


//因为是转发的流,获取源头的data长度
static uint32_t custom_get_data_len(void *data)
{
	struct data_structure  *data_s = (struct data_structure  *)data;
    return get_stream_real_data_len(data_s->data);
}

static void *custom_get_data(void *data)
{
    
	struct data_structure  *data_s = (struct data_structure  *)data;
    return get_stream_real_data(data_s->data);
}

static uint32_t custom_get_data_time(void *data)
{
	struct data_structure  *data_s = (struct data_structure  *)data;
    return get_stream_data_timestamp(data_s->data);
}

static uint32_t independ_custom_func_psram(void *data,int opcode,void *priv)
{
    struct data_structure  *data_s = (struct data_structure  *)data;
    return stream_data_custom_cmd_func(data_s->data,opcode,priv);
}

//这个是转发的流,只是添加一些必要信息,所以这里获取的data信息要更换
static const stream_ops_func stream_jpg_ops = 
{
	.get_data_len = custom_get_data_len,
    .get_data = custom_get_data,
    .get_data_time = custom_get_data_time,
    .custom_func = independ_custom_func_psram,
};

struct independ_video_usb_s
{
	struct os_work work;
	stream *s;
	struct data_structure *data_s;
};


static int32 independ_video_usb_work(struct os_work *work)
{

    struct independ_video_usb_s *video_usb  = (struct independ_video_usb_s *)work;
    struct data_structure *data_s = NULL;
    if(video_usb->data_s)
    {
        data_s = get_src_data_f(video_usb->s);
        //准备填好显示、解码参数信息,然后发送到解码的模块去解码,主要如果没有成功要如何处理,这里没有考虑
        if(data_s)
        {
            data_s->data = video_usb->data_s;
			data_s->type = video_usb->data_s->type;
            data_s->magic = video_usb->data_s->magic;
            send_data_to_stream(data_s);
            video_usb->data_s = NULL;
        }
    }
    else
    {
        //先获取一下是否有接收到需要解码的图片,下一次才进行解码
        video_usb->data_s = recv_real_data(video_usb->s);
    }


    os_run_work_delay(work, 1);
	return 0;
}


static int opcode_func_not_bind(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
			struct independ_video_usb_s *video_usb = (struct independ_video_usb_s*)STREAM_LIBC_ZALLOC(sizeof(struct independ_video_usb_s));
			s->priv = (void*)video_usb;
			video_usb->s = s;
            stream_data_dis_mem_custom(s);
			OS_WORK_INIT(&video_usb->work, independ_video_usb_work, 0);
			os_run_work_delay(&video_usb->work, 1);
		}
		break;


		case STREAM_OPEN_FAIL:
		break;


		case STREAM_DATA_DIS:
		{  

            //如果不想提前申请内存,这里可以使用的时候才申请,主要看自己规划如何去释放
            struct data_structure *data = (struct data_structure *)priv;
            data->ops = (stream_ops_func*)&stream_jpg_ops;
		}
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                //因为是转发,所以这里移除的话是交给原来的方法移除
                free_data(data->data);
                data->data = NULL;
            }
		}

		break;

        case STREAM_FILTER_DATA:
        {
			struct data_structure *data = (struct data_structure *)priv;
            //os_printf("%s:%d\tdata:%X\ttype:%d\tlen:%d\n",__FUNCTION__,__LINE__,data,GET_DATA_TYPE1(data->type),get_stream_real_data_len(data));
			if(GET_DATA_TYPE1(data->type) != JPEG)
			{
                //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
				res = 1;
				break;
			}
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
            struct independ_video_usb_s *video_usb = (struct independ_video_usb_s *)s->priv;
            os_work_cancle2(&video_usb->work, 1);
            if(video_usb->data_s)
            {
                free_data(video_usb->data_s);
            }

			STREAM_LIBC_FREE(video_usb);
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



//动态绑定,但是名称已经被固定了,因为usb数据流是固定的
stream *independ_video_usb()
{
    stream *s = open_stream_available(SR_VIDEO_USB, 8, 8, opcode_func_not_bind,(void*)NULL);
    return s;
}