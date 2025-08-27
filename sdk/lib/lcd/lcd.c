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
#include "osal/msgqueue.h"
#include "osal/task.h"
#include "lib/lcd/lcd.h"
#include "osal/mutex.h"
#include "lib/lcd/gui.h"
#include "lib/vef/video_ef.h"
#include "hal/pwm.h"
#include "hal/timer_device.h"

uint8_t osd_palette[512]__attribute__ ((aligned(4)));

extern volatile vf_cblk g_vf_cblk;
struct os_task     lcd_vef_task;
struct os_msgqueue lcd_vef_msg;
volatile uint8* lcd_vef_pt;
uint8 lcd_vef_tmp[120*160*2] __attribute__ ((aligned(4),section(".psram.src")));
// uint8 lcd_vef_tmp1[240*240*2] __attribute__ ((aligned(4),section(".psram.src")));

#if LCD_EN
#define VIDEO_EN    1
#define OSD_EN      1
struct os_mutex m2m1_mutex;

uint8 *video_psram_mem; //__attribute__ ((aligned(4)));//
uint8 *video_psram_mem1;
uint8 *video_psram_mem2;
uint8 *video_psram_mem3;

uint8 video_psram_config_mem[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __attribute__ ((aligned(4),section(".psram.src"))); //__attribute__ ((aligned(4)));//
uint8 video_psram_config_mem1[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __attribute__ ((aligned(4),section(".psram.src")));
#if LCD_THREE_BUF
uint8 video_psram_config_mem2[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __attribute__ ((aligned(4),section(".psram.src")));
#endif

#if LCD_VIDEO_EF
	uint8 video_psram_config_mem2[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __attribute__ ((aligned(4),section(".psram.src")));
	uint8 video_psram_config_mem3[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __attribute__ ((aligned(4),section(".psram.src")));
#endif

#if (LCD_THREE_BUF == 1 && LCD_VIDEO_EF ==0)
uint8 video_psram_config_mem2[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __attribute__ ((aligned(4),section(".psram.src")));
#endif

uint8 video_decode_config_mem[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2] __attribute__ ((aligned(4),section(".psram.src")));
uint8 video_decode_config_mem1[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2] __attribute__ ((aligned(4),section(".psram.src")));
uint8 video_decode_config_mem2[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2] __attribute__ ((aligned(4),section(".psram.src")));
// uint8 video_decode_config_mem3[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2] __attribute__ ((aligned(4),section(".psram.src")));


       

uint8 *video_decode_mem;
uint8 *video_decode_mem1;
uint8 *video_decode_mem2;
uint8 *video_decode_mem3;
uint8 *scaler2buf;

extern uint8_t osd_encode_buf[200*1024];
extern uint8_t osd_encode_buf1[200*1024];
extern uint8_t osd_palette[512];
volatile uint8 lcdc_decode_enable = 0;

volatile uint32 decode_num = 0;

struct os_mutex decode_mutex;

struct lcdc_device *lcd_dev_gobal;
extern uint8 *yuvbuf;
#define LCD_ROTATE_LINE      32
#if LCD_HX8282_EN
uint8 lcd_line_buf[SCALE_WIDTH*LCD_ROTATE_LINE+(SCALE_WIDTH/2)*LCD_ROTATE_LINE*2 ]__attribute__ ((aligned(4)));      //行数与rotate的BURST大小相关,要比burst大
#else
uint8 lcd_line_buf[SCALE_HIGH*LCD_ROTATE_LINE+(SCALE_HIGH/2)*LCD_ROTATE_LINE*2 ]__attribute__ ((aligned(4)));      //行数与rotate的BURST大小相关,要比burst大
#endif
extern Vpp_stream photo_msg;
extern lcd_msg lcd_info;

unsigned char osd565_encode1[]__attribute__ ((aligned(4)))={
	0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0XFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,

	//0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0XFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,
	//0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,
	//0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,
	0xFF,0XFF,0X1F,0X1F,0X00,0X01,0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,
	0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,
	0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,
	0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,
};

unsigned char osd_rgb256[2432]__attribute__ ((aligned(4))) = {
	0x00, 0x00, 0x00, 0x80, 0x00, 0x04, 0x00, 0x84, 0x10, 0x00, 0x10, 0x80, 0x10, 0x04, 0x18, 0xC6, 
	0xF8, 0xC6, 0x5E, 0xA6, 0x00, 0x41, 0x00, 0x61, 0x00, 0x81, 0x00, 0xA1, 0x00, 0xC1, 0x00, 0xE1, 
	0x00, 0x02, 0x00, 0x22, 0x00, 0x42, 0x00, 0x62, 0x00, 0x82, 0x00, 0xA2, 0x00, 0xC2, 0x00, 0xE2, 
	0x00, 0x03, 0x00, 0x23, 0x00, 0x43, 0x00, 0x63, 0x00, 0x83, 0x00, 0xA3, 0x00, 0xC3, 0x00, 0xE3, 
	0x00, 0x04, 0x00, 0x24, 0x00, 0x44, 0x00, 0x64, 0x00, 0x84, 0x00, 0xA4, 0x00, 0xC4, 0x00, 0xE4, 
	0x00, 0x05, 0x00, 0x25, 0x00, 0x45, 0x00, 0x65, 0x00, 0x85, 0x00, 0xA5, 0x00, 0xC5, 0x00, 0xE5, 
	0x00, 0x06, 0x00, 0x26, 0x00, 0x46, 0x00, 0x66, 0x00, 0x86, 0x00, 0xA6, 0x00, 0xC6, 0x00, 0xE6, 
	0x00, 0x07, 0x00, 0x27, 0x00, 0x47, 0x00, 0x67, 0x00, 0x87, 0x00, 0xA7, 0x00, 0xC7, 0x00, 0xE7, 
	0x08, 0x00, 0x08, 0x20, 0x08, 0x40, 0x08, 0x60, 0x08, 0x80, 0x08, 0xA0, 0x08, 0xC0, 0x08, 0xE0, 
	0x08, 0x01, 0x08, 0x21, 0x08, 0x41, 0x08, 0x61, 0x08, 0x81, 0x08, 0xA1, 0x08, 0xC1, 0x08, 0xE1, 
	0x08, 0x02, 0x08, 0x22, 0x08, 0x42, 0x08, 0x62, 0x08, 0x82, 0x08, 0xA2, 0x08, 0xC2, 0x08, 0xE2, 
	0x08, 0x03, 0x08, 0x23, 0x08, 0x43, 0x08, 0x63, 0x08, 0x83, 0x08, 0xA3, 0x08, 0xC3, 0x08, 0xE3, 
	0x08, 0x04, 0x08, 0x24, 0x08, 0x44, 0x08, 0x64, 0x08, 0x84, 0x08, 0xA4, 0x08, 0xC4, 0x08, 0xE4, 
	0x08, 0x05, 0x08, 0x25, 0x08, 0x45, 0x08, 0x65, 0x08, 0x85, 0x08, 0xA5, 0x08, 0xC5, 0x08, 0xE5, 
	0x08, 0x06, 0x08, 0x26, 0x08, 0x46, 0x08, 0x66, 0x08, 0x86, 0x08, 0xA6, 0x08, 0xC6, 0x08, 0xE6, 
	0x08, 0x07, 0x08, 0x27, 0x08, 0x47, 0x08, 0x67, 0x08, 0x87, 0x08, 0xA7, 0x08, 0xC7, 0x08, 0xE7, 
	0x10, 0x00, 0x10, 0x20, 0x10, 0x40, 0x10, 0x60, 0x10, 0x80, 0x10, 0xA0, 0x10, 0xC0, 0x10, 0xE0, 
	0x10, 0x01, 0x10, 0x21, 0x10, 0x41, 0x10, 0x61, 0x10, 0x81, 0x10, 0xA1, 0x10, 0xC1, 0x10, 0xE1, 
	0x10, 0x02, 0x10, 0x22, 0x10, 0x42, 0x10, 0x62, 0x10, 0x82, 0x10, 0xA2, 0x10, 0xC2, 0x10, 0xE2, 
	0x10, 0x03, 0x10, 0x23, 0x10, 0x43, 0x10, 0x63, 0x10, 0x83, 0x10, 0xA3, 0x10, 0xC3, 0x10, 0xE3, 
	0x10, 0x04, 0x10, 0x24, 0x10, 0x44, 0x10, 0x64, 0x10, 0x84, 0x10, 0xA4, 0x10, 0xC4, 0x10, 0xE4, 
	0x10, 0x05, 0x10, 0x25, 0x10, 0x45, 0x10, 0x65, 0x10, 0x85, 0x10, 0xA5, 0x10, 0xC5, 0x10, 0xE5, 
	0x10, 0x06, 0x10, 0x26, 0x10, 0x46, 0x10, 0x66, 0x10, 0x86, 0x10, 0xA6, 0x10, 0xC6, 0x10, 0xE6, 
	0x10, 0x07, 0x10, 0x27, 0x10, 0x47, 0x10, 0x67, 0x10, 0x87, 0x10, 0xA7, 0x10, 0xC7, 0x10, 0xE7, 
	0x18, 0x00, 0x18, 0x20, 0x18, 0x40, 0x18, 0x60, 0x18, 0x80, 0x18, 0xA0, 0x18, 0xC0, 0x18, 0xE0, 
	0x18, 0x01, 0x18, 0x21, 0x18, 0x41, 0x18, 0x61, 0x18, 0x81, 0x18, 0xA1, 0x18, 0xC1, 0x18, 0xE1, 
	0x18, 0x02, 0x18, 0x22, 0x18, 0x42, 0x18, 0x62, 0x18, 0x82, 0x18, 0xA2, 0x18, 0xC2, 0x18, 0xE2, 
	0x18, 0x03, 0x18, 0x23, 0x18, 0x43, 0x18, 0x63, 0x18, 0x83, 0x18, 0xA3, 0x18, 0xC3, 0x18, 0xE3, 
	0x18, 0x04, 0x18, 0x24, 0x18, 0x44, 0x18, 0x64, 0x18, 0x84, 0x18, 0xA4, 0x18, 0xC4, 0x18, 0xE4, 
	0x18, 0x05, 0x18, 0x25, 0x18, 0x45, 0x18, 0x65, 0x18, 0x85, 0x18, 0xA5, 0x18, 0xC5, 0x18, 0xE5, 
	0x18, 0x06, 0x18, 0x26, 0x18, 0x46, 0x18, 0x66, 0x18, 0x86, 0x18, 0xA6, 0xDE, 0xFF, 0x14, 0xA5, 
	0x10, 0x84, 0x00, 0xF8, 0xE0, 0x07, 0xE0, 0xFF, 0x1F, 0x00, 0x1F, 0xF8, 0xFF, 0x07, 0xDF, 0xFF, 	
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 
	0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 
	0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 
	0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 
	0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38
};


unsigned char rgb_gamma_table[256] = {
	/*****0.9******
	0x00,0x02,0x03,0x05,0x06,0x07,0x09,0x0a,0x0b,0x0d,0x0e,0x0f,0x10,0x12,0x13,0x14,
	0x15,0x16,0x17,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x20,0x21,0x22,0x23,0x24,0x25,0x26,
	0x27,0x28,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x36,0x37,0x38,
	0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,
	0x49,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
	0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
	0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,
	0x7a,0x7b,0x7c,0x7d,0x7e,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,
	0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,
	0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
	0xa8,0xa9,0xaa,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,
	0xb7,0xb8,0xb9,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,
	0xc6,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd2,0xd3,
	0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,
	0xe3,0xe4,0xe5,0xe6,0xe7,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,
	0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
	****************/
	/********0.85***
	0x00,0x02,0x04,0x06,0x07,0x09,0x0b,0x0c,0x0d,0x0f,0x10,0x12,0x13,0x14,0x16,0x17,
	0x18,0x1a,0x1b,0x1c,0x1d,0x1f,0x20,0x21,0x22,0x23,0x25,0x26,0x27,0x28,0x29,0x2b,
	0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3d,
	0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4b,0x4c,0x4d,0x4e,
	0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,
	0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,
	0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,
	0x7f,0x80,0x81,0x82,0x83,0x84,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,
	0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,
	0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,
	0xac,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb6,0xb7,0xb8,0xb9,
	0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc7,
	0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,
	0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,
	0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf0,0xf1,
	0xf2,0xf3,0xf4,0xf5,0xf6,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfc,0xfd,0xfe,0xff 
	**************/
	/*****0.75******
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x11,0x13,0x15,0x16,0x18,0x1a,0x1b,0x1d,0x1e,
	0x20,0x21,0x23,0x24,0x26,0x27,0x29,0x2a,0x2b,0x2d,0x2e,0x2f,0x31,0x32,0x33,0x34,
	0x36,0x37,0x38,0x3a,0x3b,0x3c,0x3d,0x3e,0x40,0x41,0x42,0x43,0x44,0x45,0x47,0x48,
	0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
	0x5a,0x5b,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,
	0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,
	0x7b,0x7c,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
	0x8a,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x94,0x95,0x96,0x97,
	0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa4,0xa5,
	0xa6,0xa7,0xa8,0xa9,0xaa,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb0,0xb1,0xb2,0xb3,
	0xb4,0xb5,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xbf,0xc0,
	0xc1,0xc2,0xc3,0xc4,0xc4,0xc5,0xc6,0xc7,0xc8,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xcd,
	0xce,0xcf,0xd0,0xd1,0xd1,0xd2,0xd3,0xd4,0xd5,0xd5,0xd6,0xd7,0xd8,0xd8,0xd9,0xda,
	0xdb,0xdc,0xdc,0xdd,0xde,0xdf,0xe0,0xe0,0xe1,0xe2,0xe3,0xe3,0xe4,0xe5,0xe6,0xe7,
	0xe7,0xe8,0xe9,0xea,0xea,0xeb,0xec,0xed,0xee,0xee,0xef,0xf0,0xf1,0xf1,0xf2,0xf3,
	0xf4,0xf4,0xf5,0xf6,0xf7,0xf7,0xf8,0xf9,0xfa,0xfa,0xfb,0xfc,0xfd,0xfd,0xfe,0xff 
	***************/
	/********0.65*****/
	0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x19,0x1b,0x1d,0x1f,0x21,0x23,0x25,0x27,0x28,
	0x2a,0x2c,0x2e,0x2f,0x31,0x32,0x34,0x35,0x37,0x38,0x3a,0x3b,0x3d,0x3e,0x3f,0x41,
	0x42,0x44,0x45,0x46,0x47,0x49,0x4a,0x4b,0x4c,0x4e,0x4f,0x50,0x51,0x53,0x54,0x55,
	0x56,0x57,0x58,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x64,0x65,0x66,0x67,
	0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
	0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x83,0x84,0x85,0x86,
	0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,
	0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa0,0xa1,0xa2,
	0xa3,0xa4,0xa5,0xa5,0xa6,0xa7,0xa8,0xa9,0xa9,0xaa,0xab,0xac,0xad,0xad,0xae,0xaf,
	0xb0,0xb1,0xb1,0xb2,0xb3,0xb4,0xb5,0xb5,0xb6,0xb7,0xb8,0xb9,0xb9,0xba,0xbb,0xbc,
	0xbc,0xbd,0xbe,0xbf,0xbf,0xc0,0xc1,0xc2,0xc2,0xc3,0xc4,0xc5,0xc5,0xc6,0xc7,0xc8,
	0xc8,0xc9,0xca,0xcb,0xcb,0xcc,0xcd,0xce,0xce,0xcf,0xd0,0xd0,0xd1,0xd2,0xd3,0xd3,
	0xd4,0xd5,0xd5,0xd6,0xd7,0xd8,0xd8,0xd9,0xda,0xda,0xdb,0xdc,0xdd,0xdd,0xde,0xdf,
	0xdf,0xe0,0xe1,0xe1,0xe2,0xe3,0xe4,0xe4,0xe5,0xe6,0xe6,0xe7,0xe8,0xe8,0xe9,0xea,
	0xea,0xeb,0xec,0xec,0xed,0xee,0xee,0xef,0xf0,0xf0,0xf1,0xf2,0xf2,0xf3,0xf4,0xf4,
	0xf5,0xf6,0xf6,0xf7,0xf8,0xf8,0xf9,0xfa,0xfa,0xfb,0xfc,0xfc,0xfd,0xfe,0xfe,0xff
	/****************/
	/******0.45*******
	0x2f,0x2f,0x2f,0x2f,0x2f,0x2f,0x2f,0x33,0x36,0x39,0x3b,0x3e,0x40,0x43,0x45,0x47,
	0x49,0x4b,0x4d,0x4f,0x51,0x53,0x55,0x56,0x58,0x5a,0x5b,0x5d,0x5e,0x60,0x61,0x63,
	0x64,0x66,0x67,0x68,0x6a,0x6b,0x6c,0x6e,0x6f,0x70,0x71,0x72,0x74,0x75,0x76,0x77,
	0x78,0x79,0x7a,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,
	0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x96,
	0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9c,0x9d,0x9e,0x9f,0xa0,0xa0,0xa1,0xa2,0xa3,0xa4,
	0xa4,0xa5,0xa6,0xa7,0xa7,0xa8,0xa9,0xaa,0xaa,0xab,0xac,0xad,0xad,0xae,0xaf,0xaf,
	0xb0,0xb1,0xb2,0xb2,0xb3,0xb4,0xb4,0xb5,0xb6,0xb6,0xb7,0xb8,0xb8,0xb9,0xba,0xba,
	0xbb,0xbc,0xbc,0xbd,0xbe,0xbe,0xbf,0xc0,0xc0,0xc1,0xc1,0xc2,0xc3,0xc3,0xc4,0xc5,
	0xc5,0xc6,0xc6,0xc7,0xc8,0xc8,0xc9,0xc9,0xca,0xcb,0xcb,0xcc,0xcc,0xcd,0xce,0xce,
	0xcf,0xcf,0xd0,0xd0,0xd1,0xd2,0xd2,0xd3,0xd3,0xd4,0xd4,0xd5,0xd6,0xd6,0xd7,0xd7,
	0xd8,0xd8,0xd9,0xd9,0xda,0xdb,0xdb,0xdc,0xdc,0xdd,0xdd,0xde,0xde,0xdf,0xdf,0xe0,
	0xe0,0xe1,0xe1,0xe2,0xe3,0xe3,0xe4,0xe4,0xe5,0xe5,0xe6,0xe6,0xe7,0xe7,0xe8,0xe8,
	0xe9,0xe9,0xea,0xea,0xeb,0xeb,0xec,0xec,0xed,0xed,0xee,0xee,0xef,0xef,0xf0,0xf0,
	0xf1,0xf1,0xf2,0xf2,0xf2,0xf3,0xf3,0xf4,0xf4,0xf5,0xf5,0xf6,0xf6,0xf7,0xf7,0xf8,
	0xf8,0xf9,0xf9,0xfa,0xfa,0xfa,0xfb,0xfb,0xfc,0xfc,0xfd,0xfd,0xfe,0xfe,0xff,0xff
	******************/
};

extern uint8 *yuvbuf;


void lcd_register_read_3line(struct spi_device *spi_dev,uint32 code, uint8* buf, uint32 len){
	uint8 spi_buf[4];
	spi_buf[0] = code;
	spi_buf[1] = 0x00;	
	//ll_spi_clear_cs(SPI0);
	spi_set_cs(spi_dev,0,0);
	
	//ll_spi_buf_tx(SPI0, spi_buf, 2);
	spi_write(spi_dev,spi_buf,2);

//	SPI0->CON0 &= ~LL_SPI_CON0_FRAME_SIZE(0x3F);
//	SPI0->CON0 |= LL_SPI_CON0_FRAME_SIZE(8);
	spi_ioctl(spi_dev,SPI_SET_FRAME_SIZE,8,0);

	//ll_spi_buf_rx(SPI0, buf, len);
	spi_read(spi_dev,buf,len);
	//ll_spi_set_cs(SPI0);	
	spi_set_cs(spi_dev,0,1);
	//SPI0->CON0 &= ~LL_SPI_CON0_FRAME_SIZE(0x3F);
	//SPI0->CON0 |= LL_SPI_CON0_FRAME_SIZE(9);	
	spi_ioctl(spi_dev,SPI_SET_FRAME_SIZE,9,0);
}

void lcd_register_write_3line(struct spi_device *spi_dev,uint32 code, uint8* buf, uint32 len){

	uint8 itk;
	uint8 spi_buf[2*60];
	spi_buf[0] = code;
	spi_buf[1] = 0x00;
	for(itk = 0;itk < len;itk++){
		spi_buf[2*(itk+1)] = buf[itk];
		spi_buf[2*(itk+1)+1] = 0x01;		
	}
	
//	ll_spi_clear_cs(SPI0);
	spi_set_cs(spi_dev,0,0);
	
//	ll_spi_buf_tx(SPI0, spi_buf, 2*len+2);
	spi_write(spi_dev,spi_buf,2*len+2);

//	ll_spi_set_cs(SPI0);
	spi_set_cs(spi_dev,0,1);
}


void lcd_table_init(struct spi_device *spi_dev,uint8_t *lcd_table){
	uint32 itk = 0;
	uint8  last_opt = -1;
	uint8 buf[60];
	uint8 code;
	uint8 delay_ms = 0;
	uint8 param_num = 0;


	while(((lcd_table[itk] == LCD_TAB_END) && (lcd_table[itk+1] == LCD_TAB_END)) == 0){
		switch(lcd_table[itk]){
			case LCD_CMD:
				if(last_opt == LCD_CMD){
					lcd_register_write_3line(spi_dev,code,buf,param_num);
				}else if(last_opt == DELAY_MS){
					os_sleep_ms(delay_ms);
				}
				last_opt = LCD_CMD;
				code = lcd_table[itk+1];
				param_num = 0;
			break;
			case LCD_DAT:
				buf[param_num] = lcd_table[itk+1];
				param_num++;
			break;
			case DELAY_MS:
				if(last_opt == LCD_CMD){
					lcd_register_write_3line(spi_dev,code,buf,param_num);
				}else if(last_opt == DELAY_MS){
					os_sleep_ms(delay_ms);
				}
				last_opt = DELAY_MS;
				delay_ms = lcd_table[itk+1];
			break;
			default:
			break;
		}
		itk += 2; 
	}

	if(last_opt == LCD_CMD){
		lcd_register_write_3line(spi_dev,code,buf,param_num);
	}else if(last_opt == DELAY_MS){
		os_sleep_ms(delay_ms);
	}	
}


void lcd_table_init_MCU(struct lcdc_device *lcd_dev,uint8_t (*lcd_table)[2])
{
  uint8_t (*table)[2];
  int      i; 
  uint32_t dat_cmd;
  uint32_t data;
  table = lcd_table;
  for (i = 0;; i++)
  {
    dat_cmd = table[i][0];
    data    = table[i][1];
    //结束
    switch (dat_cmd)
    {
    case LCD_CMD:
      lcdc_mcu_write_reg(lcd_dev,data);
      break;
    case LCD_DAT:
      lcdc_mcu_write_data(lcd_dev,data);
      break;
    case DELAY_MS:
      os_sleep_ms(data);
      break;

    //结束
    case LCD_TAB_END:
      goto lcd_table_init_end;
      break;
    default:
      goto lcd_table_init_end;
      break;
    }
  }
lcd_table_init_end:
  return;
}


uint32 scale3_num = 0;
uint32 scale_time_buf[10] = {0};
uint32 avg_scale_time = 0;
uint32 new_scale_time = 0;
uint8  scale_b = 0;
uint8 lcd_or_scaler_kick;
uint32 lcdc_time_buf[10] = {0};
uint8  lcdc_b = 0;
uint32 avg_lcdc_time = 0;

uint32 p1_w,p1_h;
uint32 scale_p1_w;
uint32 dec_y_offset,dec_uv_offset;

volatile uint8_t lcd_frame_flash=0;

void lcd_sema_up();
void lcd_osd_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct lcdc_device *p_lcd = (struct lcdc_device *)irq_data;
	lcd_sema_up();	
}

void lcd_te_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct lcdc_device *lcd_dev;	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	if(lcd_frame_flash)
	{
		lcdc_set_timeout_info(lcd_dev,1,3);
		lcdc_set_start_run(lcd_dev);
		lcd_frame_flash=0;
	}

    return ;
}

void scale_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	os_printf("sor");
}

void scale2_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	os_printf("sor2");
}

void scale3_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	os_printf("sor3");
}

void lcd_timeout(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;
	lcdc_set_timeout_info(lcd_dev,0,3);
	//gpio_set_val(PC_7,pc_7);
	//pc_7 ^= BIT(0);	
	
	//lcdc_close(lcd_dev);
	os_printf("........................................................................................................................lcd_timeout\r\n");
	//lcdc_open(lcd_dev);
	//lcdc_set_start_run(lcd_dev);
}

void lcd_doublebuf_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	static uint32 lcd_num = 0;
	static uint32 get_lcdc_time = 0;
	static uint32 get_mask = 0; 
	uint32 kick_time;	
	uint8 itk = 0;
	//uint32 jtk = 0;
	
	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;

	if(lcd_info.lcd_osd_mode != lcd_info.lcd_osd_cur_mode){
		lcd_info.lcd_osd_cur_mode = lcd_info.lcd_osd_mode;
		if(lcd_info.lcd_osd_mode){
			lcdc_set_osd_start_location(lcd_dev,0,33);
			lcdc_set_osd_size(lcd_dev,256,240);	
			lcdc_set_osd_lut_addr(lcd_dev,(uint32)osd_palette);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1+512);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_256);			
		}else{
			lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
		}			
	}

	if(lcd_info.lcd_run_new_lcd)
	{
		lcd_info.lcd_run_new_lcd = 0;
		lcdc_set_osd_en(lcd_dev,0);
		//osd0_enable(0);
		if(lcd_info.osd_buf_to_lcd == 0){
			//osd0_set_dma_address(osd_encode_buf);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf);
			//os_printf("s0");
			//gpio_set_val(PA_15,0);
		}else{
			//osd0_set_dma_address(osd_encode_buf1);	
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1);
			//os_printf("s1");
			//gpio_set_val(PA_15,1);
		}
		
		//osd0_len(p_lcd->OSD_ENC_DLEN * 4);
		lcdc_set_osd_dma_len(lcd_dev,lcdc_get_osd_enc_dst_len(lcd_dev));
		lcdc_set_osd_en(lcd_dev,1);
	}
	
	if(lcd_info.lcd_p0p1_state != lcd_info.lcd_p0p1_cur){
		lcd_info.lcd_p0p1_cur = lcd_info.lcd_p0p1_state;
		if(0 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,0);
		}else if(1 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,0);
		}else if(2 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,1);
		}else if(3 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,1);
		}
	}

	if(lcd_info.lcd_p1_size_reset){
		lcd_info.lcd_p1_size_reset = 0;
		lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,p1_w,p1_h);
		lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
	}
	
