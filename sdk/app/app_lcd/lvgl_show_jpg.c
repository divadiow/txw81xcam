#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/task.h"
#include "decode/jpg_decode_stream.h"
#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "osal_file.h"

//获取jpeg的w和h,没有做太多容错,所以尽量给的是正确的jpeg,否则可能异常
#define GET_16(p) (((p)[0]<<8)|(p)[1])

int parse_SOF(uint8_t *d,uint32_t *w,uint32_t *h )
{
	if( d[0] != 8 )
	{
		printf( "Invalid precision %d in SOF0\n", d[0] );
		return -1;
	}
	*h = GET_16( d + 1 );
	*w = GET_16( d + 3 );
	return 0;
}
int parse_jpg(uint8_t *jpg_buf,uint32_t maxsize,uint32_t *w,uint32_t *h)
{
	uint8_t *buf = jpg_buf;
	uint32_t i = 0;
	int res = 1;
	int blen = 0;
	for( i = 0; i < maxsize; i += blen + 2 )
	{
		if( buf[i] != 0xFF ) 
		{
			printf( "Found %02X at %d, expecting FF\n", buf[i], i );
			goto parse_jpg_end;
		}	
		while(buf[i+1] == 0xFF) ++i;
		if( buf[i + 1] == 0xD8 ) blen = 0;
		else blen = GET_16( buf + i + 2 );
		
		switch( buf[i + 1] )
		{
			case 0xDB: /* Quantization Table */
			break;
			case 0xC0: /* Start of Frame */
				parse_SOF(buf+i+4,w,h);
                res = 0;
				//printf("w:%d\th:%d\n",*w,*h);
			break;
			case 0xC4: /* Huffman Table */

			break;		
			case 0xDD:	/* DRI */
			break;
			case 0xDA: /* Start of Scan */
			goto parse_jpg_end;
		}
	}

	parse_jpg_end:
	return res;
}



static stream *lvgl_show_jpg_s = NULL;


//用于获取一些通用命令的接口,也可以加入自定义接口
//接口怎么获取数据只有源头知道,这里不同是通用的
static uint32_t lvgl_jpg_cmd_func(stream *s,int opcode,uint32_t arg)
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
        //这样返回代表结构体是兼容的
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



//out_w:最终显示的w
//
int lvgl_jpg_send(uint32_t *jpg_data,uint32_t jpg_size,uint32_t out_w,uint32_t out_h,uint32_t jpg_w,uint32_t jpg_h,uint32_t step_w,uint32_t step_h)
{
    int res = 1;
    struct data_structure  *data = get_src_data_f(lvgl_show_jpg_s);
    if(data)
    {
        data->data = (void*)custom_malloc_psram(jpg_size);
        if(data->data)
        {
           //配置显示的参数
           struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s *)data->priv;
           msg->yuv_arg.out_w = out_w;
           msg->yuv_arg.out_h = out_h;
           msg->rotate = 0;
           msg->decode_w = jpg_w;
           msg->decode_h = jpg_h;
           msg->step_w = step_w;
           msg->step_h = step_h;
           set_stream_real_data_len(data,jpg_size);
           set_stream_data_time(data,os_jiffies());
           data->type = SET_DATA_TYPE(JPEG,LVGL_JPEG);
           hw_memcpy0(data->data,jpg_data,jpg_size);
           int success = send_data_to_stream(data);
           res = 0;
           os_printf("jpg_size:%d\tsuccess:%d\n",jpg_size,success);
        }
    }
	return res;
}


int lvgl_jpg_send_type(uint32_t *jpg_data,uint32_t jpg_size,uint32_t type,uint32_t magic,uint32_t out_w,uint32_t out_h,uint32_t step_w,uint32_t step_h)
{
    int res = 1;
    struct data_structure  *data = get_src_data_f(lvgl_show_jpg_s);
    if(data)
    {
        data->data = (void*)custom_malloc_psram(jpg_size);
        if(data->data)
        {
            hw_memcpy0(data->data,jpg_data,jpg_size);
           //配置显示的参数
           struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s *)data->priv;
           msg->yuv_arg.out_w = out_w;
           msg->yuv_arg.out_h = out_h;
           msg->rotate = 0;
           msg->magic = magic;
           parse_jpg(data->data,jpg_size,&msg->decode_w,&msg->decode_h);
           
           msg->step_w = step_w;
           msg->step_h = step_h;
           set_stream_real_data_len(data,jpg_size);
           set_stream_data_time(data,os_jiffies());
           data->type = SET_DATA_TYPE(JPEG,type);
           
           int success = send_data_to_stream(data);
           res = 0;
           os_printf("jpg_size:%d\tsuccess:%d\n",jpg_size,success);
        }
    }
	return res;
}


int lvgl_jpg_from_fs_send(uint8_t *filename,uint32_t out_w,uint32_t out_h,uint32_t step_w,uint32_t step_h)
{
    int res = 1;
    void *fp = osal_fopen((const char *)filename,"rb");
    if(!fp)
    {
        goto lvgl_jpg_from_fs_send_end;
        return res;
    }
    struct data_structure  *data = get_src_data_f(lvgl_show_jpg_s);
    if(data)
    {
        uint32_t jpg_size = osal_fsize(fp);
        data->data = (void*)custom_malloc_psram(jpg_size);
        if(data->data)
        {
           //配置显示的参数
           struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s *)data->priv;
           msg->yuv_arg.out_w = out_w;
           msg->yuv_arg.out_h = out_h;
           msg->rotate = 0;
           //msg->decode_w = 640;
           //msg->decode_h = 480;
           msg->step_w = step_w;
           msg->step_h = step_h;
           set_stream_real_data_len(data,jpg_size);
           set_stream_data_time(data,os_jiffies());
           data->type = SET_DATA_TYPE(JPEG,LVGL_JPEG);
           osal_fread(data->data,1,jpg_size,fp);
           parse_jpg(data->data,jpg_size,&msg->decode_w,&msg->decode_h);
           os_printf("msg->decode_w:%d\tmsg->decode_h:%d\n",msg->decode_w,msg->decode_h);
           int success = send_data_to_stream(data);
           res = 0;
           os_printf("jpg_size:%d\tsuccess:%d\n",jpg_size,success);
        }
    }

    if(fp)
    {
        osal_fclose(fp);
    }
    lvgl_jpg_from_fs_send_end:
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
			s->priv = priv;
            register_stream_self_cmd_func(s,lvgl_jpg_cmd_func);
            stream_data_dis_mem_custom(s);
            streamSrc_bind_streamDest(s,S_JPG_DECODE);
            enable_stream(s,1);
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
            if(data->data)
            {
                os_printf("lvgl show stream %s:%d\n",__FUNCTION__,__LINE__);
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
//创建lvgl生成jpg的流,用于将jpg发送到解码的地方
//然后显示出来
void *lvgl_jpg_stream(const char *name)
{
    stream *s = open_stream_available(name, 5, 0, opcode_func,NULL);
    lvgl_show_jpg_s = s;
    return s;
}