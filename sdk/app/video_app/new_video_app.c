/****************************************************************************************************************
根据video_app.c,重写,video_app.c保留是为了旧项目尽可能兼容,所以new_video_app.c和video_app.c实际是互斥的,这个要注意
*****************************************************************************************************************/
#include "new_video_app.h"

//临时利用,V200接口需要用到
#include "devid.h"

#if JPG_EN == 1
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "jpgdef.h"
#include "hal/jpeg.h"
#include "hal/vpp.h"
#include "custom_mem/custom_mem.h"
#include "lib/lcd/lcd.h"
#include "lib/lcd/gui.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "osal/irq.h"
typedef int32_t (*jpg_pixel_done_prc)(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2);
static const char *g_name = NULL;
extern uint32_t get_jpg_default_dpi(uint8_t id);
extern void set_jpg_w_h(uint8 jpg_num,uint16_t w,uint16_t h);
extern void jpg_cfg_vpp(uint8 jpgid,enum JPG_SRC_FROM src_from);
extern void jpg_cfg_prc(uint8 jpgid,enum JPG_SRC_FROM src_from,jpg_pixel_done_prc done_prc,void *dev_id);

//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc

struct video_app_s
{
	struct os_work work;
    stream *s;
	struct jpg_device *jpg_dev;
	void *prc_done;	//保留prc的中断函数
	void *prc_arg;	//保留prc的中断参数
	uint16_t w;
	uint16_t h;
};

typedef struct
{
    void *arg1;
    void *arg2;
}custom_struct;

extern void jpg_start_dpi(uint8 jpg_num,uint16_t w,uint16_t h);
static void start_jpeg_hardware(struct video_app_s *msg)
{
//为了尽量保留旧逻辑,这里用宏去隔离,新框架要使用msg参数
	jpg_recfg(0);
    if(msg)
    {
        jpg_start_dpi(0,msg->w,msg->h);
    }
    else
    {
        jpg_start(0);
    }
}
    



static void stop_jpeg_hardware()
{
	jpg_stop(0);
}





struct jpg_frame_msg
{
	uint32_t len;
	void *malloc_mem;
};

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



static uint32_t jpg_custom_func(void *data,int opcode,void *priv);
static int32 stream_get_jpg_work(struct os_work *work);


//一次性将节点清除
static void free_all_jpg_node(struct list_head  *get_f)
{

    while(!list_empty(get_f))
    {
        del_jpeg_first_node(get_f);
    }
}



static const stream_ops_func stream_jpg_ops = 
{
	.get_data_len = get_data_len,
	.custom_func = jpg_custom_func,
};

static uint32_t jpg_custom_func(void *data,int opcode,void *priv)
{
	uint32_t res = 0;
    uint32_t flags;
	struct data_structure  *d = (struct data_structure  *)data;
	switch(opcode)
	{
		case CUSTOM_GET_NODE_LEN:
        {
            
            
			if(GET_DATA_TYPE2(d->type) == JPEG_DVP_NODE)
			{
				res = get_jpg_node_len(0);
			}
			#ifdef PSRAM_HEAP
			else if(GET_DATA_TYPE2(d->type) == JPEG_DVP_FULL)
			{
				struct jpg_frame_msg *j = (struct jpg_frame_msg*)d->priv;
				res = j->len;
			}
			#endif
            
        }
			
		break;
		case CUSTOM_GET_NODE_BUF:
			if(GET_DATA_TYPE2(d->type) == JPEG_DVP_NODE)
			{
				res = (uint32_t)get_jpeg_node_buf(priv);
			}
			#ifdef PSRAM_HEAP
			else if(GET_DATA_TYPE2(d->type) == JPEG_DVP_FULL)
			{
				res = (uint32_t)priv;
			}
			#endif
			
		break;
        
		case CUSTOM_GET_FIRST_BUF:
		{
			struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(data);
			//头是固定用,next才是真正的数据
			if(dest_list->next)
			{
				
				if(GET_DATA_TYPE2(d->type) == JPEG_DVP_NODE)
				{
					res =(uint32) get_jpeg_node_buf(dest_list->next->data);
				}
				#ifdef PSRAM_HEAP
				else if(GET_DATA_TYPE2(d->type) == JPEG_DVP_FULL)
				{
					res =(uint32) dest_list->next->data;
				}
				#endif
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
                        

						if(GET_DATA_TYPE2(d->type) == JPEG_DVP_NODE)
						{
							del_jpeg_node(el->data,0);
						}
						#ifdef PSRAM_HEAP
						else if(GET_DATA_TYPE2(d->type) == JPEG_DVP_FULL)
						{
							custom_free_psram(el->data);
						}
						#endif
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
				if(GET_DATA_TYPE2(d->type) == JPEG_DVP_NODE)
				{
					del_jpeg_node(el->data,0);
				}
				#ifdef PSRAM_HEAP
				else if(GET_DATA_TYPE2(d->type) == JPEG_DVP_FULL)
				{
					custom_free_psram(el->data);
				}
				#endif
            }
        }
		break;

		case CUSTOM_GET_NODE_COUNT:
		{
			struct data_structure *d = (struct data_structure *)data;
			struct stream_jpeg_data_s *el;
			struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)d->data;
			//struct stream_jpeg_data_s *dest_list_tmp = dest_list;
			LL_COUNT(dest_list,el,res);
			res -= 1;
		}
		break;

		default:
		break;
	}
	return res;
}

