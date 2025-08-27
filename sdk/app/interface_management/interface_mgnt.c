#include "sys_config.h"
#include "typesdef.h"
#include "lib/lcd/lcd.h"
#include "lib/lcd/gui.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "dev/lcdc/hglcdc.h"
#include "osal/semaphore.h"
#include "hal/uart.h"
#include "lib/lcd/lcd.h"
#include "lv_demo_benchmark.h"
#include "hal/adc.h"

#include "osal/msgqueue.h"
#include "keyWork.h"
#include "keyScan.h"
#include "osal/task.h"
#include "stream_frame.h"
#include "app_lcd/lvgl_show_jpg.h"
#include "ui/main_ui.h"
#include "osal/task.h"

typedef uint32_t (*lvgl_get_key)(uint32_t key);

struct os_msgqueue lvgl_key_msgq;


void lv_init(void);
void lv_port_disp_init(void *stream,uint16_t w,uint16_t h,uint8_t rotate);
void lv_port_indev_init(void);
void lv_time_set();
void lv_page_init();
void lv_page_select(uint8_t page);
lcd_msg lcd_info;
gui_msg gui_cfg;
extern uint8_t disp_updata;
uint8_t uart4_buf[16];
uint8_t uart4_buf_result[16];
//uint8_t *osd_encode_buf;
//uint8_t *osd_encode_buf1;
uint8_t osd_encode_buf[200*1024] __attribute__ ((aligned(4),section(".psram.src")));
uint8_t osd_encode_buf1[200*1024] __attribute__ ((aligned(4),section(".psram.src")));
//extern uint8_t osd_menu565_buf[SCALE_HIGH*SCALE_WIDTH*2];
extern uint8_t *osd_menu565_buf;
extern Vpp_stream photo_msg;
//lcd信号量创建
static struct os_semaphore lcd_sem   = {0,NULL};




void lcd_sema_init()
{
	os_sema_init(&lcd_sem,0);
}

void lcd_sema_down(int32 tmo_ms)
{
	os_sema_down(&lcd_sem,tmo_ms);
	//printf("$\n");
}

void lcd_sema_up()
{
	os_sema_up(&lcd_sem);
	//printf("@\n");
}

extern int globa_play_key_sound();
lvgl_get_key g_lvgl_get_key = NULL;
void set_lvgl_get_key_func(void *func)
{
	g_lvgl_get_key = (lvgl_get_key)func;
}

uint32_t key_get_data()
{
	uint32_t key_ret = 0;
	#if KEY_MODULE_EN == 1
	static uint32_t last_key = 0xff;
	uint32_t val = os_msgq_get(&lvgl_key_msgq,0);
	//如果有按键被按下,就进入判断
	//如果没有按键按下,就判断一下上一次是否为释放按键,如果不是,就执行上一次按键的键值(有可能是长按之类)
	if(g_lvgl_get_key)
	{
		if(val > 0)
		{
			return g_lvgl_get_key(val);
		}
		return key_ret;
	}
	if(val > 0 || !(((last_key&0xff) ==   KEY_EVENT_SUP) || ((last_key&0xff) ==   KEY_EVENT_LUP)))
	{
		if(!(val>0))
		{
			val = last_key;
		}
		last_key = val;
		if((val&0xff) ==   KEY_EVENT_SUP)
		{
				switch(val>>8)
				{
					case AD_UP:
						key_ret = LV_KEY_PREV;
					break;
					case AD_DOWN:
						key_ret = LV_KEY_NEXT;
					break;
					case AD_LEFT:
						key_ret = LV_KEY_PREV;
					break;
					case AD_RIGHT:
						key_ret = LV_KEY_NEXT;
					break;
					case AD_PRESS:
						key_ret = LV_KEY_ENTER;
					break;
					case AD_A:
					break;
					case AD_B:

					break;
					case AD_C:

					break;
					default:
					break;

				}
		}
		else if((val&0xff) ==   KEY_EVENT_REPEAT)
		{
			
				switch(val>>8)
				{
					//lvgl需要持续按键才能识别长按
					case AD_PRESS:
						key_ret = LV_KEY_ENTER;
					break;

					default:
					break;

				}
		}
	}
	
	
	#endif
	return key_ret;
}


