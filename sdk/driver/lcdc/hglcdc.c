#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "devid.h"
#include "dev/lcdc/hglcdc.h"
#include "osal/string.h"

#define LCD_SET_CON(con,X) 		{if(!(con&BIT(X))){con |= BIT(X);}}
#define LCD_CLEAR_CON(con,X) 	{if(con&BIT(X)){con &= ~BIT(X);}}
struct hglcdc_hw
{
    __IO uint32 LCDCON;   		//0x00
    __IO uint32 LCD_IFCON;         //0x04
    __IO uint32 LCD_BAUD;         	//0x08
    __IO uint32 LCD_CMD;         	//0x0C
    __IO uint32 LCD_DATA;         	//0x10
    __IO uint32 LCD_LEN;       	//0x14
    __IO uint32 LCD_IFSTA;       	//0x18
    __IO uint32 LCD_VSYNCON;    	//0x1C
    __IO uint32 LCD_HSYNCON;      	//0x20
    __IO uint32 LCD_WINCON;    	//0x24
    __IO uint32 LCD_VISIBLE_WIN;   //0x28
    __IO uint32 LCD_VISIBLE_START; //0x2C		
    __IO uint32 ALPHA; 		//0x30	
    __IO uint32 PICTURE_SIZE;      //0x34
    __IO uint32 OSD0_DMA_STADR; 	//0x38	
    __IO uint32 OSD0_LUTDMA_STADR; //0x3C	
    __IO uint32 OSD0_WORD_LEN; 	//0x40	
    __IO uint32 OSD0_WINCON; 	//0x44	
    __IO uint32 OSD0_START; 	//0x48		
    __IO uint32 VIDEO_YDMA_STADR; 	//0x4C	
    __IO uint32 VIDEO_UDMA_STADR; 	//0x50	
    __IO uint32 VIDEO_VDMA_STADR; 	//0x54
    __IO uint32 VIDEO_WINCON; 	//0x58	
    __IO uint32 VIDEO_START; 	//0x5C
    __IO uint32 ROTATE_PICTURE0_YDMA_STADR; //0x60
    __IO uint32 ROTATE_PICTURE0_UDMA_STADR; //0x64
    __IO uint32 ROTATE_PICTURE0_VDMA_STADR; //0x68
    __IO uint32 ROTATE_PICTURE1_YDMA_STADR; //0x6C
    __IO uint32 ROTATE_PICTURE1_UDMA_STADR; //0x70
    __IO uint32 ROTATE_PICTURE1_VDMA_STADR; //0x74
	__IO uint32 ROTATE_PICTURE0_START; //0x78
	__IO uint32 ROTATE_PICTURE0_WINCON; //0x7C
	__IO uint32 ROTATE_PICTURE1_START; //0x80
	__IO uint32 ROTATE_PICTURE1_WINCON; //0x84
    __IO uint32 ROTATE_OUTBUF_LINE_NUM; //0x88
    __IO uint32 CCM_COEF0; 		//0x8C
    __IO uint32 CCM_COEF1; 		//0x90
    __IO uint32 CCM_COEF2; 		//0x94
    __IO uint32 CCM_COEF3; 		//0x98
    __IO uint32 GAMMA_R_STADR; 	//0x9C 
    __IO uint32 GAMMA_G_STADR; 	//0xA0 
    __IO uint32 GAMMA_B_STADR; 	//0xA4 
    __IO uint32 OSD_ENC_SADR; 		//0xA8
    __IO uint32 OSD_ENC_TADR; 		//0xAC 
    __IO uint32 OSD_ENC_RLEN; 		//0xB0 
    __IO uint32 OSD_ENC_DLEN; 		//0xB4
    __IO uint32 OSD_ENC_IDENT0; 	//0xB8
    __IO uint32 OSD_ENC_IDENT1; 	//0xBC 
    __IO uint32 OSD_ENC_TRANS0; 	//0xC0
    __IO uint32 OSD_ENC_TRANS1; 	//0xC4 
    __IO uint32 TIMEOUTCON; 	//0xC8 
	
};


lcdc_irq_hdl lcdcirq_vector_table[LCD_IRQ_NUM];
volatile uint32  lcdcirq_dev_table[LCD_IRQ_NUM];

struct hglcdc_hw *lcd_glo;

void irq_lcdc_enable(struct lcdc_device *p_lcdc,uint8 mode,uint8 irq){
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc; 
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	if(mode){
		hw->LCD_IFCON |= BIT(irq+28);
	}else{
		hw->LCD_IFCON &= ~BIT(irq+28);
	}
}

void LCDC_IRQHandler_action(void *p_lcdc)
{
	uint32 sta = 0;
	uint8 loop;
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc; 
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	sta = hw->LCD_IFSTA;
	for(loop = 0;loop < LCD_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->LCD_IFSTA = BIT(loop);
			if(lcdcirq_vector_table[loop] != NULL){
				lcdcirq_vector_table[loop] (loop,lcdcirq_dev_table[loop],0);
			}

		}
	}
}



