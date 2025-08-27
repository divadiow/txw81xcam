
#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/work.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "osal/msgqueue.h"
#include "stream_frame.h"
#include "custom_mem.h"

//暂定这个scale3是在存在dvp的时候使用,所以用宏隔离
//实际scale3还有另一种模式不需要dvp那边启动的
#if DVP_EN == 1
#define SCALE3STREAM_BUF_NUM (3)

//区分主要是为了类似结构体访问频繁,如果空间足够放在sram,减少访问psram导致cache切换频繁
//data因为一般是dma并且空间需要比较多,所以用psram

//data申请空间函数
#define SCALE3STREAM_MALLOC     custom_malloc_psram
#define SCALE3STREAM_FREE       custom_free_psram
#define SCALE3STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define SCALE3STREAM_LIBC_MALLOC     custom_malloc
#define SCALE3STREAM_LIBC_FREE       custom_free
#define SCALE3STREAM_LIBC_ZALLOC     custom_zalloc


struct scale3_stream_arg_s
{
    uint16_t iw;
    uint16_t ih;
    uint16_t ow;
    uint16_t oh;
    uint16_t type;

};


struct scale3Stream
{
    struct os_work work;
    stream *s;
    struct data_structure  *now_data;
    uint8_t *scaler3buf;  //scale的buf
    struct scale_device *scale_dev;
    uint16_t iw,ih,ow,oh,type;
    struct os_msgqueue msgq;
    uint8_t del;
};

static uint32_t yuv_stream_cmd_func(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
    switch(opcode)
    {
        case YUV_ARG:
        {
            struct data_structure  *data_s = (struct data_structure *)arg;
            res = (uint32_t)data_s->priv;
        }
        break;
        default:
        break;
    }
    return res;
}

//这里可能采用信号量的形式,通知到线程去处理数据
int32_t scale3_stream_done(uint32 irq_flag,uint32 irq_data,uint32 param1)
{
    struct scale3Stream *scale3_stream = (struct scale3Stream *)irq_data;
    struct data_structure  *data;
    uint8_t *p_buf;
    data = get_src_data_f(scale3_stream->s);
    //os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
    //找不到新的空间,则返回,使用旧空间
    if(!data)
    {
        return 0;
    }

    //发送now_data,发送失败也要返回
    if(os_msgq_put(&scale3_stream->msgq,(uint32_t)scale3_stream->now_data,0))
    {
        force_del_data(data);
        return 0;
    }
    //配置新的空间地址
    p_buf = (uint8_t*)get_stream_real_data(data);
    scale_set_out_yaddr(scale3_stream->scale_dev,(uint32)p_buf);
    scale_set_out_uaddr(scale3_stream->scale_dev,(uint32)p_buf+scale3_stream->ow*scale3_stream->oh);
    scale_set_out_vaddr(scale3_stream->scale_dev,(uint32)p_buf+scale3_stream->ow*scale3_stream->oh+scale3_stream->ow*scale3_stream->oh/4);
    scale3_stream->now_data = data;
    //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return 0;
}

int32_t scale3_stream_ov(uint32 irq_flag,uint32 irq_data,uint32 param1)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return 0;
}



