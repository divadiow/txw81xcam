#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "osal/sleep.h"
#include "string.h"


#include "pdmFilter.h"
#include "pdm_audio.h"

#include "utlist.h"
#include "newaudio.h"
#include "stream_frame.h"

#include "osal/string.h"
#if AUDIO_EN == 1

//对PCM数据的放大倍数，最大127*256
#define PCM_GAIN                                (10*256)//30*256//(22*256)


stream *global_pdm_s = NULL;


static void rm_dc_filter_init(TYPE_FIRST_ORDER_FILTER_TYPE *filter)
{
    memset(filter, 0, sizeof(TYPE_FIRST_ORDER_FILTER_TYPE));
}


static void newaudio_deal_task(void *arg)
{
	 stream *s = (stream *)arg;
	 int16* addr;
	 int res;
	 int i;
	 struct data_structure  *data ;
	 TYPE_FIRST_ORDER_FILTER_TYPE filter_ctl;
	 rm_dc_filter_init(&filter_ctl);
     struct audio_pdm_s *self_priv = (struct audio_pdm_s*)s->priv;
	 while(1)
	 {
		res = csi_kernel_msgq_get(self_priv->pdm_msgq,&data,-1);
		if(!res)
		{
            addr = get_stream_real_data(data);
		    for(i=0; i<AUDIOLEN/2; i++) 
            {
				addr[i] = rm_dc_filter(&filter_ctl, addr[i]);
				addr[i] = pcm_volum_gain(addr[i], PCM_GAIN);
			}

            //发送
            send_data_to_stream(data);
			//_os_printf("!");
		}
		else
		{
		 	_os_printf("%s:%d err @@@@@@@@@@@@\n",__FUNCTION__,__LINE__);

		}
	 }
}



static uint32_t get_sound_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	return (uint32_t)d->priv;
}


static uint32_t set_sound_data_len(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->priv = (void*)AUDIOLEN;
	return (uint32_t)AUDIOLEN;
}

static uint32_t set_sound_data_time(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->timestamp = os_jiffies();
	return (uint32_t)0;
}


static stream_ops_func stream_sound_ops = 
{
	.get_data_len = get_sound_data_len,
	.set_data_len = set_sound_data_len,
    // .set_data_time = set_sound_data_time,
};

static int opcode_func(stream *s,void *priv,int opcode)
{
    static uint8_t *pdm_audio_buf = NULL;
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
			s->priv = (void*)os_malloc(sizeof(struct audio_pdm_s));
			if(s->priv)
			{
				struct audio_pdm_s *self_priv = (struct audio_pdm_s*)s->priv;
				self_priv->pdm_msgq  = (void*)csi_kernel_msgq_new(1,sizeof(uint8_t*));
				OS_TASK_INIT("pdmAudio_deal", &self_priv->thread_hdl, newaudio_deal_task, s, OS_TASK_PRIORITY_NORMAL, 1024);
			}

            pdm_audio_buf = os_malloc(AUDIONUM * AUDIOLEN);
            if(pdm_audio_buf)
            {
			    stream_data_dis_mem(s,AUDIONUM);
            }
			//绑定到对应的流
            streamSrc_bind_streamDest(s,R_RECORD_AUDIO);
            streamSrc_bind_streamDest(s,R_RTP_AUDIO);

		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			int data_num = (int)data->priv;
			data->type = DATA_TYPE_AUDIO_PDM;//设置声音的类型
            data->priv = (void*)AUDIOLEN;
			//注册对应函数
			data->ops = &stream_sound_ops;
			data->data = pdm_audio_buf + (data_num)*AUDIOLEN;
		}
		break;

		case STREAM_DATA_FREE:
			//_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
            //设置为播放sd卡
            //send_stream_cmd(s,(void*)0x10);
            //_os_printf("priv:%X\n",priv);
		break;

        case STREAM_DATA_FREE_END:
            //os_sema_up(s->priv);
        break;


		//数据发送完成,可以选择唤醒对应的任务
		case STREAM_SEND_DATA_FINISH:
		break;

		default:
			//默认都返回成功
		break;
	}
	return res;
}


void *newaudio_creat() {

	stream *s = NULL;
	s = open_stream(S_PDM,AUDIONUM,0,opcode_func,NULL);
	if(!s)
	{
		goto newaudio_creat_end;
	}
    global_pdm_s = s;
    return s;

newaudio_creat_end:
    global_pdm_s = NULL;
	return NULL;
    
}



static void *newaudio_set_buf(void *priv_el,void *el_point)
{
    stream *s = (stream *)priv_el;
    struct data_structure  *data;

    data = get_src_data_f(s);
    struct data_structure **point = (struct data_structure**)el_point;
    void *buf = NULL;
    if(data)
    {
        buf = get_stream_real_data(data);
    }
    *point = data;
	return buf;
	
}

static void newaudio_get_buf(void *priv_el,void *el_point)
{
    stream *s = (stream *)priv_el;
    struct audio_pdm_s *self_priv = (struct audio_pdm_s*)s->priv;
    struct data_structure *data = (struct data_structure*)el_point;
    int res;
	if(!data)
	{
		_os_printf("%s:%d err\n",__FUNCTION__,__LINE__);
		return;
	}
    set_stream_data_time(data,os_jiffies());
	res = csi_kernel_msgq_put(self_priv->pdm_msgq,&data,0,0);
	//正常应该保证不进这里,如果进来代表任务没有获取队列,直接配置下一个buf导致的
	if(res)
	{
        force_del_data(data);
	}
	return;
	
}




void *newaudio_task()
{
	stream *s = NULL;
	void *audio_priv;
	
	s = newaudio_creat();

    if(!s)
    {
        goto newaudio_task_end;
    }
	struct audio_pdm_s *self_priv = (struct audio_pdm_s*)s->priv;
	audio_priv = pdm_audio_open(PDM_SAMPLE_FREQ_16K,PDM_CHANNEL_RIGHT);
	if(audio_priv)
	{
		self_priv->audio_hardware_hdl = audio_priv;
		pdm_audio_register(audio_priv,s,AUDIOLEN,newaudio_set_buf,newaudio_get_buf);
    	pdm_audio_start(audio_priv);
	}
    newaudio_task_end:
	return (void*)s;
}
#endif