#include "sys_config.h"
#include "typesdef.h"
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
#include "lib/lcd/lcd.h"
#include "lib/lcd/gui.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "lib/video/dvp/jpeg/jpg.h"

#include "resorce/language.h"
#include "lv_demo_widgets.h"
#include "openDML.h"
#include "osal/mutex.h"
#include "avidemux.h"
#include "avi/play_avi.h"
#include "playback/playback.h"

extern uint8 *video_psram_mem;
extern uint8 *video_psram_mem1;
extern uint8 *video_psram_mem2;

extern uint8 *video_decode_mem;
extern uint8 *video_decode_mem1;
extern uint8 *video_decode_mem2;

lv_obj_t * page_speed_rx_obj;
lv_obj_t * page_speed_tx_obj;
lv_obj_t * page_speed_id_obj;
lv_obj_t * page_speed_num_obj;
lv_obj_t * page_main_obj;
extern lv_font_t myfont;
extern lcd_msg lcd_info;

extern uint16_t rx_speed,tx_speed;
extern uint8_t dispnum;

//添加用于获取接收的rssi等信息
extern int8 bbm_rx_rssi_get(void);
extern int8 bbm_rx_evm_get(void);
extern uint16 bbm_rx_freq_get(void);
extern int32 bbm_rx_freq_offset_get(void);

char msgtx_str[20];
char msgrx_str[20];
char msgid_str[20];
char msgnum_str[100];

extern uint8_t mcs_send;
extern uint8_t frmtype_send ;


void display_msg(uint32 rx_speed,uint32 tx_speed,uint32 lcd_num,uint32 id){
	memset(msgtx_str,0,20);
	sprintf(msgtx_str,"rx:#ff0088 %02d#Kbyte",rx_speed);
	lv_label_set_text(page_speed_rx_obj,msgtx_str);
	memset(msgrx_str,0,20);
	sprintf(msgrx_str,"tx:#ff0088 %02d#Kbyte",tx_speed);
	lv_label_set_text(page_speed_tx_obj,msgrx_str);
	memset(msgid_str,0,20);
	sprintf(msgid_str,"num:#ff0088 %02d#",lcd_num);
	lv_label_set_text(page_speed_id_obj,msgid_str);
	memset(msgnum_str,0,100);
	sprintf(msgnum_str,"mcs:#ff0088 %02x#  frm:#ff0088 %02x#  rssi:#ff0088 %d#",mcs_send,frmtype_send,123);
	lv_label_set_text(page_speed_num_obj,msgnum_str);	
}


void timer_event(){
	static uint32 timer_count = 0;
	if((timer_count%4) == 0){
		display_msg(rx_speed,tx_speed,dispnum,0x12356789);	
	}
	timer_count++;
}

void lv_time_set(){
	static uint32_t user_data = 10;
	lv_timer_create(timer_event, 500,  &user_data);
}


void lv_page_baby_config(){
	static lv_style_t style5;
	
	static lv_style_t style;	

	lv_style_init(&style);
	lv_style_set_text_font(&style,&lv_font_montserrat_24);
	

	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page_main_obj = lv_obj_create(lv_scr_act());//
	lv_obj_set_size(page_main_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page_main_obj, &style5, 0);
	lv_obj_set_style_text_font(page_main_obj, &myfont, 0);
	
	page_speed_rx_obj  = lv_label_create(page_main_obj);
	page_speed_tx_obj  = lv_label_create(page_main_obj);
	page_speed_id_obj  = lv_label_create(page_main_obj);
	page_speed_num_obj = lv_label_create(page_main_obj);
	
	lv_obj_add_style(page_speed_rx_obj,&style,LV_STATE_DEFAULT);
	lv_obj_align(page_speed_rx_obj,LV_ALIGN_BOTTOM_LEFT,0,0);
	lv_label_set_recolor(page_speed_rx_obj, 1);

	lv_obj_add_style(page_speed_tx_obj,&style,LV_STATE_DEFAULT);
	lv_obj_align(page_speed_tx_obj,LV_ALIGN_BOTTOM_LEFT,0,-30);
	lv_label_set_recolor(page_speed_tx_obj, 1);	
	
	lv_obj_add_style(page_speed_id_obj,&style,LV_STATE_DEFAULT);
	lv_obj_align(page_speed_id_obj,LV_ALIGN_BOTTOM_LEFT,0,-60);
	lv_label_set_recolor(page_speed_id_obj, 1);

	lv_obj_add_style(page_speed_num_obj,&style,LV_STATE_DEFAULT);
	lv_obj_align(page_speed_num_obj,LV_ALIGN_BOTTOM_LEFT,0,-90);
	lv_label_set_recolor(page_speed_num_obj, 1);	
	//display_msg(100,200,30,0x123789);
}


void lv_baby_display(){
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);

	lcd_info.lcd_p0p1_state = 2;
	lcdc_set_video_en(lcd_dev,0);
	
	vpp_close(vpp_dev);
	
	video_decode_mem  = video_psram_mem;
	video_decode_mem1 = video_psram_mem1;
	video_decode_mem2 = video_psram_mem2;
	jpg_dec_scale_del();
	set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
	jpg_decode_scale_config((uint32_t)video_decode_mem);

	lcdc_set_video_en(lcd_dev,1);
	
	lv_page_baby_config();
}