static int32 scale3_stream_work(struct os_work *work)
{
    struct scale3Stream *scale3_stream = (struct scale3Stream*)work;
    struct data_structure  *data;
    int32_t err = -1;
    data = (struct data_structure*)os_msgq_get2(&scale3_stream->msgq,0,&err);
    //没有数据
    if(err)
    {
        goto scale3_stream_work_end;
    }
    //将data发送出去
    set_stream_data_time(data,os_jiffies());
    send_data_to_stream(data);
    //os_printf("success:%d\tdata:%X\n",success,data);

    scale3_stream_work_end:
    //过1ms就去轮询一遍,实际如果用信号量,可以改成任务形式,等待信号量,可以节约cpu(实际cache影响可能更大,cpu占用很少)
    //由于workqueue没有支持等待信号量,只能通过1ms轮询一下
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
            struct scale3_stream_arg_s *msg = (struct scale3_stream_arg_s*)priv;
            struct scale3Stream *scale3_stream = (struct scale3Stream*)SCALE3STREAM_LIBC_ZALLOC(sizeof(struct scale3Stream));
            scale3_stream->scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
            scale3_stream->iw = msg->iw;
            scale3_stream->ih = msg->ih;
            scale3_stream->ow = msg->ow;
            scale3_stream->oh = msg->oh;
            scale3_stream->type = msg->type;
            scale3_stream->del = 0;
            extern uint8 *yuvbuf;
            scale3_stream->scaler3buf = yuvbuf;
            os_msgq_init(&scale3_stream->msgq, SCALE3STREAM_BUF_NUM);
            scale3_stream->s = s;


            //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
			s->priv = (void*)scale3_stream;
            
            register_stream_self_cmd_func(s,yuv_stream_cmd_func);
            //启动workqueue去处理,保证前面已经初始化好信号量
            streamSrc_bind_streamDest(s,R_VIDEO_P0);
            streamSrc_bind_streamDest(s,R_VIDEO_P1);
            streamSrc_bind_streamDest(s,R_YUV_TO_JPG);
			OS_WORK_INIT(&scale3_stream->work, scale3_stream_work, 0);
			os_run_work_delay(&scale3_stream->work, 1);

            //为每一个data进行分配空间
            stream_data_dis_mem_custom(s);
            
            //暂时用同样的iw ih ow oh
            scale_set_in_out_size(scale3_stream->scale_dev,scale3_stream->iw,scale3_stream->ih,scale3_stream->ow,scale3_stream->oh);
            scale_set_step(scale3_stream->scale_dev,scale3_stream->iw,scale3_stream->ih,scale3_stream->ow,scale3_stream->oh);
            scale_set_start_addr(scale3_stream->scale_dev,0,0);
            //暂时固定,如果遇到需要动态修改的,可以通过参数之类来切换
            scale_set_dma_to_memory(scale3_stream->scale_dev,1);
            scale_set_data_from_vpp(scale3_stream->scale_dev,1);
            scale_set_line_buf_num(scale3_stream->scale_dev,32);
            scale_set_in_yaddr(scale3_stream->scale_dev,(uint32)scale3_stream->scaler3buf);
            scale_set_in_uaddr(scale3_stream->scale_dev,(uint32)scale3_stream->scaler3buf+scale3_stream->iw*32);
            scale_set_in_vaddr(scale3_stream->scale_dev,(uint32)scale3_stream->scaler3buf+scale3_stream->iw*32+scale3_stream->iw*8);

            //这里分配一下scale3的空间,通过标准接口去分配吧,理论这里一定能获取到,这里就不判断异常情况了
            struct data_structure  *data;
            uint8_t *p_buf;
            data = get_src_data_f(s);
            p_buf = (uint8_t*)get_stream_real_data(data);
            scale3_stream->now_data = data;

            scale_set_out_yaddr(scale3_stream->scale_dev,(uint32)p_buf);
            scale_set_out_uaddr(scale3_stream->scale_dev,(uint32)p_buf+scale3_stream->ow*scale3_stream->oh);
            scale_set_out_vaddr(scale3_stream->scale_dev,(uint32)p_buf+scale3_stream->ow*scale3_stream->oh+scale3_stream->ow*scale3_stream->oh/4);
            scale_request_irq(scale3_stream->scale_dev,FRAME_END,scale3_stream_done,(uint32)scale3_stream);
            scale_request_irq(scale3_stream->scale_dev,INBUF_OV,scale3_stream_ov,(uint32)scale3_stream);
            scale_open(scale3_stream->scale_dev);
            //os_printf("%s:%d\t%X\n",__FUNCTION__,__LINE__,scale3_stream->scale_dev);
            struct vpp_device *vpp_dev;
            vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
            vpp_open(vpp_dev);
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
            struct scale3Stream *scale3_stream = (struct scale3Stream *)s->priv;
            struct data_structure *data = (struct data_structure *)priv;
            //os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
            data->type = SET_DATA_TYPE(YUV,scale3_stream->type);
            data->priv = (struct yuv_stream_s *)SCALE3STREAM_ZALLOC(sizeof(struct yuv_arg_s));  
            struct yuv_arg_s *yuv = (struct yuv_arg_s*)data->priv;
            //由于创建流的时候,参数已经分配好了,所以这里可以确定参数
            yuv->y_size = scale3_stream->ow*scale3_stream->oh;
            yuv->uv_off = 0;
            yuv->y_off = 0;
            yuv->out_w = scale3_stream->ow;
            yuv->out_h = scale3_stream->oh;
            yuv->del = &scale3_stream->del;
            
            set_stream_real_data_len(data,scale3_stream->ow*scale3_stream->oh*3/2);
            //尝试申请一下空间
            data->data = SCALE3STREAM_MALLOC(scale3_stream->ow*scale3_stream->oh*3/2);
            sys_dcache_clean_invalid_range(data->data,scale3_stream->ow*scale3_stream->oh*3/2);
		}
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data)
            {
                sys_dcache_clean_invalid_range(get_stream_real_data(data),get_stream_real_data_len(data));
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
            struct data_structure *data = (struct data_structure *)priv;
            SCALE3STREAM_FREE(data->data);
            SCALE3STREAM_FREE(data->priv);
		}
		break;

        case STREAM_CLOSE_ENTER:
        {
            struct scale3Stream *scale3_stream = (struct scale3Stream *)s->priv;
            os_work_cancle2(&scale3_stream->work,1);
        }
        break;

        case STREAM_CLOSE_EXIT:
        {
            struct scale3Stream *scale3_stream = (struct scale3Stream *)s->priv;
            scale3_stream->del = 1;
            //停止workqueue
            os_work_cancle2(&scale3_stream->work,1);
            scale_close(scale3_stream->scale_dev);

            //中断已经获取了,但是还没有发送或者准备好数据
            if(scale3_stream->now_data)
            {
                force_del_data(scale3_stream->now_data);
            }

            //中断已经发送了,但是workqueue还没有接收
            struct data_structure  *data;
            int32_t err = 0;
            while(!err)
            {
                data = (struct data_structure*)os_msgq_get2(&scale3_stream->msgq,0,&err);
                //os_printf("%s:%d\tdata:%X\terr:%d\n",__FUNCTION__,__LINE__,data,err);
                if(!err)
                {
                    force_del_data(data);
                }
            }

            os_msgq_del(&scale3_stream->msgq);
            
            break;
        }

        case STREAM_MARK_GC_END:
        {
            struct scale3Stream *scale3_stream = (struct scale3Stream *)s->priv;
            if(scale3_stream)
            {
                SCALE3STREAM_LIBC_FREE(scale3_stream);
            }
            break;
            
        }

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
            struct scale3_stream_arg_s *msg = (struct scale3_stream_arg_s*)priv;
            struct scale3Stream *scale3_stream = (struct scale3Stream*)SCALE3STREAM_LIBC_ZALLOC(sizeof(struct scale3Stream));
            scale3_stream->scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
            scale3_stream->iw = msg->iw;
            scale3_stream->ih = msg->ih;
            scale3_stream->ow = msg->ow;
            scale3_stream->oh = msg->oh;
            scale3_stream->type = msg->type;
            scale3_stream->del = 0;
            extern uint8 *yuvbuf;
            scale3_stream->scaler3buf = yuvbuf;
            os_msgq_init(&scale3_stream->msgq, SCALE3STREAM_BUF_NUM);
            scale3_stream->s = s;


            //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
			s->priv = (void*)scale3_stream;
            
            register_stream_self_cmd_func(s,yuv_stream_cmd_func);

            #if 0
            //启动workqueue去处理,保证前面已经初始化好信号量
            streamSrc_bind_streamDest(s,R_VIDEO_P0);
            streamSrc_bind_streamDest(s,R_VIDEO_P1);
            streamSrc_bind_streamDest(s,R_YUV_TO_JPG);
            #endif
			OS_WORK_INIT(&scale3_stream->work, scale3_stream_work, 0);
			os_run_work_delay(&scale3_stream->work, 1);

            //为每一个data进行分配空间
            stream_data_dis_mem_custom(s);
            
            //暂时用同样的iw ih ow oh
            scale_set_in_out_size(scale3_stream->scale_dev,scale3_stream->iw,scale3_stream->ih,scale3_stream->ow,scale3_stream->oh);
            scale_set_step(scale3_stream->scale_dev,scale3_stream->iw,scale3_stream->ih,scale3_stream->ow,scale3_stream->oh);
            scale_set_start_addr(scale3_stream->scale_dev,0,0);
            //暂时固定,如果遇到需要动态修改的,可以通过参数之类来切换
            scale_set_dma_to_memory(scale3_stream->scale_dev,1);
            scale_set_data_from_vpp(scale3_stream->scale_dev,1);
            scale_set_line_buf_num(scale3_stream->scale_dev,32);
            scale_set_in_yaddr(scale3_stream->scale_dev,(uint32)scale3_stream->scaler3buf);
            scale_set_in_uaddr(scale3_stream->scale_dev,(uint32)scale3_stream->scaler3buf+scale3_stream->iw*32);
            scale_set_in_vaddr(scale3_stream->scale_dev,(uint32)scale3_stream->scaler3buf+scale3_stream->iw*32+scale3_stream->iw*8);

            //这里分配一下scale3的空间,通过标准接口去分配吧,理论这里一定能获取到,这里就不判断异常情况了
            struct data_structure  *data;
            uint8_t *p_buf;
            data = get_src_data_f(s);
            p_buf = (uint8_t*)get_stream_real_data(data);
            scale3_stream->now_data = data;

            scale_set_out_yaddr(scale3_stream->scale_dev,(uint32)p_buf);
            scale_set_out_uaddr(scale3_stream->scale_dev,(uint32)p_buf+scale3_stream->ow*scale3_stream->oh);
            scale_set_out_vaddr(scale3_stream->scale_dev,(uint32)p_buf+scale3_stream->ow*scale3_stream->oh+scale3_stream->ow*scale3_stream->oh/4);
            scale_request_irq(scale3_stream->scale_dev,FRAME_END,scale3_stream_done,(uint32)scale3_stream);
            scale_request_irq(scale3_stream->scale_dev,INBUF_OV,scale3_stream_ov,(uint32)scale3_stream);
            scale_open(scale3_stream->scale_dev);
            //os_printf("%s:%d\t%X\n",__FUNCTION__,__LINE__,scale3_stream->scale_dev);
            struct vpp_device *vpp_dev;
            vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
            vpp_open(vpp_dev);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
            struct scale3Stream *scale3_stream = (struct scale3Stream *)s->priv;
            struct data_structure *data = (struct data_structure *)priv;
            //os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
            data->type = SET_DATA_TYPE(YUV,scale3_stream->type);
            data->priv = (struct yuv_stream_s *)SCALE3STREAM_ZALLOC(sizeof(struct yuv_arg_s));  
            struct yuv_arg_s *yuv = (struct yuv_arg_s*)data->priv;
            //由于创建流的时候,参数已经分配好了,所以这里可以确定参数
            yuv->y_size = scale3_stream->ow*scale3_stream->oh;
            yuv->uv_off = 0;
            yuv->y_off = 0;
            yuv->out_w = scale3_stream->ow;
            yuv->out_h = scale3_stream->oh;
            yuv->del = &scale3_stream->del;
            
            set_stream_real_data_len(data,scale3_stream->ow*scale3_stream->oh*3/2);
            //尝试申请一下空间
            data->data = SCALE3STREAM_MALLOC(scale3_stream->ow*scale3_stream->oh*3/2);
            sys_dcache_clean_invalid_range(data->data,scale3_stream->ow*scale3_stream->oh*3/2);
		}
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data)
            {
                sys_dcache_clean_invalid_range(get_stream_real_data(data),get_stream_real_data_len(data));
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
            struct data_structure *data = (struct data_structure *)priv;
            SCALE3STREAM_FREE(data->data);
            SCALE3STREAM_FREE(data->priv);
		}
		break;

        case STREAM_CLOSE_ENTER:
        {
            struct scale3Stream *scale3_stream = (struct scale3Stream *)s->priv;
            os_work_cancle2(&scale3_stream->work,1);
        }
        break;

        case STREAM_CLOSE_EXIT:
        {
            struct scale3Stream *scale3_stream = (struct scale3Stream *)s->priv;
            scale3_stream->del = 1;
            //停止workqueue
            os_work_cancle2(&scale3_stream->work,1);
            scale_close(scale3_stream->scale_dev);

            //中断已经获取了,但是还没有发送或者准备好数据
            if(scale3_stream->now_data)
            {
                force_del_data(scale3_stream->now_data);
            }

            //中断已经发送了,但是workqueue还没有接收
            struct data_structure  *data;
            int32_t err = 0;
            while(!err)
            {
                data = (struct data_structure*)os_msgq_get2(&scale3_stream->msgq,0,&err);
                //os_printf("%s:%d\tdata:%X\terr:%d\n",__FUNCTION__,__LINE__,data,err);
                if(!err)
                {
                    force_del_data(data);
                }
            }


            os_printf("scale3_stream->msgq.queue:%x\n",*(uint32_t*)(scale3_stream->msgq.hdl));

            os_msgq_del(&scale3_stream->msgq);
            
            break;
        }

        case STREAM_MARK_GC_END:
        {
            struct scale3Stream *scale3_stream = (struct scale3Stream *)s->priv;
            if(scale3_stream)
            {
                SCALE3STREAM_LIBC_FREE(scale3_stream);
            }
            break;
            
        }

		default:
			//默认都返回成功
		break;
	}
	return res;
}




