/*
    在硬件初始化后,这里就是相关video的接口
*/
#include "video_app_bbm.h"

//临时利用,V200接口需要用到
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "jpgdef.h"
#include "custom_mem/custom_mem.h"

#include "osal/irq.h"
#ifdef PSRAM_HEAP


static stream *g_bbm_net_jpeg_s = NULL;

struct jpg_frame_msg
{
	uint32_t len;
	void *malloc_mem;
};


static void stram_get_wifi_psram_thread(void *d)
{

    uint32_t jpg_len;
    stream *s = g_bbm_net_jpeg_s;
    uint32_t delete_frame_num = 4;
	uint8_t *malloc_buf = NULL;
    struct data_structure  *data_s = NULL;
	static uint32_t last_timestemp = 0;
	static uint32_t cur_timestemp = 0;
	static uint32_t temp = 0;
	int send_ret = 0;


    bbm_net_jpeg_frame* get_f = NULL;

	uint32_t count = 0;

	while(delete_frame_num)
	{
		get_f = bbm_get_frame();
		if(get_f)
		{
			os_printf("delete frame:%d\n",get_f->timeinfo);
			os_sleep_ms(900);
			bbm_del_frame(get_f);
			break;
		}else{
			os_sleep_ms(1);
		}
	}


    while(1)
    {
        get_f = bbm_get_frame();
        if(get_f)
        {
			count++;
            jpg_len = get_f->frame_len;
            data_s = get_src_data_f(s);

            if(data_s)
            {
                if(!malloc_buf)
                {
                    malloc_buf = (uint8_t*)custom_malloc_psram(jpg_len);
                }

                if(malloc_buf)
                {
                    hw_memcpy(malloc_buf,get_f->data,jpg_len);
                    //这里代表图片收集完成,申请对应链表空间
                    int get_f_count = 1;
                    struct stream_jpeg_data_s *m = NULL;
                    struct stream_jpeg_data_s *el;//,*tmp;
                    struct stream_jpeg_data_s *dest_list = NULL;
                    if(get_f_count)
                    {
                        //多一个,为了固定头指针
                        m = (struct stream_jpeg_data_s *)os_malloc((get_f_count+1)*sizeof(struct stream_jpeg_data_s));
                        memset(m,0,(get_f_count+1)*sizeof(struct stream_jpeg_data_s));
                        el = m;
                        el->next = NULL;
                        el->data = NULL;
                        el->ref = 0;
                        LL_APPEND(dest_list,el);
                        m++;
                    }

                    if(m)
                    {
                        el = m;
                        //节点先添加
                        el->data = (void*)malloc_buf;
                        el->ref = 1;
                        el->next = NULL;
                        LL_APPEND(dest_list,el);
                        malloc_buf = NULL;
                    }
                    //申请失败,那么去处理掉异常吧
                    else
                    {
                        goto bbm_get_frame_err_deal;
                    }

                    data_s->data = dest_list;
                    data_s->ref = 0;
                    data_s->type = 1;
                    set_stream_data_time(data_s,get_f->timeinfo);
					cur_timestemp = get_f->timeinfo;

                    struct jpg_frame_msg *j = (struct jpg_frame_msg*)data_s->priv;
                    j->len = get_f->frame_len;
                    j->malloc_mem = dest_list;
                    
                    // _os_printf("W");
					temp = cur_timestemp - last_timestemp;


//					os_printf("video: sleep:%dms cur_timestemp:%d last_timestemp:%d\n",cur_timestemp-last_timestemp,
//																							cur_timestemp,
//																							last_timestemp);
    
					data_s->type = SET_DATA_TYPE(JPEG,0);
					send_ret = send_data_to_stream(data_s);
					last_timestemp = cur_timestemp;
                    data_s = NULL;
                }
                else
                {
                    //申请空间失败，强制删除data_s节点
                    os_printf("malloc buf failed\n");
                    force_del_data(data_s);
                    data_s = NULL;
                }

            }
            else
            {
                _os_printf("=");
            }
            bbm_del_frame(get_f);
            get_f = NULL;
        }
        else
        {
			// printf("N");
            os_sleep_ms(1);
        }
    }



bbm_get_frame_err_deal:
    _os_printf("!");
    if(get_f)
    {
        bbm_del_frame(get_f);
        get_f = NULL;
    }

    if(data_s)
    {
        force_del_data(data_s);
        data_s = NULL;
    }

    if(malloc_buf)
    {
        custom_free_psram(malloc_buf);
        malloc_buf = NULL;
    }

    bbm_net_jpeg_psram_stream_deinit(s);

    return;

}


