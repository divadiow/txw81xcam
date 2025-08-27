#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "osal_file.h"
#include "recv_jpg_yuv_demo.h"
#include "custom_mem/custom_mem.h"
#include "yuv_stream.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "video_app/video_app.h"
#include "hal/prc.h"
#include "hal/jpeg.h"

//该流需要jpg参与编码
#if PRC_EN == 1

typedef struct
{
    void *arg1;
    void *arg2;
}custom_struct;

struct prc_arg_s
{
    stream *s;
    uint16_t type;
};

//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc


//给jpg_isr_init_prc注册使用
typedef void (*prc_callback)(void *d);


//要有一个公用的结构体
struct prc_pixel
{
	struct prc_device *prc_dev;
    stream *jpeg_s;

    
	//line_buf配置好后,可以不修改,因为line是sram空间,line_buf正常不会动态改变,所以只要配置一次就好了
	uint8_t *line_buf;
	uint8_t *yuv_buf;
	uint16_t w,h;
	uint8_t	itk;
	//硬件是否ready
	uint8_t hardward_ready;

	//注册一个回调函数,用于完成的时候进行回调,参数就是本结构体
	prc_callback finish_func;

};



struct prc_stream_s
{
    struct os_work work;
    stream *s;
    stream *jpeg_s;
    struct data_structure *now_data;
    struct prc_pixel prc_hdl;
    uint16_t filter_type;
};





void prc_finish(void *d)
{
	struct prc_pixel *pixel_prc = (struct prc_pixel*)d;
	pixel_prc->hardward_ready = 1;
	_os_printf("$");
}



static int32_t prc_kick(uint32 param)
{  
        //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
		struct prc_pixel *pixel_prc = (struct prc_pixel*)param;
        //要计算足够的长度
		if(pixel_prc->itk < (pixel_prc->h+0xf)/16) {
			hw_memcpy((void*)pixel_prc->line_buf,(void*)(pixel_prc->yuv_buf+pixel_prc->itk*16* pixel_prc->w), pixel_prc->w*16);
			hw_memcpy((void*)(pixel_prc->line_buf+ pixel_prc->w*16),(void*)(pixel_prc->yuv_buf+ pixel_prc->w*pixel_prc->h+pixel_prc->itk*4* pixel_prc->w), pixel_prc->w*4);
			hw_memcpy((void*)(pixel_prc->line_buf+ pixel_prc->w*16+4*pixel_prc->w),(void*)(pixel_prc->yuv_buf+ pixel_prc->w*pixel_prc->h+ pixel_prc->w*pixel_prc->h/4+pixel_prc->itk*4* pixel_prc->w), pixel_prc->w*4);
			pixel_prc->itk++;
            stream_self_cmd_func(pixel_prc->jpeg_s,PRC_MJPEG_KICK,1);
			
		} 
		else 
		{
			//done
			if(pixel_prc->finish_func)
			{
				pixel_prc->finish_func(pixel_prc);
			}
			

		}
		return 0;
}

int32_t done_prc(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2)
{
	return prc_kick(irq_data);
}



//软件去kick,从psram的yuv编码jpg
void soft_psram_to_jpg_prc(void *param,uint32 yuvbuf,uint32 w,uint32 h)
{
	struct prc_pixel *pixel_prc = (struct prc_pixel*)param;
	if(pixel_prc->hardward_ready)
	{
		pixel_prc->w = w;
		pixel_prc->h = h;
		pixel_prc->itk = 0;
        pixel_prc->yuv_buf = (uint8_t*)yuvbuf;
		pixel_prc->hardward_ready = 0;
		prc_set_yaddr(pixel_prc->prc_dev ,(uint32)pixel_prc->line_buf);
		prc_set_uaddr(pixel_prc->prc_dev ,(uint32)(pixel_prc->line_buf+ pixel_prc->w*16));
		prc_set_vaddr(pixel_prc->prc_dev ,(uint32)(pixel_prc->line_buf+ pixel_prc->w*16+4* pixel_prc->w));
		prc_kick((uint32_t)param);
	}

}