static uint32_t stream_cmd_func(stream *s,int cmd,uint32_t arg)
{
    uint32_t res = 0;

    switch(cmd)
    {
		//暂时没有考虑重复close  jpg_hardware的情况
		case RESET_MJPEG_DPI:
		{
			struct video_app_s *msg = (struct video_app_s*)s->priv;
			os_work_cancle2(&msg->work,1);
			stop_jpeg_hardware();	
			uint32_t w_h = (uint32_t)arg;
			os_printf("w_h:%X\n",w_h);
			msg->w = w_h>>16;
			msg->h = w_h&0xffff;
			start_jpeg_hardware(msg);
			OS_WORK_REINIT(&msg->work);
			os_run_work_delay(&msg->work, 1);
		}
		break;

		//停止命令
		case STREAM_STOP_CMD:
		{
			struct video_app_s *msg = (struct video_app_s*)s->priv;
			os_work_cancle2(&msg->work,1);
			stop_jpeg_hardware();
		}
		break;

		case RESET_MJPEG_FROM:
		{
			if(arg == 0)
			{
				struct video_app_s *msg = (struct video_app_s*)s->priv;
				jpg_cfg_prc(HG_JPG0_DEVID,SOFT_DATA,(jpg_pixel_done_prc)msg->prc_done,(void*)msg->prc_arg);
			}
			else if(arg == 1)
			{
				jpg_cfg_vpp(HG_JPG0_DEVID,VPP_DATA0);
			}
			else if(arg == 2)
			{
				jpg_cfg_vpp(HG_JPG0_DEVID,SCALER_DATA);
			}
			
		}
		break;

		case PRC_MJPEG_KICK:
		{
			struct video_app_s *msg = (struct video_app_s*)s->priv;
			jpg_soft_kick(msg->jpg_dev,arg);
		}
		break;

		case PRC_REGISTER_ISR:
		{
			struct video_app_s *msg = (struct video_app_s*)s->priv;
			custom_struct *rec_arg = (custom_struct*)arg;
			msg->prc_done = (void*)rec_arg->arg1;
			msg->prc_arg = (void*)rec_arg->arg2;
			
			jpg_cfg_prc(HG_JPG0_DEVID,SOFT_DATA,(jpg_pixel_done_prc)msg->prc_done,(void*)msg->prc_arg);
		}
		break;

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
        case STREAM_OPEN_CHECK_NAME:
            //因为这个流要和硬件绑定,所以无论名字是否被改,都只能一个,这里判断如果已经创建了,就返回错误,不要创建
            //已经创建了,比较一下
            if(g_name)
            {
                //比较一下名字,不同名称,代表已经有创建过了
                if(strcmp(g_name,(char*)priv))
                {
                    res = 1;
                    os_printf("new video app err\t%s:%d\n",__FUNCTION__,__LINE__);
                }
            }
            g_name = (char*)priv;
        break;
		case STREAM_OPEN_EXIT:
		{
			s->priv = (struct video_app_s*) STREAM_LIBC_ZALLOC(sizeof(struct video_app_s));
			struct video_app_s *msg = (struct video_app_s*)s->priv;
            msg->s = s;
			msg->jpg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);
			stream_data_dis_mem_custom(s);
			register_stream_self_cmd_func(s,stream_cmd_func);
			//启动jpeg流


			//这里实际是为了兼容旧逻辑产生的
			//设置mjpeg的默认值
			uint32_t w_h = get_jpg_default_dpi(0);
			msg->w = w_h>>16;
			msg->h = w_h&0xffff;
			start_jpeg_hardware(msg);
			//启动workqueue
			OS_WORK_INIT(&msg->work, stream_get_jpg_work, 0);
			os_run_work_delay(&msg->work, 1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			//注册对应函数
			data->ops = (stream_ops_func*)&stream_jpg_ops;
			data->data = NULL;
			data->priv = (struct jpg_frame_msg*)STREAM_LIBC_MALLOC(sizeof(struct jpg_frame_msg));
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
				STREAM_LIBC_FREE(data->priv);
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
					//这里只要判断type2,因为默认type1一定是JPEG
					if(GET_DATA_TYPE2(data->type) == JPEG_DVP_NODE)
					{
						del_jpeg_node(el->data,0);
					}
					#ifdef PSRAM_HEAP
					else if(GET_DATA_TYPE2(data->type) == JPEG_DVP_FULL)
					{
						custom_free_psram(el->data);
					}
					#endif
					
				}
			}
			struct jpg_frame_msg *j = (struct jpg_frame_msg*)data->priv;
			if(j)
			{
				STREAM_LIBC_FREE(j->malloc_mem);
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
					//这里只要判断type2,因为默认type1一定是JPEG
					if(GET_DATA_TYPE2(data->type) == JPEG_DVP_NODE)
					{
						del_jpeg_node(el->data,0);
					}
					#ifdef PSRAM_HEAP
					else if(GET_DATA_TYPE2(data->type) == JPEG_DVP_FULL)
					{
						custom_free_psram(el->data);
					}
					#endif

                }
			}
			enable_irq(flags);

		}
		break;

		//退出,则关闭对应得流
		case STREAM_CLOSE_EXIT:
		{
			struct video_app_s *msg = (struct video_app_s*)s->priv;
			os_work_cancle2(&msg->work,1);
			stop_jpeg_hardware();
			if(msg)
			{
				STREAM_LIBC_FREE(msg);
				s->priv = NULL;
			}
			
			
		}
		break;

		case STREAM_MARK_GC_END:
			//这里代表流已经完毕,也代表整个数据流已经被关闭了
            g_name = NULL;  //代表该硬件已经用完
		break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}

