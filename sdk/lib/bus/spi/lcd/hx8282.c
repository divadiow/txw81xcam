#include "lib/lcd/lcd.h"


#if LCD_HX8282_EN

#define CMD(x)    {LCD_CMD,x}
#define DAT(x)    {LCD_DAT,x}
#define DLY(x)    {DELAY_MS,x}
#define END		{LCD_TAB_END,LCD_TAB_END}


const uint8_t hx8282_register_init_tab[][2] = {
	

	END

};


lcddev_t  lcdstruct = {
    .name = "st7701s",
    .lcd_bus_type = LCD_BUS_RGB,
    .bus_width = LCD_BUS_WIDTH_18,
    .color_mode = LCD_MODE_666,
    .osd_scan_mode = LCD_ROTATE_0,
    .scan_mode = LCD_ROTATE_0,//rotate 90
    .te_mode = 0xff,//te mode, 0xff:disable
    .colrarray = 0,//0:_RGB_ 1:_RBG_,2:_GBR_,3:_GRB_,4:_BRG_,5:_BGR_
    .pclk = 48000000,
    .even_order = 0,
    .odd_order = 0,

    .screen_w = 1024,
    .screen_h = 600,
    .video_x  = 0,
    .video_y  = 0,
    .video_w  = 1024,
    .video_h  = 600,
	.osd_x = 0,
	.osd_y = 0,
	.osd_w = 1024, // 0 : value will set to video_w  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.osd_h = 600, // 0 : value will set to video_h  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.init_table = NULL,

    .pclk_inv = 1,

    .vlw 			= 30,
    .vbp 			= 22,
    .vfp 			= 60,

    .hlw 			= 8,
    .hbp 			= 160,
    .hfp 			= 160,

	
	.de_inv = 0,
	.hs_inv = 0,
	.vs_inv = 0,


	.de_en  = 1,
	.vs_en	= 1,
	.hs_en	= 1,

};

#endif
