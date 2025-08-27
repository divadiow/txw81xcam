/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "lv_port_disp.h"
#include <stdbool.h>
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "lib/lcd/lcd.h"
#include "stream_frame.h"
#include "app_lcd/lvgl_osd_stream.h"
#include "osal/string.h"
#define LV_PORT_DISP_LINE   (32)
extern struct os_mutex m2m1_mutex;
/*********************
 *      DEFINES
 *********************/
#if 0
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    240
#endif
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
static void disp_flush_rotate(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
void lv_wait_cb(struct _lv_disp_drv_t * disp_drv)
{
    os_sleep_ms(1);
}
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
///uint8_t osd_menu565_buf[SCALE_HIGH*SCALE_WIDTH*2] __attribute__ ((aligned(4),section(".psram.src")));;
uint8_t *osd_menu565_buf;

//static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES]__attribute__ ((aligned(4),section(".psram.src")));;            /*A screen sized buffer*/
//static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES]__attribute__ ((aligned(4),section(".psram.src")));;            /*Another screen sized buffer*/
//	  static lv_color_t *buf_3_1; 		   /*A screen sized buffer*/
//	  static lv_color_t *buf_3_2; 		   /*Another screen sized buffer*/

void lv_port_disp_init(void *stream,uint16_t w,uint16_t h,uint8_t rotate)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();
    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/
    
    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
    static lv_disp_draw_buf_t draw_buf_dsc_1;


    /* Example for 2) */
//    static lv_disp_draw_buf_t draw_buf_dsc_2;
//    static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];                        /*A buffer for 10 rows*/
//    static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];                        /*An other buffer for 10 rows*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 3) also set disp_drv.full_refresh = 1 below*/
//    static lv_disp_draw_buf_t draw_buf_dsc_3;
//    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
//    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
//	  static lv_color_t *buf_3_1; 		   /*A screen sized buffer*/
//	  static lv_color_t *buf_3_2; 		   /*Another screen sized buffer*/

//	buf_3_1 = (uint8*)malloc_psram(MY_DISP_HOR_RES * MY_DISP_VER_RES*2);
//	buf_3_2 = (uint8*)malloc_psram(MY_DISP_HOR_RES * MY_DISP_VER_RES*2);
//  lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
//                          MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_color_t *buf_1;
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/
    //重新配置一下是否旋转,由屏参数配置
    if(rotate)
    {
        disp_drv.sw_rotate   = 1;
    }
    else
    {
        disp_drv.sw_rotate   = 0;
    }
    disp_drv.rotated           = rotate;

    //如果旋转,就使用单buf
    if(disp_drv.sw_rotate)
    {
        buf_1 = (lv_color_t*)lv_malloc(w*LV_PORT_DISP_LINE*sizeof(lv_color_t));
        lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, w * LV_PORT_DISP_LINE);   /*Initialize the display buffer*/
        //旋转,需要中间层
        osd_menu565_buf = (uint8_t *)lv_malloc(w*h*2);
        disp_drv.flush_cb = disp_flush_rotate;
    }
    else
    {
        //非旋转,直接绘制就好了
        disp_drv.direct_mode = 1;
        buf_1 = (lv_color_t*)lv_malloc(w*h*sizeof(lv_color_t));
        osd_menu565_buf = (uint8_t *)buf_1;
        lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, w * h);   /*Initialize the display buffer*/
        disp_drv.flush_cb = disp_flush;
    }

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = w;
    disp_drv.ver_res = h;
    disp_drv.user_data = stream;

    disp_drv.disp_buf_len = w*h*sizeof(lv_color_t);

    /*Used to copy the buffer's content to the display*/
    
    disp_drv.wait_cb = lv_wait_cb;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

typedef struct
{
    __IO uint32 DMA_CON;
    __IO uint32 DMA_SAIE;
    __IO uint32 DMA_DATA;
    __IO uint32 DMA_SADR;
    __IO uint32 DMA_TADR;
    __IO uint32 DMA_DLEN;
    __IO uint32 DMA_ISIZE;
} M2M_DMA_TypeDef;
#include "hal/dma.h"
extern struct dma_device *m2mdma;
void hw_blkcpy(uint32_t src,uint32_t src_w,uint32_t dest,uint32_t dest_w,uint32_t cpy_w,uint32_t cpy_h){

	M2M_DMA_TypeDef *p_dma;
	p_dma = ((M2M_DMA_TypeDef    *) 0x4002121c);
	p_dma->DMA_CON = 2<<1;
	p_dma->DMA_SADR = src;
	p_dma->DMA_TADR = dest;
	p_dma->DMA_DLEN = ((cpy_h-1)<<20)|(cpy_w-1);
	p_dma->DMA_ISIZE= (dest_w<<16)|(src_w<<0);	
	p_dma->DMA_CON |= BIT(0);
	while(p_dma->DMA_CON & BIT(0));
	
}


