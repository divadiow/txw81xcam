
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/msgqueue.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "tx_platform.h"
#include "utlist.h"
#include "hal/usb_device.h"
#include "lib/usb/usb_device_wifi.h"
#include "lib/usb/usb_device_mass.h"
#include "jpgdef.h"
#include "osal/work.h"
#include "osal/event.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/hgusb_dev_tbl.h"
#include "hal/gpio.h"
#include "dev/usb/uvc_host.h"
#include "lib/video/uvc/hg_usb_device.h"
#include "osal/mutex.h"
#include "lib/sdhost/sdhost.h"
#include "video_app_usb.h"

#if 0

#define USB_TEST_WIFI           1
#define USB_TEST_AUDIO          0


struct hg_usb_device_test {
#if USB_TEST_WIFI

    //正常host不需要使用
    #if USB_HOST_EN
    int8   rx_buf[4];   /* usb rx buffer reserved 4byte for no overflow control */
    int8   tx_buf[4];

    #else
    int8   rx_buf[512*16+4];   /* usb rx buffer reserved 4byte for no overflow control */
    int8   tx_buf[512*16];
    #endif
#endif
#if USB_TEST_AUDIO
    int8   rx_buf[512*4+4];
    int8   tx_buf[512*4];
#endif
    int32  ready;
    uint32 total_tx_len;
    uint32 total_rx_len;
    
    struct os_mutex    tx_lock;
	struct os_semaphore    sem;
    struct usb_device *dev;
    struct os_task state_task;
    struct os_task scsi_task;
} usb_test;
#endif
struct hg_usb_device_test usb_test;
//volatile uint8_t rx_done = 0;
#if 1
static void usb_bus_irq_test(uint32 irq, uint32 param1, uint32 param2, uint32 param3)
{
	struct usb_device *usb_dev = (struct usb_device *)param1;
    switch (irq) {
        case USB_DEV_RESET_IRQ: 
            os_printf("reset\r\n");
			os_sema_up(&usb_test.sem);
            //usb_device_wifi_read(usb_test.dev, usb_test.rx_buf, sizeof(usb_test.rx_buf));
            usb_test.ready = 1;
            break;
        case USB_DEV_SUSPEND_IRQ: break;
        case USB_DEV_RESUME_IRQ: break;
        case USB_DEV_SOF_IRQ: break;
        case USB_DEV_CTL_IRQ:             
            usb_device_mass_uvc_ep0_setup_irq(usb_dev, param3);
		break;
        case USB_EP_RX_IRQ: 
            //os_printf("rx=%d\r\n", param3);
			//rx_done = 1;
            usb_test.total_rx_len += param3;
			if(usb_test.ready)
				os_sema_up(&usb_test.sem);
            break;
        case USB_EP_TX_IRQ: 
            //os_printf("tx_ok\r\n");
            //os_mutex_unlock(&usb_test.tx_lock);
            //usb_test.ready = 1;
            usb_test.total_tx_len += param3;
            break;
        default:
            break;
    }
}

uint32_t scsi_count = 0;
extern struct sdh_device *sdh_test;
void usb_scsi_rqu_deal() {
	
	if(get_cbw()) {
		scsi_count++;
		#if PINGPANG_BUF_EN
		scsi_cmd_analysis();
		#else
		os_mutex_lock(&sdh_test->lock,osWaitForever);
		scsi_cmd_analysis();
		os_mutex_unlock(&sdh_test->lock);
		#endif
	} 
    usb_device_mass_read(usb_test.dev, usb_test.rx_buf, sizeof(usb_test.rx_buf));
}

void usb_scsi_rqu_deal_flash() {
	
	if(get_cbw()) {
		scsi_count++;
		scsi_cmd_analysis_flash();
	} 
    usb_device_mass_read(usb_test.dev, usb_test.rx_buf, sizeof(usb_test.rx_buf));
}

void usb_dev_thread_flash() {
    usb_bulk_init();
	//flash作为文件系统的时候,扇区是4096(如果是只读,是可以设置512的)
    init_usb_disk_buf(4096);
	while(1){
		os_sema_down(&usb_test.sem, osWaitForever);
		usb_scsi_rqu_deal_flash();
        usb_test.total_rx_len = 0;
        usb_test.total_tx_len = 0;
	}
}

void usb_dev_thread() {
    usb_bulk_init();
	//sd卡作为文件系统,扇区大小是512(一般都是配置512,如果特殊需要)
    init_usb_disk_buf(512);
	while(1){
		os_sema_down(&usb_test.sem, osWaitForever);
		usb_scsi_rqu_deal();
        usb_test.total_rx_len = 0;
        usb_test.total_tx_len = 0;
	}
}