#if (SCALE_DIRECT_TO_LCD == 0)
	if(get_lcdc_time == 0){
		lcdc_time_buf[lcdc_b] = 0;
	}else{
		lcdc_time_buf[lcdc_b] = (os_jiffies()-get_lcdc_time);
	}
		
	get_lcdc_time = os_jiffies();
	lcdc_b++;
	if(lcdc_b == 10)
		lcdc_b = 0;

	
	if(get_mask == 0){
		avg_lcdc_time = 0;
		for(itk = 0;itk < 10;itk++){
			if(lcdc_time_buf[itk] != 0){
				avg_lcdc_time += lcdc_time_buf[itk];
			}
			else{
				itk++;
				break;
			}
		}
		
		if(itk == 10){
			avg_lcdc_time = avg_lcdc_time/itk;
			get_mask = 1;
		}
	}	

	if(((get_lcdc_time+avg_lcdc_time) > (new_scale_time + avg_scale_time)) && (avg_scale_time != 0)){
		kick_time = (get_lcdc_time+avg_lcdc_time) - (new_scale_time + avg_scale_time);
		if(kick_time < (avg_lcdc_time*2/10) ){
			lcd_or_scaler_kick  = 1;	
		}else{
			lcd_or_scaler_kick	= 0;
		}
	}else{
		lcd_or_scaler_kick  = 1;
	}

	if(lcd_or_scaler_kick){	
		if((scale3_num%2) == 0){
			lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem1+Y_OFFSET);
			lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
			lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);	
			if(lcdc_decode_enable){
				lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
				lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset);
				lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
			}
		}else{
			lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem+Y_OFFSET);
			lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
			lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);	
			if(lcdc_decode_enable){
				lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
				lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset);
				lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
			}			
		}		
	}
	//os_printf("--");
	if(lcd_or_scaler_kick)
		lcdc_set_start_run(lcd_dev);

