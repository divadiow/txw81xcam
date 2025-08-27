#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/task.h"
#include "jpg_decode_stream.h"
#include "osal/string.h"
#include "osal_file.h"
#include "custom_mem/custom_mem.h"
void read_file_thread(void*d)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    stream *s = (stream *)d;
    //struct data_structure  *data;
    void *fp = osal_fopen("0:0.jpg","rb");
    uint32_t filesize = 0;
    uint8_t *tmp = (uint8_t*)os_malloc(4096);
    uint8_t *sd_buf = NULL;
    uint32_t offset = 0;
    uint32_t readlen;
    if(fp)
    {
        filesize = osal_fsize(fp);
    }
    os_printf("%s:%d\ttmp:%X\n",__FUNCTION__,__LINE__,tmp);
    if(filesize)
    {
        sd_buf = (uint8_t*)custom_malloc_psram(filesize);
        if(sd_buf)
        {
            while(filesize)
            {
                if(filesize < 4096)
                {
                    readlen = osal_fread(tmp,filesize,1,fp);
                }
                else
                {
                    readlen = osal_fread(tmp,4096,1,fp);
                }
                if(readlen)
                {
                    hw_memcpy(sd_buf+offset,tmp,readlen);
                }
                offset += readlen;
                filesize -= readlen;
            }
        }
    }
    while(1)
    {
        struct data_structure  *data = get_src_data_f(s);
        if(data)
        {
            os_printf("data:%X\n",data);
            if(sd_buf)
            {
                data->data = (void*)custom_malloc_psram(offset);
                if(data->data)
                {
                    //配置显示的参数
                    struct jpg_decode_arg_s *msg = data->priv;
                    msg->yuv_arg.out_w = 320;
                    msg->yuv_arg.out_h = 240;
                    msg->rotate = 0;
                    msg->decode_w = 640;
                    msg->decode_h = 480;
                    msg->step_w = 320;
                    msg->step_h = 240;
                    hw_memcpy(data->data,sd_buf,offset);
                    set_stream_real_data_len(data,offset);
                    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
                    data->type = SET_DATA_TYPE(JPEG,JPEG_FILE);
                    int success = send_data_to_stream(data);
                    os_printf("success:%d\n",success);
                    data = NULL;
                }
                else
                {
                    force_del_data(data);
                }
            }
            else
            {
                force_del_data(data);
            }
            
        }
        os_sleep_ms(1000);
    }
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
			s->priv = priv;
            stream_data_dis_mem_custom(s);
            streamSrc_bind_streamDest(s,S_JPG_DECODE);
            enable_stream(s,1);
            //创建一个线程
            os_task_create("aaa", read_file_thread, s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
            struct data_structure *data = (struct data_structure *)priv;
            data->priv = (void *)custom_zalloc(sizeof(struct jpg_decode_arg_s));   
            data->data = NULL; 
		}
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
            if(data->data)
            {
                custom_free_psram(data->data);
                data->data = NULL;
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
			if(data->priv)
			{
				custom_free(data->priv);
			}
		}
		break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}

void *read_jpg_file_stream()
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    stream *s = open_stream_available("read_jpg", 3, 0, opcode_func,NULL);
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    return s;
}