#include "stream_frame.h"
#include "uthash/utlist.h"
#include "osal/string.h"
#include "avidemux.h"
#include "osal_file.h"
#include "jpgdef.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "play_avi.h"
#include "osal/irq.h"


#define WEBFILE_NODE_LEN 1400
struct web_jpg_frame_msg
{
	uint32_t len;
	void *malloc_mem;
};

#define SEND_VIDEO_MAX_COUNT    2
#define SEND_AUDIO_MAX_COUNT    2
struct play_web_avi_status
{
    uint32_t status;
    void *fp;
    struct aviinfo *info;
    uint32_t sound_count;
    uint32_t video_count;

};

static uint32_t get_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	struct web_jpg_frame_msg *j = (struct web_jpg_frame_msg*)d->priv;
	uint32_t len = 0;
	if(j)
	{
		len = j->len;
	}
	return len;
}

#ifndef PSRAM_HEAP
static uint32_t avi_custom_func_sram(void *data,int opcode,void *priv)
{
	uint32_t res = 0;
    uint32_t flags;
	switch(opcode)
	{
		case CUSTOM_GET_NODE_LEN:
            res = WEBFILE_NODE_LEN;
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
				res =(uint32_t) dest_list->next->data;
			}
		}
		break;

		case CUSTOM_FREE_NODE:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
            //sram上,快速删除节点,如果是psram,可以不管,两种情况不一样,sram上的是不可以转发,psram如果不删除不改变数据,是可以转发的
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
                        //del_jpeg_node(el->data);
                        custom_free(el->data);
                        el->data = NULL;
                    }
                }
			}
			enable_irq(flags);
		}
		break;

        //单个节点删除,sram上快速删除,psram不删除
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
			    custom_free(el->data);
            }
        }
		break;

		default:
		break;
	}
	return res;
}

static void play_avi_free_sram(void *d)
{
	struct data_structure *data = (struct data_structure *)d;
    if(GET_DATA_TYPE1(data->type) == JPEG && GET_DATA_TYPE2(data->type) == JPEG_FILE)  {
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
                //del_jpeg_node(el->data);
                custom_free(el->data);
                el->data = NULL;
            }
        }
    }
    else if(GET_DATA_TYPE1(data->type) == SOUND) {
        custom_free(data->data);
        data->data = NULL;       
    }
}
#else
static uint32_t avi_custom_func_psram(void *data,int opcode,void *priv)
{
	uint32_t res = 0;
    //uint32_t flags;
	switch(opcode)
	{
        //psram的情况,是保存一张图片,所以直接返回图片大小就可以了
		case CUSTOM_GET_NODE_LEN:
            res = get_data_len(data);
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
				res =(uint32_t) dest_list->next->data;
			}
		}
		break;

		case CUSTOM_FREE_NODE:

		break;

        //单个节点删除,sram上快速删除,psram不删除
		case CUSTOM_DEL_NODE:
		break;

		default:
		break;
	}
	return res;
}

static void play_avi_free_psram(void *d)
{
	struct data_structure *data = (struct data_structure *)d;
    if(GET_DATA_TYPE1(data->type) == JPEG && GET_DATA_TYPE2(data->type) == JPEG_FILE)
    {
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
                //del_jpeg_node(el->data);
                //os_printf("free data:%X\n",el->data);
                custom_free_psram(el->data);
                el->data = NULL;
            }
        }
        //_os_printf("Del");
    }
    else if(GET_DATA_TYPE1(data->type) == SOUND)
    {
        custom_free_psram(data->data);
        data->data = NULL;
    }

}

#endif


#ifdef PSRAM_HEAP
    #define AVI_GET_DATA_LEN    get_data_len
    #define AVI_CUSTOM_FUNC     avi_custom_func_psram
    #define AVI_FREE            play_avi_free_psram
