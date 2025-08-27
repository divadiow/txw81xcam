#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "lib/lcd/lcd.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "jpg_decode_stream.h"
#include "custom_mem/custom_mem.h"


//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc






struct jpg_decode_s
{
    struct os_work work;
    stream *s;
    uint8_t *scaler2buf;  //scale的buf
    uint32_t scalerbuf_size;
	struct jpg_device *jpg_dev;
	struct scale_device *scale_dev;
    uint32_t last_decode_time;  //记录上一次解码的时间,预防有异常的时候可以计算超时
    uint32_t dec_y_offset;
    uint32_t dec_uv_offset;
    uint32_t scale_p1_w;
    uint32_t p1_w;
    uint32_t p1_h;


	//这两个值如果有用,会同时存在,不能只是存在一个
	//图片的data_s,等图片解码完成后,可以选择释放
	struct data_structure *parent_data_s;

	//当前的使用的data_s,已经分配好了空间
	struct data_structure *current_data_s;
    //硬件模块是否准备好
    uint8_t hardware_ready;
    uint8_t hardware_err;
    uint8_t is_register_isr;
};



struct jpg_decode_cmd_s
{
    struct jpg_decode_arg_s *msg;
    union
    {
        struct 
        {
            uint32_t w;
            uint32_t h;
            uint32_t rotate;
        }config;

        struct 
        {
            uint32_t in_w;
            uint32_t in_h;
            uint32_t out_w;
            uint32_t out_h;
        }in_out_size;

        struct 
        {
            uint32_t in_w;
            uint32_t in_h;
            uint32_t out_w;
            uint32_t out_h;
        }step;
    };


};









static void stream_jpg_decode_scale2_done(uint32 irq_flag,uint32 irq_data,uint32 param1){

    //struct jpg_decode_s *jpg_decode = (struct jpg_decode_s *)irq_data;
	//struct jpg_device *jpg_dev  = jpg_decode->jpg_dev;
	//struct scale_device *scale_dev = jpg_decode->scale_dev;
    //jpg_decode->hardware_ready = 1;
    //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    //_os_printf("S");

}

static void stream_jpg_decode_scale2_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	os_printf("sor2");
}


static void stream_jpg_decode_done(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2)
{
    struct jpg_decode_s *jpg_decode = (struct jpg_decode_s *)irq_data;
	//os_printf("!!!!!!!!!!!!!!!!decode finish\r\n");
    //_os_printf("D");
    jpg_decode->hardware_ready = 1;
}



//这里解码失败,暂时代表硬件完成,后面应该有错误标志位,主要为了解码失败移除当前frame
static void stream_jpg_decode_err(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2)
{
	struct jpg_decode_s *jpg_decode = (struct jpg_decode_s *)irq_data;
	os_printf("decode err\r\n");
    os_printf("jpg_de->parent_data_s data:%X\n",get_stream_real_data(jpg_decode->parent_data_s));
    //jpg_decode->hardware_ready = 1;
    jpg_decode->hardware_err = 1;
}