void usbdisk_thread_exit()
{
	os_mutex_lock(&sdh_test->lock,osWaitForever);
	os_task_del(&usb_test.scsi_task);
	deinit_usb_disk_buf();
	os_mutex_unlock(&sdh_test->lock);
}

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

extern volatile uint8_t uvc_device_on;


void set_uvc_head_scr(uint8 *headbuf){
	static uint32 time_scr;
	static uint8_t n=0;
	uint32 source_time = 0;
	uint16 Sof_1Khz   = 0;    //只取11位

	Sof_1Khz	= os_jiffies();
	Sof_1Khz	= Sof_1Khz&0x7ff;
	source_time = os_jiffies();
	source_time = source_time*15000;
	if(time_scr != source_time){
		time_scr = source_time;
		n = 1;
	}else{
		source_time += 1875*n;
		n++;
	}
	headbuf[6]	= source_time&0xff;
	headbuf[7]	= (source_time&0xff00)>>8;
	headbuf[8]	= (source_time&0xff0000)>>16;
	headbuf[9]	= (source_time&0xff000000)>>24;
	
	headbuf[10]  = Sof_1Khz&0xff;
	headbuf[11]  = (Sof_1Khz&0xff00)>>8;

}


static uint8_t *g_uvcbuf = NULL;
static struct data_structure *g_data_s = NULL;
static stream *g_usbdev_s = NULL;
void usb_uvc_thread(){
#define DAT_LEN   1012
	struct data_structure *get_f = NULL;
    struct stream_jpeg_data_s *dest_list;
    struct stream_jpeg_data_s *dest_list_tmp;

	stream *s = NULL;
	uint32_t node_len;
	uint32_t flen;
	uint8_t *uvcbuf;
	uint8_t *jpeg_buf_addr = NULL;
	uint8  headbuf[12];
	uint32 time_info = 0;
	uint32 cur_tick = 0;
	uint32 start_tick = 0;
	uint32 send_loop = 0;
	uint32 offset_frame;
	start_tick = os_jiffies();
	cur_tick   = start_tick;
	headbuf[0] = 2;					//12个字节头，字节长度
	headbuf[1] = 0x00;
	uvcbuf = malloc(DAT_LEN+12); 
	g_uvcbuf = uvcbuf;
	os_sleep_ms(1000);
	s = open_stream_available(R_RTP_JPEG,0,8,opcode_func,NULL);
	g_usbdev_s = s;
	while(1){
		if(uvc_device_on){
			scsi_count++;
			headbuf[2]  = time_info&0xff;//(u8*)time_info[0];
			headbuf[3]  = (time_info&0xff00)>>8;
			headbuf[4]  = (time_info&0xff0000)>>16;
			headbuf[5]  = (time_info&0xff000000)>>24;
			


			headbuf[1] &= ~BIT(1);
			get_f = recv_real_data(s);
			g_data_s = get_f;
			if(get_f){
				cur_tick = os_jiffies();
				time_info = cur_tick - start_tick;
				time_info = time_info*15000;


				headbuf[1] ^= BIT(0);       //0X8C与0X8D互相切换
				
				dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(get_f);
				dest_list_tmp = dest_list;
				flen = get_stream_real_data_len(get_f);
				node_len = GET_NODE_LEN(get_f);
				jpeg_buf_addr = (uint8_t *)GET_JPG_BUF(get_f);
				//os_printf("flen:%d     jpeg_buf_addr:%x\r\n",flen,jpeg_buf_addr);

				send_loop = (flen+DAT_LEN-1)/DAT_LEN;
				offset_frame = 0;

				while(flen){
					if(flen > DAT_LEN){
						set_uvc_head_scr(headbuf);
						flen -= DAT_LEN;
						if(flen == 0){
							headbuf[1] |= BIT(1);
						}
						memcpy(uvcbuf,headbuf,2);
						memcpy(uvcbuf+2,jpeg_buf_addr+offset_frame,DAT_LEN);
						usb_uvc_tx(2,(uint32_t)uvcbuf,DAT_LEN+2);
						if(uvc_device_on == 0){
							break;
						} 
						offset_frame += DAT_LEN;
					}else{
						set_uvc_head_scr(headbuf);
						headbuf[1] |= BIT(1);
						memcpy(uvcbuf,headbuf,2);
						memcpy(uvcbuf+2,jpeg_buf_addr+offset_frame,flen);
						
						usb_uvc_tx(2,(uint32_t)uvcbuf,flen+2);
						if(uvc_device_on == 0){
							break;
						} 						
						flen = 0;
					}
				}

			    free_data(get_f);
            	get_f = NULL;
				g_data_s = NULL;

			}else{
				set_uvc_head_scr(headbuf);
				usb_uvc_tx(2,(uint32_t)headbuf,2);
				//os_sleep_ms(1);
			}
		}else{
			os_sleep_ms(1);
		}
	}
}