#else
    #define AVI_GET_DATA_LEN    get_data_len
    #define AVI_CUSTOM_FUNC     avi_custom_func_sram
    #define AVI_FREE            play_avi_free_sram
#endif


static const stream_ops_func stream_avi_ops = 
{
	.get_data_len = AVI_GET_DATA_LEN,
	.custom_func = AVI_CUSTOM_FUNC,
    .free = AVI_FREE,
};


static uint32_t stream_cmd_func(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
    switch(opcode)
    {
        //设置播放状态
        case 0:
        {
            os_printf("%s:%d\n",__FUNCTION__,__LINE__);
            struct play_web_avi_status *p_status = (struct play_web_avi_status*)s->priv;
            p_status->status = arg;
            if(PLAY_AVI_CMD_STOP == arg)
            {
                close_stream(s);
            }

        }
        break;

        //强行停止
        case 1:
            close_stream(s);
        break;


        default:
        break;
    }
    return res;
}

 
#ifndef PSRAM_HEAP
static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	//_os_printf("%s:%d\topcode:%d\t%X\n",__FUNCTION__,__LINE__,opcode,__builtin_return_address(0));
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            s->priv = priv;
            register_stream_self_cmd_func(s,stream_cmd_func);
			//stream_data_dis_mem(s,2);
			stream_data_dis_mem_custom(s);
			//streamSrc_bind_streamDest(s,R_RECORD_JPEG);
		}
		break;
		case STREAM_OPEN_FAIL:
            custom_free(priv);
		break;
		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			//注册对应函数
			data->ops = (stream_ops_func*)&stream_avi_ops;
			data->data = NULL;
			data->priv = (struct web_jpg_frame_msg*)custom_malloc(sizeof(struct web_jpg_frame_msg));
			if(data->priv)
			{
				memset(data->priv,0,sizeof(struct web_jpg_frame_msg));
			}
			else
			{
				_os_printf("custom_malloc mem fail,%s:%d\n",__FUNCTION__,__LINE__);
			}
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

		//如果释放空间,则删除所有的节点
		case STREAM_DATA_FREE:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *data = (struct data_structure *)priv;
			struct web_jpg_frame_msg *j = (struct web_jpg_frame_msg*)data->priv;
			if(j)
			{
                data->data = NULL;
				custom_free(j->malloc_mem);
				j->malloc_mem = NULL;
			}
		}



		break;

        case STREAM_DATA_FREE_END:
        {
            struct play_web_avi_status *p_status =(struct play_web_avi_status *)s->priv;
            struct data_structure *data = (struct data_structure *)priv;
            if(GET_DATA_TYPE1(data->type) == JPEG && GET_DATA_TYPE2(data->type) == JPEG_FILE)
            {
                p_status->video_count--;
            }
            else if(GET_DATA_TYPE1(data->type) == SOUND)
            {
                p_status->sound_count--;
            }
        }
        break;

		//每次即将发送到一个流,就调用一次
		case STREAM_SEND_TO_DEST:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *data = (struct data_structure *)priv;
            if(GET_DATA_TYPE1(data->type) == JPEG && GET_DATA_TYPE2(data->type) == JPEG_FILE)
            {
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
		}
		break;


		//数据发送完成,单sram的情况,需要尝试清除
		case STREAM_SEND_DATA_FINISH:
		{

			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *data = (struct data_structure *)priv;
            if(GET_DATA_TYPE1(data->type) == JPEG && GET_DATA_TYPE2(data->type) == JPEG_FILE)
            {
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
                            //del_jpeg_node(el->data);
                            custom_free(el->data);
                            el->data = NULL;
                        }
                    }
                }
                enable_irq(flags);
            }
		}
		break;
        case STREAM_CLOSE_EXIT:
            if(s->priv)
            {
                struct play_web_avi_status *p_status =(struct play_web_avi_status *)s->priv;
                if(p_status->info)
                {
				    if (p_status->info->strmsk & BIT(1))
					{
                        broadcast_cmd_to_destStream(s,SET_CMD_TYPE(CMD_AUDIO_DAC,SOUND_NONE));
					}
                    custom_free(p_status->info);
                }
                custom_free(s->priv);
            }
        break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}