#else
	lcdc_set_start_run(lcd_dev);
#endif
	lcd_num++;
		
}


void lcd_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	static uint32 lcd_num = 0;	
	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;

	if(lcd_info.lcd_osd_mode != lcd_info.lcd_osd_cur_mode){
		lcd_info.lcd_osd_cur_mode = lcd_info.lcd_osd_mode;
		if(lcd_info.lcd_osd_mode){
			lcdc_set_osd_start_location(lcd_dev,0,33);
			lcdc_set_osd_size(lcd_dev,256,240);	
			lcdc_set_osd_lut_addr(lcd_dev,(uint32)osd_palette);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1+512);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_256);			
		}else{
			lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
		}			
	}

	if(lcd_info.lcd_run_new_lcd)
	{
		lcd_info.lcd_run_new_lcd = 0;
		lcdc_set_osd_en(lcd_dev,0);
		//osd0_enable(0);
		if(lcd_info.osd_buf_to_lcd == 0){
			//osd0_set_dma_address(osd_encode_buf);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf);
			//os_printf("s0");
			//gpio_set_val(PA_15,0);
		}else{
			//osd0_set_dma_address(osd_encode_buf1);	
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1);
			//os_printf("s1");
			//gpio_set_val(PA_15,1);
		}
		
		//osd0_len(p_lcd->OSD_ENC_DLEN * 4);
		lcdc_set_osd_dma_len(lcd_dev,lcdc_get_osd_enc_dst_len(lcd_dev));
		lcdc_set_osd_en(lcd_dev,1);
	}
	
	if(lcd_info.lcd_p0p1_state != lcd_info.lcd_p0p1_cur){
		lcd_info.lcd_p0p1_cur = lcd_info.lcd_p0p1_state;
		if(0 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,0);
		}else if(1 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,0);
		}else if(2 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,1);
		}else if(3 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,1);
		}
	}

	if(lcd_info.lcd_p1_size_reset){
		lcd_info.lcd_p1_size_reset = 0;
		lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,p1_w,p1_h);
		lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
	}