int32 lcdcirq_register(struct lcdc_device *p_lcdc,uint32 irq, lcdc_irq_hdl isr, uint32 dev_id){
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc; 	
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	request_irq(lcdc_hw->irq_num, LCDC_IRQHandler_action, p_lcdc);
	
	irq_lcdc_enable(p_lcdc, 1, irq);
	lcdcirq_vector_table[irq] = isr;
	lcdcirq_dev_table[irq] = dev_id;
	hw->LCD_IFSTA |= BIT(irq);
	os_printf("lcdcirq1_register:%d %x  %x\r\n",irq,(uint32)lcdcirq_vector_table[irq],(uint32)isr);
	return 0;
}


int32 lcdcirq_unregister(struct lcdc_device *p_lcdc,uint32 irq){
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc;	
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	irq_lcdc_enable(p_lcdc, 0, irq);
	lcdcirq_vector_table[irq] = NULL;
	lcdcirq_dev_table[irq] = 0;
	hw->LCD_IFSTA |= BIT(irq);
	return 0;
}

static int32 hglcdc_ioctl(struct lcdc_device *p_lcdc, enum lcdc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
	int32  ret_val = RET_OK;
	uint32_t flags;
	uint32 h,w;
	uint32 x,y;
	uint8 pixel_dot_num = 0;
	uint8 vs_en,hs_en,de_en,vs_inv,hs_inv,de_inv,clk_inv;
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc;	
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	switch(ioctl_cmd){
		case LCDC_SET_TIMEOUT_INF:
			if(param1){
				if(param2 < 4){
					hw->TIMEOUTCON &= ~(0x3<<1);
					hw->TIMEOUTCON |= (param2<<1);
				}
				hw->TIMEOUTCON |= BIT(0);
			}else{
				hw->TIMEOUTCON &= ~BIT(0);
			}
		break;
	
		case LCDC_SET_AUTO_KICK_EN:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,28);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,28);
			}
			enable_irq(flags);			
		break;
	
		case LCDC_SET_GAMMA_SATURATION_EN:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,27);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,27);
			}

			if(param2){
				LCD_SET_CON(hw->LCDCON,21);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,21);
			}
			enable_irq(flags);
		break;

		case LCDC_SET_GAMMA_SATURATION_VAL:
			flags = disable_irq();
			if(param1 <= 0x10){
				hw->LCDCON &= ~(0x1f<<22);
				hw->LCDCON |= (param1<<22);
			}

			if(param2 <= 0x1f){
				hw->LCDCON &= ~(0x1f<<16);
				hw->LCDCON |= (param2<<16);
			}	
			enable_irq(flags);		
		break;

		case LCDC_SET_CCM_EN:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,14);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,14);
			}
			enable_irq(flags);

		break;
		
		case LCDC_SET_COLOR_MODE:
			hw->LCD_IFCON &= ~(3<<3);
			if(LCD_MODE_565 == param1){
				hw->LCD_IFCON |= (2<<3);
			}else if(LCD_MODE_666 == param1){
				hw->LCD_IFCON |= (1<<3);
			}else if(LCD_MODE_888 == param1){
				hw->LCD_IFCON |= (0<<3);
			}
		break;
			
		case LCDC_SET_BUS_MODE:
			hw->LCD_IFCON &= ~(7<<5);
			if(LCD_BUS_WIDTH_6 == param1){
				hw->LCD_IFCON |= (0<<5);
			}else if(LCD_BUS_WIDTH_8 == param1){
				hw->LCD_IFCON |= (1<<5);
			}else if(LCD_BUS_WIDTH_9 == param1){
				hw->LCD_IFCON |= (2<<5);
			}else if(LCD_BUS_WIDTH_12 == param1){
				hw->LCD_IFCON |= (3<<5);
			}else if(LCD_BUS_WIDTH_16 == param1){
				hw->LCD_IFCON |= (4<<5);
			}else if(LCD_BUS_WIDTH_18 == param1){
				hw->LCD_IFCON |= (5<<5);
			}else if(LCD_BUS_WIDTH_24 == param1){
				hw->LCD_IFCON |= (6<<5);
			}
		break;
			
		case LCDC_SET_TE_EDGE:
			if(param1 == 0)
				hw->LCD_IFCON &= ~(1<<21);
			else if (param1 == 1)
				hw->LCD_IFCON |= (1<<21);
		break;

		case LCDC_SET_INTERFACE_MODE:
			hw->LCD_IFCON &= ~(3<<8);
			if(LCD_BUS_RGB == param1){
				hw->LCD_IFCON |= (0<<8);
			}else if(LCD_BUS_I80 == param1){
				hw->LCD_IFCON |= (1<<8);
			}else if(LCD_BUS_I68 == param1){
				hw->LCD_IFCON |= (2<<8);
			}
		break;

		case LCDC_SET_COLRARRAY:
			hw->LCD_IFCON &= ~(7<<0);
			if(param1 < 6)
				hw->LCD_IFCON |= (param1<<0);
		break;

		case LCDC_SET_LCD_VAILD_SIZE:
			pixel_dot_num = param2;
			h = param1&0xffff;
			w = (param1>>16)&0xffff;
			hw->LCD_WINCON &= ~(0X7FF<<16);
			hw->LCD_WINCON &= ~(0X1FFF<<0);
			hw->LCD_WINCON |= ((h-1)<<16);
			hw->LCD_WINCON |= ((w*pixel_dot_num - 1)<<0);
		break;
		case LCDC_SET_LCD_VISIBLE_SIZE:
			pixel_dot_num = param2;
			h = param1&0xffff;
			w = (param1>>16)&0xffff;			
			hw->LCD_VISIBLE_WIN &= ~(0X7FF<<16);
			hw->LCD_VISIBLE_WIN &= ~(0X7FF<<0);
			hw->PICTURE_SIZE &= ~(0X7FF<<16);
			hw->PICTURE_SIZE &= ~(0X7FF<<0);
			hw->LCD_VISIBLE_WIN |= ((h-1)<<16);
			hw->LCD_VISIBLE_WIN |= ((w*pixel_dot_num - 1)<<0);
			hw->PICTURE_SIZE |= ((h-1)<<16);
			hw->PICTURE_SIZE |= ((w-1)<<0);	
			
			hw->LCD_LEN	   = h * w * pixel_dot_num - 1;
		break;

		case LCDC_SET_SIGNAL_CONFIG:
			vs_en  = ((param1&BIT(0))>>0);
			hs_en  = ((param1&BIT(1))>>1);
			de_en  = ((param1&BIT(2))>>2);
			vs_inv = ((param1&BIT(3))>>3);
			hs_inv = ((param1&BIT(4))>>4);
			de_inv = ((param1&BIT(5))>>5);
			clk_inv= ((param1&BIT(6))>>6);

			if(vs_en == 1){
				hw->LCD_IFCON |= (1<<10);
				if(vs_inv == 1){
					hw->LCD_IFCON |= (1<<13);
				}else{
					hw->LCD_IFCON &= ~(1<<13);
				}
			}else{
				hw->LCD_IFCON &= ~(1<<10);
			}

			if(hs_en == 1){
				hw->LCD_IFCON |= (1<<11);
				if(hs_inv == 1){
					hw->LCD_IFCON |= (1<<14);
				}else{
					hw->LCD_IFCON &= ~(1<<14);
				}		
			}else{
				hw->LCD_IFCON &= ~(1<<11);
			}

			if(de_en == 1){
				hw->LCD_IFCON |= (1<<12);
				if(de_inv == 1){
					hw->LCD_IFCON |= (1<<15);
				}else{
					hw->LCD_IFCON &= ~(1<<15);
				}
			}else{
				hw->LCD_IFCON &= ~(1<<12);
			}	

			if(clk_inv == 1){
				hw->LCD_IFCON |= (1<<16);
			}else{
				hw->LCD_IFCON &= ~(1<<16);
			}			
		break;
		case LCDC_SET_MCU_SIGNAL_CONFIG:
			hw->LCD_IFCON &= ~(param1<<13);
			hw->LCD_IFCON |= (param1<<13);
		break;
		case LCDC_SET_INVALID_LINE:
			hw->LCD_IFCON &= ~(0x1f<<23);
			if(param1 < 0x20)
				hw->LCD_IFCON |= (param1<<23);		
		break;
			
		case LCDC_SET_VALID_DOT:
			hw->LCD_VISIBLE_START = 0;
			
			hw->LCD_VISIBLE_START |= ((param2)<<16);
			hw->LCD_VISIBLE_START |= ((param1)<<0);
		break;

		case LCDC_SET_HLW_VLW:
			hw->LCD_HSYNCON = 0;
			hw->LCD_HSYNCON |= (param1<<16);
			hw->LCD_HSYNCON |= ((0)<<0);	
			
			hw->LCD_VSYNCON = 0;
			hw->LCD_VSYNCON |= (param2<<16);
			hw->LCD_VSYNCON |= ((0)<<0);
		break;
		
		case LCDC_SET_VIDEO_BIG_ENDIAN:
			if(param1)
				hw->LCD_IFCON &= ~(1<<22);
			else
				hw->LCD_IFCON |= (1<<22);
		break;
			
		case LCDC_SET_VIDEO_SIZE:
			hw->VIDEO_WINCON = 0;
			hw->VIDEO_WINCON |= ((param1-1)<<0);
			hw->VIDEO_WINCON |= ((param2-1)<<16);
		break;

		case LCDC_SET_P0_ROTATE_Y_SRC_ADDR:	
			hw->ROTATE_PICTURE0_YDMA_STADR = param1;
		break;

		case LCDC_SET_P0_ROTATE_U_SRC_ADDR:	
			hw->ROTATE_PICTURE0_UDMA_STADR = param1;
		break;

		case LCDC_SET_P0_ROTATE_V_SRC_ADDR:	
			hw->ROTATE_PICTURE0_VDMA_STADR = param1;
		break;

		case LCDC_SET_P1_ROTATE_Y_SRC_ADDR:	
			hw->ROTATE_PICTURE1_YDMA_STADR = param1;
		break;

		case LCDC_SET_P1_ROTATE_U_SRC_ADDR:		
			hw->ROTATE_PICTURE1_UDMA_STADR = param1;
		break;

		case LCDC_SET_P1_ROTATE_V_SRC_ADDR:	
			hw->ROTATE_PICTURE1_VDMA_STADR = param1;
		break;

		case LCDC_SET_ROTATE_P0_UP:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,10);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,10);
			}
			enable_irq(flags);
		break;

		case LCDC_SET_P0P1_ROTATE_START_ADDR:
			x = param1&0xffff;
			y = (param1>>16)&0xffff;
			hw->ROTATE_PICTURE0_START = x|(y<<16);
			x = param2&0xffff;
			y = (param2>>16)&0xffff;			
			hw->ROTATE_PICTURE1_START = x|(y<<16);
		break;

		case LCDC_SET_ROTATE_LINE_BUF_NUM:
			hw->ROTATE_OUTBUF_LINE_NUM = param1;
		break;		

		case LCDC_SET_ROTATE_LINE_BUF_Y:
			hw->VIDEO_YDMA_STADR = param1;
		break;

		case LCDC_SET_ROTATE_LINE_BUF_U:
			hw->VIDEO_UDMA_STADR = param1;
		break;

		case LCDC_SET_ROTATE_LINE_BUF_V:
			hw->VIDEO_VDMA_STADR = param1;
		break;

		case LCDC_SET_ROTATE_MIRROR:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,13);	
			}else{
				LCD_CLEAR_CON(hw->LCDCON,13);
			}
			if(param2 == LCD_ROTATE_0){
				LCD_CLEAR_CON(hw->LCDCON,11);
				LCD_CLEAR_CON(hw->LCDCON,12);
			}
			if(param2 == LCD_ROTATE_90){
				LCD_SET_CON(hw->LCDCON,11);
				LCD_CLEAR_CON(hw->LCDCON,12);
			}else if(param2 == LCD_ROTATE_180){
				LCD_CLEAR_CON(hw->LCDCON,11);
				LCD_SET_CON(hw->LCDCON,12);		
			}else if(param2 == LCD_ROTATE_270){
				LCD_SET_CON(hw->LCDCON,11);
				LCD_SET_CON(hw->LCDCON,12);		
			}
			enable_irq(flags);
		break;
		case LCDC_SET_ROTATE_SET_SIZE:
			h = param1&0xffff;
			w = (param1>>16)&0xffff;			
			hw->ROTATE_PICTURE0_WINCON = 0;
			hw->ROTATE_PICTURE0_WINCON |= ((h-1)<<16);
			hw->ROTATE_PICTURE0_WINCON |= ((w-1)<<0);
			h = param2&0xffff;
			w = (param2>>16)&0xffff;			
			hw->ROTATE_PICTURE1_WINCON = 0;
			hw->ROTATE_PICTURE1_WINCON |= ((h-1)<<16);
			hw->ROTATE_PICTURE1_WINCON |= ((w-1)<<0);

		break;
		
		case LCDC_SET_VIDEO_MODE:
			flags = disable_irq();
			hw->LCDCON &= ~(3<<4);
			hw->LCDCON |= (param1<<4);
			enable_irq(flags);
		break;

		case LCDC_SET_VIDEO_START_ADDR:
			x = param1&0xffff;
			y = (param1>>16)&0xffff;	
			hw->VIDEO_START = 0;
			hw->VIDEO_START |= ((y)<<16);
			hw->VIDEO_START |= ((x)<<0);
		break;
		
		case LCDC_SET_VIDEO_EN:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,1);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,1);
			}
			enable_irq(flags);
		break;
			
		case LCDC_SET_OSD_SIZE:
			w = param1&0xffff;
			h = (param1>>16)&0xffff;			
			hw->OSD0_WINCON = 0;
			hw->OSD0_WINCON |= ((w-1)<<0);
			hw->OSD0_WINCON |= ((h-1)<<16);
		break;

		case LCDC_SET_OSD_START_ADDR:
			hw->OSD0_START = 0;	
			hw->OSD0_START |= ((param1)<<16);
			hw->OSD0_START |= ((param2)<<0);	
		break;

		case LCDC_SET_OSD_LUTDMA_ADDR:
			hw->OSD0_LUTDMA_STADR = param1;
		break;
		
		case LCDC_SET_OSD_DMA_ADDR:
			hw->OSD0_DMA_STADR = param1;
		break;
		
		case LCDC_SET_OSD_DMA_LEN:
			hw->OSD0_WORD_LEN = param1/4;
		break;

		case LCDC_SET_OSD_FORMAT:
			flags = disable_irq();
			hw->LCDCON &= ~(3<<6);
			if(OSD_RGB_256 == param1){
				hw->LCDCON |= (0<<6);
			}else if(OSD_RGB_565 == param1){
				hw->LCDCON |= (1<<6);
			}else if(OSD_RGB_888 == param1){
				hw->LCDCON |= (2<<6);
			}			
			enable_irq(flags);
		break;
		case LCDC_SET_OSD_ALPHA:
			hw->ALPHA &= ~(0x1ff<<0);
			hw->ALPHA |= (param1<<0);
		break;
		
		case LCDC_SET_ROTATE_P0_P1_EN:
			flags = disable_irq();
			if(param1)
			{
				LCD_SET_CON(hw->LCDCON,8);
			}
			else
			{
				LCD_CLEAR_CON(hw->LCDCON,8);
			}
				

			if(param2)
			{
				LCD_SET_CON(hw->LCDCON,9);
			}
			else
			{
				LCD_CLEAR_CON(hw->LCDCON,9);
			}

			enable_irq(flags);
		break;
		
		case LCDC_SET_OSD_ENC_HEAD:
			hw->OSD_ENC_IDENT0 = param1;
			hw->OSD_ENC_IDENT1 = param2;
		break;

		case LCDC_SET_OSD_ENC_DIAP:
			hw->OSD_ENC_TRANS0 = param1;
			hw->OSD_ENC_TRANS1 = param2;
		break;
		
		case LCDC_SET_OSD_EN:
			flags = disable_irq();
			if(param1)
			{
				LCD_SET_CON(hw->LCDCON,2);
			}
			else
			{
				LCD_CLEAR_CON(hw->LCDCON,2);
			}
			enable_irq(flags);
		break;
			
		case LCDC_SET_OSD_ENC_SRC_ADDR:
			hw->OSD_ENC_SADR = param1;
		break;
			
		case LCDC_SET_OSD_ENC_DST_ADDR:
			hw->OSD_ENC_TADR = param1;
		break;
		
		case LCDC_SET_OSD_ENC_SRC_LEN:
			hw->OSD_ENC_RLEN = param1/4;
		break;

		case LCDC_GET_OSD_ENC_DST_LEN:
			ret_val = hw->OSD_ENC_DLEN*4;
		break;	

		case LCDC_SET_OSD_ENC_START:
			flags = disable_irq();
			if(param1)
			{
				LCD_SET_CON(hw->LCDCON,3);
			}
			else
			{
				LCD_CLEAR_CON(hw->LCDCON,3);
			}
			enable_irq(flags);
			
		break;	

		case LCDC_MCU_CPU_WRITE:
			hw->LCD_IFCON &= ~BIT(17); // LCD_CS==0

			//mcu的命令,DC的io拉低
			if(!param2)
			{
				hw->LCD_IFCON &= ~BIT(18); // LCD_DC==0,tx cmd
			}
			//mcu的data,DC的io拉高
			else
			{
				hw->LCD_IFCON |= BIT(18);  // LCD_DC==1,tx data
			}	
			hw->LCD_IFCON |= BIT(19);  // LCD_CMD_WR=1

			hw->LCD_CMD = param1;
			while (hw->LCD_IFSTA & BIT(17));
			//hw->LCD_IFCON |= BIT(18);
			//os_sleep_ms(1);
			hw->LCD_IFCON |= BIT(17);
		break;	


		case LCDC_MCU_CPU_READ:
		{
			uint8_t cmd = param2>>16;
			uint8_t len = param2 & 0xff;
			uint8_t read_cmd;
			uint8_t *read_buf = (uint8_t*)param1;
			uint32_t baud_tmp = hw->LCD_BAUD;

			uint32_t len_tmp = hw->LCD_LEN;
			if(baud_tmp %2 == 0)
			{
				hw->LCD_BAUD = baud_tmp + 1;
			}
			hw->LCD_IFCON &= ~BIT(17);// LCD_CS==0
			hw->LCD_LEN = len-1;
			hw->LCD_IFCON &= ~BIT(18); // LCD_DC==0,tx cmd
			hw->LCD_IFCON &= ~BIT(19); // LCD_CMD_WR=0
			hw->LCD_CMD = cmd;
			for (int i = 0; i < len; i++)
			{
				while ((hw->LCD_IFSTA & BIT(18)) == 0)
				{
				}; // wait read data valid
				//data[i] = hw->LCD_CMD;
				read_cmd = hw->LCD_CMD;
				//printf("cmd:%X\n",read_cmd);
				*read_buf = read_cmd;
				read_buf++;
			}
			hw->LCD_IFCON |= BIT(17); // LCD_CS==1

			hw->LCD_LEN = len_tmp;
			hw->LCD_BAUD = baud_tmp;
		}
			
		break;

		case LCDC_SET_START_RUN:
			flags = disable_irq();
			hw->LCD_IFCON &= ~BIT(17); // LCD_CS==0
			hw->LCD_IFCON |= (1<<20);	
			enable_irq(flags);			
		break;
		
		case LCDC_SET_SATURATION_EN:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,21);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,21);
			}
			enable_irq(flags);
		break;

		case LCDC_SET_SATURATION_VALUE:
			flags = disable_irq();
			if(param1 <= 0x10){
				hw->LCDCON &= ~(0x1f<<22);
				hw->LCDCON |= (param1<<22);
			}
			enable_irq(flags);
		break;

		case LCDC_SET_CONSTRAST_EN:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,15);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,15);
			}
			enable_irq(flags);
		break;

		case LCDC_SET_CONSTRAST_VALUE:
			flags = disable_irq();
			if(param1 <= 0x1f){
				hw->LCDCON &= ~(0x1f<<16);
				hw->LCDCON |= (param1<<16);
			}
			enable_irq(flags);
		break;

		case LCDC_SET_GAMMA_EN:
			flags = disable_irq();
			if(param1){
				LCD_SET_CON(hw->LCDCON,27);
			}else{
				LCD_CLEAR_CON(hw->LCDCON,27);
			}
			enable_irq(flags);
		break;

		case LCDC_SET_GAMMA_R:
			hw->GAMMA_R_STADR = param1;
		break;

		case LCDC_SET_GAMMA_G:
			hw->GAMMA_G_STADR = param1;
		break;

		case LCDC_SET_GAMMA_B:
			hw->GAMMA_B_STADR = param1;
		break;

		case LCDC_SET_CCM_COEF0:
			hw->CCM_COEF0 = param1;
		break;

		case LCDC_SET_CCM_COEF1:
			hw->CCM_COEF1 = param1;
		break;

		case LCDC_SET_CCM_COEF2:
			hw->CCM_COEF2 = param1;
		break;

		case LCDC_SET_CCM_COEF3:
			hw->CCM_COEF3 = param1;
		break;

		case LCDC_GET_CON:
			ret_val = hw->LCDCON;
		break;


		default:
			os_printf("NO LCDC IOCTL:%d\r\n",ioctl_cmd);
			ret_val = -ENOTSUPP;
		break;
	}

	
	return ret_val;
}

