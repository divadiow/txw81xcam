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

static stream *lvgl_yuv_s = NULL;
static int32 yuv_recv_work(struct os_work *work)
{
    static int flag = 0;
    struct data_structure *data_s;
    struct recv_jpg_yuv_s *yuv = (struct recv_jpg_yuv_s *)work;
    data_s = recv_real_data(yuv->s);
    if(data_s)
    {
        if(!flag)
        {
            flag = 0;
            //写卡
            {
                void *fp = osal_fopen("0:0.yuv","wb");
                os_printf("%s:%d\tfp:%X\n",__FUNCTION__,__LINE__,fp);
                if(fp)
                {
                    osal_fwrite(get_stream_real_data(data_s),1,get_stream_real_data_len(data_s),fp);
                    osal_fclose(fp);
                }
            }
        }

        free_data(data_s);
    }


    os_run_work_delay(work, 1);
	return 0;
}


static uint32_t lvgl_yuv_cmd_func(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
    switch(opcode)
    {
        case SET_LVGL_YUV_UPDATE_TIME:
        {
            struct recv_jpg_yuv_s *yuv = (struct recv_jpg_yuv_s *)s->priv;
            yuv->show_photo_time_start = arg;
            break;
        }
        case GET_LVGL_YUV_UPDATE_TIME:
        {
            struct recv_jpg_yuv_s *yuv = (struct recv_jpg_yuv_s *)s->priv;
            res = yuv->show_photo_time_start;
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
            s->priv = (void*)custom_zalloc(sizeof(struct recv_jpg_yuv_s));
            struct recv_jpg_yuv_s *yuv = (struct recv_jpg_yuv_s *)s->priv;
            yuv->s = s;
            register_stream_self_cmd_func(s,lvgl_yuv_cmd_func);
            //启动一个workqueue去接收数据
			//OS_WORK_INIT(&yuv->work, yuv_recv_work, 0);
			//os_run_work_delay(&yuv->work, 1);
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
            //先特殊处理一下某些帧不显示,遇到是需要转换rgb的,就不要接收
            if(GET_DATA_TYPE2(data->type) == LVGL_JPEG_TO_RGB)
            {
                res = 1;
                break;
            }
            
            if(!(GET_DATA_TYPE2(data->type)& YUV_P1))
            {
                res = 1;
                break;
            }
        }
		break;

		//流接收后,数据包也要检查是否需要过滤或者是不是因为逻辑条件符合需要过滤
		case STREAM_RECV_FILTER_DATA:
		break;

		default:
		break;
	}
	return res;
}




stream *yuv_recv_stream(const char *name)
{
    stream *s = open_stream_available(name,0,2,opcode_func,NULL);
    lvgl_yuv_s = s;
    return s;
}

//如果需要重新换页面,可以更新一下这个时间,可以保证旧的没有用的video数据不显示,保证显示最新的
void update_yuv_stream_time(uint32_t time)
{
    if(lvgl_yuv_s)
    {
        stream_self_cmd_func(lvgl_yuv_s,SET_LVGL_YUV_UPDATE_TIME,(uint32_t)time);
    }
}