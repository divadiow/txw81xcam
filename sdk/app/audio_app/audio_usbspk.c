#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "dev/usb/uvc_host.h"
#include "osal/task.h"
#include <csi_kernel.h>

#define AUDIO_LEN  (1024)
k_task_handle_t send_usbspk_audio_handle;
struct os_task *send_usbspk_audio_hd = NULL;
stream *g_usbspk_s = NULL;

static struct data_structure *g_usbspk_stream_current_data = NULL;

void usbspk_audio_stream_del(void);
void send_usbspk_audio(void *d);

static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	//_os_printf("%s:%d\topcode:%d\n",__FUNCTION__,__LINE__,opcode);
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		default:
			//默认都返回成功
		break;
	}
	return res;
}

extern uint32_t usbspk_tx_cnt;
extern int usb_dma_irq_times;
extern void del_usbspk_frame(UAC_MANAGE *uac_manage);
extern void usbspk_room_del(void);
void send_usbspk_audio(void *d)
{
    struct data_structure *data_s = NULL;
    stream* s = NULL;
    volatile int16 *buf = NULL;
    uint32 len = 0;
    volatile uint8 *audio_addr = NULL;
	UAC_MANAGE *usbspk_manage = NULL;
	uint32_t usbspk_timeout = 0;
	uint32_t usb_dma_irq_count = 0;
	
    s = (stream *)d;
    if(!s)
    {
        os_printf("open usbspk stream err\n");
    }
    while(1)
    {
		if(usbspk_timeout > 500) {
			if(usb_dma_irq_count == usb_dma_irq_times) {
				goto send_usbspk_audio_end;
			}
			usb_dma_irq_count = usb_dma_irq_times;
			usbspk_timeout = 0;
		}
		usbspk_timeout++;
		usbspk_manage = get_usbspk_new_frame(0);
		if(usbspk_manage) {
			data_s = recv_real_data(s);
			g_usbspk_stream_current_data = data_s;
			if(data_s)
			{
				printf("S");
				buf = get_stream_real_data(data_s);
				len = get_stream_real_data_len(data_s); 
				audio_addr = get_uac_frame_data(usbspk_manage);
				os_memcpy((uint8*)audio_addr, buf, len);
				set_uac_frame_datalen(usbspk_manage, len);
				set_uac_frame_sta(usbspk_manage, 1);
				put_usbspk_frame_to_use(usbspk_manage);
				usbspk_manage = NULL;
				free_data(data_s);
				g_usbspk_stream_current_data = NULL;
				usbspk_timeout = 0;
			} 
			else { 
				del_usbspk_frame(usbspk_manage);
				usbspk_manage = NULL;
			}
		}
		os_sleep_ms(1);   
    }
send_usbspk_audio_end:
	usbspk_audio_stream_del();
	usbspk_room_del();
	send_usbspk_audio_hd = NULL;
}

void usbspk_audio_stream_init(void)
{
	if(!g_usbspk_s) {
		os_printf("%s %d\n",__FUNCTION__,__LINE__);
		g_usbspk_s = open_stream_available(R_USB_SPK,0,8,opcode_func,NULL);
		// OS_TASK_INIT("send_usbspk_audio", &send_usbspk_audio_task, send_usbspk_audio, g_usbspk_s, OS_TASK_PRIORITY_NORMAL, 512);
	}
	if(!g_usbspk_s) {
		_os_printf("%s open stream err!\n",__FUNCTION__);
		return;
	}
	if(!send_usbspk_audio_hd) {
		if(csi_kernel_task_new((k_task_entry_t)send_usbspk_audio, "send_usbspk_audio", (void*)g_usbspk_s, 17, 0, NULL, 512, &send_usbspk_audio_handle)==0)
			send_usbspk_audio_hd = (struct os_task *)&send_usbspk_audio_handle;
	}
}

void usbspk_audio_stream_del(void)
{
	int ret = 0;
	if(g_usbspk_s) {
		ret = close_stream(g_usbspk_s);
		if(!ret)
			g_usbspk_s = NULL;
		os_printf("%s!\n",__FUNCTION__);
	}
}

void usbspk_audio_stream_deinit(void)
{
	if(send_usbspk_audio_hd) {
		csi_kernel_task_del(send_usbspk_audio_handle);
		send_usbspk_audio_hd = NULL;
	}
	if(g_usbspk_stream_current_data)
		free_data(g_usbspk_stream_current_data);

}
void usbspk_enum_finish(void)
{
	usbspk_audio_stream_init();
} 
