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

struct other_jpg_s
{
	struct os_work work;
    stream *s;
    struct data_structure *data_s;
    uint16_t out_w;
    uint16_t out_h;
    uint16_t step_w;
    uint16_t step_h;
    uint32_t type;
    uint8_t del;
};

struct other_jpg_arg_s
{
    uint16_t out_w;
    uint16_t out_h;
    uint16_t step_w;
    uint16_t step_h;
    uint32_t type;
};



//这里主要是接收jpg数据,然后分析jpg的分辨率,设置好后,输出到lcd
//仅仅提供测试代码,后面自己的流如何实现参考这个
//这里从usb获取jpg数据,然后发送到解码
//之所以没有直接从usb发送到lcd,因为默认usb只是获取jpg数据,没有对应要显示的参数
//这里可以配置解码参数,显示大小等信息



//用于获取一些通用命令的接口,也可以加入自定义接口
//接口怎么获取数据只有源头知道,这里不同是通用的
static uint32_t other_jpg_cmd_func(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
    switch(opcode)
    {
        case JPG_DECODE_ARG:
        {
            struct data_structure  *data_s = (struct data_structure *)arg;
            res = (uint32_t)data_s->priv;
        }
        break;
        case SET_LVGL_VIDEO_ARG:
        {
            //struct other_jpg_s *other_jpg_msg  = (struct other_jpg_s *)s->priv;
            //uint32_t 
        }
        break;

        default:
        break;
    }
    return res;
}


//因为是转发的流,获取源头的data长度
static uint32_t custom_get_data_len(void *data)
{
	struct data_structure  *data_s = (struct data_structure  *)data;
    return get_stream_real_data_len(data_s->data);
}

static void *custom_get_data(void *data)
{
	struct data_structure  *data_s = (struct data_structure  *)data;
    uint8_t *jpeg_buf_addr = NULL;
    struct data_structure *data_s_tmp = (struct data_structure  *)data_s->data;
    if(GET_DATA_TYPE2(data_s_tmp->type) == JPEG_FULL)
    {
        jpeg_buf_addr = (uint8_t*)get_stream_real_data(data_s->data);
    }
    else
    {
        struct stream_jpeg_data_s *dest_list;
        struct stream_jpeg_data_s *dest_list_tmp;
        struct stream_jpeg_data_s *el,*tmp;
        dest_list = (struct stream_jpeg_data_s *)get_stream_real_data(data_s->data);
        dest_list_tmp = dest_list;
        


        LL_FOREACH_SAFE(dest_list,el,tmp)
        {
            if(dest_list_tmp == el)
            {
                continue;
            }

            //读取完毕删除
            //图片保存起来
            //理论这里就是一张图片,由于这里是节点形式,所以这里暂时先去实现,后续获取一张图片形式显示

            jpeg_buf_addr = (uint8_t *)stream_data_custom_cmd_func(data_s->data,CUSTOM_GET_NODE_BUF,el->data);
            break;
        }
    }

    return jpeg_buf_addr;
}

static uint32_t custom_get_data_time(void *data)
{
	struct data_structure  *data_s = (struct data_structure  *)data;
    return get_stream_data_timestamp(data_s->data);
}

//这个是转发的流,只是添加一些必要信息,所以这里获取的data信息要更换
static const stream_ops_func stream_jpg_ops = 
{
	.get_data_len = custom_get_data_len,
    .get_data = custom_get_data,
    .get_data_time = custom_get_data_time,
};