#else
static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	//_os_printf("%s:%d\topcode:%d\t%X\n",__FUNCTION__,__LINE__,opcode,__builtin_return_address(0));
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            s->priv = priv;
            register_stream_self_cmd_func(s,stream_cmd_func);
			//stream_data_dis_mem(s,2);
            stream_data_dis_mem_custom(s);
			//streamSrc_bind_streamDest(s,R_RECORD_JPEG);
		}
		break;
		case STREAM_OPEN_FAIL:
            custom_free(priv);
		break;
		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			//注册对应函数
			data->ops = (stream_ops_func*)&stream_avi_ops;
			data->data = NULL;
			data->priv = (struct web_jpg_frame_msg*)custom_malloc(sizeof(struct web_jpg_frame_msg));
			if(data->priv)
			{
				memset(data->priv,0,sizeof(struct web_jpg_frame_msg));
			}
			else
			{
				_os_printf("custom_malloc mem fail,%s:%d\n",__FUNCTION__,__LINE__);
			}
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

		//如果释放空间,则删除所有的节点
		case STREAM_DATA_FREE:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *data = (struct data_structure *)priv;
			struct web_jpg_frame_msg *j = (struct web_jpg_frame_msg*)data->priv;
			if(j)
			{
                data->data = NULL;
				custom_free(j->malloc_mem);
				j->malloc_mem = NULL;
			}
		}
		break;

        case STREAM_DATA_FREE_END:
        {
            struct play_web_avi_status *p_status =(struct play_web_avi_status *)s->priv;
            struct data_structure *data = (struct data_structure *)priv;
            if(GET_DATA_TYPE1(data->type) == JPEG && GET_DATA_TYPE2(data->type) == JPEG_FILE)
            {
                p_status->video_count--;
            }
            else if(GET_DATA_TYPE1(data->type) == SOUND)
            {
                p_status->sound_count--;
            }
        }
        break;

		//每次即将发送到一个流,就调用一次
		case STREAM_SEND_TO_DEST:
		{
			//释放data->data里面的图片数据
			//清除已经提取的图片节点
			struct data_structure *data = (struct data_structure *)priv;
            if(GET_DATA_TYPE1(data->type) == JPEG && GET_DATA_TYPE2(data->type) == JPEG_FILE)
            {
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

		}
		break;


		//数据发送完成,单sram的情况,需要尝试清除
		case STREAM_SEND_DATA_FINISH:
		{
		}
		break;
        case STREAM_CLOSE_EXIT:

            if(s->priv)
            {
                struct play_web_avi_status *p_status =(struct play_web_avi_status *)s->priv;

                if(p_status->info)
                {
					if (p_status->info->strmsk & BIT(1))
					{
                        broadcast_cmd_to_destStream(s,SET_CMD_TYPE(CMD_AUDIO_DAC,SOUND_NONE));
					}
                    custom_free(p_status->info);
                }
                custom_free(s->priv);
            }
        break;



		default:
			//默认都返回成功
		break;
	}
	return res;
}
#endif