static void jpeg_in_sram(struct os_work *work,struct list_head  *get_f)
{
    struct video_app_s *msg = (struct video_app_s *)work;
	stream *s = msg->s;
	struct list_head  *data;
	int err = 1;
	if(get_f)
	{
		struct stream_jpeg_data_s *el,*tmp;
		struct stream_jpeg_data_s *dest_list = NULL;
		int get_f_count = get_node_count(get_f);
		struct stream_jpeg_data_s *m = NULL;
		if(get_f_count)
		{
			//多一个,为了固定头指针
			m = (struct stream_jpeg_data_s *)STREAM_LIBC_MALLOC((get_f_count+1)*sizeof(struct stream_jpeg_data_s));
			memset(m,0,(get_f_count+1)*sizeof(struct stream_jpeg_data_s));
			el = m;
			el->next = NULL;
			el->data = NULL;
			el->ref = 0;
			LL_APPEND(dest_list,el);
			m++;
		}

		//节点进行提取到stream里面,如果节点不够,也要删除已经提取的节点(理论节点与jpeg节点一致,不应该不一样)
		//先判断get_f的节点数量是多少
		while(m && get_f_count--)
		{
			//图片节点提取
			if(list_empty(get_f))
			{
				err = 0;
				break;
			}
			else
			{
				el = m++;
				//保存next
				data = get_f->next;
				//将next断开
				list_del(get_f->next);
				el->data = (void*)data;
				el->ref = 1;
				el->next = NULL;
				err = 0;
				//添加到另一个链表
				LL_APPEND(dest_list,el);
			}
		}

		
		struct data_structure  *data_s = NULL;
		//没有错误的情况才去申请空间
		if(!err)
		{
			data_s = get_src_data_f(s);
			if(data_s)
			{
				//设置这个为正常图片的的形式
				data_s->type = SET_DATA_TYPE(JPEG,JPEG_DVP_NODE);
			}
		}


		if(err || !data_s || !data_s->priv)
		{
			del_jpeg_frame(get_f);
			//清除已经提取的图片节点
			LL_FOREACH_SAFE(dest_list,el,tmp)
			{
				el->ref = 0;
				el->next = NULL;
				if(el->data)
				{
					del_jpeg_node(el->data,0);
				}
				el->data = NULL;
			}
			STREAM_LIBC_FREE(dest_list);

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
			//设置这个为图片节点形式,接收端需要根据这个去实现读取图片方式
			data_s->type = SET_DATA_TYPE(JPEG,JPEG_DVP_NODE);
			set_stream_data_time(data_s,os_jiffies()); 
			struct jpg_frame_msg *j = (struct jpg_frame_msg*)data_s->priv;
			j->len = get_jpeg_len(get_f);
			j->malloc_mem = dest_list;
			del_jpeg_frame(get_f);
			//统一发送数据,这个时候,其他流才有可能获取到数据
            _os_printf("$");
			send_data_to_stream(data_s);
		}
	}
}

