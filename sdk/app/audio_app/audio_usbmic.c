#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "dev/usb/uvc_host.h"
#include "osal/task.h"
#include <csi_kernel.h>

#define AUDIO_LEN  (1024)
k_task_handle_t send_usbmic_audio_handle;
k_task_handle_t *send_usbmic_audio_hd;
stream *g_usbmic_s = NULL;

static struct data_structure *g_usbmic_stream_current_data = NULL;

int get_usbmic_audio(void *d);
void usbmic_audio_stream_del(void);

static uint32_t get_sound_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	return (uint32_t)d->priv;
}
static uint32_t set_sound_data_len(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->priv = (void*)len;
	return len;
}
static stream_ops_func stream_sound_ops = 
{
	.get_data_len = get_sound_data_len,
	.set_data_len = set_sound_data_len,
};

static int opcode_func(stream *s,void *priv,int opcode)
{
	static uint8 *audio_buf = NULL;
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{			
			audio_buf = os_malloc(4 * AUDIO_LEN);
			if(audio_buf)
			{
				stream_data_dis_mem(s,4);
			}
			streamSrc_bind_streamDest(s,R_USB_SPK);	
//			streamSrc_bind_streamDest(s,R_SPEAKER);	
		}
		break;
		case STREAM_OPEN_FAIL:
		break;

		case STREAM_FILTER_DATA:
		break;

		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			int data_num = (int)data->priv;
			data->ops = &stream_sound_ops;
			data->data = audio_buf + (data_num)*AUDIO_LEN;
		}
		break;
        case STREAM_DATA_DESTORY:
            {
                if(audio_buf)
					os_free(audio_buf);
            }
        break;
		case STREAM_DATA_FREE:
			//_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
		break;


		//数据发送完成,可以选择唤醒对应的任务
		case STREAM_RECV_DATA_FINISH:
		break;

		default:
			//默认都返回成功
		break;
	}
	return res;	
}

extern int usb_dma_irq_times;
extern int get_audio_dac_set_filter_type(void);
extern void audio_dac_set_filter_type(int filter_type);
extern void usbmic_room_del(void);
int get_usbmic_audio(void *d)
{
	uint32_t usbmic_timeout = 0;
	uint32_t usb_dma_irq_count = 0;
	struct data_structure *data = NULL;
    volatile int16 *realdata = NULL;
	//int former_dac_priv = 0; 
    int16 *audio_addr = NULL;
	uint32 audio_len = 0;
	UAC_MANAGE *usbmic_manage = NULL;
	stream *src = (stream *)d;


	if(!src)
	{
		os_printf("\n*******open usbmic stream err");
		return 0;
	}
//	former_dac_priv = get_audio_dac_set_filter_type();
//	audio_dac_set_filter_type(SOUND_MIC);
	while(1)
	{
		if(usbmic_timeout > 500) {
			if(usb_dma_irq_count == usb_dma_irq_times) {
				goto get_usbmic_audio_end;
			}
			usb_dma_irq_count = usb_dma_irq_times;
			usbmic_timeout = 0;
		}
		usbmic_timeout++;
		usbmic_manage = get_usbmic_frame();
		if(usbmic_manage)
		{
			data = get_src_data_f(src);
			g_usbmic_stream_current_data = data;
			if(data) {
				printf("L");
				audio_len = get_uac_frame_datalen(usbmic_manage);
				audio_addr = (int16*)get_uac_frame_data(usbmic_manage);
				realdata = get_stream_real_data(data);
				os_memcpy(realdata, audio_addr, audio_len);
				del_usbmic_frame(usbmic_manage);
				data->type = SET_DATA_TYPE(SOUND, SOUND_MIC);
				set_stream_real_data_len(data,audio_len);
				send_data_to_stream(data);
				g_usbmic_stream_current_data = NULL;
				data = NULL;
				usbmic_timeout = 0;
			}
			else {
				del_usbmic_frame(usbmic_manage);
				usbmic_manage = NULL;
			}
		}
		os_sleep_ms(1);
	}
get_usbmic_audio_end:
	usbmic_audio_stream_del();
	usbmic_room_del();
	send_usbmic_audio_hd = NULL;
	return 0;
}

void usbmic_audio_stream_init(void)
{
	if(!g_usbmic_s) {
		os_printf("%s %d\n",__FUNCTION__,__LINE__);
		g_usbmic_s = open_stream_available(S_USB_MIC, 4, 0, opcode_func,NULL);
		// OS_TASK_INIT("get_usbmic_audio", &get_usbmic_audio_task, get_usbmic_audio, g_usbmic_s, OS_TASK_PRIORITY_NORMAL, 512);
	}
	if(!g_usbmic_s) {
		_os_printf("%s open stream err!\n",__FUNCTION__);
		return;
	}
	if(!send_usbmic_audio_hd) {
		if(csi_kernel_task_new((k_task_entry_t)get_usbmic_audio, "get_usbmic_audio", (void*)g_usbmic_s, 17, 0, NULL, 512, &send_usbmic_audio_handle)==0)
			send_usbmic_audio_hd = &send_usbmic_audio_handle;
	}
}

void usbmic_audio_stream_del(void)
{
	int ret = 0;
	if(g_usbmic_s) {
		ret = close_stream(g_usbmic_s);
		if(!ret)
			g_usbmic_s = NULL;
		os_printf("%s!\n",__FUNCTION__);
	}
}

void usbmic_audio_stream_deinit(void)
{
	if(send_usbmic_audio_hd) {
		csi_kernel_task_del(send_usbmic_audio_handle);
		send_usbmic_audio_hd = NULL;
	}
	if(g_usbmic_stream_current_data)
		force_del_data(g_usbmic_stream_current_data);
}
void usbmic_enum_finish(void)
{
	usbmic_audio_stream_init();
}