#if (SCALE_DIRECT_TO_LCD == 0)
	if((scale3_num%3) == 1){
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem2+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}else if((scale3_num%3) == 2){		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}else{		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem1+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}
	
	if(lcdc_decode_enable){
		if((decode_num%3) == 0){			
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem2+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
		}else if((decode_num%3) == 1){
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
		}else{
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem1+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
		}
	}
	lcdc_set_timeout_info(lcd_dev,1,3);
	lcdc_set_start_run(lcd_dev);
#else
	lcdc_set_start_run(lcd_dev);
#endif
	lcd_num++;
	
}


void lcd_squralbuf_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	static uint32 lcd_num = 0;	
	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;

	if(lcd_info.lcd_osd_mode != lcd_info.lcd_osd_cur_mode){
		lcd_info.lcd_osd_cur_mode = lcd_info.lcd_osd_mode;
		if(lcd_info.lcd_osd_mode){
			lcdc_set_osd_start_location(lcd_dev,0,33);
			lcdc_set_osd_size(lcd_dev,256,240);	
			lcdc_set_osd_lut_addr(lcd_dev,(uint32)osd_palette);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1+512);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_256);			
		}else{
			lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
		}			
	}

	if(lcd_info.lcd_run_new_lcd)
	{
		lcd_info.lcd_run_new_lcd = 0;
		lcdc_set_osd_en(lcd_dev,0);
		//osd0_enable(0);
		if(lcd_info.osd_buf_to_lcd == 0){
			//osd0_set_dma_address(osd_encode_buf);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf);
			//os_printf("s0");
			//gpio_set_val(PA_15,0);
		}else{
			//osd0_set_dma_address(osd_encode_buf1);	
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1);
			//os_printf("s1");
			//gpio_set_val(PA_15,1);
		}
		
		//osd0_len(p_lcd->OSD_ENC_DLEN * 4);
		lcdc_set_osd_dma_len(lcd_dev,lcdc_get_osd_enc_dst_len(lcd_dev));
		lcdc_set_osd_en(lcd_dev,1);
	}
	
	if(lcd_info.lcd_p0p1_state != lcd_info.lcd_p0p1_cur){
		lcd_info.lcd_p0p1_cur = lcd_info.lcd_p0p1_state;
		if(0 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,0);
		}else if(1 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,0);
		}else if(2 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,1);
		}else if(3 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,1);
		}
		os_printf("lcd_p0p1_cur %d\r\n", lcd_info.lcd_p0p1_cur);
	}

	if(lcd_info.lcd_p1_size_reset){
		lcd_info.lcd_p1_size_reset = 0;
		lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,p1_w,p1_h);
		lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
	}
#if (SCALE_DIRECT_TO_LCD == 0)
	if((scale3_num%4) == 1){
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem2+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
		
	}else if((scale3_num%4) == 2){		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem3+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem3+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem3+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
		
	}else if((scale3_num%4) == 3){		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
		
	} else {
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem1+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}
	
	if(lcdc_decode_enable){
		if((decode_num%3) == 1){			
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem1+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
		}else if((decode_num%3) == 2){
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem2+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
		// }else if((decode_num%3) == 3){
		// 	lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		// 	lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		// 	lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
		} else {
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
		}
	}
	#if (LCD_TE == 255)
	lcdc_set_timeout_info(lcd_dev,1,3);
	lcdc_set_start_run(lcd_dev);
	#else
		lcd_frame_flash=1;
	#endif
#else
	lcdc_set_start_run(lcd_dev);
#endif
	lcd_num++;
	
}


volatile uint8 scale2_finish = 1;
volatile uint8 scale_take_photo = 1;
void scale_take_photo_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	scale_take_photo = 1;
	os_printf("===============================================================================================take photo done\r\n");
}	



void scale_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//os_printf("sd..\r\n");
}


void scale2_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct jpg_device *jpg_dev;
	struct scale_device *scale_dev = (struct scale_device *)irq_data;
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);
	if((decode_num%3) == 0){
		scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h);
		scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	}else if((decode_num%3) == 1){
		scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem1);
		scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h);
		scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	}else if((decode_num%3) == 2){
		scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem2);
		scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h);
		scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	} else {
		// scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem3);
		// scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem3+scale_p1_w*p1_h);
		// scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem3+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	}
	// os_printf("scale2 done\r\n");
	scale2_finish = 1;
	decode_num++;
}

void scale3_doublebuf_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct scale_device *scale_dev = (struct scale_device *)irq_data;
	uint8  itk = 0;
	static uint8_t enlarge_state = 10;
	//struct vpp_device *vpp_dev;
	//vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);

	if(gui_cfg.enlarge_lcd != 0){
		if(gui_cfg.enlarge_lcd != enlarge_state){
			enlarge_state = gui_cfg.enlarge_lcd;
			set_lcd_enlarge_config(gui_cfg.dvp_w,gui_cfg.dvp_h,SCALE_WIDTH,SCALE_HIGH,gui_cfg.enlarge_lcd); 			
		}
	}


