#include "osd_encode_stream.h"
#include "osal/work.h"
#include "custom_mem/custom_mem.h"


//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc


struct lvgl_osd_encode_s
{
	struct os_work work;
	struct lcdc_device *lcd_dev;
	stream *s;
	uint8_t *osd_tmp_buf;
	uint32_t osd_tmp_buf_size;
	//如果data_send有,并且useful可用,代表data_send需要发送出去
	struct data_structure *parent_data_s;
	uint32_t parent_type;
	//osd压缩后的数据长度
	uint32_t data_len;
	//解码模块是否可用
	uint8_t hardware_ready;
};

static int32 osd_encode_work(struct os_work *work)
{
	struct lvgl_osd_encode_s *osd_encode = (struct lvgl_osd_encode_s*)work;
	struct data_structure *data_s = NULL;
	//static int osd_encode_count = 0;
	static uint32_t osd_send_times = 0;
	//先检测一下解码模块是否可用
	if(osd_encode->hardware_ready)
	{
		//os_printf("osd_encode work!!!!!!!!!!!!!!!!\n");
		//检测一下是否有data_s需要先发送出去
		if(osd_encode->parent_data_s)
		{
			//获取data_s,申请空间,发送数据
			data_s = get_src_data_f(osd_encode->s);
			if(data_s)
			{
				data_s->type = osd_encode->parent_data_s->type;
				free_data(osd_encode->parent_data_s);
				osd_encode->parent_data_s = NULL;

				//为data_s申请空间
				data_s->data = (void*)STREAM_MALLOC(osd_encode->data_len);
				if(data_s->data)
				{

					//os_printf("memcpy buf:%X\n",stream_self_cmd_func(osd_encode->s,OSD_HARDWARE_ENCODE_BUF,0));
					//拷贝数据
					hw_memcpy(data_s->data,(void*)stream_self_cmd_func(osd_encode->s,OSD_HARDWARE_ENCODE_BUF,0),(uint32)osd_encode->data_len);
					sys_dcache_clean_range(data_s->data, osd_encode->data_len); 
					//数据类型保持一致
					
					set_stream_real_data_len(data_s,osd_encode->data_len);
					//os_printf("%s:%d\tdata_s:%X\tosd_encode->data_len:%d\n",__FUNCTION__,__LINE__,data_s,osd_encode->data_len);
					send_data_to_stream(data_s);
					osd_send_times++;

				}
				//没有发送成功,则释放data_s先吧,下一次再发送
				else
				{
					force_del_data(data_s);
					_os_printf("**********************");
				}
			}
		}
		

		//重新检测一下,parent_data_s是否还存在,不存在就去看看是否有需要压缩的osd
		//会进入中断
		if(!osd_encode->parent_data_s)
		{
			osd_encode->parent_data_s = recv_real_data(osd_encode->s);
			
			if(osd_encode->parent_data_s)
			{


				//判断一下osd_tmp_buf_size是否足够,如果足够就不需要重新申请空间,否则重新申请一下空间
				if(osd_encode->osd_tmp_buf_size < get_stream_real_data_len(osd_encode->parent_data_s))
				{
					//释放原来空间
					if(osd_encode->osd_tmp_buf)
					{
						STREAM_FREE(osd_encode->osd_tmp_buf);
					}
					
					//重新申请一下空间
					osd_encode->osd_tmp_buf = (uint8_t*)STREAM_MALLOC(get_stream_real_data_len(osd_encode->parent_data_s));
					//申请失败,不压缩了,丢弃
					if(!osd_encode->osd_tmp_buf)
					{
						osd_encode->osd_tmp_buf_size = 0;
						free_data(osd_encode->parent_data_s);
						osd_encode->parent_data_s = NULL;
						goto osd_encode_work_end;

					}

					osd_encode->osd_tmp_buf_size = get_stream_real_data_len(osd_encode->parent_data_s);
				}
				//硬件模块开始被使用,标志置一下
				stream_self_cmd_func(osd_encode->s,OSD_HARDWARE_REAY_CMD,0);
				//去压缩
                //先清除cache
                sys_dcache_clean_invalid_range((uint32_t *)stream_self_cmd_func(osd_encode->s,OSD_HARDWARE_ENCODE_BUF,0), osd_encode->osd_tmp_buf_size);
				lcdc_set_osd_enc_src_addr(osd_encode->lcd_dev,(uint32)get_stream_real_data(osd_encode->parent_data_s));
				lcdc_set_osd_enc_dst_addr(osd_encode->lcd_dev,(uint32)stream_self_cmd_func(osd_encode->s,OSD_HARDWARE_ENCODE_BUF,0));
				lcdc_set_osd_enc_src_len(osd_encode->lcd_dev,get_stream_real_data_len(osd_encode->parent_data_s));
				//gpio_set_val(PA_15,1);
				lcdc_osd_enc_start_run(osd_encode->lcd_dev,1);
			}
		}
	
	}
	osd_encode_work_end:
    os_run_work_delay(work, 1);
	return 0;
}