static int32 hglcdc_set_baudrate(struct lcdc_device *p_lcdc,uint32 baudrate){
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc;
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	uint32 baud = 0;
	baud = system_clock_get()/baudrate;
	_os_printf("lcd baud:%d\r\n",baud);
	hw->LCD_BAUD = baud;
	return 0;
}


static int32 hglcdc_init(struct lcdc_device *p_lcdc){
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc;	
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	//SCHED->BW_STA_CYCLE = 60000000;
	//SCHED->CTRL_CON      |=BIT(1);
	SYSCTRL->CLK_CON4      |= BIT(29)|BIT(30);
	lcd_glo = hw;
	pin_func(HG_LCDC_DEVID,1);
	hw->LCD_IFCON = 0x80000;
	hw->LCDCON = 0; 

	return 0;
}

static int32 hglcdc_open(struct lcdc_device *p_lcdc){
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc;	
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	lcdc_hw->opened = 1;
	lcdc_hw->dsleep = 0;
	hw->LCDCON |= (1<<0); 
	irq_enable(lcdc_hw->irq_num);
	return 0;
}

static int32 hglcdc_close(struct lcdc_device *p_lcdc){
	struct hglcdc *lcdc_hw = (struct hglcdc*)p_lcdc;	
	struct hglcdc_hw *hw  = (struct hglcdc_hw *)lcdc_hw->hw;
	lcdc_hw->opened = 0;
	lcdc_hw->dsleep = 0;
	hw->LCDCON &= ~(1<<0); 
	irq_disable(lcdc_hw->irq_num);
	return 0;
}


