#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "jpgdef.h"
#include "custom_mem/custom_mem.h"
#include "lib/video/uvc/hg_usb_device.h"
#include "osal/irq.h"

#include "osal_file.h"
#include "media.h"


//需要获取usb的句柄
#include "list.h"
#include "dev.h"
#include "devid.h"

#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/uvc_host.h"
#if USB_EN == 1 && SD_SAVE == 1

extern void uvc_reset_dev(uint8_t en);
extern void uvc_stream_open(struct hgusb20_dev *p_dev,uint8 enable);
extern void usb_host_h264_uvc_reset();
extern int no_frame_record_video_psram(void *fp,void *d,int flen);
int32_t usb_to_card_work(struct os_work *work);
static struct os_task     uvc_h264_task;
struct os_semaphore     h264_sem;

#define H264_FNUM   3
struct usb_to_card_s
{
    struct os_work work;
    struct data_structure *get_f;
    struct data_structure *get_f_tmp;
};

struct h264_msg_s
{
	uint8* addr;
	uint32 len;
	uint32 num;
};

struct h264_msg_s h264msg[H264_FNUM];
uint8_t *pwsram;

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

            //hgusb20_host_reset_phy(dev);	
			#if RTT_USB_EN
			
			#else
			void *dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
            uvc_reset_dev_mjpeg(1);
			usb_host_h264_uvc_reset();
            uvc_stream_open((struct hgusb20_dev *)dev,1);
			#endif
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
        case STREAM_CLOSE_EXIT:
        {            
            
            #if RTT_USB_EN

			#else
			void *dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
			uvc_stream_open((struct hgusb20_dev *)dev,0);
            uvc_reset_dev_mjpeg(0);
			#endif
        }
        break;
		default:
			//默认都返回成功
		break;
	}
	return res;
}


//测试写h264,写大概300帧
void stream_write_h264_thread2(void *d){
	void *fp;
	stream *s = (stream *)d;

	struct data_structure *get_f = NULL;
	uint32 flen;
	uint32_t cnt = 0;;
	fp = create_video_file("0:/DCIM");
	uint32_t timeout = 0;
	while(fp)
	{
		get_f = recv_real_data(s);
		if(get_f)
		{

			timeout = 0;
			flen = get_stream_real_data_len(get_f);
			#ifdef PSRAM_HEAP
			no_frame_record_video_psram(fp,get_f,flen);
			#endif
			free_data(get_f);
			cnt++;
			os_printf("w:%d",cnt);

			get_f = NULL;
			if(cnt > 300)
			{
				break;
			}

			
		}
		else
		{
			os_sleep_ms(1);
			timeout++;
		}
		if(timeout > 10000)
		{
			break;
		}
	}

	if(fp)
	{
		osal_fclose(fp);
	}
	os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	close_stream(s);

}


uint8_t  enable_video_usb_to_tfcard(uint8_t enable)
{
   if(enable)
   {
        stream *s = NULL;
        s = open_stream_available(R_RECORD_H264,0,8,opcode_func,NULL);
		if(!s)
		{
			return 1;
		}
		os_printf("%s:%d\n",__FUNCTION__,__LINE__);
		OS_TASK_INIT("h264_stream", &uvc_h264_task, stream_write_h264_thread2, s, OS_TASK_PRIORITY_ABOVE_NORMAL, 1024);
		
   }
   return 0;
}
#endif
int32 demo_atcmd_save_h264(const char *cmd, char *argv[], uint32 argc)
{
	#if USB_EN == 1 && SD_SAVE == 1
    if(argc < 1)
    {
        os_printf("%s argc too small:%d,should more 2 arg\n",__FUNCTION__,argc);
        return 0;
    }
	if(os_atoi(argv[0]) == 1)
	{
		enable_video_usb_to_tfcard(1);
	}
	#endif
	return 0;
}