static uint32_t bbm_net_jpg_custom_func_psram(void *data,int opcode,void *priv)
{
	uint32_t res = 0;
    uint32_t flags;
	switch(opcode)
	{
		case CUSTOM_GET_NODE_LEN:
        {
            struct data_structure  *d = (struct data_structure  *)data;
            struct jpg_frame_msg *j = (struct jpg_frame_msg*)d->priv;
            res = j->len;
        }
		break;
		case CUSTOM_GET_NODE_BUF:
			res = (uint32_t)priv;
		break;
		case CUSTOM_GET_FIRST_BUF:
		{
			struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(data);
			//头是固定用,next才是真正的数据
			if(dest_list->next)
			{
				res =(uint32) dest_list->next->data;
			}

		}
		break;

		case CUSTOM_FREE_NODE:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *d = (struct data_structure *)data;
			struct stream_jpeg_data_s *el,*tmp;
			struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)d->data;
			struct stream_jpeg_data_s *dest_list_tmp = dest_list;
			uint32_t flags;
			flags = disable_irq();
			//释放el->data后,最后将链表释放
			LL_FOREACH_SAFE(dest_list,el,tmp)
			{
				if(el == dest_list_tmp)
				{
					continue;
				}
				el->ref--;
				//是否要释放
                if(!el->ref)
                {
                    LL_DELETE(dest_list,el);
                    if(el->data)
                    {
                        custom_free_psram(el->data);
						el->data = NULL;
                    }
                }
			}
			enable_irq(flags);

		}
		break;

		case CUSTOM_DEL_NODE:
        {  
            uint32_t ref;
            struct data_structure *d = (struct data_structure *)data;
            struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)d->data;
            struct stream_jpeg_data_s *el = (struct stream_jpeg_data_s *)priv;
            flags = disable_irq();
            ref = --el->ref;
            enable_irq(flags);
            if(!ref)
            {
                LL_DELETE(dest_list,el);
			    custom_free_psram(el->data);
				el->data = NULL;
            }
        }
		break;

		default:
		break;
	}
	return res;
}



static uint32_t get_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	struct jpg_frame_msg *j = (struct jpg_frame_msg*)d->priv;
	uint32_t len = 0;
	if(j)
	{
		len = j->len;
	}
	return len;
}

static stream_ops_func stream_usb_jpg_ops = 
{
	.get_data_len = get_data_len,
	.custom_func = bbm_net_jpg_custom_func_psram,
};