int32 hglcdc_suspend(struct dev_obj *obj){
	struct hglcdc *lcdc_hw = (struct hglcdc*)obj;
	struct hglcdc_hw *hw;
	struct hglcdc_hw *hw_cfg;
	printf("........................................................................hglcd_suspend :%d  %d\r\n",lcdc_hw->opened,lcdc_hw->dsleep);	
	//确保已经被打开并且休眠过,直接返回
	if(!lcdc_hw->opened || lcdc_hw->dsleep)
	{
		return RET_OK;
	}
	lcdc_hw->dsleep = 1;
	lcdc_hw->cfg_backup = (uint32 *)os_malloc(sizeof(struct hglcdc_hw));
	hw_cfg = (struct hglcdc_hw *)lcdc_hw->cfg_backup;
	hw     = (struct hglcdc_hw *)lcdc_hw->hw;

	irq_disable(lcdc_hw->irq_num);
	
	while(1){
		if((hw->LCD_IFSTA & BIT(17)) == 0){
			break;
		}
	}

	hw_cfg->LCDCON					   = hw->LCDCON;					  
	hw_cfg->LCD_IFCON				   = hw->LCD_IFCON; 				  
	hw_cfg->LCD_BAUD				   = hw->LCD_BAUD;					
	hw_cfg->LCD_CMD 				   = hw->LCD_CMD; 						
	hw_cfg->LCD_DATA				   = hw->LCD_DATA;					
	hw_cfg->LCD_LEN 				   = hw->LCD_LEN;						  
	hw_cfg->LCD_IFSTA				   = hw->LCD_IFSTA;						
	hw_cfg->LCD_VSYNCON 			   = hw->LCD_VSYNCON;				  
	hw_cfg->LCD_HSYNCON 			   = hw->LCD_HSYNCON; 				
	hw_cfg->LCD_WINCON				   = hw->LCD_WINCON;					  
	hw_cfg->LCD_VISIBLE_WIN 		   = hw->LCD_VISIBLE_WIN;			  
	hw_cfg->LCD_VISIBLE_START		   = hw->LCD_VISIBLE_START; 		  
	hw_cfg->ALPHA					   = hw->ALPHA;							
	hw_cfg->PICTURE_SIZE			   = hw->PICTURE_SIZE;				  
	hw_cfg->OSD0_DMA_STADR			   = hw->OSD0_DMA_STADR;			  
	hw_cfg->OSD0_LUTDMA_STADR		   = hw->OSD0_LUTDMA_STADR; 		  
	hw_cfg->OSD0_WORD_LEN			   = hw->OSD0_WORD_LEN; 				  
	hw_cfg->OSD0_WINCON 			   = hw->OSD0_WINCON; 					
	hw_cfg->OSD0_START				   = hw->OSD0_START;				  
	hw_cfg->VIDEO_YDMA_STADR		   = hw->VIDEO_YDMA_STADR;			
	hw_cfg->VIDEO_UDMA_STADR		   = hw->VIDEO_UDMA_STADR;			
	hw_cfg->VIDEO_VDMA_STADR		   = hw->VIDEO_VDMA_STADR;			
	hw_cfg->VIDEO_WINCON			   = hw->VIDEO_WINCON;				
	hw_cfg->VIDEO_START 			   = hw->VIDEO_START; 					
	hw_cfg->ROTATE_PICTURE0_YDMA_STADR = hw->ROTATE_PICTURE0_YDMA_STADR;  
	hw_cfg->ROTATE_PICTURE0_UDMA_STADR = hw->ROTATE_PICTURE0_UDMA_STADR;  
	hw_cfg->ROTATE_PICTURE0_VDMA_STADR = hw->ROTATE_PICTURE0_VDMA_STADR;  
	hw_cfg->ROTATE_PICTURE1_YDMA_STADR = hw->ROTATE_PICTURE1_YDMA_STADR;  
	hw_cfg->ROTATE_PICTURE1_UDMA_STADR = hw->ROTATE_PICTURE1_UDMA_STADR;  
	hw_cfg->ROTATE_PICTURE1_VDMA_STADR = hw->ROTATE_PICTURE1_VDMA_STADR;  
	hw_cfg->ROTATE_PICTURE0_START	   = hw->ROTATE_PICTURE0_START; 	  
	hw_cfg->ROTATE_PICTURE0_WINCON	   = hw->ROTATE_PICTURE0_WINCON;	  
	hw_cfg->ROTATE_PICTURE1_START	   = hw->ROTATE_PICTURE1_START; 	  
	hw_cfg->ROTATE_PICTURE1_WINCON	   = hw->ROTATE_PICTURE1_WINCON;	  
	hw_cfg->ROTATE_OUTBUF_LINE_NUM	   = hw->ROTATE_OUTBUF_LINE_NUM;	  
	hw_cfg->CCM_COEF0					 = hw->CCM_COEF0;						
	hw_cfg->CCM_COEF1					 = hw->CCM_COEF1;						
	hw_cfg->CCM_COEF2					 = hw->CCM_COEF2;						
	hw_cfg->CCM_COEF3					 = hw->CCM_COEF3;						
	hw_cfg->GAMMA_R_STADR			   = hw->GAMMA_R_STADR; 				  
	hw_cfg->GAMMA_G_STADR			   = hw->GAMMA_G_STADR; 				  
	hw_cfg->GAMMA_B_STADR			   = hw->GAMMA_B_STADR; 				  
	hw_cfg->OSD_ENC_SADR			   = hw->OSD_ENC_SADR;				  
	hw_cfg->OSD_ENC_TADR			   = hw->OSD_ENC_TADR;				  
	hw_cfg->OSD_ENC_RLEN			   = hw->OSD_ENC_RLEN;				  
	hw_cfg->OSD_ENC_DLEN			   = hw->OSD_ENC_DLEN;				  
	hw_cfg->OSD_ENC_IDENT0			   = hw->OSD_ENC_IDENT0;			  
	hw_cfg->OSD_ENC_IDENT1			   = hw->OSD_ENC_IDENT1;			  
	hw_cfg->OSD_ENC_TRANS0			   = hw->OSD_ENC_TRANS0;			  
	hw_cfg->OSD_ENC_TRANS1			   = hw->OSD_ENC_TRANS1;			  
	hw_cfg->TIMEOUTCON				   = hw->TIMEOUTCON;				  

	printf("hw_cfg LCDCON:%x\r\n",hw_cfg->LCDCON);

	return 0;
}

