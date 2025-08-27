#include "lvgl_osd_stream.h"
#include "custom_mem/custom_mem.h"
//lvgl中osd的流,主要用于copy,效率会低(占用内存少,如果想不copy,可能lvgl内部要用动态内存的buf或者等待压缩完毕后再重复使用对应buf,但需要改动内部实现),psram竞争加大,但为了统一流程
//统一发送到压缩流,由压缩流去管理压缩硬件模块
//这个是作为源头
static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
			stream_data_dis_mem_custom(s);
			//osd流去进行编码
			streamSrc_bind_streamDest(s,R_OSD_ENCODE);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
				struct data_structure *data = (struct data_structure *)priv;
				
				data->priv = (struct encode_data_s_callback*)custom_zalloc_psram(sizeof(struct encode_data_s_callback));
		}
		break;

		//使用固定的buf,所以不需要释放,但是要回调函数,通知lvgl已经完成
		case STREAM_DATA_FREE:
		{
				struct data_structure *data = (struct data_structure *)priv;
				struct encode_data_s_callback *callback = data->priv;
				if(callback->finish_cb)
				{
					callback->finish_cb(callback->user_data);
				}
				if(callback->free_cb)
				{
					callback->free_cb(callback->user_data,data->data);
				}
				data->data = NULL;
				callback->finish_cb = NULL;
				callback->free_cb = NULL;
				callback->user_data = NULL;

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

		default:
			//默认都返回成功
		break;
	}
	return res;
}

//拷贝osd到内存,然后发送出去,使用workqueue去作为任务吧
stream *lvgl_osd_stream(const char *name)
{
	stream *s = open_stream_available(name,2,0,opcode_func,NULL);
	return s;
}