stream* creat_play_avi_stream(const char *name,char *filepath,char *jpeg_recv_name,char *audio_recv_name)    
{
    _os_printf("%s path:%s\n",__FUNCTION__,filepath);
    struct play_web_avi_status *status = (struct play_web_avi_status*)custom_malloc(sizeof(struct play_web_avi_status));
	stream *s = NULL;
    void *fp = NULL;
	struct aviinfo *info = NULL;
    struct aviinfo *info_tmp = NULL;
    if(status)
    {
        memset(status,0,sizeof(struct play_web_avi_status));
        fp = osal_fopen(filepath,"rb");
        info = (struct aviinfo *)custom_malloc(sizeof(struct aviinfo));
        
        if(fp && info)
        {
            memset(info,0,sizeof(struct aviinfo));
            info_tmp = avidemux_parse(fp,info);
        }
        if(fp && info_tmp)
        {
            status->fp = fp;
            status->info = info_tmp;
            s = open_stream_available(name,SEND_VIDEO_MAX_COUNT+SEND_AUDIO_MAX_COUNT,0,opcode_func,status);
        }
        else
        {
            custom_free(status);
        }

    }
 
    if(!s)
    {
        if(info)
        {
            if(info_tmp)
            {
                free_aviinfo_point(info);
            }
            custom_free(info);
        }

        if(fp)
        {
            osal_fclose(fp);
        }
    }
    //存在流,则要绑定recv_name
    else
    {   
        if(jpeg_recv_name)
        {
            streamSrc_bind_streamDest(s,jpeg_recv_name);          
        }
        if(audio_recv_name) {
            streamSrc_bind_streamDest(s,audio_recv_name);
        }
    }

    return s;
}
 
//自带流删除功能,所以如果调用这个,不需要主动调用close_stream
uint32_t stop_avi_stream(stream* s)
{
    //广播,文件线程已经要停止了
    broadcast_cmd_to_destStream(s,SET_CMD_TYPE(CMD_JPEG_FILE,PLAY_AVI_CMD_STOP));

    stream_self_cmd_func(s,0,PLAY_AVI_CMD_STOP);
    return 0;
}


uint32_t play_avi_stream(stream* s)
{
    //广播开始播放文件
    broadcast_cmd_to_destStream(s,SET_CMD_TYPE(CMD_JPEG_FILE,PLAY_AVI_CMD_PLAY));

    stream_self_cmd_func(s,0,PLAY_AVI_CMD_PLAY);
    
    return 0;
}


uint32_t pause_avi_stream(stream* s)
{
    //广播已经暂停播放文件了
    broadcast_cmd_to_destStream(s,SET_CMD_TYPE(CMD_JPEG_FILE,PLAY_AVI_CMD_PAUSE));

    stream_self_cmd_func(s,0,PLAY_AVI_CMD_PAUSE);
    return 0;
}


uint32_t pause_1FPS_avi_stream(stream* s)
{
    //广播已经暂停播放文件了
    broadcast_cmd_to_destStream(s,SET_CMD_TYPE(CMD_JPEG_FILE,PLAY_AVI_CMD_1FPS_PAUSE));

    stream_self_cmd_func(s,0,PLAY_AVI_CMD_1FPS_PAUSE);
    return 0;
}