static int32 other_jpg_work(struct os_work *work)
{
    struct other_jpg_s *other_jpg_msg  = (struct other_jpg_s *)work;
    struct data_structure *data_s = NULL;
    int res;
	//static int count = 0;
    if(other_jpg_msg->data_s)
    {
        data_s = get_src_data_f(other_jpg_msg->s);
        //准备填好显示、解码参数信息,然后发送到解码的模块去解码,主要如果没有成功要如何处理,这里没有考虑
        if(data_s)
        {

            //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
            //因为是转发,所以赋值
            data_s->data = other_jpg_msg->data_s;
            data_s->magic = other_jpg_msg->data_s->magic;

            //写入对应的参数,这里使用固定值
            struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s*)data_s->priv;
            struct yuv_arg_s    *yuv_msg  = &msg->yuv_arg;
            yuv_msg->out_w = other_jpg_msg->out_w;
            yuv_msg->out_h = other_jpg_msg->out_h;
            msg->rotate = 0;
            msg->step_w = other_jpg_msg->step_w;
            msg->step_h = other_jpg_msg->step_h;
            data_s->type = SET_DATA_TYPE(JPEG,other_jpg_msg->type);
            extern int parse_jpg(uint8_t *jpg_buf,uint32_t maxsize,uint32_t *w,uint32_t *h);
            res = parse_jpg(get_stream_real_data(data_s),get_stream_real_data_len(data_s),&msg->decode_w,&msg->decode_h);
            // os_printf("w:%d\th:%d\tlen:%d\tdata_s:%X\n",msg->decode_w,msg->decode_h,get_stream_real_data_len(data_s),other_jpg_msg->data_s);
            if(!res)
            {
                send_data_to_stream(data_s);
            }
            else
            {
                force_del_data(data_s);
            }

            other_jpg_msg->data_s = NULL;
        }
    }
    else
    {
        //先获取一下是否有接收到需要解码的图片,下一次才进行解码
        other_jpg_msg->data_s = recv_real_data(other_jpg_msg->s);

    }
    os_run_work_delay(work, 1);
	return 0;
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
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s*)STREAM_LIBC_ZALLOC(sizeof(struct other_jpg_s));
            struct other_jpg_arg_s *arg = (struct other_jpg_arg_s*)priv;
			s->priv = (void*)other_jpg_msg;
            other_jpg_msg->s = s;
            other_jpg_msg->del = 0;
            other_jpg_msg->out_w = arg->out_w;
            other_jpg_msg->out_h = arg->out_h;
            other_jpg_msg->step_w = arg->step_w;
            other_jpg_msg->step_h = arg->step_h;
            other_jpg_msg->type = arg->type;
            register_stream_self_cmd_func(s,other_jpg_cmd_func);
            stream_data_dis_mem_custom(s);
            streamSrc_bind_streamDest(s,S_JPG_DECODE);
            enable_stream(s,1);
			OS_WORK_INIT(&other_jpg_msg->work, other_jpg_work, 0);
			os_run_work_delay(&other_jpg_msg->work, 1);
		}
		break;


		case STREAM_OPEN_FAIL:
		break;


		case STREAM_DATA_DIS:
		{  
            //如果不想提前申请内存,这里可以使用的时候才申请,主要看自己规划如何去释放
            struct data_structure *data = (struct data_structure *)priv;
            data->priv = (void *)STREAM_LIBC_ZALLOC(sizeof(struct jpg_decode_arg_s));  
            data->data = NULL; 
            data->ops = (stream_ops_func*)&stream_jpg_ops;
            struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s*)data->priv;
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s *)s->priv;
            msg->yuv_arg.del = &other_jpg_msg->del;
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
            //os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
			if(GET_DATA_TYPE1(data->type) != JPEG)
			{
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
			struct data_structure *data = (struct data_structure *)priv;
			if(data->priv)
			{
				STREAM_LIBC_FREE(data->priv);
			}
		}
		break;

        case STREAM_CLOSE_ENTER:
        {
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s *)s->priv;
            os_work_cancle2(&other_jpg_msg->work, 1);
        }
        break;

        case STREAM_CLOSE_EXIT:
        {
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s *)s->priv;
            other_jpg_msg->del = 1;
            os_work_cancle2(&other_jpg_msg->work, 1);
            if(other_jpg_msg->data_s)
            {
                free_data(other_jpg_msg->data_s);
            }
        }
        break;

        case STREAM_MARK_GC_END:
        {
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s *)s->priv;
            if(other_jpg_msg)
            {
                STREAM_LIBC_FREE((void*)other_jpg_msg);
            }
        }
        break;

		default:
			//默认都返回成功
		break;
	}
	return res;
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
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s*)STREAM_LIBC_ZALLOC(sizeof(struct other_jpg_s));
            struct other_jpg_arg_s *arg = (struct other_jpg_arg_s*)priv;
			s->priv = (void*)other_jpg_msg;
            other_jpg_msg->s = s;
            other_jpg_msg->del = 0;
            other_jpg_msg->out_w = arg->out_w;
            other_jpg_msg->out_h = arg->out_h;
            other_jpg_msg->step_w = arg->step_w;
            other_jpg_msg->step_h = arg->step_h;
            other_jpg_msg->type = arg->type;
            register_stream_self_cmd_func(s,other_jpg_cmd_func);
            stream_data_dis_mem_custom(s);
            //streamSrc_bind_streamDest(s,S_JPG_DECODE);
            //enable_stream(s,1);
			OS_WORK_INIT(&other_jpg_msg->work, other_jpg_work, 0);
			os_run_work_delay(&other_jpg_msg->work, 1);
		}
		break;


		case STREAM_OPEN_FAIL:
		break;


		case STREAM_DATA_DIS:
		{  
            //如果不想提前申请内存,这里可以使用的时候才申请,主要看自己规划如何去释放
            struct data_structure *data = (struct data_structure *)priv;
            data->priv = (void *)STREAM_LIBC_ZALLOC(sizeof(struct jpg_decode_arg_s));  
            data->data = NULL; 
            data->ops = (stream_ops_func*)&stream_jpg_ops;
            struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s*)data->priv;
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s *)s->priv;
            msg->yuv_arg.del = &other_jpg_msg->del;
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
			struct data_structure *data = (struct data_structure *)priv;
			if(data->priv)
			{
				STREAM_LIBC_FREE(data->priv);
			}
		}
		break;

        case STREAM_CLOSE_ENTER:
        {
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s *)s->priv;
            os_work_cancle2(&other_jpg_msg->work, 1);
        }
        break;

        case STREAM_CLOSE_EXIT:
        {
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s *)s->priv;
            other_jpg_msg->del = 1;
            os_work_cancle2(&other_jpg_msg->work, 1);
            if(other_jpg_msg->data_s)
            {
                free_data(other_jpg_msg->data_s);
            }
        }
        break;

        case STREAM_MARK_GC_END:
        {
            struct other_jpg_s *other_jpg_msg = (struct other_jpg_s *)s->priv;
            if(other_jpg_msg)
            {
                os_printf("other_jpg_msg:%X\n",other_jpg_msg);
                STREAM_LIBC_FREE((void*)other_jpg_msg);
            }
        }
        break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}


//这里的作为产生流,主要是为了类似转发,没有特别作用
//占用空间不多

stream *other_jpg_stream(const char *name,uint16_t out_w,uint16_t out_h,uint16_t step_w,uint16_t step_h,uint32_t type)
{
    struct other_jpg_arg_s arg;
    arg.out_w = out_w;
    arg.out_h = out_h;
    arg.step_w = step_w;
    arg.step_h = step_h;
    arg.type = type;
    stream *s = open_stream_available(name, 8, 8, opcode_func,(void*)&arg);
    return s;
}

//不绑定,由外部进行绑定,实现动态绑定效果
stream *other_jpg_stream_not_bind(const char *name,uint16_t out_w,uint16_t out_h,uint16_t step_w,uint16_t step_h,uint32_t type)
{
    struct other_jpg_arg_s arg;
    arg.out_w = out_w;
    arg.out_h = out_h;
    arg.step_w = step_w;
    arg.step_h = step_h;
    arg.type = type;
    stream *s = open_stream_available(name, 8, 8, opcode_func_not_bind,(void*)&arg);
    return s;
}