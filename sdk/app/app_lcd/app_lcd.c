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
#include "stream_frame.h"
#include "decode/recv_jpg_yuv_demo.h"
#include "decode/jpg_decode_stream.h"
#include "app_lcd.h"
#include "app_lcd/osd_encode_stream.h"
#ifdef LCD_EN


#define LCD_ROTATE_LINE      32

#define OSD_EN      1
struct app_lcd_s lcd_msg_s;

void lcd_hardware_init565();
void lcd_hardware_init666();
extern void lcd_register_read_3line(struct spi_device *spi_dev,uint32 code, uint8* buf, uint32 len);
extern void lcd_table_init(struct spi_device *spi_dev,uint8_t *lcd_table);
extern void lcd_table_init_MCU(struct lcdc_device *lcd_dev,uint8_t (*lcd_table)[2]);

static void lcd_osd_isr1(uint32 irq_flag,uint32 irq_data,uint32 param1)
{

    //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	//struct lcdc_device *p_lcd = (struct lcdc_device *)irq_data;
    struct app_lcd_s *lcd_s= (struct app_lcd_s *)irq_data;
	struct lcdc_device *lcd_dev;
	stream *s = (stream*)lcd_s->encode_osd_s;
	lcd_dev = lcd_s->lcd_dev;//stream_self_cmd_func(s,0x05,0);
    //os_printf("lcd_dev:%X\n",lcd_dev);
	int32_t val = lcdc_get_con(lcd_dev);
	//硬件没有准备好
	if(val & BIT(3))
	{
		return;
	}
	stream_self_cmd_func(s,OSD_HARDWARE_ENCODE_SET_LEN,lcdc_get_osd_enc_dst_len(lcd_dev));
	stream_self_cmd_func(s,OSD_HARDWARE_REAY_CMD,1);
	//lcd_sema_up();	
}

//暂时没有作处理
static void lcd_timeout1(uint32 irq_flag,uint32 irq_data,uint32 param1){
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    struct app_lcd_s *lcd_s= (struct app_lcd_s *)irq_data;
	struct lcdc_device *lcd_dev = lcd_s->lcd_dev;
	//lcdc_set_timeout_info(lcd_dev,0,3);
	//gpio_set_val(PC_7,pc_7);
	//pc_7 ^= BIT(0);	
	
	lcdc_close(lcd_dev);
	os_printf("........................................................................................................................lcd_timeout\r\n");
	//出现timeout,说明可能哪里卡住了,让应用层重新启动
	lcdc_open(lcd_dev);
	//lcd_s->hardware_ready = 1;
	//lcdc_open(lcd_dev);
	lcdc_set_start_run(lcd_dev);
}

//暂时没有作处理
static void lcd_te_isr1(uint32 irq_flag,uint32 irq_data,uint32 param1){
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    struct app_lcd_s *lcd_s= (struct app_lcd_s *)irq_data;
	struct lcdc_device *lcd_dev;	
	lcd_dev = lcd_s->lcd_dev;
    os_printf("--------------------------------------------te---------------------------------\r\n");
}

static void lcd_squralbuf_done1(uint32 irq_flag,uint32 irq_data,uint32 param1)
{
	struct app_lcd_s *lcd_s= (struct app_lcd_s *)irq_data;
    //理论这里只是唤醒信号量之类,由线程来决定是否继续显示
    //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	if(lcd_s->hardware_auto_ks)
	{
		lcd_s->hardware_ready = 0;
		//判断是否启动了硬件自动kick
		if(!lcd_s->get_auto_ks)
		{
			//启动自动kick
			lcd_s->get_auto_ks = 1;
			lcdc_video_enable_auto_ks(lcd_s->lcd_dev, 1);
		}
	}
	else
	{
		if(lcd_s->get_auto_ks)
		{
			lcd_s->get_auto_ks = 0;
			lcdc_video_enable_auto_ks(lcd_s->lcd_dev, 0);
		}
		else
		{
			lcd_s->hardware_ready = 1;
		}
	}
}


uint8_t g_read_hardware_auto_ks()
{
	// os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return lcd_msg_s.get_auto_ks;
}

uint8_t g_set_hardware_auto_ks(uint8_t en) 
{
	uint32_t flags;
	uint32_t timeout = 0;
	if(en) 
	{
		lcd_msg_s.hardware_auto_ks = en;
		while(!lcd_msg_s.get_auto_ks && (timeout < 100)) 
		{
			timeout++;
			os_sleep_ms(1);
		}
	} 
	else 
	{
		flags = disable_irq();
		lcd_msg_s.hardware_auto_ks = en;
		enable_irq(flags);
	}

	return lcd_msg_s.get_auto_ks;
}