void uvcdev_stream_deinit()
{
	if(g_uvcbuf) {
		free(g_uvcbuf);
	}
	if(g_data_s) {
		free_data(g_data_s);
	}
	if(g_usbdev_s) {
		close_stream(g_usbdev_s);
	}
}

static void hg_usb_test_state(void *argument)
{
    os_printf("hg_usb_test_state running ...\r\n");
    while (1) {
        os_sleep(5);
        os_printf("RX/TX Speed:%dKB/s, %dKB/s\r\n",
                  (usb_test.total_rx_len / 1024) / 5,
                  (usb_test.total_tx_len / 1024) / 5);
        usb_test.total_rx_len = 0;
        usb_test.total_tx_len = 0;
    }
}






struct os_semaphore     global_delay_sem;
//struct test_start         global_start;

struct os_task global_uvc_handle;





#if 0
//需要查找有没有足够bank才能去kick
void delay_kick(void *d)
{
    global_start.dev = NULL;
    global_start.buf = NULL;
    global_start.len = 0;
    while(1)
    {
        os_sema_down(&global_delay_sem, osWaitForever); 
        if(global_start.dev)
        {
            os_sleep_ms(1);
            //_os_printf("d:%d\n",krhino_sys_tick_get()-time);
            //time = krhino_sys_tick_get();

            usb_uvc_rx_kick_dma_start(global_start.dev,(uint32)global_start.buf,global_start.len);

        }
    }
}
#endif

extern void uvc_user2(void *d);
int32 usb_host_uvc_ioctl(struct usb_device *p_usb_d, uint32 cmd, uint32 param1, uint32 param2);

void hg_usb_disable()
{
    usb_device_request_irq(usb_test.dev, NULL, 0);

}

uint8 uvc_ioctl_index(uint8 uvc_idx){
    uint8 msgbuf[1];
    uint8* pt;

    struct usb_device *dev;
    dev = usb_test.dev;
    pt = msgbuf;
    pt[0] = uvc_idx;
    return usb_host_uvc_ioctl(dev,USB_HOST_IO_CMD_SET_IDX,(uint32)msgbuf[0],0);   
}





extern void uvc_avi_thread(void *d);

struct uvc_user_arg uvc_arg;
struct uvc_user_arg uvc_arg_dual;
void usb_hub_device_insert(struct hgusb20_dev *p_dev);
int enum_deal= 0;
extern int usb_dma_irq_times;
static void usb_host_enum_task(void *argument)
{
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)argument;
    while (1) {
        os_sema_down(&p_dev->usb_os_msg->ep0_sema, osWaitForever);                              //收到connect信号，开始枚举        
        bool usb_uvc_host_enum(struct hgusb20_dev *p_dev);
        usb_uvc_host_enum(p_dev);
		enum_deal = 1;
    }
}



struct usb_connect_workqueue
{
    struct os_work wk;
    void *arg;
};

int32 usb_iso_device_connect_det_loop(struct os_work *work)
{
    struct usb_connect_workqueue *usb_connect_work = (struct usb_connect_workqueue*)work;
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)usb_connect_work->arg;
	static uint32_t count_iso = 0;

	_os_printf("enum_deal:%d  count_iso:%d  usb_dma_irq_times:%d\r\n",enum_deal,count_iso,usb_dma_irq_times);
	if(enum_deal)
    {
		if(count_iso == usb_dma_irq_times)
        {
            os_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%s\n",__FUNCTION__);
			if(uvc_ep_bulk_or_isoc() == USB_ENDPOINT_XFER_ISOC)
			{
				hgusb20_host_reset_phy(p_dev);
			}
			enum_deal = 0;
		}
		count_iso = usb_dma_irq_times;
	}
	os_run_work_delay(work, 2000);
    return 0;
}
void usb_iso_device_connect_det(void *argument){
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)argument;
	int count_iso = 0;
	while(1){
		_os_printf("enum_deal:%d  count_iso:%d  usb_dma_irq_times:%d\r\n",enum_deal,count_iso,usb_dma_irq_times);
		if(enum_deal){
			if(count_iso == usb_dma_irq_times){
				hgusb20_host_reset_phy(p_dev);	
				enum_deal = 0;
			}
			count_iso = usb_dma_irq_times;
		}
		os_sleep_ms(2000);
	}
}

