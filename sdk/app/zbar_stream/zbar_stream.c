
#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/work.h"
#include "osal/task.h"
#include "stream_frame.h"
#include "custom_mem.h"
#include "zbar.h"
#include "osal/event.h"


typedef struct 
{
    stream *s;
    struct os_task task;
    struct os_event evt;
    uint32_t filter;
}zbar_stream_s;

//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc

enum
{
    ZBAR_STREAM_STOP = BIT(0),
    ZBAR_STREAM_EXIT = BIT(1),
};

extern int mask_type;
zbar_image_scanner_t *zbar_image_scanner_create ();
void zbar_set_image_tmp_buf(zbar_image_t *img,unsigned char *buf);
static void zbar_work_thread(void *d)
{
    struct data_structure *zbar_data_s;
    zbar_image_scanner_t *scanner = NULL;
    zbar_image_t *image;
    zbar_symbol_type_t typ_mask;
    
    uint8_t *tmp_buf = NULL;
    uint32_t now_zbar_tmp_buf_size = 0;
    struct yuv_arg_s *yuv_msg;
    uint32_t out_w,out_h;
    zbar_stream_s *msg_s = (zbar_stream_s *)d;
    os_printf("%s start\n",__FUNCTION__);
    mask_type = ZBAR_NONE;
    scanner = zbar_image_scanner_create();
    while(os_event_wait(&msg_s->evt,ZBAR_STREAM_STOP,NULL,OS_EVENT_WMODE_OR,0))
    {
        zbar_data_s = recv_real_data(msg_s->s);
        if(zbar_data_s)
        {
            _os_printf("Z");
            yuv_msg = (struct yuv_arg_s *)stream_self_cmd_func(zbar_data_s->s,YUV_ARG,(uint32_t)zbar_data_s);
            out_w = yuv_msg->out_w;
            out_h = yuv_msg->out_h;
            
            zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
            zbar_image_scanner_set_config(scanner, ZBAR_QRCODE, ZBAR_CFG_UNCERTAINTY, 1);


            image = zbar_image_create();
            zbar_image_set_format(image, *(int*)"Y800");
            zbar_image_set_size(image, out_w, out_h);
            zbar_image_set_data(image, get_stream_real_data(zbar_data_s), out_w * out_h, zbar_image_free_data);
            //分配空间给二维码使用
            if(tmp_buf)
            {
                if(now_zbar_tmp_buf_size < out_w*out_h)
                {
                    STREAM_FREE(tmp_buf);
                    now_zbar_tmp_buf_size = 0;
                    tmp_buf = NULL;
                }
            }

            if(!tmp_buf)
            {
                tmp_buf = STREAM_MALLOC(out_w*out_h);
                if(tmp_buf)
                {
                    now_zbar_tmp_buf_size = out_w*out_h;
                }
				else
                {
                    os_printf("%s malloc failed\n",__FUNCTION__);
                }
            }
            zbar_set_image_tmp_buf(image,tmp_buf);
            zbar_scan_image(scanner, image);
            const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
            if(symbol == NULL){
                typ_mask = ZBAR_NONE;
            }

            for(; symbol; symbol = zbar_symbol_next(symbol)) 
            {
                /* do something useful with results */
                zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
                typ_mask = typ;
                const char *data = zbar_symbol_get_data(symbol);
                os_printf("decoded %s symbol \"%s\r\n",zbar_get_symbol_name(typ), data);
            }
            if((typ_mask == ZBAR_EAN2)||(typ_mask == ZBAR_EAN5)||(typ_mask == ZBAR_EAN8)||(typ_mask == ZBAR_EAN13)||(typ_mask == ZBAR_QRCODE)||(typ_mask == ZBAR_CODE128)){
                mask_type = typ_mask;
            }
            zbar_image_destroy(image);
            free_data(zbar_data_s);
        }
        os_sleep_ms(1);
    }
    zbar_image_scanner_destroy(scanner);

    if(tmp_buf)
    {
        STREAM_FREE(tmp_buf);
    }
    os_printf("%s end\n",__FUNCTION__);
    //退出线程
    os_event_set(&msg_s->evt,ZBAR_STREAM_EXIT,NULL);

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
            zbar_stream_s *msg_s = (zbar_stream_s*)STREAM_LIBC_ZALLOC(sizeof(zbar_stream_s));
            os_event_init(&msg_s->evt);
            msg_s->filter = (uint32_t)priv;
            msg_s->s = s;
            s->priv = (void*)msg_s;
            enable_stream(s,1);
            OS_TASK_INIT("test",&msg_s->task,zbar_work_thread,msg_s,OS_TASK_PRIORITY_NORMAL,8192);
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

			struct data_structure *data = (struct data_structure *)priv;
            zbar_stream_s *msg_s = (zbar_stream_s *)s->priv;
			if(GET_DATA_TYPE1(data->type) != YUV)
			{
				res = 1;
				break;
			}

			if(!(GET_DATA_TYPE2(data->type)&msg_s->filter))
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
		}
		break;


        case STREAM_MARK_GC_END:
        {
            
            
        }
		break;

        case STREAM_CLOSE_ENTER:
        {
            zbar_stream_s *msg_s = (zbar_stream_s *)s->priv;
            //发送停止命令
            os_event_set(&msg_s->evt,ZBAR_STREAM_STOP,NULL);
            //等待线程退出
            os_event_wait(&msg_s->evt,ZBAR_STREAM_EXIT,NULL,OS_EVENT_WMODE_OR,-1);
            break;
        }

        case STREAM_CLOSE_EXIT:
        {
            zbar_stream_s *msg_s = (zbar_stream_s *)s->priv;
            os_event_del(&msg_s->evt);
            STREAM_LIBC_FREE(s->priv);
            break;
        }
        break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}


//filter需要接收的数据类型
stream *zbar_stream(const char *name,uint32_t filter)
{
    stream *s = open_stream_available(name,0,1,opcode_func,(void*)filter);
	return s;
}