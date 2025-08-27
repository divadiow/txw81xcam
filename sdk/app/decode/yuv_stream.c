#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "osal_file.h"
#include "jpg_decode_stream.h"
#include "recv_jpg_yuv_demo.h"
#include "custom_mem/custom_mem.h"
#include "yuv_stream.h"


static uint32_t yuv_stream_cmd_func(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
    switch(opcode)
    {
        case SET_LVGL_YUV_UPDATE_TIME:
        {
            struct yuv_stream_s *yuv = (struct yuv_stream_s *)s->priv;
            yuv->show_yuv_time_start = arg;
            break;
        }
        case GET_LVGL_YUV_UPDATE_TIME:
        {
            struct yuv_stream_s *yuv = (struct yuv_stream_s *)s->priv;
            res = yuv->show_yuv_time_start;
            break;
        }
        
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
            s->priv = (void*)custom_zalloc(sizeof(struct yuv_stream_s));
            struct yuv_stream_s *yuv = (struct yuv_stream_s *)s->priv;
            yuv->last_data_s = NULL;
            register_stream_self_cmd_func(s,yuv_stream_cmd_func);
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
			if(GET_DATA_TYPE1(data->type) != YUV)
			{
				res = 1;
				break;
			}

			if(!(GET_DATA_TYPE2(data->type)& YUV_P0))
			{
				res = 1;
				break;
			}
        }
		break;
		//流接收后,数据包也要检查是否需要过滤或者是不是因为逻辑条件符合需要过滤
		case STREAM_RECV_FILTER_DATA:
		break;

        case STREAM_CLOSE_EXIT:
        {
            struct yuv_stream_s *yuv = (struct yuv_stream_s *)s->priv;
            //相当于已经不需要显示了,接收到对应帧的流需要主动释放该流
            yuv->show_yuv_time_start = -1;
        }
        break;

        case STREAM_MARK_GC_END:
        {
            struct yuv_stream_s *yuv = (struct yuv_stream_s *)s->priv;
            if(yuv)
            {
                custom_free(yuv);
            }
        }
            
        break;


		default:
		break;
	}
	return res;
}




stream *yuv_stream(const char *name)
{
    stream *s = open_stream_available(name,0,2,opcode_func,NULL);
    return s;
}