//返回的是osd旋转和w h
static void get_osd_w_h(uint16_t *w,uint16_t *h,uint8_t *rotate)
{
	*w = lcdstruct.osd_w;
	*h = lcdstruct.osd_h; 
	*rotate = lcdstruct.osd_scan_mode;
}

//返回屏幕的size和旋转角度
static void get_screen_w_h(uint16_t *screen_w,uint16_t *screen_h,uint8_t *video_rotate)
{
	*screen_w = lcdstruct.screen_w;
	*screen_h = lcdstruct.screen_h; 
	*video_rotate = lcdstruct.scan_mode;
}

//注意,这部分不同屏以及硬件设计不一样,需要修改
void lcd_hardware_init(uint16_t *w,uint16_t *h,uint8_t *rotate,uint16_t *screen_w,uint16_t *screen_h,uint8_t *video_rotate)
{
	if(lcdstruct.lcd_bus_type == LCD_BUS_I80)
	{
		if(lcdstruct.color_mode == LCD_MODE_666)
		{
			lcd_hardware_init666();
		}
		else if(lcdstruct.color_mode == LCD_MODE_565)
		{
			lcd_hardware_init565();
		}
		else
		{
			lcd_hardware_init565();
		}
	}
	else if(lcdstruct.lcd_bus_type == LCD_BUS_RGB)
	{
		lcd_hardware_init565();
	}

	//返回osd的w和h
	get_osd_w_h(w,h,rotate);
	get_screen_w_h(screen_w,screen_h,video_rotate);
}

//主要硬件初始化,但由于存在rgb屏,还是会有部分寄存器需要配置
void lcd_hardware_init565()
{
    uint8 pixel_dot_num = 1;
	struct lcdc_device *lcd_dev;	
	struct spi_device *spi_dev;
    
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	spi_dev = (struct spi_device * )dev_get(HG_SPI0_DEVID);	
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

    //scale_to_lcd_config();
    lcdc_init(lcd_dev);
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
	lcdc_set_lcd_visible_size(lcd_dev,lcdstruct.screen_w,lcdstruct.screen_h,pixel_dot_num);
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
}



void lcd_hardware_init666()
{
    uint8 pixel_dot_num = 1;
	struct lcdc_device *lcd_dev;	
	struct spi_device *spi_dev;
    
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	spi_dev = (struct spi_device * )dev_get(HG_SPI0_DEVID);	
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

    //scale_to_lcd_config();
    lcdc_init(lcd_dev);
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
	lcdc_set_lcd_visible_size(lcd_dev,lcdstruct.screen_w,lcdstruct.screen_h,3);
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

//gamma表之类没有配置,这个考虑是放在其他地方,这个函数专注硬件的初始化好了

#if 0 
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
#endif

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

//lcd相关参数配置,w、h、rotate
void lcd_arg_setting(uint16_t w,uint16_t h,uint8_t rotate,uint16_t screen_w,uint16_t screen_h,uint8_t video_rotate)
{
	struct app_lcd_s *lcd_s = &lcd_msg_s;
	lcd_s->w = w;
	lcd_s->h = h;
	lcd_s->rotate = rotate;
	lcd_s->screen_w = screen_w;
	lcd_s->screen_h = screen_h;
	lcd_s->video_rotate = video_rotate;
}

//芯片lcd的驱动初始化
void lcd_driver_init(void *s1,void *s2,void *s3,void *s4)
{
    struct app_lcd_s *lcd_s = &lcd_msg_s;
    lcd_s->hardware_ready 	= 1;
	lcd_s->thread_exit 		= 0;
	lcd_s->hardware_auto_ks = 0;
	lcd_s->get_auto_ks 		= 0;
	lcd_s->encode_osd_s = s1;
	lcd_s->osd_show_s = s2;
	lcd_s->video_p1_s = s3;
	lcd_s->video_p0_s = s4;


	struct lcdc_device *lcd_dev;	
    
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	

    lcd_s->lcd_dev = lcd_dev;


	lcdc_set_video_size(lcd_dev,lcdstruct.video_w,lcdstruct.video_h);


	lcdc_set_rotate_p0_up(lcd_dev,0);
	lcdc_set_rotate_p0p1_start_location(lcd_dev,0,0,0,0);


	lcdc_set_rotate_linebuf_num(lcd_dev,LCD_ROTATE_LINE);
	lcd_s->line_buf_num = LCD_ROTATE_LINE;
	lcdc_set_video_data_from(lcd_dev,VIDEO_FROM_MEMORY_ROTATE);


	lcdc_set_video_start_location(lcd_dev,lcdstruct.video_x,lcdstruct.video_y);
	lcdc_set_video_en(lcd_dev,0);


    //设置默认值,因为这个一定要配置,并且地址与p1的地址范围要一样(就是全部是psram或者全部是sram,所以这里最好都是设置psram)
    lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)0x38000000);
    lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)0x38000000);
    lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)0x38000000);

	lcdc_set_osd_start_location(lcd_dev,lcdstruct.osd_x,lcdstruct.osd_y);
	lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);
	lcdc_set_osd_format(lcd_dev,OSD_RGB_565);

	lcdc_set_osd_alpha(lcd_dev,0x100);
	lcdc_set_osd_enc_head(lcd_dev,0xFFFFFF,0xFFFBFF);
	lcdc_set_osd_enc_diap(lcd_dev,0x000000,0x000000);
	lcdc_set_osd_en(lcd_dev,0);

	lcdc_video_enable_auto_ks(lcd_dev,0);
	lcdc_set_timeout_info(lcd_dev,1,3);

	lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_squralbuf_done1,(uint32)lcd_s);	

	lcdc_request_irq(lcd_dev,OSD_EN_IRQ,  (lcdc_irq_hdl )&lcd_osd_isr1,(uint32)lcd_s);
	lcdc_request_irq(lcd_dev,TIMEOUT_IRQ,(lcdc_irq_hdl )&lcd_timeout1,(uint32)lcd_s);