static uint32_t jpg_decode_stream_cmd_func(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
//    uint32_t flags;
    switch(opcode)
    {

        case JPG_DECODE_ARG:
        {
            struct data_structure  *data_s = (struct data_structure *)arg;
            res = (uint32_t)data_s->priv;
        }
        break;
        case YUV_ARG:
        {
            struct data_structure  *data_s = (struct data_structure *)arg;
            struct jpg_decode_arg_s *msg  = (struct jpg_decode_arg_s*)data_s->priv;
            res = (uint32_t)&msg->yuv_arg;
        }
        break;
        //配置解码的参数大小
        //配置输出到屏的size
		case JPG_DECODE_SET_CONFIG:
        {
            struct jpg_decode_cmd_s *cfg = (struct jpg_decode_cmd_s*)arg;
            struct jpg_decode_s *jpg_decode = (struct jpg_decode_s*)s->priv;
            jpg_decode->p1_w = cfg->config.w;
            jpg_decode->p1_h = cfg->config.h;
            jpg_decode->scale_p1_w = ((jpg_decode->p1_w+3)/4)*4;
            if(cfg->config.rotate)
            {
                jpg_decode->dec_y_offset  = jpg_decode->p1_w*(jpg_decode->p1_h-1);
                jpg_decode->dec_uv_offset = ((jpg_decode->scale_p1_w/2+3)/4)*4 * (jpg_decode->p1_h/2-1);
            }
            else
            {
                jpg_decode->dec_y_offset  = 0;
                jpg_decode->dec_uv_offset = 0;
            }

            cfg->msg->yuv_arg.y_off = jpg_decode->dec_y_offset;
            cfg->msg->yuv_arg.uv_off = jpg_decode->dec_uv_offset;
            cfg->msg->yuv_arg.y_size = jpg_decode->scale_p1_w*jpg_decode->p1_h;
        }
		break;

        //这只scale的输入参数大小和输出参数大小
        case JPG_DECODE_SET_IN_OUT_SIZE:
        {
            struct jpg_decode_cmd_s *cfg = (struct jpg_decode_cmd_s*)arg;
            struct jpg_decode_s *jpg_decode = (struct jpg_decode_s*)s->priv;
            scale_set_in_out_size(jpg_decode->scale_dev,cfg->in_out_size.in_w,cfg->in_out_size.in_h,cfg->in_out_size.out_w,cfg->in_out_size.out_h);
        }
            
        break;

        //设置scale的步长,这里可以决定是否裁剪,如果step设置过大,到最后就不会有数据,解码的图片会补充0(黑色)
        case JPG_DECODE_SET_STEP:
        {
            struct jpg_decode_cmd_s *cfg = (struct jpg_decode_cmd_s*)arg;
            struct jpg_decode_s *jpg_decode = (struct jpg_decode_s*)s->priv;
            scale_set_step(jpg_decode->scale_dev,cfg->step.in_w,cfg->step.in_h,cfg->step.out_w,cfg->step.out_h);
        }
            
        break;

        case JPG_DECODE_READY:
        {
            struct jpg_decode_s *jpg_decode = (struct jpg_decode_s*)s->priv;
            uint32 dst = (uint32)arg;
            //scale_close(jpg_decode->scale_dev);
            scale_set_out_yaddr(jpg_decode->scale_dev,(uint32)dst);
            scale_set_out_uaddr(jpg_decode->scale_dev,(uint32)dst+jpg_decode->scale_p1_w*jpg_decode->p1_h);
            scale_set_out_vaddr(jpg_decode->scale_dev,(uint32)dst+jpg_decode->scale_p1_w*jpg_decode->p1_h+jpg_decode->scale_p1_w*jpg_decode->p1_h/4);
            //os_printf("scale_p1_w:%d\tp1_w:%d\tp1_h:%d\r\n",jpg_decode->scale_p1_w,jpg_decode->p1_w,jpg_decode->p1_h);
            //如果空间不够,就重新申请把
            if(32+jpg_decode->p1_w*2+(32*SRAMBUF_WLEN*4*3)/2 > jpg_decode->scalerbuf_size)
            {
                if(jpg_decode->scaler2buf)
                {
                    STREAM_LIBC_FREE(jpg_decode->scaler2buf);
                }
                //默认一定申请到,没有做申请失败的处理
                jpg_decode->scaler2buf = STREAM_LIBC_MALLOC(32+jpg_decode->p1_w*2+(32*SRAMBUF_WLEN*4*3)/2);
                jpg_decode->scalerbuf_size = 32+jpg_decode->p1_w*2+(32*SRAMBUF_WLEN*4*3)/2;
            }

            
            scale_set_line_buf_addr(jpg_decode->scale_dev,(uint32)jpg_decode->scaler2buf);
            scale_set_srambuf_wlen(jpg_decode->scale_dev,SRAMBUF_WLEN);
            scale_set_start_addr(jpg_decode->scale_dev,0,0);



            if(!jpg_decode->is_register_isr)
            {
                scale_request_irq(jpg_decode->scale_dev,FRAME_END,(scale_irq_hdl )&stream_jpg_decode_scale2_done,(uint32)jpg_decode);	
                scale_request_irq(jpg_decode->scale_dev,INBUF_OV,(scale_irq_hdl )&stream_jpg_decode_scale2_ov_isr,(uint32)jpg_decode);
                jpg_request_irq(jpg_decode->jpg_dev,(jpg_irq_hdl )&stream_jpg_decode_done,JPG_IRQ_FLAG_JPG_DONE,(void *)jpg_decode);
                jpg_request_irq(jpg_decode->jpg_dev,(jpg_irq_hdl )&stream_jpg_decode_err,JPG_IRQ_FLAG_ERROR,(void *)jpg_decode);	
                jpg_decode->is_register_isr = 1;
            }

            jpg_decode_target(jpg_decode->jpg_dev,1);
            
            break;
        }

        case JPG_DECODE_START:
        {
            struct jpg_decode_s *jpg_decode = (struct jpg_decode_s*)s->priv;
            uint32 dst = (uint32)arg;
            jpg_decode->hardware_ready = 0;
            jpg_decode->last_decode_time = os_jiffies();
            scale_open(jpg_decode->scale_dev);
            jpg_decode_photo(jpg_decode->jpg_dev,dst);
            
        }
        break;
        default:
        break;
    }
    return res;
}