static uint32_t osd_stream_cmd_func(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
	struct lvgl_osd_encode_s *osd_encode = (struct lvgl_osd_encode_s*)s->priv;
//    uint32_t flags;
    switch(opcode)
    {
		//设置硬件是否准备好
		case OSD_HARDWARE_REAY_CMD:
			osd_encode->hardware_ready = arg;
		break;
		//设置压缩后的数据长度
		case OSD_HARDWARE_ENCODE_SET_LEN:
			osd_encode->data_len = arg;
		break;
		case OSD_HARDWARE_ENCODE_GET_LEN:
			res = osd_encode->data_len;
		break;
		//返回压缩的buf
		case OSD_HARDWARE_ENCODE_BUF:
			res = (uint32_t)osd_encode->osd_tmp_buf;
		break;
		case OSD_HARDWARE_DEV:
			res = (uint32_t)osd_encode->lcd_dev;
		break;

		case OSD_HARDWARE_STREAM_RESET:
		{
            #if 0
			os_printf("%s:%d\n",__FUNCTION__,__LINE__);
			if(osd_encode->parent_data_s)
			{
				free_data(osd_encode->parent_data_s);
				osd_encode->parent_data_s = NULL;
			}
            #endif
			//硬件模块可用
			osd_encode->hardware_ready = 1;
			OS_WORK_REINIT(&osd_encode->work);
			os_run_work(&osd_encode->work);
		}
		break;

		//硬件模块停止,workqueue停止
		case OSD_HARDWARE_STREAM_STOP:
			//停止硬件模块
			lcdc_osd_enc_start_run(osd_encode->lcd_dev,0);
			//workqueue停止
			os_work_cancle2(&osd_encode->work,1);
		break;

        default:
        break;
    }
    return res;
}

static int opcode_func_r_osd(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
			struct lvgl_osd_encode_s *osd_encode = (struct lvgl_osd_encode_s*)STREAM_LIBC_ZALLOC(sizeof(struct lvgl_osd_encode_s));

			if(osd_encode)
			{
				osd_encode->lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
				osd_encode->hardware_ready = 1;
				osd_encode->osd_tmp_buf_size = 0;
				osd_encode->osd_tmp_buf = NULL;
				
				s->priv = (void*)osd_encode;
				streamSrc_bind_streamDest(s,R_OSD_SHOW);
				register_stream_self_cmd_func(s,osd_stream_cmd_func);
				osd_encode->s = s;
				OS_WORK_INIT(&osd_encode->work, osd_encode_work, 0);
				os_run_work_delay(&osd_encode->work, 1);
				//启动一个任务去管理?或者workqueue都是可行的,就是实时性的问题
				//先用workqueue实现,如果性能不行再换成任务的形式吧
				enable_stream(s,1);
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

        case STREAM_DATA_FREE_END:
        break;


		//数据发送完成,可以选择唤醒对应的任务
		case STREAM_SEND_DATA_FINISH:
		break;


		//音频发送之前先拷贝到psram
		case STREAM_SEND_DATA_START:
		{
		}
		break;

		case STREAM_CLOSE_EXIT:
		{
			struct lvgl_osd_encode_s *osd_encode = (struct lvgl_osd_encode_s *)s->priv;
			os_work_cancle2(&osd_encode->work,1);
			//关闭硬件
			lcdc_osd_enc_start_run(osd_encode->lcd_dev,0);
			//释放资源
			free_data(osd_encode->parent_data_s);
			osd_encode->parent_data_s = NULL;

					//释放原来空间
			if(osd_encode->osd_tmp_buf)
			{
				STREAM_FREE(osd_encode->osd_tmp_buf);
				osd_encode->osd_tmp_buf_size = 0;
				osd_encode->osd_tmp_buf = NULL;
			}
		}
		break;

		case STREAM_MARK_GC_END:
		{
			if(s->priv)
			{
				STREAM_FREE(s->priv);
			}
		}
		break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}

//osd压缩是一个模块,所以在这里统一管理自己的模块,所有需要压缩的数据都到这里,然后再发出去
//然后类型可以跟着父流一致
//要预先申请一个空间给到osd压缩,最后再读取寄存器看看要拷贝多少压缩的数据
//所以空间是:w*h*2+压缩的size,压缩的size根据不同ui去压缩的,一般不会超过原来的size

stream *lvgl_R_osd_stream(const char *name)
{
	stream *s = open_stream_available(name,8,8,opcode_func_r_osd,NULL);
	return s;
}