/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
uint8_t disp_updata = 0;
//不需要旋转
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{

	lv_color_t *p_16;
	p_16 = (lv_color_t*)osd_menu565_buf;
#if LVGL_STREAM_ENABLE == 1
    if(disp_drv->draw_buf->flushing_last)
    {
            stream *s = (stream*)disp_drv->user_data;
            struct data_structure *data_s;
            int count = 0;
            disp_flush_again:
			data_s = get_src_data_f(s);
			if(data_s)
			{
                struct encode_data_s_callback *callback = (struct encode_data_s_callback*)data_s->priv;
                callback->user_data = (void*)disp_drv;
                callback->finish_cb = (osd_finish_cb)lv_disp_flush_ready;

				data_s->data = (void*)p_16;
                //回写空间
                sys_dcache_clean_range(data_s->data, disp_drv->disp_buf_len); 
				set_stream_real_data_len(data_s,disp_drv->disp_buf_len);
				send_data_to_stream(data_s);
			}
            //如果其他地方处理慢,要考虑丢帧了
            else
            {
                if(++count%1000 == 0)
                {
                    os_printf("%s drop frame\n",__FUNCTION__);
                }
                os_sleep_ms(1);
                goto disp_flush_again;
            }
    }
    else
    {
        lv_disp_flush_ready(disp_drv);
    }
#else
    if(disp_drv->draw_buf->flushing_last)
    {
        sys_dcache_clean_range((uint32_t*)p_16, disp_drv->disp_buf_len);
        disp_updata = 1;
    }
    lv_disp_flush_ready(disp_drv);
#endif

}
//旋转部分
static void disp_flush_rotate(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{

	lv_color_t *p_16;
	p_16 = (lv_color_t*)osd_menu565_buf;

    if(area->x1 == 0 && area->x2 == disp_drv->hor_res-1)
    {
        hw_memcpy(p_16+area->y1*disp_drv->hor_res,color_p,disp_drv->hor_res*(area->y2-area->y1+1) *sizeof(lv_color_t));
    }
    else
    {
        uint32_t y;
        for(y = area->y1; y <= area->y2; y++) 
        {
            hw_memcpy(p_16+y*disp_drv->hor_res+area->x1,color_p,(area->x2-area->x1+1)*sizeof(lv_color_t));
            color_p += (area->x2-area->x1+1);
        }
    }


#if LVGL_STREAM_ENABLE == 1
    if(disp_drv->draw_buf->flushing_last)
    {
            stream *s = (stream*)disp_drv->user_data;
            struct data_structure *data_s;
            int count = 0;
            disp_flush_again:
			data_s = get_src_data_f(s);
			if(data_s)
			{
                struct encode_data_s_callback *callback = (struct encode_data_s_callback*)data_s->priv;
                callback->user_data = (void*)disp_drv;
                callback->finish_cb = (osd_finish_cb)lv_disp_flush_ready;

				data_s->data = (void*)p_16;
                //回写空间
                sys_dcache_clean_range(data_s->data, disp_drv->disp_buf_len); 
				set_stream_real_data_len(data_s,disp_drv->disp_buf_len);
				send_data_to_stream(data_s);
			}
            //如果其他地方处理慢,要考虑丢帧了
            else
            {
                if(++count%1000 == 0)
                {
                    os_printf("%s drop frame\n",__FUNCTION__);
                }
                os_sleep_ms(1);
                goto disp_flush_again;
            }
    }
    else
    {
        lv_disp_flush_ready(disp_drv);
    }
#else
    if(disp_drv->draw_buf->flushing_last)
    {
        sys_dcache_clean_range((uint32_t*)p_16, disp_drv->disp_buf_len);
        disp_updata = 1;
    }
    
    lv_disp_flush_ready(disp_drv);
#endif

}




/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}


#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