uint32_t start_play_avi_thread(stream *s)
{
    void play_web_avi_thread(void *d);
    os_task_create("play_avi", play_web_avi_thread, (void*)s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    return 0;
}

#ifndef PSRAM_HEAP
void play_web_avi_thread(void *d)       
{
    stream *src = NULL;
    uint32_t base,size;
    uint32_t base_tmp = 0;
    int read_len;
    src = (stream*)d;
    struct aviinfo *info = NULL;
    struct data_structure  *data_s = NULL;
    struct stream_jpeg_data_s *dest_list = NULL;
    struct stream_jpeg_data_s *m = NULL;
    struct stream_jpeg_data_s *el;
    uint32_t ready_readLen = 0;
    int res = 0;
    int err = 0;
    char  *data = NULL;
    int get_f_count;
    struct play_web_avi_status *p_status = NULL;
    struct data_structure  *audio_data_s = NULL;
    uint8_t video_end_flag = 0;
    uint8_t audio_end_flag = 1;
    uint32_t audio_timestamp = 0;

    if(src)
    {
        p_status = (struct play_web_avi_status *)src->priv;
    }

    F_FILE *fp = p_status->fp;
    info = p_status->info;

    //检查是否有音频数据
    uint32_t audio_msk = info->strmsk & BIT(1);
    os_printf("audio_msk:%X\n",audio_msk);
    if(audio_msk && src)
    {
        broadcast_cmd_to_destStream(src,SET_CMD_TYPE(CMD_AUDIO_DAC,SOUND_FILE));
        audio_end_flag = 0;
    }

    if(!src || !info)
    {
        goto play_web_avi_thread_end;
    }

    while( src && info)
    {
        play_avi_again:
        if(p_status->status == PLAY_AVI_CMD_STOP)
        {
            break;
        }
        if(p_status->status == PLAY_AVI_CMD_PAUSE)
        {
            os_sleep_ms(1);
            goto play_avi_again;
        }
        if(!data_s && p_status->video_count<SEND_VIDEO_MAX_COUNT)
        {
            data_s = get_src_data_f(src);
        }
        if(audio_msk && !audio_data_s && p_status->sound_count<SEND_AUDIO_MAX_COUNT)
        {
            audio_data_s = get_src_data_f(src);
        }
        if(!data_s && !audio_data_s)
        {
            os_sleep_ms(1);
            continue;
        }
        if(data_s && !video_end_flag)
        {
            err = 0;
            res = read_avi_offset(&info->str[0],&base,&size);
            if(base == 0 || res)
            {
                force_del_data(data_s);
                data_s = NULL;
                video_end_flag = 1;
                goto read_video_end;
            }
            if(base_tmp != base)
            {
                dest_list = NULL;
                struct web_jpg_frame_msg *j = (struct web_jpg_frame_msg*)data_s->priv;
                j->len = size;

                //计算需要多少个节点保存
                get_f_count = (size+WEBFILE_NODE_LEN-1)/WEBFILE_NODE_LEN;
                //预先分配节点
                if(get_f_count)
                {
                    //多一个,为了固定头指针
                    m = (struct stream_jpeg_data_s *)custom_malloc((get_f_count+1)*sizeof(struct stream_jpeg_data_s));
                    j->malloc_mem = m;  //记录malloc头部,释放是一次性释放
                    memset(m,0,(get_f_count+1)*sizeof(struct stream_jpeg_data_s));
                    el = m;
                    el->next = NULL;
                    el->data = NULL;
                    el->ref = 0;
                    LL_APPEND(dest_list,el);
                    m++;
                }
                //节点内容读取
                while(m && get_f_count--)
                {
                    el = m++;
                    //保存next
                    data = (char*)custom_malloc(WEBFILE_NODE_LEN);
                    //打印错误,暂时没有处理这个容错
                    if(!data)
                    {
                        os_printf("%s:%d err!!!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
                    }
                    else
                    {   
                        if(size >= WEBFILE_NODE_LEN)
                        {
                            ready_readLen = WEBFILE_NODE_LEN;
                        }
                        else
                        {
                            ready_readLen = size;
                        }
                        //读取数据内容
                        read_len = read_avi_data(&info->str[0],(char*)data,ready_readLen);
                        if(read_len < 0)
                        {
                            err = 1;
                            break;
                            //打印错误,暂时没有处理这个容错
                            os_printf("%s:%d err!!!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
                        }
                        else
                        {
                            size -= read_len;
                        }
                    }
                    el->data = (void*)data;
                    el->ref = 1;
                    el->next = NULL;
                    err = 0;
                    //添加到另一个链表
                    LL_APPEND(dest_list,el);
                }

                //数据绑定
                data_s->data = dest_list;
                data_s->ref = 0;
                data_s->type = SET_DATA_TYPE(JPEG,JPEG_FILE);
                if(err > 0)
                {
                    video_end_flag = 1;
                    if(data) {
                        custom_free(data);
                        data = NULL;
                    }
                    goto read_video_end;
                }
                set_stream_data_time(data_s,(get_avi_cur(&info->str[0])-1)*1000/33); 
                //统一发送数据,这个时候,其他流才有可能获取到数据
                p_status->video_count++;
                send_data_to_stream(data_s);
                data_s = NULL;
                data = NULL;
                base_tmp = base;
            }
            else
            {
            }
            //这里代表播放一帧,所以就暂停,大概率用于预览第一张图片
            if(p_status->status == PLAY_AVI_CMD_1FPS_PAUSE)
            {
                pause_avi_stream(src);
                goto play_avi_again;
            }
        }
		read_video_end:
        if(audio_data_s && !audio_end_flag)
        {
            err = 0;
            res = read_avi_offset(&info->str[1],&base,&size);
            if(base == 0 || res)
            {
                force_del_data(audio_data_s);
                audio_data_s = NULL;
                audio_end_flag = 1;
                goto read_audio_end;
            }
            dest_list = NULL;
            struct web_jpg_frame_msg *j = (struct web_jpg_frame_msg*)audio_data_s->priv;
            j->len = size;
            data = (char*)custom_malloc(size);
            ready_readLen = size;
            if(!data)
            {
                os_printf("%s:%d err\n",__FUNCTION__,__LINE__);
            }
            else
            {
                read_len = read_avi_data(&info->str[1],(char*)data,ready_readLen);
                if(read_len < 0)
                {
                    //打印错误,暂时没有处理这个容错
                    err = 2;
                    os_printf("%s:%d err!!!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
                    break;
                }
            }
            audio_data_s->data = data;
            audio_data_s->ref = 0;
            audio_data_s->type = SET_DATA_TYPE(SOUND,SOUND_FILE);
            if(err>2)
            {
                audio_end_flag = 1;
                if(data) {
                    custom_free(data);
                    data = NULL;
                }
                goto read_audio_end;
            }
            set_stream_data_time(audio_data_s,audio_timestamp); 
            audio_timestamp += (read_len>>1)/8;    //暂时固定采样率为8000，后续再从avi文件解析出来
            p_status->sound_count++;
            send_data_to_stream(audio_data_s);
            audio_data_s = NULL;
            data = NULL;
        }               
        read_audio_end:
        //如果视频和音频结束,则退出
        if(audio_end_flag && video_end_flag)
        {
            break;
        }
    }
play_web_avi_thread_end:

    _os_printf("%s:%d\tend\n",__FUNCTION__,__LINE__);
    if(info) {
        free_aviinfo_point(info);
    }
    if(fp) {
        osal_fclose(fp);
    }
    if(data) {
        custom_free(data);
    }
    if(data_s) {
        force_del_data(data_s);
    }
    if(audio_data_s) {
        force_del_data(audio_data_s);
    }   
    if(src) {
        stop_avi_stream(src);
    }
}

#else

#define SD_TMP_BUF_LEN  1024
void play_web_avi_thread(void *d)
{
    stream *src = NULL;
    uint32_t base,size;
    uint32_t base_tmp = 0;
    int read_len;
    src = (stream*)d;
    struct aviinfo *info = NULL;
    struct data_structure  *data_s = NULL;
    struct stream_jpeg_data_s *dest_list = NULL;
    struct stream_jpeg_data_s *m = NULL;
    struct stream_jpeg_data_s *el;
    uint32_t ready_readLen = 0;
    int res = 0;
    int err = 0;
    //int err;
    char  *data;
    int get_f_count;
    struct play_web_avi_status *p_status = NULL;
    uint8_t *sd_tmp_buf = (uint8_t*)custom_malloc(SD_TMP_BUF_LEN);
    uint32_t read_offset = 0;
    struct data_structure  *audio_data_s = NULL;
    uint8_t video_end_flag = 0;
    uint8_t audio_end_flag = 1;

    if(src)
    {
        p_status = (struct play_web_avi_status *)src->priv;
    }

    F_FILE *fp = p_status->fp;
    info = p_status->info;

    //检查是否有音频数据
    uint32_t audio_msk = info->strmsk & BIT(1);
    os_printf("audio_msk:%X\n",audio_msk);
    if(audio_msk && src)
    {
        //广播设置对应音频播放类型
        broadcast_cmd_to_destStream(src,SET_CMD_TYPE(CMD_AUDIO_DAC,SOUND_FILE));
        audio_end_flag = 0;
    }
    //audio_msk = 0;
    if(!sd_tmp_buf || !src || !info)
    {
        goto play_web_avi_thread_end;
    }

    while( 1)
    {
        play_avi_again:
        if(p_status->status == PLAY_AVI_CMD_STOP)
        {
            break;
        }

        if(p_status->status == PLAY_AVI_CMD_PAUSE)
        {
            os_sleep_ms(1);
            goto play_avi_again;
        }
        if(!data_s && p_status->video_count<SEND_VIDEO_MAX_COUNT)
        {
            data_s = get_src_data_f(src);
        }

        if(audio_msk && !audio_data_s && p_status->sound_count<SEND_AUDIO_MAX_COUNT)
        {
            audio_data_s = get_src_data_f(src);
        }
        if(!data_s && !audio_data_s)
        {
            os_sleep_ms(1);
            continue;
        }

        if(data_s && !video_end_flag)
        {
            err = 0;
            res = read_avi_offset(&info->str[0],&base,&size);
         //os_printf("base:%X\tres:%d\tsize:%d\tbase_tmp:%X\n",base,res,size,base_tmp);
            if(base == 0 || res)
            {
                force_del_data(data_s);
                data_s = NULL;
                video_end_flag = 1;
                goto read_video_end;
            }
            if(base_tmp != base)
            {
                dest_list = NULL;
                read_offset = 0;
                //read_len = read_avi_data(&info->str[0],(char*)test_space,size);
                struct web_jpg_frame_msg *j = (struct web_jpg_frame_msg*)data_s->priv;
                j->len = size;

                //因为是psram,所以保存整一张图片,只有一个节点
                get_f_count = 1;
                //预先分配节点
                if(get_f_count)
                {
                    //多一个,为了固定头指针
                    m = (struct stream_jpeg_data_s *)custom_malloc((get_f_count+1)*sizeof(struct stream_jpeg_data_s));
                    j->malloc_mem = m;  //记录malloc头部,释放是一次性释放
                    memset(m,0,(get_f_count+1)*sizeof(struct stream_jpeg_data_s));
                    el = m;
                    el->next = NULL;
                    el->data = NULL;
                    el->ref = 0;
                    LL_APPEND(dest_list,el);
                    m++;
                }
                //节点内容读取
                data = (char*)custom_malloc_psram(size);
                //os_printf("malloc data:%X\n",data);
                if(!data)
                {
                    os_printf("%s:%d err\n",__FUNCTION__,__LINE__);
                }
                else
                {
                    
                    //csi_dcache_invalid_range(data,size);
                    while(size)
                    {
                        if(size >= SD_TMP_BUF_LEN)
                        {
                            ready_readLen = SD_TMP_BUF_LEN;
                        }
                        else
                        {
                            ready_readLen = size;
                        }
                        //_os_printf("P");
                        read_len = read_avi_data(&info->str[0],(char*)sd_tmp_buf,ready_readLen);
                        if(read_len <= 0)
                        {
                            err = 1;
                            break;
                        }
                        else
                        {
                            hw_memcpy0(data+read_offset,sd_tmp_buf,ready_readLen);
                            size -= read_len;
                            read_offset += read_len;
                        }
                    }
                    el = m;
                    el->data = (void*)data;
                    el->ref = 1;
                    el->next = NULL;
                    LL_APPEND(dest_list,el);
                }

                //数据绑定
                data_s->data = dest_list;
                data_s->ref = 0;
                data_s->type = SET_DATA_TYPE(JPEG,JPEG_FILE);
                if(err > 0)
                {
                    video_end_flag = 1;
                    goto read_video_end;
                }
                //记录视频帧的时间
                set_stream_data_time(data_s,(get_avi_cur(&info->str[0])-1)*1000/30); 
                //统一发送数据,这个时候,其他流才有可能获取到数据
                //_os_printf("W");
                p_status->video_count++;
                send_data_to_stream(data_s);
                data_s = NULL;

                base_tmp = base;
            }
            else
            {
            }

            //os_printf("base:%X\tsize:%d\tcount:%d\tread_len:%d\n",base,size,count++,read_len);
            //这里代表播放一帧,所以就暂停,大概率用于预览第一张图片
            if(p_status->status == PLAY_AVI_CMD_1FPS_PAUSE)
            {
                pause_avi_stream(src);
                goto play_avi_again;
            }
        }
        read_video_end:

        //这里取读取音频,在这里读取,是防止读取第一张图片，读取一张照片的时候,就不要播音频了
        if(audio_data_s && !audio_end_flag)
        {
            err = 0;
            res = read_avi_offset(&info->str[1],&base,&size);
            //os_printf("base:%X\tres:%d\tsize:%d\tbase_tmp:%X\n",base,res,size,base_tmp);
            if(base == 0 || res)
            {
                force_del_data(audio_data_s);
                audio_data_s = NULL;
                audio_end_flag = 1;
                goto read_audio_end;
            }

            dest_list = NULL;
            read_offset = 0;
            //read_len = read_avi_data(&info->str[0],(char*)test_space,size);
            struct web_jpg_frame_msg *j = (struct web_jpg_frame_msg*)audio_data_s->priv;
            j->len = size;
            data = (char*)custom_malloc_psram(size);
            if(!data)
            {
                os_printf("%s:%d err\n",__FUNCTION__,__LINE__);
            }
            else
            {               
                //csi_dcache_invalid_range(data,size);
                while(size)
                {
                    if(size >= SD_TMP_BUF_LEN)
                    {
                        ready_readLen = SD_TMP_BUF_LEN;
                    }
                    else
                    {
                        ready_readLen = size;
                    }
                    read_len = read_avi_data(&info->str[1],(char*)sd_tmp_buf,ready_readLen);
                    if(read_len < 0)
                    {
                        //打印错误,暂时没有处理这个容错
                        err = 2;
                        os_printf("%s:%d err!!!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
                        break;
                    }
                    else
                    {
                        hw_memcpy0(data+read_offset,sd_tmp_buf,ready_readLen);
                        size -= read_len;
                        read_offset += read_len;
                    }
                }

                audio_data_s->data = data;
                audio_data_s->ref = 0;
                audio_data_s->type = SET_DATA_TYPE(SOUND,SOUND_FILE);
                if(err>2)
                {
                    audio_end_flag = 1;
                    goto read_audio_end;
                }
                set_stream_data_time(audio_data_s,0*33); 
                //统一发送数据,这个时候,其他流才有可能获取到数据
                //这里是jpeg发送一张,所以发送数量+1
                p_status->sound_count++;
                _os_printf("A");
                send_data_to_stream(audio_data_s);
                audio_data_s = NULL;
            }
        }

        read_audio_end:
        //如果视频和音频结束,则退出
        if(audio_end_flag && video_end_flag)
        {
            break;
        }

    }

play_web_avi_thread_end:

    if(info)
    {
        free_aviinfo_point(info);
    }

    if(fp)
    {
        osal_fclose(fp);
    }

    if(sd_tmp_buf)
    {
        custom_free(sd_tmp_buf);
    }
    
    if(data_s)
    {
        force_del_data(data_s);
    }

    if(audio_data_s)
    {
        force_del_data(audio_data_s);
    }

    if(src)
    {
        //广播一个视频流播放完毕的命令
        stop_avi_stream(src);
    }

}
#endif