//yuv编码jpg的workqueue
static int32 yuv_to_jpg_stream_work(struct os_work *work)
{
    //检查硬件是否已经准备好,然后再决定是否重新编码
    struct prc_stream_s *prc = (struct prc_stream_s*)work;


    //已经获取到了data,需要等待编码完成才能编码下一张图
    if(prc->now_data)
    {
        if(prc->prc_hdl.hardward_ready)
        {
            free_data(prc->now_data);
            prc->now_data = NULL;
            //完成,下一次再去尝试编码
            goto yuv_to_jpg_stream_work_end;
        }
    }
    else
    {
        prc->now_data = recv_real_data(prc->s);
        if(!prc->s->enable)
        {
            free_data(prc->now_data);
            prc->now_data = NULL;
        }
        //去编码
        if(prc->now_data)
        {
            //先去判断一下w和h是否一致,没有一致,需要重新修改硬件的编码参数
            struct yuv_arg_s *yuv_msg = (struct yuv_arg_s *)stream_self_cmd_func(prc->now_data->s,YUV_ARG,(uint32_t)prc->now_data);

            if(yuv_msg->out_w != prc->prc_hdl.w || yuv_msg->out_h != prc->prc_hdl.h)
            {
                if(prc->prc_hdl.line_buf)
                {
                    STREAM_LIBC_FREE(prc->prc_hdl.line_buf);
                    prc->prc_hdl.line_buf = NULL;
                }
                prc->prc_hdl.line_buf = (uint8_t*)STREAM_LIBC_MALLOC(yuv_msg->out_w*16*3/2);
                if(!prc->prc_hdl.line_buf)
                {
                    //申请不到空间,就不编码,下一帧再去编码
                    free_data(prc->now_data);
                    prc->now_data = NULL;
                    prc->prc_hdl.w = ~0;
                    prc->prc_hdl.h = ~0;
                    goto yuv_to_jpg_stream_work_end;
                }
                //修改一下编码参数
                prc->prc_hdl.w = yuv_msg->out_w;
                prc->prc_hdl.h = yuv_msg->out_h;

                stream_self_cmd_func(prc->jpeg_s,STREAM_STOP_CMD,0);
                stream_self_cmd_func(prc->jpeg_s,RESET_MJPEG_FROM,0);

                stream_self_cmd_func(prc->jpeg_s,RESET_MJPEG_DPI,(prc->prc_hdl.w<<16 | prc->prc_hdl.h));
            }
            soft_psram_to_jpg_prc(&prc->prc_hdl,(uint32_t)get_stream_real_data(prc->now_data),yuv_msg->out_w,yuv_msg->out_h);
        }
    }



    yuv_to_jpg_stream_work_end:
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
            struct prc_arg_s *msg = (struct prc_arg_s*)priv;
            s->priv = (struct prc_stream_s*)STREAM_LIBC_ZALLOC(sizeof(struct prc_stream_s));
            
            struct prc_stream_s *prc_stream_msg = (struct prc_stream_s*)s->priv;
            prc_stream_msg->s = s;
            prc_stream_msg->filter_type = msg->type;
            struct prc_pixel *pixel_prc = &prc_stream_msg->prc_hdl;
            pixel_prc->hardward_ready = 1;
            pixel_prc->prc_dev = (struct prc_device *)dev_get(HG_PRC_DEVID);
            pixel_prc->finish_func = prc_finish;
            pixel_prc->w = ~0;
            pixel_prc->h = ~0;

            //申请一个line_buf,这里默认不会动态修改分辨率,所以固定申请空间
            custom_struct arg;
            arg.arg1 = (void*)done_prc;
            arg.arg2 = (void*)pixel_prc;
            stream_self_cmd_func(msg->s,PRC_REGISTER_ISR,(uint32_t)&arg);
            prc_stream_msg->jpeg_s =   msg->s;
            pixel_prc->jpeg_s  = msg->s;
            if(msg->s)
            {
                open_stream_again(msg->s);
            }
            OS_WORK_INIT(&prc_stream_msg->work, yuv_to_jpg_stream_work, 0);
            os_run_work_delay(&prc_stream_msg->work, 1);
			enable_stream(s,1);   
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_RECV_DATA_FINISH:
		break;
		//在发送到这个流的时候,进行数据包过滤
		case STREAM_FILTER_DATA:
        {
			struct data_structure *data = (struct data_structure *)priv;
            struct prc_stream_s *prc_stream_msg = (struct prc_stream_s*)s->priv;
			if(GET_DATA_TYPE1(data->type) != YUV)
			{

				res = 1;
				break;
			}
            //类型不符合,抛弃

			if(!(GET_DATA_TYPE2(data->type) & prc_stream_msg->filter_type))
			{

				res = 1;
				break;
			}
        }
		break;
		//流接收后,数据包也要检查是否需要过滤或者是不是因为逻辑条件符合需要过滤
		case STREAM_RECV_FILTER_DATA:
		break;


        case STREAM_MARK_GC_END:
        {
        }
            
        break;

        case STREAM_CLOSE_EXIT:
        {
            struct prc_stream_s *prc = (struct prc_stream_s*)s->priv;
            os_work_cancle2(&prc->work,1);

            if(prc->jpeg_s)
            {
                close_stream(prc->jpeg_s);
                prc->jpeg_s = NULL;
            }

            if(prc->now_data)
            {
                free_data(prc->now_data);
                prc->now_data = NULL;
            }

            if(prc->prc_hdl.line_buf)
            {
                STREAM_LIBC_FREE(prc->prc_hdl.line_buf);
                prc->prc_hdl.line_buf = NULL;
            }
            //关闭
            if(s->priv)
            {
                STREAM_LIBC_FREE(s->priv);
            }
        }
        break;


		default:
		break;
	}
	return res;
}


stream *prc_stream(const char *name,stream *d_s)
{
    uint16_t type = ~0;
    struct prc_arg_s msg;
    msg.type = type;
    msg.s = d_s;
    stream *s = open_stream_available(name,0,2,opcode_func,(void*)&msg);
    return s;
}


stream *prc_stream_filter(const char *name,stream *d_s,uint16_t type)
{
    struct prc_arg_s msg;
    msg.type = type;
    msg.s = d_s;
    stream *s = open_stream_available(name,0,2,opcode_func,(void*)&msg);
    return s;
}
#endif