static struct usb_connect_workqueue usb_connect_work;	
void usb_host_h264_uvc_reset()
{
	struct hgusb20_dev *p_dev = (struct hgusb20_dev *)usb_connect_work.arg;
	if(p_dev)
	{
		hgusb20_host_reset_phy(p_dev);
	}
	
}
void usb_host_task_init(struct hgusb20_dev *p_dev)
{
    
    os_sema_init(&p_dev->usb_os_msg->ep0_sema, 0);
	os_sema_init(&p_dev->usb_os_msg->hub_sema, 0);
	os_sema_init(&p_dev->usb_os_msg->hub_finish_sema, 0);
    OS_TASK_INIT((const uint8 *)"USB_ENUM", &p_dev->usb_os_msg->ep0_task,
                 usb_host_enum_task, (uint32)p_dev, OS_TASK_PRIORITY_BELOW_NORMAL, 2048);
	
    OS_TASK_INIT((const uint8 *)"USB_HUB", &p_dev->usb_os_msg->ep0_hub_task,
                 usb_hub_device_insert, (uint32)p_dev, OS_TASK_PRIORITY_BELOW_NORMAL, 1024);	
	
    //OS_TASK_INIT((const uint8 *)"USB_DISCONN", &p_dev->usb_os_msg->ep0_usb_connect_task,usb_iso_device_connect_det, (uint32)p_dev, OS_TASK_PRIORITY_BELOW_NORMAL, 1024);	
    usb_connect_work.arg = (void*)p_dev;
    //OS_WORK_INIT(&usb_connect_work.wk, usb_iso_device_connect_det_loop, 0);
    //os_run_work_delay(&usb_connect_work.wk, 1000);
}
void usb_host_task_del(struct hgusb20_dev *p_dev)
{
	os_sema_del(&p_dev->usb_os_msg->ep0_sema);
	os_sema_del(&p_dev->usb_os_msg->hub_sema);
	os_sema_del(&p_dev->usb_os_msg->hub_finish_sema);
	os_task_del(&p_dev->usb_os_msg->ep0_task);
	os_task_del(&p_dev->usb_os_msg->ep0_hub_task);
	os_work_cancle(&usb_connect_work.wk, 1);
}
void uvc_user4(void *d);
void hg_usb_test(void)
{
#if USB_HOST_EN
    os_printf("hg_usb20_test init2 \r\n");
    os_sema_init(&global_delay_sem, 0);

    usb_test.dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
    if (usb_test.dev) {
        usb_host_task_init((struct hgusb20_dev *)usb_test.dev);
        if (!usb_device_open(usb_test.dev, NULL)) {
            _os_printf("%s:%d\n",__FUNCTION__,__LINE__);
            uint32 usb_host_bus_irq(uint32 irq, uint32 param1, uint32 param2, uint32 param3);
			
            usb_device_request_irq(usb_test.dev, usb_host_bus_irq, (int32)usb_test.dev);
        }
    }
    uvc_sema_init();
#if TEST_UVC_DEBUG
	OS_TASK_INIT("uvc_user2", &global_uvc_handle, uvc_user2, usb_test.dev, OS_TASK_PRIORITY_NORMAL, 1024);
#else
    #if LCD_EN == 1

    #else
		#ifndef PSRAM_HEAP
		usb_jpeg_stream_init();
		#endif
    #endif
#endif
 

    //uvc_user2();
#else
	struct hgusb20_dev *p_dev;
	usb_test.dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
	p_dev = (struct hgusb20_dev *)usb_test.dev;
	os_sema_init(&usb_test.sem, 0);
	p_dev->usb_os_msg->dev_type = UDISK_DEV;
	if (usb_test.dev) {
		if (!usb_device_mass_uvc_open(usb_test.dev)) 
		{
			usb_device_mass_auto_tx_null_pkt_disable(usb_test.dev);
			usb_device_request_irq(usb_test.dev, (usbdev_irq_hdl)usb_bus_irq_test, (int32)usb_test.dev);
			if(p_dev->usb_os_msg->dev_type == UDISK_DEV){
				#if USBDISK == 1
				OS_TASK_INIT("scsi_task",&usb_test.scsi_task, usb_dev_thread, NULL, OS_TASK_PRIORITY_NORMAL, 2048);//1024也可
				#elif USBDISK == 2
				OS_TASK_INIT("scsi_task",&usb_test.scsi_task, usb_dev_thread_flash, NULL, OS_TASK_PRIORITY_NORMAL, 2048);//1024也可
				#endif
			}else{
				OS_TASK_INIT("uvc_task",&usb_test.scsi_task, usb_uvc_thread, NULL, OS_TASK_PRIORITY_NORMAL, 2048);//1024也可
			}
		}
	}

#endif
}