extern volatile uint8_t uvc_format;
#define BBM_PSRAM_MALLOC_SIZE_INIT	(30*1024)
static int opcode_func_psram(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{

			stream_data_dis_mem(s,3);

			streamSrc_bind_streamDest(s,R_AT_AVI_JPEG);
			streamSrc_bind_streamDest(s,R_AT_SAVE_PHOTO);
		#if LVGL_STREAM_ENABLE
			streamSrc_bind_streamDest(s,SR_OTHER_JPG);
		#endif
			os_task_create("bbm_wifi_stream", stram_get_wifi_psram_thread, (void *)s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			data->type = 0x10;//设置声音的类型,后续用宏代替
			//注册对应函数
			data->ops = &stream_usb_jpg_ops;
			data->data = NULL;
			data->priv = (struct jpg_frame_msg*)os_malloc(sizeof(struct jpg_frame_msg));
			if(data->priv)
			{
				memset(data->priv,0,sizeof(struct jpg_frame_msg));
			}
			else
			{
				os_printf("malloc mem fail,%s:%d\n",__FUNCTION__,__LINE__);
			}
		}
		break;

		case STREAM_DATA_DESTORY:
		{
			struct data_structure *data = (struct data_structure *)priv;
			if(data->priv)
			{
				os_free(data->priv);
				data->priv = NULL;
			}
		}
		break;

		//如果释放空间,则删除所有的节点
		case STREAM_DATA_FREE:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *data = (struct data_structure *)priv;
			struct stream_jpeg_data_s *el,*tmp;
			struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)data->data;
			struct stream_jpeg_data_s *dest_list_tmp = dest_list;
			//释放el->data后,最后将链表释放
			LL_FOREACH_SAFE(dest_list,el,tmp)
			{
				if(dest_list_tmp == el)
				{
					continue;
				}
				if(el->data)
				{
					custom_free_psram(el->data);
					el->data = NULL;
				}
			}
			struct jpg_frame_msg *j = (struct jpg_frame_msg*)data->priv;
			if(j)
			{
				os_free(j->malloc_mem);
				j->malloc_mem = NULL;
			}
			data->data = NULL;
		}
		break;

        case STREAM_DATA_FREE_END:
        break;

		//每次即将发送到一个流,就调用一次
		case STREAM_SEND_TO_DEST:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *data = (struct data_structure *)priv;
			struct stream_jpeg_data_s *el,*tmp;
			struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)data->data;
			uint32_t flags;
			flags = disable_irq();
			//释放el->data后,最后将链表释放
			LL_FOREACH_SAFE(dest_list,el,tmp)
			{
				el->ref++;
			}
			enable_irq(flags);

		}
		break;


		//数据发送完成
		case STREAM_SEND_DATA_FINISH:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *data = (struct data_structure *)priv;
			struct stream_jpeg_data_s *el,*tmp;
			struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)data->data;
			struct stream_jpeg_data_s *dest_list_tmp = dest_list;
			uint32_t flags;
			flags = disable_irq();
			//释放el->data后,最后将链表释放
			LL_FOREACH_SAFE(dest_list,el,tmp)
			{
				if(el == dest_list_tmp)
				{
					continue;
				}
				el->ref--;
				//是否要释放
                if(!el->ref)
                {
                    LL_DELETE(dest_list,el);
                    custom_free_psram(el->data);
					el->data = NULL;
                }
			}
			enable_irq(flags);

		}
		break;

		//退出,则关闭对应得流
		case STREAM_CLOSE_EXIT:
		{

			
		}
		break;

		case STREAM_DEL:
		{
			//struct bbm_net_jpeg_frame *userdata = (struct bbm_net_jpeg_frame*)s->priv;
		}

		default:
			//默认都返回成功
		break;
	}
	return res;
}


stream *bbm_net_jpeg_psram_stream_init()
{
	stream *src = NULL;
	
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	if(g_bbm_net_jpeg_s)
	{
		return NULL;
	}
	
	src = open_stream_available(S_NET_JPEG,3,0,opcode_func_psram,NULL);

	g_bbm_net_jpeg_s = src;
	os_printf("src:%X\n",src);
	if(src)
	{
		os_printf("src open_ref:%d\n",src->open_ref);
	}
    return src;
}


void bbm_net_jpeg_psram_stream_deinit(stream *s)
{
	int res;
	os_printf("%s g_bbm_net_jpeg_s:%X\n",__FUNCTION__,g_bbm_net_jpeg_s);
	if(g_bbm_net_jpeg_s)
	{
		res = close_stream(g_bbm_net_jpeg_s);
		os_printf("%s res:%d\n",__FUNCTION__,res);
		if(!res)
		{
			g_bbm_net_jpeg_s = NULL;
		}
		
	}
}


void bbm_net_jpeg_init()
{
	bbm_net_jpeg_psram_stream_init();
}


#endif