#if (SCALE_DIRECT_TO_LCD == 0)
	if(new_scale_time == 0){
		scale_time_buf[scale_b] = 0;
	}else{
		scale_time_buf[scale_b] = (os_jiffies()-new_scale_time);
	}
		
	new_scale_time = os_jiffies();
	scale_b++;
	if(scale_b == 10)
		scale_b = 0;

	avg_scale_time = 0;
	for(itk = 0;itk < 10;itk++){
		if(scale_time_buf[itk] != 0){
			avg_scale_time += scale_time_buf[itk];
		}
		else{
			itk++;
			break;
		}
	}
	avg_scale_time = avg_scale_time/itk;
	
	if((scale3_num%2) == 0){
		//scale3_set_output_addr(SCALE3,video_psram_mem1,video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH,video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem1);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}else{
		//scale3_set_output_addr(SCALE3,video_psram_mem,video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH,video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4); 
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}

	if(lcd_or_scaler_kick == 0){
		if((scale3_num%2) == 0){
			lcdc_set_p0_rotate_y_src_addr(lcd_dev_gobal,(uint32)video_psram_mem+Y_OFFSET);
			lcdc_set_p0_rotate_u_src_addr(lcd_dev_gobal,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
			lcdc_set_p0_rotate_v_src_addr(lcd_dev_gobal,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);	
			if(lcdc_decode_enable){
				lcdc_set_p1_rotate_y_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+DEC_Y_OFFSET/**/);
				lcdc_set_p1_rotate_u_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+SCALE_PHOTO1_CONFIG_W*PHOTO1_H+DEC_UV_OFFSET/**/);
				lcdc_set_p1_rotate_v_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/4+DEC_UV_OFFSET/**/);
			}
		}else{
			lcdc_set_p0_rotate_y_src_addr(lcd_dev_gobal,(uint32)video_psram_mem1+Y_OFFSET);
			lcdc_set_p0_rotate_u_src_addr(lcd_dev_gobal,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
			lcdc_set_p0_rotate_v_src_addr(lcd_dev_gobal,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
			if(lcdc_decode_enable){
				lcdc_set_p1_rotate_y_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+DEC_Y_OFFSET/**/);
				lcdc_set_p1_rotate_u_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+SCALE_PHOTO1_CONFIG_W*PHOTO1_H+DEC_UV_OFFSET/**/);
				lcdc_set_p1_rotate_v_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/4+DEC_UV_OFFSET/**/);
			}
		}
		lcdc_set_start_run(lcd_dev_gobal);		
	}
	

#endif
	scale3_num++;

}


void scale3_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct scale_device *scale_dev = (struct scale_device *)irq_data;
	static uint8_t enlarge_state = 10;
	//struct vpp_device *vpp_dev;
	//vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);

	if(gui_cfg.enlarge_lcd != 0){
		if(gui_cfg.enlarge_lcd != enlarge_state){
			enlarge_state = gui_cfg.enlarge_lcd;
			set_lcd_enlarge_config(gui_cfg.dvp_w,gui_cfg.dvp_h,SCALE_WIDTH,SCALE_HIGH,gui_cfg.enlarge_lcd); 			
		}
	}

	
#if (SCALE_DIRECT_TO_LCD == 0)
	if((scale3_num%3) == 0){		
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}else if((scale3_num%3) == 1){
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem1);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}else{
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem2);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}
#endif
	scale3_num++;
	// os_printf("scale%d done\r\n", scale3_num);
}

#if LCD_VIDEO_EF
void scale3_squralbuf_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct scale_device *scale_dev = (struct scale_device *)irq_data;
	static uint8_t enlarge_state = 10;
	//struct vpp_device *vpp_dev;
	//vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);

	if(gui_cfg.enlarge_lcd != 0){
		if(gui_cfg.enlarge_lcd != enlarge_state){
			enlarge_state = gui_cfg.enlarge_lcd;
			set_lcd_enlarge_config(gui_cfg.dvp_w,gui_cfg.dvp_h,SCALE_WIDTH,SCALE_HIGH,gui_cfg.enlarge_lcd); 			
		}
	}


#if (SCALE_DIRECT_TO_LCD == 0)
	if((scale3_num%4) == 0){		
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
		
	}else if((scale3_num%4) == 1){
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem1);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
		
	}else if((scale3_num%4) == 2){
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem2);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
		
	} else {
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem3);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem3+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem3+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
		
	}
#endif
	switch (scale3_num%4)
	{
		case 0:
			os_msgq_put(&lcd_vef_msg, (uint32_t)video_psram_config_mem3, 0);
			break;
		case 1:
			os_msgq_put(&lcd_vef_msg, (uint32_t)video_psram_config_mem, 0);
			break;
		case 2:
			os_msgq_put(&lcd_vef_msg, (uint32_t)video_psram_config_mem1, 0);
			break;
		case 3:
			os_msgq_put(&lcd_vef_msg, (uint32_t)video_psram_config_mem2, 0);
			break;
		default:
			break;
	}

	scale3_num++;
}
#endif

void jpg_decode_done(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){
	//os_printf("decode finish\r\n");
}

void jpg_decode_err(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){
	os_printf("decode err\r\n");
}

void set_lcd_enlarge_config(uint32_t sw,uint32_t sh,uint32_t ow,uint32_t oh,uint8_t enlarge){
	uint32_t x_offset,y_offset;
	uint32_t w_start,h_start;
	//float  w_enlarge,h_enlarge;
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);	
	scale_set_in_out_size(scale_dev,sw,sh,ow,oh);
	scale_set_step(scale_dev,sw,sh,ow*enlarge/10,oh*enlarge/10);
	w_start = ow*enlarge/10;				//实际放大后的大小
	h_start = oh*enlarge/10;	
	x_offset = (w_start - ow)/2;
	y_offset = (h_start - oh)/2;
	scale_set_start_addr(scale_dev,x_offset,y_offset);

}



void set_lcd_photo1_config(uint32 w,uint32 h,uint8 rotate_180){
	extern uint32 p1_w,p1_h;
	extern uint32 scale_p1_w;
	extern uint32 dec_y_offset,dec_uv_offset;
	uint32 flags;
	flags = disable_irq();
	lcd_info.lcd_p1_size_reset = 1;
	p1_w = w;
	p1_h = h;
	scale_p1_w    = ((p1_w+3)/4)*4;
	if(rotate_180){
		dec_y_offset  = p1_w*(p1_h-1);
		dec_uv_offset = ((scale_p1_w/2+3)/4)*4 * (p1_h/2-1);
	}else{
		dec_y_offset  = 0;
		dec_uv_offset = 0;
	}
	enable_irq(flags);
}


void scale_soft_run(uint8_t *softbuf,uint32_t w,uint32 h){
	struct scale_device *scale_dev;
	uint16 icount = 2;	
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
	//icount	= h/16;
	scale_set_inbuf_num(scale_dev,0,0);
	if(scale_get_inbuf_num(scale_dev) == 0){
		icount = 2;
		//os_printf("new frame...\r\n");
		scale_set_new_frame(scale_dev,1);
	}else{
		return;
	}
	hw_memcpy(yuvbuf,softbuf,(16*icount)*w);
	hw_memcpy(yuvbuf+(16*icount)*w,softbuf+h*w,(16*icount)*w/4);
	hw_memcpy(yuvbuf+(16*icount)*w+(16*icount)*w/4,softbuf+h*w+h*w/4,(16*icount)*w/4);
	
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf+(16*icount)*w);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf+(16*icount)*w+(16*icount)*w/4);
	
	scale_set_inbuf_num(scale_dev,icount*16-1,0);    //0~31
	
	while(1){
		//if(scale_get_inbuf_num(scale_dev) == (scale_get_heigh_cnt(scale_dev)+2))
		if(scale_get_heigh_cnt(scale_dev) > ((icount-1)*16) )
		{			
			if((icount%2) == 0){
				//os_printf("get_height_cnt:%d===>up head\r\n",scale_get_heigh_cnt(scale_dev));
				hw_memcpy(yuvbuf,                  softbuf+(16*icount)*w,                            16*w);
				hw_memcpy(yuvbuf+32*w,             softbuf+h*w+(16*icount)*w/4,                    16*w/4);
				hw_memcpy(yuvbuf+32*w+32*w/4,      softbuf+h*w+h*w/4+(16*icount)*w/4,              16*w/4);	
				
			}else{
				//os_printf("get_height_cnt:%d===>up tail\r\n",scale_get_heigh_cnt(scale_dev));
				hw_memcpy(yuvbuf+16*w,                  softbuf+(16*icount)*w,                       16*w);
				hw_memcpy(yuvbuf+32*w+32*w/8,           softbuf+h*w+(16*icount)*w/4,               16*w/4);
				hw_memcpy(yuvbuf+32*w+32*w/4+32*w/8,    softbuf+h*w+h*w/4+(16*icount)*w/4,         16*w/4);	
			}
			icount++;
			if(icount == (h/16)){	
				scale_set_inbuf_num(scale_dev,icount*16,16);
				//os_printf("end frame...\r\n");
				//scale_set_end_frame(scale_dev,1);
				break;
			}
			if((icount%2) == 1)     //3   start
				scale_set_inbuf_num(scale_dev,icount*16,16);
			else
				scale_set_inbuf_num(scale_dev,icount*16,0);
		}
		//os_sleep_ms(1);
	}

	
}