static int32 jpg_decode_work(struct os_work *work)
{
    struct jpg_decode_s *jpg_de = (struct jpg_decode_s *)work;
    struct data_structure *data_s;
    //static int count = 0;
    //检测解码模块是否完成或者超时代表解码失败
    //能进去,模块需要reset或者已经解码完毕,可以重新去解码,否则只能慢慢等超时或者硬件解码完成
    if(jpg_de->hardware_err || jpg_de->hardware_ready || os_jiffies() - jpg_de->last_decode_time > 1000)
    {
        if(jpg_de->current_data_s && (os_jiffies() - jpg_de->last_decode_time > 1000))
        {
            //解码可能失败了
            _os_printf("decode failed1\n");
            jpg_de->hardware_ready = 1;
            //无论是完成还是失败,都要释放这张图片了
            free_data(jpg_de->parent_data_s);
            jpg_de->parent_data_s = NULL;

            //不再解码了
            force_del_data(jpg_de->current_data_s);
            jpg_de->current_data_s = NULL;

            //这里最好将硬件模块停止
        }
        //解码异常
        else if(jpg_de->hardware_err)
        {
            _os_printf("decode failed2\n");
            jpg_de->hardware_err = 0;
            jpg_de->hardware_ready = 1;
            if(jpg_de->current_data_s)
            {
                //无论是完成还是失败,都要释放这张图片了
                free_data(jpg_de->parent_data_s);
                jpg_de->parent_data_s = NULL;

                //不再解码了
                force_del_data(jpg_de->current_data_s);
                jpg_de->current_data_s = NULL;
            }
        }
        //解码完成,检查有解码完的数据需要发送
        else if(jpg_de->current_data_s)
        {
            //os_printf("%s:%d\tbuf:%X\n",__FUNCTION__,__LINE__,get_stream_real_data(jpg_de->current_data_s));
            data_s = jpg_de->current_data_s;
            set_stream_data_time(data_s,get_stream_data_timestamp(jpg_de->parent_data_s));
            data_s->magic = jpg_de->parent_data_s->magic;
            //os_printf("%s:%d\tdata_s:%X\tdata->data:%X\n",__FUNCTION__,__LINE__,data_s,data_s->data);
            _os_printf("&");
            send_data_to_stream(data_s);
            //无论是完成还是失败,都要释放这张图片了
            free_data(jpg_de->parent_data_s);
            jpg_de->parent_data_s = NULL;
            jpg_de->current_data_s = NULL;
        }
        //硬件可用,但是current_data_s不存在
        //有两种可能
        //一、没有需要解码的图片
        //二、有需要解码的图片,但是data_s没有申请成功或者内存空间不足以去解码图片数据
        else
        {

        }





        //这里没有解码的图片,尝试去看看有没有需要解码图片
        if(!jpg_de->parent_data_s)
        {
            //接收图片,尝试看看是否要解码
            jpg_de->parent_data_s = recv_real_data(jpg_de->s);

            //需要解码,然后申请空间
            if(jpg_de->parent_data_s)
            {
                //os_printf("parent_data_s:%X\n",jpg_de->parent_data_s);
                goto start_decode;
            }
            //不需要解码
            else
            {
                goto not_decode;
            }
        }
        else
        {
            goto start_decode;
        }

    //开始尝试解码
    start_decode:
        data_s = get_src_data_f(jpg_de->s);
        //申请到data_s,申请解码空间
        if(data_s)
        {
            //struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s *)jpg_de->parent_data_s->priv;
            struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s *)stream_self_cmd_func(jpg_de->parent_data_s->s,JPG_DECODE_ARG,(uint32_t)jpg_de->parent_data_s);
            //没有找到,代表发送过来的不是jpg或者说对应参数没有配置,不解码
            if(msg)
            {
                //为data_s申请解码空间,申请不到下次申请
                data_s->data = (uint8 *)STREAM_MALLOC(msg->yuv_arg.out_w*msg->yuv_arg.out_h*3/2);
                
                if(data_s->data)
                {  

                    struct jpg_decode_cmd_s cfg;
                    memset(&cfg,0,sizeof(struct jpg_decode_cmd_s));
                    //提前配置好data的长度
                    set_stream_real_data_len(data_s,msg->yuv_arg.out_w*msg->yuv_arg.out_h*3/2);
                    memcpy(data_s->priv,msg,sizeof(struct jpg_decode_arg_s));
                    //将data_s的struct jpg_yuv_decode_msg_s接入cfg,后面有参数改动
                    cfg.msg = (struct jpg_decode_arg_s*)data_s->priv;
                    cfg.config.w = msg->yuv_arg.out_w;
                    cfg.config.h = msg->yuv_arg.out_h;
                    cfg.config.rotate = msg->rotate;
                    stream_self_cmd_func(jpg_de->s,JPG_DECODE_SET_CONFIG,(uint32_t)&cfg);


                    cfg.in_out_size.in_w = msg->decode_w;
                    cfg.in_out_size.in_h = msg->decode_h;
                    cfg.in_out_size.out_w = msg->yuv_arg.out_w;
                    cfg.in_out_size.out_h = msg->yuv_arg.out_h;
                    stream_self_cmd_func(jpg_de->s,JPG_DECODE_SET_IN_OUT_SIZE,(uint32_t)&cfg);

                    cfg.in_out_size.in_w  = msg->decode_w;
                    cfg.in_out_size.in_h  = msg->decode_h;
                    cfg.in_out_size.out_w = msg->step_w;
                    cfg.in_out_size.out_h = msg->step_h;

                    data_s->type = SET_DATA_TYPE(YUV,GET_DATA_TYPE2(jpg_de->parent_data_s->type));

                    void *ptr = get_stream_real_data(jpg_de->parent_data_s);
                    uint32_t len = get_stream_real_data_len(jpg_de->parent_data_s);
                    sys_dcache_clean_range((uint32_t*)((uint32_t)(ptr) & CACHE_CIR_INV_ADDR_Msk), len + ((uint32_t)(ptr)-((uint32_t)(ptr) & CACHE_CIR_INV_ADDR_Msk)));
                    
                    stream_self_cmd_func(jpg_de->s,JPG_DECODE_SET_STEP,(uint32_t)&cfg);
                    stream_self_cmd_func(jpg_de->s,JPG_DECODE_READY,(uint32_t)get_stream_real_data(data_s));
                    stream_self_cmd_func(jpg_de->s,JPG_DECODE_START,(uint32_t)ptr);
                    jpg_de->current_data_s = data_s;
                }
                else
                {
                    force_del_data(data_s);
                    data_s = NULL;
                }
            }
            else
            {
                os_printf("%s:%d\tdecode msg isn't normal\tmsg:%X\tname:%s\n",__FUNCTION__,__LINE__,msg,jpg_de->parent_data_s->s->name);
                force_del_data(data_s);
                data_s = NULL;
            }

        }

        
    }
    not_decode:
    os_run_work_delay(work, 1);
	return 0;
}