#if LVGL_STREAM_ENABLE == 1
void lvgl_run(void *d){
	uint32 cur_tick;
	cur_tick = os_jiffies();
	while(1){
		os_sleep_ms(1);
		lv_tick_inc(os_jiffies()-cur_tick);
		cur_tick = os_jiffies();
        lv_timer_handler();
		if(os_jiffies()-cur_tick > 30)
		{
			os_printf("%s:%d\tuse_time:%d\n",__FUNCTION__,__LINE__,os_jiffies()-cur_tick);
		}
	}
}
#else

void lvgl_run(){
	uint32 lvgl_len;
	uint32 cur_tick;
	uint8 sleep_time;
	//uint32 itk;
	uint32_t flags;
	struct lcdc_device *lcd_dev;	
	lcd_sema_init();
	lvgl_len = SCALE_WIDTH*SCALE_HIGH;//320*240;//	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
	cur_tick = os_jiffies();
	sleep_time = 10;
	while(1){
		os_sleep_ms(sleep_time);	
		lv_tick_inc(os_jiffies() - cur_tick);
		cur_tick = os_jiffies();
        lv_timer_handler();
		if(disp_updata == 1){
			flags = disable_irq();	
			//__BKPT();
			//lcd_osd_tran_config(0xFFFFFF,0xFFFBFF,0x000000,0x000000);
			
			if(lcd_info.osd_buf_to_lcd == 0){		
				//lcd_osd_encode_cfg(osd_menu565_buf,lvgl_len*2,osd_encode_buf1);
				lcdc_set_osd_enc_src_addr(lcd_dev,(uint32)osd_menu565_buf);
				lcdc_set_osd_enc_dst_addr(lcd_dev,(uint32)osd_encode_buf1);
				lcdc_set_osd_enc_src_len(lcd_dev,lvgl_len*2);
				//gpio_set_val(PA_15,1);
				lcdc_osd_enc_start_run(lcd_dev,1);
				//os_printf("e1");
			}					
			else{
				//lcd_osd_encode_cfg(osd_menu565_buf,lvgl_len*2,osd_encode_buf);
				lcdc_set_osd_enc_src_addr(lcd_dev,(uint32)osd_menu565_buf);
				lcdc_set_osd_enc_dst_addr(lcd_dev,(uint32)osd_encode_buf);
				lcdc_set_osd_enc_src_len(lcd_dev,lvgl_len*2);
				//gpio_set_val(PA_15,0);
				lcdc_osd_enc_start_run(lcd_dev,1);
				//os_printf("e0");
			}
			
			lcd_info.updata_finish = 0;
			enable_irq(flags);
			lcd_sema_down(-1);
			//os_printf("enc_len:%d\r\n",lcdc_get_osd_enc_dst_len(lcd_dev));
			if(lcd_info.osd_buf_to_lcd == 0){
				lcd_info.osd_buf_to_lcd = 1;
			}else{
				lcd_info.osd_buf_to_lcd = 0;
			}
			lcd_info.lcd_run_new_lcd = 1;
		}
		disp_updata = 0;
		
		//sleep_time = 10 - (os_jiffies() - cur_tick);
	}
}
#endif




extern Vpp_stream photo_msg;
#if KEY_MODULE_EN == 1
uint32_t lvgl_push_key(struct key_callback_list_s *callback_list,uint32_t keyvalue,uint32_t extern_value)
{
	os_msgq_put(&lvgl_key_msgq,keyvalue,0);
	return 0;
}
#endif



#if LVGL_STREAM_ENABLE == 1
void *g_lvgl_hdl = NULL;
lv_style_t g_style;
extern stream *lvgl_osd_stream(const char *name);