void scale_to_lcd_config_soft(uint8_t *softbuf,uint32_t w,uint32_t h){
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
	scale_close(scale_dev);
	scale_set_in_out_size(scale_dev,w,h,SCALE_WIDTH,SCALE_HIGH);
	scale_set_step(scale_dev,w,h,SCALE_WIDTH,SCALE_HIGH);
	scale_set_start_addr(scale_dev,0,0);
	scale_set_dma_to_memory(scale_dev,1);
	scale_set_data_from_vpp(scale_dev,0);  
	scale_set_line_buf_num(scale_dev,32);       //soft的line buf

	scale_set_in_yaddr(scale_dev,(uint32)softbuf);
	scale_set_in_uaddr(scale_dev,(uint32)softbuf+w*h);
	scale_set_in_vaddr(scale_dev,(uint32)softbuf+w*h+w*h/4);	

	scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
	scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
	scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);

	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale3_ov_isr,(uint32)scale_dev);
	scale_open(scale_dev);	
}



void scale_to_lcd_config(){	
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
#if SCALE_DIRECT_TO_LCD
	scale_set_in_out_size(scale_dev,photo_msg.in_w,photo_msg.in_h,SCALE_HIGH,SCALE_WIDTH);
	scale_set_step(scale_dev,photo_msg.in_w,photo_msg.in_h,SCALE_HIGH,SCALE_WIDTH);
	scale_set_start_addr(scale_dev,0,0);
	scale_set_dma_to_memory(scale_dev,0);
	scale_set_data_from_vpp(scale_dev,1);  
	scale_set_line_buf_num(scale_dev,32);       //vpp的line buf
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf+photo_msg.in_w*32);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf+photo_msg.in_w*32+photo_msg.in_w*8);	
#else
	scale_set_in_out_size(scale_dev,photo_msg.in_w,photo_msg.in_h,SCALE_WIDTH,SCALE_HIGH);
#if LCD_33_WVGA
	scale_set_step(scale_dev,photo_msg.in_w,photo_msg.in_h,SCALE_WIDTH,640);
#else
	scale_set_step(scale_dev,photo_msg.in_w,photo_msg.in_h,SCALE_WIDTH,SCALE_HIGH);
#endif
	scale_set_start_addr(scale_dev,0,0);
	scale_set_dma_to_memory(scale_dev,1);
	scale_set_data_from_vpp(scale_dev,1);  
	scale_set_line_buf_num(scale_dev,32);       //vpp的line buf
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf+photo_msg.in_w*32);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf+photo_msg.in_w*32+photo_msg.in_w*8);
	scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
	scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
	scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
#endif
#if LCD_THREE_BUF
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_done,(uint32)scale_dev);	
#elif LCD_VIDEO_EF
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_squralbuf_done,(uint32)scale_dev);	
#else 
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_doublebuf_done,(uint32)scale_dev);	
#endif
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale3_ov_isr,(uint32)scale_dev);
	scale_open(scale_dev);
}

void scale_from_vpp_to_jpg(struct scale_device *scale_dev,uint32 yuvbuf_addr,uint32 s_w,uint32 s_h,uint32 d_w,uint32 d_h){
	scale_set_in_out_size(scale_dev,s_w,s_h,d_w,d_h);
	scale_set_step(scale_dev,s_w,s_h,d_w,d_h);
	scale_set_line_buf_num(scale_dev,32);
	scale_set_in_yaddr(scale_dev,yuvbuf_addr);
	scale_set_in_uaddr(scale_dev,yuvbuf_addr+s_w*32);
	scale_set_in_vaddr(scale_dev,yuvbuf_addr+s_w*32+s_w*8);	
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale_ov_isr,(uint32)scale_dev);
	scale_set_data_from_vpp(scale_dev,1);  
	scale_open(scale_dev);
}

//uint8 itp_sram_linbuf[12*640*3/2]__attribute__ ((aligned(4)));
void scale_from_soft_to_jpg(struct scale_device *scale_dev,uint32 yuvbuf_addr,uint32 s_w,uint32 s_h,uint32 d_w,uint32 d_h){
	uint8 line_num;
	struct vpp_device *vpp_dev;
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	scale_close(scale_dev);
	vpp_set_itp_enable(vpp_dev,0);	
	_os_printf("take:%d %d %d\r\n",s_h,d_h,((s_h*16)/d_h));
	line_num = (((s_h*16)/d_h) + ((((s_h*16)/d_h)%2)?1:0) + 2)/2; 
	_os_printf("line_num:%d\r\n",line_num*3*2);
	scale_set_in_out_size(scale_dev,s_w,s_h,d_w,d_h);
	scale_set_step(scale_dev,s_w,s_h,d_w,d_h);
	scale_set_line_buf_num(scale_dev,line_num*3*2);

	//scale_set_in_yaddr(scale_dev,itp_sram_linbuf);
	//scale_set_in_uaddr(scale_dev,itp_sram_linbuf+line_num*3*2*s_w);
	//scale_set_in_vaddr(scale_dev,itp_sram_linbuf+line_num*3*2*s_w+line_num*3*2*s_w/4);
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf+s_w*32);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf+s_w*32+s_w*8);	
	
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale_take_photo_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale_ov_isr,(uint32)scale_dev);	
	scale_set_data_from_vpp(scale_dev,0);
	scale_set_inbuf_num(scale_dev,0,0);
	scale_open(scale_dev);
	scale_set_new_frame(scale_dev,1);
	
	vpp_set_itp_y_addr(vpp_dev,yuvbuf_addr);
	vpp_set_itp_u_addr(vpp_dev,yuvbuf_addr+s_w*s_h);
	vpp_set_itp_v_addr(vpp_dev,yuvbuf_addr+s_w*s_h+s_w*s_h/4);
	vpp_set_itp_linebuf(vpp_dev,line_num);	
	vpp_set_itp_auto_close(vpp_dev,1);
	vpp_set_itp_enable(vpp_dev,1);		
}

void jpg_decode_scale_config(uint32 dst){
	struct jpg_device *jpg_dev;
	struct scale_device *scale_dev;
	if(decode_mutex.magic != 0xa8b4c2d5){
//		os_mutex_init(&decode_mutex);
	}

	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);


	scale_set_in_out_size(scale_dev,photo_msg.in_w,photo_msg.in_h,SCALE_HIGH,SCALE_WIDTH);
	scale_set_step(scale_dev,photo_msg.in_w,photo_msg.in_h,SCALE_HIGH,SCALE_WIDTH);

	scale_set_out_yaddr(scale_dev,(uint32)dst);
	scale_set_out_uaddr(scale_dev,(uint32)dst+scale_p1_w*p1_h);
	scale_set_out_vaddr(scale_dev,(uint32)dst+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	scaler2buf = os_malloc(32+p1_w*2+(32*SRAMBUF_WLEN*4*3)/2);
	if(scaler2buf == NULL){
		return;
	}	
	scale_set_line_buf_addr(scale_dev,(uint32)scaler2buf);
	scale_set_srambuf_wlen(scale_dev,SRAMBUF_WLEN);
	scale_set_start_addr(scale_dev,0,0);
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale2_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale2_ov_isr,(uint32)scale_dev);
	jpg_request_irq(jpg_dev,(jpg_irq_hdl )&jpg_decode_done,JPG_IRQ_FLAG_JPG_DONE,(void *)jpg_dev);
	jpg_request_irq(jpg_dev,(jpg_irq_hdl )&jpg_decode_err,JPG_IRQ_FLAG_ERROR,(void *)jpg_dev);	
	jpg_decode_target(jpg_dev,1);
	decode_num = 0;
	scale_open(scale_dev);
	os_printf("mj decode scale config done\r\n");
	
}

void jpg_dec_scale_del(){
	os_free(scaler2buf);
	scaler2buf = NULL;
}

void jpg_decode_to_lcd(uint32 photo,uint32 jpg_w,uint32 jpg_h,uint32 video_w,uint32 video_h){
	struct jpg_device *jpg_dev;
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);	
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);		
	scale_close(scale_dev);
	scale_set_in_out_size(scale_dev,jpg_w,jpg_h,video_w,video_h);
	scale_set_step(scale_dev,jpg_w,jpg_h,video_w,video_h);	
	scale_open(scale_dev);	
	jpg_decode_photo(jpg_dev,photo);
}

int32 jpg_decode_is_finish(){
	struct jpg_device *jpg_dev;
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);	

	return jpg_is_idle(jpg_dev);
}

void lcd_user_frame(uint32 frame_addr){
	video_decode_mem  = (uint8 *)frame_addr;
	video_decode_mem1 = (uint8 *)frame_addr;
	video_decode_mem2 = (uint8 *)frame_addr;
}

