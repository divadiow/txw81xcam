#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "jpgdef.h"
#include "custom_mem/custom_mem.h"
#include "lib/video/uvc/hg_usb_device.h"
#include "osal/irq.h"



//需要获取usb的句柄
#include "list.h"
#include "dev.h"
#include "devid.h"

#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/uvc_host.h"
static stream *g_usb_to_lcd_s = NULL;
uint8_t dispuvc = 0;
extern uint8 uvc_dpi;
extern uint8_t uvc_open;
extern volatile uint8 scale2_finish;
extern void uvc_reset_dev(uint8_t en);
extern void uvc_stream_open(struct hgusb20_dev *p_dev,uint8 enable);
void jpg_decode_to_lcd(uint32 photo,uint32 jpg_w,uint32 jpg_h,uint32 video_w,uint32 video_h);
int32_t  usb_to_lcd_work(struct os_work *work);
struct usb_to_lcd_s
{
    struct os_work work;
    struct data_structure *get_f;
    struct data_structure *get_f_tmp;
};
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

            s->priv = custom_malloc(sizeof(struct usb_to_lcd_s));
            memset(s->priv,0,sizeof(struct usb_to_lcd_s));
            enable_stream(s,1);
            #if RTT_USB_EN

            #else
            void *dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
            //hgusb20_host_reset_phy(dev);	
            uvc_reset_dev_mjpeg(1);
            uvc_stream_open((struct hgusb20_dev *)dev,1);
            #endif

            //启动workqueue中usb_to_lcd事件
            {
                struct usb_to_lcd_s *usb_to_lcd = (struct usb_to_lcd_s*)s->priv;
                OS_WORK_INIT(&usb_to_lcd->work, usb_to_lcd_work, 0);
                os_run_work_delay(&usb_to_lcd->work, 1);
            }
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
        case STREAM_CLOSE_EXIT:
        {
            if(s->priv)
            {
                struct usb_to_lcd_s *usb_to_lcd = (struct usb_to_lcd_s*)s->priv;
                os_work_cancle2(&usb_to_lcd->work,1);

                if(usb_to_lcd->get_f)
                {
                    free_data(usb_to_lcd->get_f);
                    usb_to_lcd->get_f = NULL;
                }

                if(usb_to_lcd->get_f_tmp)
                {
                    free_data(usb_to_lcd->get_f_tmp);
                    usb_to_lcd->get_f_tmp = NULL;
                }
                custom_free(usb_to_lcd);
                s->priv = NULL;
            }
            
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




int32_t usb_to_lcd_work(struct os_work *work)
{
    stream *s = g_usb_to_lcd_s;
	static uint32_t old_time;
	static uint32_t timeout;
    struct usb_to_lcd_s* usb_to_lcd = (struct usb_to_lcd_s*)s->priv;
    struct stream_jpeg_data_s *dest_list;
    struct stream_jpeg_data_s *dest_list_tmp;
    uint32_t total_len;
    uint32_t uint_len;
	static uint8_t  count = 0;
    struct stream_jpeg_data_s *el,*tmp;
    uint8_t *jpeg_buf_addr = NULL;
    static uint32_t wait_time = 0;
	
	if((os_jiffies() - old_time) > 1000){
		old_time = os_jiffies();
		dispuvc = count;
		count = 0;
	}else{
		if(os_jiffies() < old_time){  //os_jiffies 溢出处理
			old_time = os_jiffies();
		}
	}
	

    if(usb_to_lcd->get_f == NULL)
    {
        usb_to_lcd->get_f = recv_real_data(s);
    }


	//准备解码下一帧,释放上一次的
	if((os_jiffies() - timeout) > 200){
		if(usb_to_lcd->get_f_tmp )
		{
			free_data(usb_to_lcd->get_f_tmp);
			usb_to_lcd->get_f_tmp  = NULL;
		}		
	}

		
		
	if(usb_to_lcd->get_f)
	{

        if(!scale2_finish)
        {
            wait_time++;
            if(wait_time > 100)
            {
				_os_printf("Dcod err\r\n");
                scale2_finish = 1;
				wait_time = 0;
            }
            goto usb_to_lcd_work_end;
        }
        dest_list = (struct stream_jpeg_data_s *)get_stream_real_data(usb_to_lcd->get_f);
        dest_list_tmp = dest_list;
        total_len = get_stream_real_data_len(usb_to_lcd->get_f);
        uint_len = stream_data_custom_cmd_func(usb_to_lcd->get_f,CUSTOM_GET_NODE_LEN,NULL);
		count++;
		//准备解码下一帧,释放上一次的
		if(usb_to_lcd->get_f_tmp )
		{
			free_data(usb_to_lcd->get_f_tmp);
			usb_to_lcd->get_f_tmp  = NULL;
		}
		usb_to_lcd->get_f_tmp = usb_to_lcd->get_f;
		timeout = os_jiffies();

        LL_FOREACH_SAFE(dest_list,el,tmp)
        {
            if(dest_list_tmp == el)
            {
                continue;
            }

            //读取完毕删除
            //图片保存起来
            //理论这里就是一张图片,由于这里是节点形式,所以这里暂时先去实现,后续获取一张图片形式显示

            jpeg_buf_addr = (uint8_t *)stream_data_custom_cmd_func(usb_to_lcd->get_f,CUSTOM_GET_NODE_BUF,el->data);//GET_JPG_SELF_BUF(data,el->data);
            scale2_finish = 0;
			_os_printf("M");
            #if RTT_USB_EN
                jpg_decode_to_lcd((uint32)jpeg_buf_addr,640,480,320,240);
            #else
			if(uvc_dpi == 1){
				jpg_decode_to_lcd((uint32)jpeg_buf_addr,640,480,160,120);
			}else if(uvc_dpi == 2){
				jpg_decode_to_lcd((uint32)jpeg_buf_addr,1280,720,160,120);
			}
            #endif

            //jpg_decode_to_lcd((uint32)jpeg_buf_addr,1280,720,160,120);
            //DEL_JPEG_NODE(usb_to_lcd->get_f,el);
            break;
        }
		wait_time = 0;
		//读取图片,显示在lcd
		//free_data(usb_to_lcd->get_f);
        usb_to_lcd->get_f = NULL;
	}

	
usb_to_lcd_work_end:
    os_run_work_delay(work, 1);
	return 0;
}


uint8_t  enable_video_usb_to_lcd(uint8_t enable)
{
   if(enable)
   {
        stream *s = NULL;
        s = open_stream_available(R_JPEG_TO_LCD,0,8,opcode_func,NULL);
        g_usb_to_lcd_s = s;
        if(s)
        {
            return 0;
        }
   }
   else
   {
        int res;
        if(g_usb_to_lcd_s)
        {
            res = close_stream(g_usb_to_lcd_s);
            if(!res)
            {
                g_usb_to_lcd_s = NULL;
                os_printf("%s close end\n",__FUNCTION__);
            }
            
        }
        return 0;
   }
   return 1;
}