//这个流主要是为了解码流使用
//为了充分利用psram，这里需要申请psram,可能效率会低,但是空间不至于容易浪费
//只有在极端情况下才会需要比原来更多的内存,但可以通过错开时间使用,来缓解这个问题
static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{

            struct jpg_decode_s *jpg_de = (struct jpg_decode_s*)STREAM_LIBC_ZALLOC(sizeof(struct jpg_decode_s));
            jpg_de->jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);
            jpg_de->scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);
            //通过命令去申请空间以及配置寄存器,如果停止,则需要释放空间
            jpg_de->scaler2buf = NULL;
			s->priv = (void*)jpg_de;
            register_stream_self_cmd_func(s,jpg_decode_stream_cmd_func);
            stream_data_dis_mem_custom(s);
            streamSrc_bind_streamDest(s,R_VIDEO_P0);
            streamSrc_bind_streamDest(s,R_VIDEO_P1);
            streamSrc_bind_streamDest(s,R_JPG_TO_RGB);
            streamSrc_bind_streamDest(s,R_YUV_TO_JPG);
            //启动一个workqueueu去接收图片,然后将图片解码成yuv数据,并且发送出去
            jpg_de->s = s;
			OS_WORK_INIT(&jpg_de->work, jpg_decode_work, 0);
			os_run_work_delay(&jpg_de->work, 1);
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
            struct data_structure *data = (struct data_structure *)priv;
            data->priv = (struct jpg_decode_arg_s *)STREAM_LIBC_ZALLOC(sizeof(struct jpg_decode_arg_s));    
		}
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                //os_printf("%s:%d\tdata:%X\tdata->data:%X\n",__FUNCTION__,__LINE__,data,data->data);
                STREAM_FREE(data->data);
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
            struct jpg_decode_s *jpg_de = (struct jpg_decode_s *)s->priv;
            os_work_cancle2(&jpg_de->work, 1);            
        }
        break;

        case STREAM_CLOSE_EXIT:
        {
            struct jpg_decode_s *jpg_de = (struct jpg_decode_s *)s->priv;
            os_work_cancle2(&jpg_de->work, 1);
            //关闭硬件
            scale_close(jpg_de->scale_dev);
            jpg_close(jpg_de->jpg_dev);

            if(jpg_de->parent_data_s)
            {
                os_printf("parent_data_s:%X\n",jpg_de->parent_data_s);
                free_data(jpg_de->parent_data_s);
                jpg_de->parent_data_s = NULL;
            }

            if(jpg_de->current_data_s)
            {
                os_printf("parent_data_s:%X\n",jpg_de->current_data_s);
                force_del_data(jpg_de->current_data_s);
                jpg_de->current_data_s = NULL;
            }

            //释放line_buf
            STREAM_LIBC_FREE(jpg_de->scaler2buf);


        }
        break;

        case STREAM_MARK_GC_END:
        {
            STREAM_LIBC_FREE(s->priv);
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

            struct jpg_decode_s *jpg_de = (struct jpg_decode_s*)STREAM_LIBC_ZALLOC(sizeof(struct jpg_decode_s));
            jpg_de->jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);
            jpg_de->scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);
            //通过命令去申请空间以及配置寄存器,如果停止,则需要释放空间
            jpg_de->scaler2buf = NULL;
            jpg_de->scalerbuf_size = 0;
			s->priv = (void*)jpg_de;
            register_stream_self_cmd_func(s,jpg_decode_stream_cmd_func);
            stream_data_dis_mem_custom(s);
            //启动一个workqueueu去接收图片,然后将图片解码成yuv数据,并且发送出去
            jpg_de->s = s;
			OS_WORK_INIT(&jpg_de->work, jpg_decode_work, 0);
			os_run_work_delay(&jpg_de->work, 1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
            struct data_structure *data = (struct data_structure *)priv;
            data->priv = (struct jpg_decode_arg_s *)STREAM_LIBC_ZALLOC(sizeof(struct jpg_decode_arg_s));    
            os_printf("jpg decode data->priv:%x\n",data->priv);
		}
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                //os_printf("%s:%d\tdata:%X\tdata->data:%X\n",__FUNCTION__,__LINE__,data,data->data);
                STREAM_FREE(data->data);
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
            struct jpg_decode_s *jpg_de = (struct jpg_decode_s *)s->priv;
            os_work_cancle2(&jpg_de->work, 1);            
        }
        break;

        case STREAM_CLOSE_EXIT:
        {
            struct jpg_decode_s *jpg_de = (struct jpg_decode_s *)s->priv;
            os_work_cancle2(&jpg_de->work, 1);
            //关闭硬件
            scale_close(jpg_de->scale_dev);
            jpg_close(jpg_de->jpg_dev);

            if(jpg_de->parent_data_s)
            {
                os_printf("parent_data_s:%X\n",jpg_de->parent_data_s);
                free_data(jpg_de->parent_data_s);
                jpg_de->parent_data_s = NULL;
            }

            if(jpg_de->current_data_s)
            {
                os_printf("parent_data_s:%X\n",jpg_de->current_data_s);
                force_del_data(jpg_de->current_data_s);
                jpg_de->current_data_s = NULL;
            }

            //释放line_buf
            STREAM_LIBC_FREE(jpg_de->scaler2buf);
            jpg_de->scalerbuf_size  = 0;


        }
        break;

        case STREAM_MARK_GC_END:
        {
            STREAM_LIBC_FREE(s->priv);
        }
        break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}



stream *jpg_decode_stream(const char *name)
{

	stream *s = open_stream_available(name,8,8,opcode_func,NULL);
	return s;
}


//不是立刻绑定,动态绑定
stream *jpg_decode_stream_not_bind(const char *name)
{

	stream *s = open_stream_available(name,8,8,opcode_func_not_bind,NULL);
	return s;
}