volatile uint32 ve_sel = 0;
volatile uint32 fr_down = 0;
void ve_func()
{
// extern void hexagon_ve(uint8* in, uint8* tem, uint32 w, uint32 h);
// extern void block_9(uint8* in, uint8* tem, uint32 w, uint32 h);
// extern void block_4(uint8* in, uint8* tem, uint32 w, uint32 h);
// extern void block_2_yinv(uint8* in, uint8* tem, uint32 w, uint32 h);
// extern void block_2_xinv(uint8* in, uint8* tem, uint32 w, uint32 h);
// extern void uv_offset(uint8* in, uint8* tmp, uint32 w, uint32 h, int32 uoff, int32 voff);
// extern void uv_offset1(uint8* in, uint8* tmp, uint32 w, uint32 h, int32 uoff, int32 voff);
	volatile uint8* p_video;
	uint32 tick;
	uint32 max_tick = 0;
	uint32 max_sel = 0;
	uint32 tick_tmp;
	uint32 t_cond = 0;
	uint32 s_cond = os_jiffies();
	p_video = (uint8*)os_msgq_get(&lcd_vef_msg, osWaitForever);

	while(1)
	{
		if(g_vf_cblk.desp.open == 0) {
			break;
		}
		p_video = (uint8*)os_msgq_get(&lcd_vef_msg, osWaitForever);
		tick = os_jiffies();
		ve_sel = g_vf_cblk.desp.ve_sel;
		// ve_sel = (t_cond / 3000) % 7;
		// ve_sel = 1;
		// ve_sel = 0;
		// if(fr_down) {
		// 	fr_down--;
		// 	continue;
		// }
		switch (ve_sel)
		{
		case 1:
			block_9((uint8*)p_video, 0, 320, 240);
			break;
		case 2:
			block_4((uint8*)p_video, 0, 320, 240);
			break;
		case 3:
			block_2_yinv((uint8*)p_video, 0, 320, 240);
			break;
		case 4:
			block_2_xinv((uint8*)p_video, 0, 320, 240);
			break;
		case 5 :
			hexagon_ve1((uint8*)p_video, 0, 320, 240);
			break;
		case 6:
			uv_offset((uint8*)p_video, 0, 320, 240, 30, 30);
			break;
		default:
			break;
		}
		
		t_cond = os_jiffies()-s_cond;
		tick_tmp = os_jiffies()-tick;
		if(tick_tmp > max_tick) {
			max_tick = tick_tmp;
			max_sel = ve_sel;
		}
		// os_printf("max_tick : %d, sel: %d ", max_tick, max_sel);
		// os_printf("tick : %d ve_sel: %d \r\n", tick_tmp, ve_sel);
		// os_printf("cur block:  %d \r\n", block);
		// os_printf("ve_func: %x\r\n", p_video);
	}
	
}

void vf_open()
{
	vf_switch(1);
	OS_TASK_INIT("lcd_vef", &lcd_vef_task, ve_func, NULL, OS_TASK_PRIORITY_NORMAL, 2048);
	os_printf("vf open\r\n");
}

void vf_close()
{
	vf_switch(0);
	os_task_del(&lcd_vef_task);
	os_printf("vf close\r\n");
}
static void get_osd_w_h(uint16_t *w,uint16_t *h,uint8_t *rotate)
{
	*w = lcdstruct.osd_w;
	*h = lcdstruct.osd_h; 
	*rotate = lcdstruct.osd_scan_mode;
}

#if LCD_FILL_CLK
static struct timer_device *global_timer = NULL;
static uint32 capture_count = 0;

void hs_capture_irq(uint32 irq, uint32 irq_data)
{
	gpio_iomap_output(DOTCLK_RWR,GPIO_IOMAP_OUT_TMR2_PWM_OUT);     
}

void vs_invaild_happen(int32 data, enum gpio_irq_event event)
{
	gpio_iomap_output(DOTCLK_RWR, 56); 
	timer_device_start(global_timer, capture_count-1, hs_capture_irq, 0);		//1000 = capture_count
}
#endif

void lcd_module_run(uint16_t *w,uint16_t *h,uint8_t *rotate){

	get_osd_w_h(w,h,rotate);
	os_msgq_init(&lcd_vef_msg, 1);
	uint8 pixel_dot_num = 1;
	
	struct lcdc_device *lcd_dev;	
	struct spi_device *spi_dev;
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	spi_dev = (struct spi_device * )dev_get(HG_SPI0_DEVID);	

#if LCD_FILL_CLK
	//RGB屏，pwm补dotclk操作(使用timer1、timer2)
	if(lcdstruct.lcd_bus_type == LCD_BUS_RGB)
	{
		uint32_t lcd_fill_clk_en = 1;

		if(lcdstruct.vs_en == 0 || lcdstruct.hs_en == 0)
		{
			os_printf("RGB LCD FILL DOTCLK ERR!!!\n");
			lcd_fill_clk_en = 0;
		}

		if(lcd_fill_clk_en)
		{
			capture_count = lcdstruct.screen_h + lcdstruct.vlw + lcdstruct.vbp;
			os_printf("capture_count:%d\n",capture_count);

			struct pwm_device * global_hgpwm = (struct pwm_device*)dev_get(HG_PWM0_DEVID);						
			pwm_init(global_hgpwm, PWM_CHANNEL_2, 10-1, 5-1);								//24M clk TIMER2
			pwm_start(global_hgpwm,PWM_CHANNEL_2);

			gpio_ioctl(HS_DC, GPIO_CMD_SET_IEEN, 1, 0);
			gpio_iomap_input(HS_DC, GPIO_IOMAP_IN_TMR1_CAP_IN);

			global_timer = (struct timer_device*)dev_get(HG_TIMER1_DEVID);			
			timer_device_open(global_timer, TIMER_TYPE_ONCE, 0);
			if(lcdstruct.hs_inv)
			{
				timer_device_ioctl(global_timer, TIMER_SET_CLK_SRC, 5, 0);					//选择gpio下降沿输入时钟源
			}
			else
			{
				timer_device_ioctl(global_timer, TIMER_SET_CLK_SRC, 4, 0);					//选择gpio上升沿输入时钟源
			}

			
			timer_device_start(global_timer, capture_count-1, hs_capture_irq, 0);

			gpio_ioctl(VS_CS, GPIO_CMD_SET_IEEN, 1, 0);

			if(lcdstruct.vs_inv)
			{
				gpio_request_pin_irq(VS_CS, vs_invaild_happen, 0, GPIO_IRQ_EVENT_FALL);		//VS high valid
			}
			else
			{
				gpio_request_pin_irq(VS_CS, vs_invaild_happen, 0, GPIO_IRQ_EVENT_RISE);		//VS low valid
			}
		}
	}
#endif
	os_mutex_init(&m2m1_mutex);	

#if LCD_FROM_DEC
	lcdc_decode_enable = 1;
#endif
	lcd_dev_gobal = lcd_dev;
	if(PIN_LCD_RESET != 255){
		gpio_iomap_output(PIN_LCD_RESET,GPIO_IOMAP_OUTPUT); 
		gpio_set_val(PIN_LCD_RESET,0);
		os_sleep_ms(100);
		gpio_set_val(PIN_LCD_RESET,1);
		os_sleep_ms(100);
	}

	if(lcdstruct.lcd_bus_type == LCD_BUS_RGB)
	{
		
		uint8 spi_buf[10];
		if(lcdstruct.init_table != NULL){
			spi_open(spi_dev, 1000000, SPI_MASTER_MODE, SPI_WIRE_SINGLE_MODE, SPI_CPOL_1_CPHA_1);
			spi_ioctl(spi_dev,SPI_SET_FRAME_SIZE,9,0);
			//lcd spi_cfg
			_os_printf("read lcd id(%x)\r\n",(unsigned int)spi_dev);
			lcd_register_read_3line(spi_dev,0xda,spi_buf,4);
			_os_printf("***ID:%02x %02x %02x %02x\r\n", spi_buf[0], spi_buf[1], spi_buf[2], spi_buf[3]);

			lcd_table_init(spi_dev,(uint8_t *)lcdstruct.init_table);
			gpio_set_val(PIN_SPI0_CS,1);
			gpio_iomap_output(PIN_SPI0_CS,GPIO_IOMAP_OUTPUT);
		}
	}


	

	
	lcdc_init(lcd_dev);
	video_psram_mem = video_psram_config_mem;
	video_psram_mem1 = video_psram_config_mem1;
#if LCD_THREE_BUF
	video_psram_mem2 = video_psram_config_mem2;
#endif
#if LCD_VIDEO_EF
	video_psram_mem2 = video_psram_config_mem2;
	video_psram_mem3 = video_psram_config_mem3;
#endif
	scale_to_lcd_config();


	
	//mcuÆÁÌáÇ°open,ÐèÒª·¢ËÍÃüÁî
	if(lcdstruct.lcd_bus_type == LCD_BUS_I80)
	{
		lcdc_open(lcd_dev);
	}


	
	lcdc_set_color_mode(lcd_dev,lcdstruct.color_mode);
	lcdc_set_bus_width(lcd_dev,lcdstruct.bus_width);
	lcdc_set_interface(lcd_dev,lcdstruct.lcd_bus_type);
	lcdc_set_colrarray(lcd_dev,lcdstruct.colrarray);
	pixel_dot_num = lcdstruct.color_mode/lcdstruct.bus_width;
	lcdc_set_lcd_vaild_size(lcd_dev,lcdstruct.screen_w+lcdstruct.hlw+lcdstruct.hbp+lcdstruct.hfp,lcdstruct.screen_h+lcdstruct.vlw+lcdstruct.vbp+lcdstruct.vfp,pixel_dot_num);

	if(lcdstruct.lcd_bus_type == LCD_BUS_I80 && lcdstruct.color_mode == LCD_MODE_666)
	{
		lcdc_set_lcd_visible_size(lcd_dev,lcdstruct.screen_w,lcdstruct.screen_h,3);
	}
	else if(lcdstruct.lcd_bus_type == LCD_BUS_I80 && lcdstruct.color_mode == LCD_MODE_565)
	{
		lcdc_set_lcd_visible_size(lcd_dev,lcdstruct.screen_w,lcdstruct.screen_h,pixel_dot_num);
	}
	//rgb走这一路
	else
	{
		lcdc_set_lcd_visible_size(lcd_dev,lcdstruct.screen_w,lcdstruct.screen_h,pixel_dot_num);
	}
//	
	
	if(lcdstruct.lcd_bus_type == LCD_BUS_I80)
	{
		lcdc_mcu_signal_config(lcd_dev,lcdstruct.signal_config.value);
	}
	else if(lcdstruct.lcd_bus_type == LCD_BUS_RGB)
	{
		lcdc_signal_config(lcd_dev,lcdstruct.vs_en,lcdstruct.hs_en,lcdstruct.de_en,lcdstruct.vs_inv,lcdstruct.hs_inv,lcdstruct.de_inv,lcdstruct.pclk_inv);
		lcdc_set_invalid_line(lcd_dev,lcdstruct.vlw);
		lcdc_set_valid_dot(lcd_dev,lcdstruct.hlw+lcdstruct.hbp,lcdstruct.vlw+lcdstruct.vbp);
		lcdc_set_hlw_vlw(lcd_dev,lcdstruct.hlw,0);
	}


	

	lcdc_set_baudrate(lcd_dev,lcdstruct.pclk);
	lcdc_set_bigendian(lcd_dev,1);

	if(lcdstruct.lcd_bus_type == LCD_BUS_I80)
	{
		lcd_table_init_MCU(lcd_dev,lcdstruct.init_table);
	}
	

#if VIDEO_EN
#if SCALE_DIRECT_TO_LCD
	lcdc_set_video_size(lcd_dev,lcdstruct.video_w,lcdstruct.video_h);
	lcdc_set_video_data_from(lcd_dev,VIDEO_FROM_SCALE);
#else
	lcdc_set_video_size(lcd_dev,lcdstruct.video_w,lcdstruct.video_h);
#if LCD_SET_ROTATE_180	
	lcdc_set_rotate_mirror(lcd_dev,0,LCD_ROTATE_180);
	set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,1);