//参数分别是vpp的图像iw和ih,要scale的ow和oh,如果和屏有关,可以传入屏幕的宽高
stream *scale3_stream(const char *name,uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh,uint16_t type)
{

    struct scale3_stream_arg_s msg;
    msg.iw = iw;
    msg.ih = ih;
    msg.ow = ow;
    msg.oh = oh;
    msg.type = type;


    stream *s = open_stream_available(name,SCALE3STREAM_BUF_NUM,0,opcode_func,&msg);
	return s;
}

//不绑定流,需要由外部绑定然后使能,这样可以动态绑定,不需要创建后就绑定(如果运行过程绑定,可能会有异步问题,所以暂时没有使用动态绑定方式)
stream *scale3_stream_not_bind(const char *name,uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh,uint16_t type)
{

    struct scale3_stream_arg_s msg;
    msg.iw = iw;
    msg.ih = ih;
    msg.ow = ow;
    msg.oh = oh;
    msg.type = type;


    stream *s = open_stream_available(name,SCALE3STREAM_BUF_NUM,0,opcode_func_not_bind,&msg);
	return s;
}
#else
stream *scale3_stream_not_bind(const char *name,uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh,uint16_t type)
{
    os_printf("%s:%d err,dvp not open\n",__FUNCTION__,__LINE__);
    return NULL;
}
#endif