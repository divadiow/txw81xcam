
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
#include "hal/dma.h"

//暂定这个scale3是在存在dvp的时候使用,所以用宏隔离
//实际scale3还有另一种模式不需要dvp那边启动的
#if DVP_EN == 1


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

struct scale1_stream_arg_s
{
    uint16_t iw;
    uint16_t ih;
    uint16_t ow;
    uint16_t oh;
};

struct scale1Stream
{
    struct os_work work;
    stream *s;
    uint8_t *scaler1buf;  //scale的buf
    struct scale_device *scale_dev;
    struct dma_device *dma1;
    uint16_t iw,ih,ow,oh;
    uint8_t dma1_lock;
};


static int32_t scale_done(uint32 irq_flag,uint32 irq_data,uint32 param1)
{
    _os_printf("=");
    return 0;
}

static int32_t scale_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1)
{
	os_printf("sor");
    return 0;
}


static int32 scale1_stream_work(struct os_work *work)
{
    struct scale1Stream *scale1_stream = (struct scale1Stream*)work;
    if(scale1_stream->dma1_lock)
    {
        goto scale1_stream_work_lock;
    }
    //
    dma_ioctl(scale1_stream->dma1, DMA_IOCTL_CMD_DMA1_LOCK, 0, 0);
    scale1_stream->dma1_lock = dma_ioctl(scale1_stream->dma1, DMA_IOCTL_CMD_CHECK_DMA1_STATUS, 0, 0);
    os_printf("scale1_stream->dma1:%X\n",scale1_stream->dma1);
    os_printf("scale1_stream->dma1_lock:%d\n",scale1_stream->dma1_lock);

scale1_stream_work_lock:
    if(scale1_stream->dma1_lock)
    {
        uint16_t s_w,s_h,d_w,d_h;
        struct scale_device *scale_dev = scale1_stream->scale_dev;
        uint8_t *yuvbuf_addr = scale1_stream->scaler1buf;
        s_w = scale1_stream->iw;
        s_h = scale1_stream->ih;
        d_w = scale1_stream->ow;
        d_h = scale1_stream->oh;
        scale_close(scale_dev);
        scale_set_in_out_size(scale_dev,s_w,s_h,d_w,d_h);
        scale_set_step(scale_dev,s_w,s_h,d_w,d_h);
        scale_set_line_buf_num(scale_dev,32);
        scale_set_in_yaddr(scale_dev,(uint32_t)yuvbuf_addr);
        scale_set_in_uaddr(scale_dev,(uint32_t)yuvbuf_addr+s_w*32);
        scale_set_in_vaddr(scale_dev,(uint32_t)yuvbuf_addr+s_w*32+s_w*8);	
        scale_request_irq(scale_dev,FRAME_END,scale_done,(uint32)scale_dev);	
        scale_request_irq(scale_dev,INBUF_OV,scale_ov_isr,(uint32)scale_dev);
        scale_set_data_from_vpp(scale_dev,1);  
        scale_open(scale_dev);
        os_printf("%s lock success\n",__FUNCTION__,__LINE__);
    }
    //下一次重新获取
    else
    {
        os_run_work_delay(work, 1000);
    }
    return 0;
}


static uint32_t stream_cmd_func(stream *s,int cmd,uint32_t arg)
{
    uint32_t res = 0;
    switch(cmd)
    {
        //支持配置输出,因为摄像头应该是固定的,除非多摄像头要切换,才需要对输入源也要修改
        case SCALE1_RESET_DPI:
        {
            struct scale1Stream *scale1_stream = (struct scale1Stream*)s->priv;
			uint32_t w_h = (uint32_t)arg;
			scale1_stream->ow = w_h>>16;
			scale1_stream->oh = w_h&0xffff;
            //启动work,重新配置一下分辨率
            OS_WORK_REINIT(&scale1_stream->work);
			os_run_work_delay(&scale1_stream->work, 1);
        }
        break;

        case STREAM_STOP_CMD:
        {
            struct scale1Stream *scale1_stream = (struct scale1Stream*)s->priv;
            os_work_cancle2(&scale1_stream->work,1);
            scale_close(scale1_stream->scale_dev);
        }
        break;
        default:
        break;
    }
	return res;
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
            struct scale1_stream_arg_s *msg = (struct scale1_stream_arg_s*)priv;
            struct scale1Stream *scale1_stream = (struct scale1Stream*)SCALE3STREAM_LIBC_ZALLOC(sizeof(struct scale1Stream));
            scale1_stream->dma1 = (struct dma_device *)dev_get(HG_M2MDMA_DEVID); 
            s->priv = (void*)scale1_stream;
            scale1_stream->iw = msg->iw;
            scale1_stream->ih = msg->ih;
            scale1_stream->ow = msg->ow;
            scale1_stream->oh = msg->oh;
            extern uint8 *yuvbuf;
            scale1_stream->scaler1buf = yuvbuf;
            scale1_stream->scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
            register_stream_self_cmd_func(s,stream_cmd_func);	
			OS_WORK_INIT(&scale1_stream->work, scale1_stream_work, 0);
			os_run_work_delay(&scale1_stream->work, 1);
        
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

        case STREAM_CLOSE_EXIT:
        {
            struct scale1Stream *scale1_stream = (struct scale1Stream *)s->priv;
            os_work_cancle2(&scale1_stream->work,1);

            //关闭scale1
            scale_close(scale1_stream->scale_dev);
            if(scale1_stream->dma1_lock)
            {
                dma_ioctl(scale1_stream->dma1, DMA_IOCTL_CMD_DMA1_UNLOCK, 0, 0);
            }

            SCALE3STREAM_LIBC_FREE(scale1_stream);
            break;
        }

        case STREAM_MARK_GC_END:
        {
            break;
            
        }

		default:
			//默认都返回成功
		break;
	}
	return res;
}




//不绑定流,需要由外部绑定然后使能,这样可以动态绑定,不需要创建后就绑定(如果运行过程绑定,可能会有异步问题,所以暂时没有使用动态绑定方式)
stream *scale1_stream_not_bind(const char *name,uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh)
{
    struct scale1_stream_arg_s msg;
    msg.iw = iw;
    msg.ih = ih;
    msg.ow = ow;
    msg.oh = oh;
    stream *s = open_stream_available(name,0,0,opcode_func,&msg);
	return s;
}
#else
stream *scale1_stream_not_bind(const char *name,uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh)
{
    os_printf("%s:%d err,dvp not open\n",__FUNCTION__,__LINE__);
    return NULL;
}
#endif