int32 hglcdc_resume(struct dev_obj *obj){
	struct hglcdc *lcdc_hw = (struct hglcdc*)obj;
	struct hglcdc_hw *hw;
	struct hglcdc_hw *hw_cfg;
	//如果已经被打开并且没有休眠过,直接返回
	printf("........................................................................hglcd_resume\r\n");
	if(!lcdc_hw->opened || !lcdc_hw->dsleep)
	{
		return RET_OK;
	}
	lcdc_hw->dsleep = 0;	
	hw_cfg = (struct hglcdc_hw*)lcdc_hw->cfg_backup;
	hw     = (struct hglcdc_hw*)lcdc_hw->hw;
	pin_func(HG_LCDC_DEVID,1);
	hw->LCDCON   		               = hw_cfg->LCDCON;   	
	hw->LCD_IFCON                  = hw_cfg->LCD_IFCON;                   
	hw->LCD_BAUD         	         = hw_cfg->LCD_BAUD;         	        
	hw->LCD_CMD         	         = hw_cfg->LCD_CMD;         	            
	hw->LCD_DATA         	         = hw_cfg->LCD_DATA;         	        
	hw->LCD_LEN       	           = hw_cfg->LCD_LEN;       	              
	hw->LCD_IFSTA       	         = hw_cfg->LCD_IFSTA;       	            
	hw->LCD_VSYNCON    	           = hw_cfg->LCD_VSYNCON;    	          
	hw->LCD_HSYNCON      	         = hw_cfg->LCD_HSYNCON;      	        
	hw->LCD_WINCON    	           = hw_cfg->LCD_WINCON;    	              
	hw->LCD_VISIBLE_WIN            = hw_cfg->LCD_VISIBLE_WIN;             
	hw->LCD_VISIBLE_START          = hw_cfg->LCD_VISIBLE_START;           
	hw->ALPHA 		                 = hw_cfg->ALPHA; 		                    
	hw->PICTURE_SIZE               = hw_cfg->PICTURE_SIZE;                
	hw->OSD0_DMA_STADR 	           = hw_cfg->OSD0_DMA_STADR; 	          
	hw->OSD0_LUTDMA_STADR          = hw_cfg->OSD0_LUTDMA_STADR;           
	hw->OSD0_WORD_LEN 	           = hw_cfg->OSD0_WORD_LEN; 	              
	hw->OSD0_WINCON 	             = hw_cfg->OSD0_WINCON; 	                
	hw->OSD0_START 	               = hw_cfg->OSD0_START; 	              
	hw->VIDEO_YDMA_STADR 	         = hw_cfg->VIDEO_YDMA_STADR; 	        
	hw->VIDEO_UDMA_STADR 	         = hw_cfg->VIDEO_UDMA_STADR; 	        
	hw->VIDEO_VDMA_STADR 	         = hw_cfg->VIDEO_VDMA_STADR; 	        
	hw->VIDEO_WINCON 	             = hw_cfg->VIDEO_WINCON; 	            
	hw->VIDEO_START 	             = hw_cfg->VIDEO_START; 	                
	hw->ROTATE_PICTURE0_YDMA_STADR = hw_cfg->ROTATE_PICTURE0_YDMA_STADR;  
	hw->ROTATE_PICTURE0_UDMA_STADR = hw_cfg->ROTATE_PICTURE0_UDMA_STADR;  
	hw->ROTATE_PICTURE0_VDMA_STADR = hw_cfg->ROTATE_PICTURE0_VDMA_STADR;  
	hw->ROTATE_PICTURE1_YDMA_STADR = hw_cfg->ROTATE_PICTURE1_YDMA_STADR;  
	hw->ROTATE_PICTURE1_UDMA_STADR = hw_cfg->ROTATE_PICTURE1_UDMA_STADR;  
	hw->ROTATE_PICTURE1_VDMA_STADR = hw_cfg->ROTATE_PICTURE1_VDMA_STADR;  
	hw->ROTATE_PICTURE0_START      = hw_cfg->ROTATE_PICTURE0_START;       
	hw->ROTATE_PICTURE0_WINCON     = hw_cfg->ROTATE_PICTURE0_WINCON;      
	hw->ROTATE_PICTURE1_START      = hw_cfg->ROTATE_PICTURE1_START;       
	hw->ROTATE_PICTURE1_WINCON     = hw_cfg->ROTATE_PICTURE1_WINCON;      
	hw->ROTATE_OUTBUF_LINE_NUM     = hw_cfg->ROTATE_OUTBUF_LINE_NUM;      
	hw->CCM_COEF0 		             = hw_cfg->CCM_COEF0; 		                
	hw->CCM_COEF1 		             = hw_cfg->CCM_COEF1; 		                
	hw->CCM_COEF2 		             = hw_cfg->CCM_COEF2; 		                
	hw->CCM_COEF3 		             = hw_cfg->CCM_COEF3; 		                
	hw->GAMMA_R_STADR 	           = hw_cfg->GAMMA_R_STADR; 	              
	hw->GAMMA_G_STADR 	           = hw_cfg->GAMMA_G_STADR; 	              
	hw->GAMMA_B_STADR 	           = hw_cfg->GAMMA_B_STADR; 	              
	hw->OSD_ENC_SADR 		           = hw_cfg->OSD_ENC_SADR; 		          
	hw->OSD_ENC_TADR 		           = hw_cfg->OSD_ENC_TADR; 		          
	hw->OSD_ENC_RLEN 		           = hw_cfg->OSD_ENC_RLEN; 		          
	hw->OSD_ENC_DLEN 		           = hw_cfg->OSD_ENC_DLEN; 		          
	hw->OSD_ENC_IDENT0 	           = hw_cfg->OSD_ENC_IDENT0; 	          
	hw->OSD_ENC_IDENT1 	           = hw_cfg->OSD_ENC_IDENT1; 	          
	hw->OSD_ENC_TRANS0 	           = hw_cfg->OSD_ENC_TRANS0; 	          
	hw->OSD_ENC_TRANS1 	           = hw_cfg->OSD_ENC_TRANS1; 	          
	hw->TIMEOUTCON 	               = hw_cfg->TIMEOUTCON; 	              	
	printf("hw_cfg2 LCDCON:%x    hw->LCDCON :%x\r\n",hw_cfg->LCDCON,hw->LCDCON );
	irq_enable(lcdc_hw->irq_num);
	os_free(lcdc_hw->cfg_backup);
	return 0;
}



static const struct lcdc_hal_ops dev_ops = {
	.init        = hglcdc_init,
	.baudrate    = hglcdc_set_baudrate,
    .open        = hglcdc_open,
    .close       = hglcdc_close,
    .ioctl       = hglcdc_ioctl,
    .request_irq = lcdcirq_register,
    .release_irq = lcdcirq_unregister,
#ifdef CONFIG_SLEEP	
	.ops.suspend = hglcdc_suspend,
	.ops.resume  = hglcdc_resume,
#endif   	
};


int32 hglcdc_attach(uint32 dev_id, struct hglcdc *lcdc){
    lcdc->opened          = 0;
    lcdc->use_dma         = 0;
    lcdc->irq_hdl                   = NULL;
    lcdc->irq_data                  = 0;
	lcdc->dev.dev.ops = (const struct devobj_ops *)&dev_ops;
    irq_disable(lcdc->irq_num);
    dev_register(dev_id, (struct dev_obj *)lcdc);	
	return 0;
}