static void label_change(lv_timer_t * timer)
{
	char buf[32];
	lv_obj_t * label = (lv_obj_t *)timer->user_data;
	sprintf(buf,"time:%d\n",(uint32_t)os_jiffies());
	lv_label_set_text(label,buf);
}


extern lv_indev_t * indev_keypad;
void lvgl_init(uint16_t w,uint16_t h,uint8_t rotate)
{

	memset(&gui_cfg,0x00,sizeof(gui_cfg));
	gui_cfg.dvp_h   = photo_msg.in_h;
	gui_cfg.dvp_w   = photo_msg.in_w;
	gui_cfg.photo_h = photo_msg.in_h;
	gui_cfg.photo_w = photo_msg.in_w;	
	gui_cfg.rec_h   = photo_msg.in_h;
	gui_cfg.rec_w   = photo_msg.in_w;
	gui_cfg.sound_en= 0;
	gui_cfg.cycle_rec_en = 0;
	gui_cfg.take_photo_num = 1;
	gui_cfg.iso_en  = 0; 
	gui_cfg.enlarge_lcd = 10;


#if KEY_MODULE_EN == 1
	memset(&lvgl_key_msgq,0,sizeof(lvgl_key_msgq));
	os_msgq_init(&lvgl_key_msgq,10);
	add_keycallback(lvgl_push_key,NULL);
#endif

	//初始化流
	lvgl_jpg_stream(S_LVGL_JPG);
	stream *s = lvgl_osd_stream(S_LVGL_OSD);


	lv_init();                  // lvgl初始化，如果这个没有初始化，那么下面的初始化会崩溃
    lv_port_disp_init((void*)s,w,h,rotate);        // 显示器初始化
    lv_port_indev_init();


	lv_style_reset(&g_style);
	lv_style_init(&g_style);
	lv_style_set_bg_color(&g_style, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&g_style, 0); 

	main_ui(NULL);
	
	g_lvgl_hdl = os_task_create("gui_thread", lvgl_run, s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 4096);

}


//挂起lvgl的任务,暂时没有考虑到是否支持任何时候挂起该任务,是否会有其他影响
//默认这里没有影响
void lvgl_hdl_suspend()
{
	if(g_lvgl_hdl)
	{
		os_task_suspend2(g_lvgl_hdl);
	}
}

void lvgl_hdl_resume()
{
	if(g_lvgl_hdl)
	{
		os_task_resume2(g_lvgl_hdl);
	}
}

#else

void lvgl_init(uint16_t w,uint16_t h,uint8_t rotate){

	memset(&gui_cfg,0x00,sizeof(gui_cfg));
	gui_cfg.dvp_h   = photo_msg.in_h;
	gui_cfg.dvp_w   = photo_msg.in_w;
	gui_cfg.photo_h = photo_msg.in_h;
	gui_cfg.photo_w = photo_msg.in_w;	
	gui_cfg.rec_h   = photo_msg.in_h;
	gui_cfg.rec_w   = photo_msg.in_w;
	gui_cfg.sound_en= 0;
	gui_cfg.cycle_rec_en = 0;
	gui_cfg.take_photo_num = 1;
	gui_cfg.iso_en  = 0; 
	gui_cfg.enlarge_lcd = 10;


#if KEY_MODULE_EN == 1
	memset(&lvgl_key_msgq,0,sizeof(lvgl_key_msgq));
	os_msgq_init(&lvgl_key_msgq,10);
	add_keycallback(lvgl_push_key,NULL);
#endif


	lv_init();                  // lvgl初始化，如果这个没有初始化，那么下面的初始化会崩溃
    lv_port_disp_init(NULL,w,h,rotate);        // 显示器初始化
    lv_port_indev_init();
	
#if BBM_DEMO
	void lv_baby_display();
	lv_time_set();
	lv_baby_display();
#elif 1	
	lv_page_init();
    lv_page_select(0);
	lv_time_set();
#else
	void lv_uvc_display();
	lv_time_set();
	lv_uvc_display();	
#endif

	os_task_create("gui_thread", lvgl_run, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 4096);

}
#endif