void hgusb_dev_recfg(uint8_t dev_tpye){
	struct hgusb20_dev *p_dev;
	usb_test.dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
	p_dev = (struct hgusb20_dev *)usb_test.dev;
	os_task_del(&usb_test.scsi_task);
	usb_device_mass_uvc_close(usb_test.dev);
	p_dev->usb_os_msg->dev_type = dev_tpye;
	if (usb_test.dev) {
		if (!usb_device_mass_uvc_open(usb_test.dev)) 
		{
			usb_device_mass_auto_tx_null_pkt_disable(usb_test.dev);
			usb_device_request_irq(usb_test.dev, (usbdev_irq_hdl)usb_bus_irq_test, (int32)usb_test.dev);
			if(p_dev->usb_os_msg->dev_type == UDISK_DEV){
				OS_TASK_INIT("scsi_task",&usb_test.scsi_task, usb_dev_thread, NULL, OS_TASK_PRIORITY_NORMAL, 2048);//1024也可
			}else{
				OS_TASK_INIT("uvc_task",&usb_test.scsi_task, usb_uvc_thread, NULL, OS_TASK_PRIORITY_NORMAL, 2048);//1024也可
			}
		}
	}	
}

uint32_t hgusb_get_devtype(){
	struct hgusb20_dev *p_dev;
	struct usb_device  *u_dev;
	u_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
	p_dev = (struct hgusb20_dev *)u_dev;
	return p_dev->usb_os_msg->dev_type;
}


extern void uvc_stream_open(struct hgusb20_dev *p_dev,uint8 enable);
extern void uac_stream_open(struct hgusb20_dev *p_dev,uint8 enable);
extern void usb_jpeg_psram_stream_deinit();
extern void usbmic_audio_stream_deinit(void);
extern void usbspk_audio_stream_deinit(void);


void usb_host_suspend(void *dev)
{
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev;
    uvc_stream_open(p_dev,0);
	os_sleep_ms(10);
	uac_stream_open(p_dev,0);
	os_sleep_ms(10);
	usb_device_close((struct usb_device *)p_dev); 
	usb_host_task_del(p_dev);
	usb_jpeg_psram_stream_deinit();
	force_reset_uvc_frame();
	usbmic_audio_stream_deinit();
	usbspk_audio_stream_deinit(); 
	force_reset_usbmic_frame();
	force_reset_usbspk_frame();
}

void usb_host_resume(void *dev)
{
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev;
	usb_host_task_init((struct hgusb20_dev *)p_dev);
	if(!usb_device_open((struct usb_device *)p_dev, NULL)) {
		uint32 usb_host_bus_irq(uint32 irq, uint32 param1, uint32 param2, uint32 param3);
		usb_device_request_irq((struct usb_device *)p_dev, usb_host_bus_irq, (int32)p_dev);
		os_printf("hgusb20_host open again\n");
	}
}

void usb_device_suspend(void *dev)
{
	struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev;
	if(p_dev->usb_os_msg->dev_type == UVC_DEV) {
		uvc_device_on = 0;
		os_sleep_ms(50);
		os_task_del(&usb_test.scsi_task);
		usb_device_mass_uvc_close((struct usb_device *)p_dev);
		uvcdev_stream_deinit();
	}
	else if(p_dev->usb_os_msg->dev_type == UDISK_DEV) {
		usbdisk_thread_exit();
		usb_device_mass_uvc_close((struct usb_device *)p_dev);
	}
	os_sema_del(&usb_test.sem);
}

void usb_device_resume(void *dev)
{
	struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev;
	os_sema_init(&usb_test.sem, 0);
	if (!usb_device_mass_uvc_open(usb_test.dev)) 
	{
		usb_device_mass_auto_tx_null_pkt_disable(usb_test.dev);
		usb_device_request_irq(usb_test.dev, (usbdev_irq_hdl)usb_bus_irq_test, (int32)usb_test.dev);
		if(p_dev->usb_os_msg->dev_type == UDISK_DEV){
			OS_TASK_INIT("scsi_task",&usb_test.scsi_task, usb_dev_thread, NULL, OS_TASK_PRIORITY_NORMAL, 2048);//1024也可
		}else{
			OS_TASK_INIT("uvc_task",&usb_test.scsi_task, usb_uvc_thread, NULL, OS_TASK_PRIORITY_NORMAL, 2048);//1024也可
		}
	}
}

#endif