#else
	lcdc_set_rotate_mirror(lcd_dev,0,lcdstruct.scan_mode);
	set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);	
#endif

	lcdc_set_rotate_p0_up(lcd_dev,0);
	lcdc_set_rotate_p0p1_start_location(lcd_dev,0,0,0,0);
	lcdc_set_rotate_linebuf_num(lcd_dev,LCD_ROTATE_LINE);
	lcdc_set_rotate_linebuf_y_addr(lcd_dev,(uint32)lcd_line_buf);
#if LCD_HX8282_EN	
	lcdc_set_rotate_linebuf_u_addr(lcd_dev,(uint32)lcd_line_buf+SCALE_WIDTH*LCD_ROTATE_LINE);
	lcdc_set_rotate_linebuf_v_addr(lcd_dev,(uint32)lcd_line_buf+SCALE_WIDTH*LCD_ROTATE_LINE+(SCALE_WIDTH/2)*LCD_ROTATE_LINE);
#else
	lcdc_set_rotate_linebuf_u_addr(lcd_dev,(uint32)lcd_line_buf+SCALE_HIGH*LCD_ROTATE_LINE);
	lcdc_set_rotate_linebuf_v_addr(lcd_dev,(uint32)lcd_line_buf+SCALE_HIGH*LCD_ROTATE_LINE+(SCALE_HIGH/2)*LCD_ROTATE_LINE);
#endif
	// video_decode_mem  = video_decode_config_mem;
	// video_decode_mem1 = video_decode_config_mem1;
	// video_decode_mem2 = video_decode_config_mem2;
	// video_decode_mem3 = video_decode_config_mem3;

	lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem+Y_OFFSET);
	lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
	lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);

	if(lcdc_decode_enable){
		lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
	}	

#if LCD_FROM_DEC		
	lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,p1_w,p1_h);
	jpg_decode_scale_config((uint32)video_decode_mem);	
#else
	lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,SCALE_WIDTH,SCALE_HIGH);
#endif	
	lcdc_set_video_data_from(lcd_dev,VIDEO_FROM_MEMORY_ROTATE);
#endif

	lcdc_set_video_start_location(lcd_dev,lcdstruct.video_x,lcdstruct.video_y);
	
	lcdc_set_p0p1_enable(lcd_dev,0,lcdc_decode_enable);
	if(lcdc_decode_enable){
		lcd_info.lcd_p0p1_state = 2;
		lcd_info.lcd_p0p1_cur   = 2;
	}
	else{
		lcd_info.lcd_p0p1_state = 0;
		lcd_info.lcd_p0p1_cur   = 0;
	}
	
	lcdc_set_video_en(lcd_dev,0);
#endif


#if OSD_EN
	lcd_info.lcd_osd_mode       = 0;
	lcd_info.lcd_osd_cur_mode	= 0;

	lcdc_set_osd_start_location(lcd_dev,lcdstruct.osd_x,lcdstruct.osd_y);
#if 1	
	lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);
	lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd565_encode1);
	lcdc_set_osd_dma_len(lcd_dev,sizeof(osd565_encode1)+1024);
	lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
#else	
	lcdc_set_osd_size(lcd_dev,24,80);	
	lcdc_set_osd_lut_addr(lcd_dev,osd_rgb256);
	lcdc_set_osd_dma_addr(lcd_dev,osd_rgb256+512);	
	lcdc_set_osd_format(lcd_dev,OSD_RGB_256);
#endif	
	lcdc_set_osd_alpha(lcd_dev,0x100);
	lcdc_set_osd_enc_head(lcd_dev,0xFFFFFF,0xFFFBFF);
	lcdc_set_osd_enc_diap(lcd_dev,0x000000,0x000000);
	lcdc_set_osd_en(lcd_dev,1);
#endif
	lcdc_video_enable_auto_ks(lcd_dev,0);
	lcdc_set_timeout_info(lcd_dev,1,3);
#if LCD_THREE_BUF
    lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_done,(uint32)lcd_dev);	
#elif LCD_VIDEO_EF
	lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_squralbuf_done,(uint32)lcd_dev);	
#else 
	lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_doublebuf_done,(uint32)lcd_dev);	
#endif
	lcdc_request_irq(lcd_dev,OSD_EN_IRQ,  (lcdc_irq_hdl )&lcd_osd_isr,(uint32)lcd_dev);
	lcdc_request_irq(lcd_dev,TIMEOUT_IRQ,(lcdc_irq_hdl )&lcd_timeout,(uint32)lcd_dev);
#if (LCD_TE != 255)
	lcdc_set_te_edge(lcd_dev,1);
	lcdc_request_irq(lcd_dev,LCD_TE_IRQ,(lcdc_irq_hdl )&lcd_te_isr,(uint32)lcd_dev);
#endif
	lcdc_open(lcd_dev);
	
	//注意,这里是针对学习板去修改,不同硬件需要重新实现这整个函数
	if(lcdstruct.lcd_bus_type == LCD_BUS_I80 && lcdstruct.color_mode == LCD_MODE_666)
	{
		lcdc_video_enable_gamma(lcd_dev, 1);
		lcdc_video_enable_ccm(lcd_dev, 1);
		lcdc_video_enable_constarast(lcd_dev, 1);
		lcdc_video_set_CCM_COEF0(lcd_dev, 0x00df0100);  
		lcdc_video_set_CCM_COEF1(lcd_dev, 0x0003b3f3);    
		lcdc_video_set_CCM_COEF2(lcd_dev, 0x0df0000d);        
		lcdc_video_set_CCM_COEF3(lcd_dev, 0x00000000); 
		lcdc_video_set_constarast_val(lcd_dev, 0x12);
		lcdc_video_set_gamma_R(lcd_dev, (uint32_t)rgb_gamma_table);
		lcdc_video_set_gamma_G(lcd_dev, (uint32_t)rgb_gamma_table);
		lcdc_video_set_gamma_B(lcd_dev, (uint32_t)rgb_gamma_table);
		lcdc_set_bus_width(lcd_dev,LCD_BUS_WIDTH_6);
		gpio_iomap_output(LCD_D0, GPIO_IOMAP_OUTPUT);
		gpio_iomap_output(LCD_D1, GPIO_IOMAP_OUTPUT);
		gpio_iomap_inout(LCD_D2, GPIO_IOMAP_IN_LCD_D0_IN_MASK1_19,2);
		gpio_iomap_inout(LCD_D3, GPIO_IOMAP_IN_LCD_D1_IN_MASK1_20,8);
		gpio_iomap_inout(LCD_D4, GPIO_IOMAP_IN_LCD_D2_IN_MASK1_21,9);
		gpio_iomap_inout(LCD_D5, GPIO_IOMAP_IN_STMR0_CAP_IN__LCD_D3_IN_MASK1_22,10);
		gpio_iomap_inout(LCD_D6, GPIO_IOMAP_IN_STMR1_CAP_IN__LCD_D4_IN_MASK1_23,11);
		gpio_iomap_inout(LCD_D7, GPIO_IOMAP_IN_STMR2_CAP_IN__LCD_D5_IN_MASK1_24,12);
			
	}
	lcdc_set_start_run(lcd_dev);
   
}



#endif