#ifdef PSRAM_HEAP
static void jpeg_in_psram(struct os_work *work,struct list_head  *get_f)
{

    struct video_app_s *msg = (struct video_app_s *)work;
	stream *s = msg->s;
    uint8_t *tmp_buf;
	int err = 1;
    uint32_t jpg_len,jpg_len_tmp,cp_offset,cp_len;
    uint8_t *jpg_psram_space = NULL;
    uint32_t node_len;

	if(get_f)
	{
        node_len = get_jpg_node_len_new((void*)get_f);
		struct stream_jpeg_data_s *el,*tmp;
		struct stream_jpeg_data_s *dest_list = NULL;
        jpg_len = get_jpeg_len(get_f);
        //os_printf("get_f:%X\tjpg_len:%d\n",get_f,jpg_len);
        //带psram的情况下,这里总是等于1,因为psram空间大,一张图片约30KB的情况,可以尽量保证图片在内存是连续的
		int get_f_count = 1;
		struct stream_jpeg_data_s *m = NULL;

        //与无psram的结构同步,用于兼容
		if(get_f_count)
		{
			//多一个,为了固定头指针
			m = (struct stream_jpeg_data_s *)STREAM_LIBC_MALLOC((get_f_count+1)*sizeof(struct stream_jpeg_data_s));
			memset(m,0,(get_f_count+1)*sizeof(struct stream_jpeg_data_s));
			el = m;
			el->next = NULL;
			el->data = NULL;
			el->ref = 0;
			LL_APPEND(dest_list,el);
			m++;
		}
        jpg_psram_space = (uint8_t*)custom_malloc_psram(jpg_len);
        jpg_len_tmp = jpg_len;
        cp_offset = 0;

        if(m)
        {
            el = m;
            //节点先添加
            el->data = (void*)jpg_psram_space;
            el->ref = 1;
            el->next = NULL;
            LL_APPEND(dest_list,el);
        }
		//节点进行提取到stream里面,如果节点不够,也要删除已经提取的节点(理论节点与jpeg节点一致,不应该不一样)
		//先判断get_f的节点数量是多少
		while(jpg_psram_space && m)
		{
            
			//图片节点提取
			if(list_empty(get_f) || jpg_len_tmp == 0)
			{
				err = 0;
				break;
			}

            if(jpg_len_tmp > node_len)
            {
                cp_len = node_len;
            }
            else
            {
                cp_len = jpg_len_tmp;
            }
            jpg_len_tmp -= cp_len;
            tmp_buf = get_jpeg_first_buf(get_f);


            hw_memcpy0(jpg_psram_space+cp_offset,get_jpeg_first_buf(get_f),cp_len);
            del_jpeg_first_node(get_f);
            cp_offset += cp_len;
		}

		struct data_structure  *data_s = NULL;
		//没有错误的情况才去申请空间
		if(!err)
		{
			data_s = get_src_data_f(s);
			if(data_s)
			{
				//设置这个为正常图片的的形式
				data_s->type = SET_DATA_TYPE(JPEG,JPEG_DVP_FULL);
			}
            
		}
		if(err || !data_s || !data_s->priv || !jpg_psram_space)
		{
			del_jpeg_frame(get_f);
            struct stream_jpeg_data_s *dest_list_tmp = dest_list;
			//清除已经提取的图片节点
			LL_FOREACH_SAFE(dest_list,el,tmp)
			{
                if(el == dest_list_tmp)
                {
                    continue;
                }
				el->ref = 0;
				el->next = NULL;
                //理论psram的空间在这里释放
				if(el->data)
				{
					custom_free_psram(el->data);
				}
				el->data = NULL;
			}
			STREAM_LIBC_FREE(dest_list);
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

			set_stream_data_time(data_s,os_jiffies()); 
			struct jpg_frame_msg *j = (struct jpg_frame_msg*)data_s->priv;
			j->len = jpg_len;
			j->malloc_mem = dest_list;
			del_jpeg_frame(get_f);
			//统一发送数据,这个时候,其他流才有可能获取到数据
			_os_printf("V");
			send_data_to_stream(data_s);
		}

	}
}
#endif

