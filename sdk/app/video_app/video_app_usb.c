/*
    在硬件初始化后,这里就是相关video的接口
*/
#include "video_app_usb.h"

//临时利用,V200接口需要用到
#include "devid.h"


#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "jpgdef.h"
#include "custom_mem/custom_mem.h"
#include "dev/usb/uvc_host.h"
#include "osal/irq.h"
static stream *g_usb_jpeg_s = NULL;
static struct os_work usb_jpeg_stream_work;

struct jpg_frame_msg
{
	uint32_t len;
	void *malloc_mem;
};


//usb的情况下,如果是在sram,则获取的get_uvc_frame一定是独占,只有等自己释放
static int32 stream_get_usb_jpg_work_sram(struct os_work *work)
{

    struct list_head* get_f;
	UVC_MANAGE* uvc_message;
	UVC_BLANK* data;
	stream *s = g_usb_jpeg_s;
	int err = 1;
	uint32_t flags;

		flags = disable_irq();
        get_f = get_uvc_frame();
		enable_irq(flags);

		if(get_f)
		{

			uvc_message = list_entry(get_f,UVC_MANAGE,list);
			set_uvc_frame_using(uvc_message); 
			//os_printf("usb jpeg len:%d\n",uvc_message->frame_len);

			struct stream_jpeg_data_s *el,*tmp;
			struct stream_jpeg_data_s *dest_list = NULL;
			int get_f_count = get_usb_node_count(get_f);
			struct stream_jpeg_data_s *m = NULL;
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

			//os_printf("get_f_count:%d\n",get_f_count);

			while(m && get_f_count--)
			{
				//图片节点提取
				if(list_empty(&uvc_message->list))
				{
					err = 0;
					break;
				}
				else
				{
					el = m++;
					//保存next
					data = list_entry(uvc_message->list.prev,UVC_BLANK,list);
					list_del((struct list_head *)data);
					data->list.next = NULL;
					data->list.prev = NULL;

					el->data = (void*)data;
					el->ref = 1;
					el->next = NULL;
					err = 0;
					//添加到另一个链表
					LL_APPEND(dest_list,el);
				}
			}


			//os_printf("%s:%d\n",__FUNCTION__,__LINE__);



			struct data_structure  *data_s = NULL;
			//没有错误的情况才去申请空间
			if(!err)
			{
				data_s = get_src_data_f(s);
			}


			if(err || !data_s || !data_s->priv)
			{
				//清除已经提取的图片节点
				LL_FOREACH_SAFE(dest_list,el,tmp)
				{
					el->ref = 0;
					el->next = NULL;
					if(el->data)
					{
						free_usb_node(el->data);
					}
					el->data = NULL;
				}
				os_free(dest_list);

				if(data_s)
				{
					force_del_data(data_s);
				}
				
			}
			else
			{
				//数据绑定
				data_s->data = dest_list;
				data_s->ref = 0;
				data_s->type = SET_DATA_TYPE(JPEG,JPEG_DVP_NODE);
				set_stream_data_time(data_s,os_jiffies()); 
				struct jpg_frame_msg *j = (struct jpg_frame_msg*)data_s->priv;
				j->len = uvc_message->frame_len;
				j->malloc_mem = dest_list;
				//统一发送数据,这个时候,其他流才有可能获取到数据
				_os_printf("S");
				send_data_to_stream(data_s);
			}


			del_usb_frame(get_f);   
			del_uvc_frame(uvc_message);
			get_f = NULL;

		}
	os_run_work_delay(&usb_jpeg_stream_work, 1);
	return 0;
}	




static uint32_t usb_jpg_custom_func_sram(void *data,int opcode,void *priv)
{
	uint32_t res = 0;
    uint32_t flags;
	switch(opcode)
	{
		case CUSTOM_GET_NODE_LEN:
			res = get_node_uvc_len();
		break;
		case CUSTOM_GET_NODE_BUF:
			res = (uint32_t)get_usb_jpeg_node_buf(priv);
		break;
		case CUSTOM_GET_FIRST_BUF:
		{
			struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(data);
			//头是固定用,next才是真正的数据
			if(dest_list->next)
			{
				res =(uint32) get_usb_jpeg_node_buf(dest_list->next->data);
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
                        free_usb_node(el->data);
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
			    free_usb_node(el->data);
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
	.custom_func = usb_jpg_custom_func_sram,
};

static int opcode_func_sram(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
			stream_data_dis_mem_custom(s);

			streamSrc_bind_streamDest(s,R_RTP_JPEG);
			streamSrc_bind_streamDest(s,R_RECORD_JPEG);

			//启动jpeg流
			//extern void start_jpeg_stream();
			//start_jpeg_stream();
			//启动workqueue
			OS_WORK_INIT(&usb_jpeg_stream_work, stream_get_usb_jpg_work_sram, 0);
			os_run_work_delay(&usb_jpeg_stream_work, 1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
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
					free_usb_node(el->data);
				}
			}
			struct jpg_frame_msg *j = (struct jpg_frame_msg*)data->priv;
			if(j)
			{
				os_free(j->malloc_mem);
				j->malloc_mem = NULL;
			}
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
                    if(el->data)
                    {
                        free_usb_node(el->data);
                    }
                }
			}
			enable_irq(flags);

		}
		break;

		//退出,则关闭对应得流
		case STREAM_CLOSE_EXIT:
		{
			extern void stop_jpeg_stream();
			os_work_cancle2(&usb_jpeg_stream_work,1);
			//stop_jpeg_stream();
			
		}
		break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}


stream *usb_jpeg_stream_init()
{
	stream *src = NULL;
	
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	src = open_stream_available(S_USB_JPEG,2,0,opcode_func_sram,NULL);

	g_usb_jpeg_s = src;
	os_printf("src:%X\n",src);
	if(src)
	{
		os_printf("src open_ref:%d\n",src->open_ref);
	}
    return src;
}


void usb_jpeg_stream_deinit()
{
	int res;
	if(g_usb_jpeg_s)
	{
		res = close_stream(g_usb_jpeg_s);
		if(!res)
		{
			g_usb_jpeg_s = NULL;
		}
		
	}
}