#if (LCD_TE != 255)
	lcdc_set_te_edge(lcd_dev,1);
	lcdc_request_irq(lcd_dev,LCD_TE_IRQ,(lcdc_irq_hdl )&lcd_te_isr1,(uint32)lcd_s);
#endif
	lcdc_open(lcd_dev);

    extern void lcd_thread(void *d);
    lcd_s->thread_hdl = os_task_create("lcd_thread", lcd_thread, lcd_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
}


void wait_lcd_exit()
{
	struct app_lcd_s *lcd_s = &lcd_msg_s;
	lcd_s->thread_exit = 1;
	while(lcd_s->thread_hdl)
	{
		os_sleep_ms(1);
	}
}

//重复初始化,一般用于意外断电或者说休眠起来后使用
void lcd_driver_reinit()
{
    struct app_lcd_s *lcd_s = &lcd_msg_s;
    lcd_s->hardware_ready 	= 1;
	lcd_s->thread_exit 		= 0;
	lcd_s->hardware_auto_ks = 0;
	lcd_s->get_auto_ks 		= 0;
	struct lcdc_device *lcd_dev;	
    
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	

    lcd_s->lcd_dev = lcd_dev;

	lcdc_set_video_size(lcd_dev,lcdstruct.video_w,lcdstruct.video_h);

	//VIDEO
	lcdc_set_rotate_p0_up(lcd_dev,0);
	lcdc_set_rotate_p0p1_start_location(lcd_dev,0,0,0,0);
	//line_buf后续再分配
	lcdc_set_rotate_linebuf_num(lcd_dev,LCD_ROTATE_LINE);
	lcdc_set_video_data_from(lcd_dev,VIDEO_FROM_MEMORY_ROTATE);
	lcdc_set_video_start_location(lcd_dev,lcdstruct.video_x,lcdstruct.video_y);
	lcdc_set_video_en(lcd_dev,0);


    //设置默认值,因为这个一定要配置,并且地址与p1的地址范围要一样(就是全部是psram或者全部是sram,所以这里最好都是设置psram)
    lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)0x38000000);
    lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)0x38000000);
    lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)0x38000000);




	//OSD
	lcdc_set_osd_start_location(lcd_dev,lcdstruct.osd_x,lcdstruct.osd_y);
	lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);
	lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
	lcdc_set_osd_alpha(lcd_dev,0x100);
	lcdc_set_osd_enc_head(lcd_dev,0xFFFFFF,0xFFFBFF);
	lcdc_set_osd_enc_diap(lcd_dev,0x000000,0x000000);
	lcdc_set_osd_en(lcd_dev,0);


	
	lcdc_video_enable_auto_ks(lcd_dev,0);
	lcdc_set_timeout_info(lcd_dev,1,3);

	lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_squralbuf_done1,(uint32)lcd_s);	

	lcdc_request_irq(lcd_dev,OSD_EN_IRQ,  (lcdc_irq_hdl )&lcd_osd_isr1,(uint32)lcd_s);
	lcdc_request_irq(lcd_dev,TIMEOUT_IRQ,(lcdc_irq_hdl )&lcd_timeout1,(uint32)lcd_s);
#if (LCD_TE != 255)
	lcdc_set_te_edge(lcd_dev,1);
	lcdc_request_irq(lcd_dev,LCD_TE_IRQ,(lcdc_irq_hdl )&lcd_te_isr1,(uint32)lcd_s);
#endif
	lcdc_open(lcd_dev);
	//lcdc_set_start_run(lcd_dev);


    extern void lcd_thread(void *d);
    lcd_s->thread_hdl = os_task_create("lcd_thread", lcd_thread, (void*)lcd_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
}

#endif