#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/task.h"
#include "custom_mem/custom_mem.h"
#include "osal/work.h"


uint32_t rgb_limit(int32_t val)
{
    if(val > 255)
        return 255;
    else if(val < 0)
        return 0;
    else
        return val;
}

void *yuv2rgb(uint8_t *inbuf, uint32_t width, uint32_t height)
{
    uint16_t *outbuf = (uint16_t *)custom_malloc_psram(width*height*2);
    if(!outbuf)
    {
        return NULL;
    }
    uint8_t *ybuf = inbuf;
    uint8_t *ubuf = inbuf+width*height;
    uint8_t *vbuf = inbuf+width*height+width*height/4;
    int32_t temp = 0;
    int16_t y=0;
    int16_t u,v;
    uint8_t r,g,b;
    uint32_t uv_w = width/2;
    uint32_t uv_w_temp = 0;
    uint32_t uv_h_temp = 0;

    for(uint32_t i=0; i<height; i++) {
        for(uint32_t j=0; j<width; j++) {
            uv_w_temp = j/2;
            uv_h_temp = i/2;
            y = ybuf[i*width+j];
            u = ubuf[uv_h_temp*uv_w+uv_w_temp]-128;
            v = vbuf[uv_h_temp*uv_w+uv_w_temp]-128;
            temp = ((y<<10) + 1441*v)>>10;
            r = (uint8_t)rgb_limit(temp);
            temp = ((y<<10) - 354*u -734*v)>>10;
            g = (uint8_t)rgb_limit(temp);
            temp = ((y<<10) + 1842*u)>>10;
            b = (uint8_t)rgb_limit(temp);

            outbuf[i*width+j] = ((r&0xF8)<<8)+((g&0xFC)<<3)+((b&0xF8)>>3);
        }
    }
	return outbuf;

}



struct jpg_to_rgb_s
{
    struct os_work work;
    stream *s;
};
static int32 jpg_to_rgb_work(struct os_work *work)
{
    //static int flag = 0;
    struct jpg_to_rgb_s *jpg_to_rgb = (struct jpg_to_rgb_s *)work;
    struct data_structure *data_s;
    data_s = recv_real_data(jpg_to_rgb->s);
    if(data_s)
    {
        #if 0
        if(!flag)
        {
            flag = 1;
            //写卡
            {
                #if 0
                void *fp = osal_fopen("0:my.lv","wb");
                os_printf("%s:%d\tfp:%X\n",__FUNCTION__,__LINE__,fp);
                if(fp)
                {
                    osal_fwrite(get_stream_real_data(data_s),1,get_stream_real_data_len(data_s),fp);
                    osal_fclose(fp);
                }
                #else
                    void *rgb_buf = yuv2rgb(get_stream_real_data(data_s),64,64);
                    os_printf("%s:%d\trgb_buf:%X\n",__FUNCTION__,__LINE__,rgb_buf);
                    if(rgb_buf)
                    {
                        save_fs_lv_img_lzo("0:my.lv",rgb_buf,64,64,1);
                        custom_free_psram(rgb_buf);
                    }
                #endif
            }
        }
        #endif
        free_data(data_s);
        os_printf("!!!!!!!!!!!!!!!!!%s:%d\n",__FUNCTION__,__LINE__);
    }
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
            s->priv = (void*)custom_zalloc(sizeof(struct jpg_to_rgb_s));
            struct jpg_to_rgb_s *jpg_to_rgb = (struct jpg_to_rgb_s *)s->priv;
            jpg_to_rgb->s = s;
			OS_WORK_INIT(&jpg_to_rgb->work, jpg_to_rgb_work, 0);
			os_run_work_delay(&jpg_to_rgb->work, 1);
            
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
            //os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
			if(GET_DATA_TYPE1(data->type) != YUV || GET_DATA_TYPE2(data->type) != LVGL_JPEG_TO_RGB)
			{
				res = 1;
				break;
			}
        }
		break;

		//流接收后,数据包也要检查是否需要过滤或者是不是因为逻辑条件符合需要过滤
		case STREAM_RECV_FILTER_DATA:
        {
			struct data_structure *data = (struct data_structure *)priv;
            //os_printf("type1:%d\ttype2:%d\n",GET_DATA_TYPE1(data->type),GET_DATA_TYPE2(data->type));
			if(GET_DATA_TYPE1(data->type) != YUV)
			{

				res = 1;
				break;
			}
            //先特殊处理一下某些帧不显示,遇到是需要转换rgb的,就不要接收
            if(GET_DATA_TYPE2(data->type) != LVGL_JPEG_TO_RGB)
            {
                res = 1;
                break;
            }
        }
		break;

		default:
		break;
	}
	return res;
}

stream *YUV_TO_RGB_stream(const char *name)
{
    stream *s = open_stream_available(name,0,2,opcode_func,NULL);
    return s;
}
