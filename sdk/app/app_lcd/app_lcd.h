#ifndef __APP_LCD_H
#define __APP_LCD_H
#include "sys_config.h"
#include "typesdef.h"
#include "stream_frame.h"
#include "hal/lcdc.h"
#include "hal/spi.h"

struct app_lcd_s
{
	stream *encode_osd_s;
	stream *osd_show_s;
	stream *video_p1_s;
	stream *video_p0_s;
	struct lcdc_device *lcd_dev;	
    //只有thread_hdl退出后,才可以休眠
    void *thread_hdl;
	void *line_buf;
	uint16_t w,h;
	uint16_t screen_w,screen_h;
	uint8_t line_buf_num;
	uint8_t rotate;
	uint8_t video_rotate;
	uint8_t hardware_auto_ks:1;	//由应用层写入,由lcd模块读取
	uint8_t get_auto_ks:1;	//由应用层去读取,由lcd的模块去设置
	uint8_t hardware_ready:1;
	uint8_t thread_exit:1;
	
	
};
extern struct app_lcd_s lcd_msg_s;

void lcd_arg_setting(uint16_t w,uint16_t h,uint8_t rotate,uint16_t screen_w,uint16_t screen_h,uint8_t video_rotate);
void lcd_driver_init(void *s1,void *s2,void *s3,void *s4);
void wait_lcd_exit();
void lcd_driver_reinit();
void lcd_hardware_init(uint16_t *w,uint16_t *h,uint8_t *rotate,uint16_t *screen_w,uint16_t *screen_h,uint8_t *video_rotate);
uint8_t g_read_hardware_auto_ks();
uint8_t g_set_hardware_auto_ks(uint8_t en);
#endif