static int32 stream_get_jpg_work(struct os_work *work)
{
	//首先获取节点先
	uint32_t flags;

	flags = disable_irq();
	struct list_head  *get_f = (void*)get_frame(HG_JPG0_DEVID);
	enable_irq(flags);
	
	if(get_f)
	{
        struct video_app_s *msg = (struct video_app_s *)work;
        //如果流没有使能,就移除节点,这个时候硬件应该是动起来了
        if(msg->s->enable)
        {
            #ifdef PSRAM_HEAP
				uint32_t jpg_len;
                jpg_len = get_jpeg_len(get_f);
				if(jpg_len < 200*1024)
                {
                    jpeg_in_psram(work,get_f);
                }
                else
                {
                    jpeg_in_sram(work,get_f);
                }
            #else
                jpeg_in_sram(work,get_f);
            #endif
        }
        else
        {
            free_all_jpg_node(get_f);
			del_jpeg_frame(get_f);
        }

		
	}
    os_run_work_delay(work, 1);
	return 0;
}



//创建后,需要使能才能生效(需要手动去绑定一下数据流,然后使能,可以做到动态绑定)
stream *new_video_app_stream(const char *name)
{
	stream *src = NULL;
    src = open_stream_available(name,2,0,opcode_func,NULL);
    return src;
}

stream *new_video_app_stream_with_mode(const char *name,uint8_t from_vpp,uint16_t w,uint16_t h)
{
	stream *src = NULL;
	//从vpp那边过来,需要配置一下
	if(from_vpp == 1)
	{
		#if DVP_EN == 1
			//配置一下数据来源
			jpg_cfg_vpp(HG_JPG0_DEVID,VPP_DATA0);
			set_jpg_w_h(0,0,0);
			src = open_stream_available(name,2,0,opcode_func,NULL);
		#endif
	}
	//从scale过来
	else if(from_vpp == 2)
	{
		set_jpg_w_h(0,w,h);
		jpg_cfg(HG_JPG0_DEVID,SCALER_DATA);
		src = open_stream_available(name,2,0,opcode_func,NULL);
	}
	else if(from_vpp == 0)
	{
		src = open_stream_available(name,2,0,opcode_func,NULL);
	}
    
    return src;
}

#else
//为了不打开jpg,不会报错
stream *new_video_app_stream(const char *name)
{

}

#endif
