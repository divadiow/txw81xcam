#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "devid.h"
#include "dev/vpp/hgvpp.h"
#include "osal/string.h"

struct hgvpp_hw
{
    __IO uint32_t CON;
    __IO uint32_t SIZE;  
    __IO uint32_t DLT;  
    __IO uint32_t DHT; 
    __IO uint32_t STA;  
    __IO uint32_t DMA_YADR;   
    __IO uint32_t DMA_UADR;
    __IO uint32_t DMA_VADR;
    __IO uint32_t DMA_YADR1;   
    __IO uint32_t DMA_UADR1;
    __IO uint32_t DMA_VADR1; 
    __IO uint32_t IWM0_CON;
    __IO uint32_t IWM0_SIZE;    
    __IO uint32_t IWM0_YUV;
    __IO uint32_t IWM0_LADR;
    __IO uint32_t IWM0_IDX0;    
    __IO uint32_t IWM0_IDX1;
    __IO uint32_t IWM0_IDX2;    
    __IO uint32_t IWM1_CON;
    __IO uint32_t IWM1_SIZE;
    __IO uint32_t IWM1_YUV;
    __IO uint32_t IWM1_LADR;  
    __IO uint32_t IPF_SADR;    
	__IO uint32_t MD_CON;
	__IO uint32_t MD_WIN_CON0;
	__IO uint32_t MD_WIN_CON1;
	__IO uint32_t MD_BASE_ADDR;
	__IO uint32_t ITP_YADR;
	__IO uint32_t ITP_UADR;
	__IO uint32_t ITP_VADR;
	
};


vpp_irq_hdl vppirq_vector_table[VPP_IRQ_NUM];
volatile uint32 vppirq_dev_table[VPP_IRQ_NUM];


static int32 hgvpp_ioctl(struct vpp_device *p_vpp, enum vpp_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2){
    int32  ret_val = RET_OK;
	uint32  w,h,num;
	uint32 x_s,y_s,x_e,y_e;
	//uint32 *size_array;
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp;	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	switch(ioctl_cmd){
		case VPP_IOCTL_CMD_ITP_AUTO_CLOSE:
			if(param1)
				hw->CON |= BIT(30);		
			else
				hw->CON &= ~BIT(30);	
		break;
			
		case VPP_IOCTL_CMD_SET_TAKE_PHOTO_LINEBUF:
			if(param1 < 8){
				hw->CON &= ~(0x7<<17);
				hw->CON |= (param1<<17);				
			}
		break;
	
		case VPP_IOCTL_CMD_DIS_UV_MODE:
			if(param1)
				hw->CON |= BIT(4);		
			else
				hw->CON &= ~BIT(4);	
		break;
			
		case VPP_IOCTL_CMD_SET_THRESHOLD:
			hw->DLT = param1;	
			hw->DHT = param2;
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK0_RC:
			if(param1)
				hw->IWM0_CON |= BIT(5);
			else
				hw->IWM0_CON &= ~BIT(5);	
		break;
			
		case VPP_IOCTL_CMD_SET_WATERMARK1_RC:
			if(param1)
				hw->IWM1_CON |= BIT(5);
			else
				hw->IWM1_CON &= ~BIT(5);	
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK0_COLOR:
			hw->IWM0_YUV = 	param1;
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK1_COLOR:
			hw->IWM1_YUV =	param1;
		break;
				
		case VPP_IOCTL_CMD_SET_WATERMARK0_BMPADR:
			hw->IWM0_LADR =	param1;
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK1_BMPADR:
			hw->IWM1_LADR =	param1;
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_LOCATED:
			hw->IWM0_CON &= ~(0xff<<16);
			hw->IWM0_CON &= ~(0xff<<24);
			hw->IWM0_CON |= (((param1&0Xff00)>>8)<<24);
			hw->IWM0_CON |= ((param1&0Xff)<<16);			
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK1_LOCATED:
			hw->IWM1_CON &= ~(0xff<<16);
			hw->IWM1_CON &= ~(0xff<<24);
			hw->IWM1_CON |= (((param1&0Xff00)>>8)<<24);
			hw->IWM1_CON |= ((param1&0Xff)<<16);			
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_CONTRAST:
			hw->IWM0_CON &= ~(0x7<<2);
			hw->IWM0_CON |= (param1<<2);
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK1_CONTRAST:
			
			hw->IWM1_CON &= ~(0x7<<2);
			hw->IWM1_CON |= (param1<<2);
		break;		
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_SIZE_AND_NUM:
			w = param1&0xff;
			h = ((param1&0xff00)>>8)&0xff; 
			num = param2&0xff;
			hw->IWM0_SIZE &= ~(0x1f<<0);
			hw->IWM0_SIZE |= (num<<0);
			hw->IWM0_SIZE &= ~(0x3f<<5);
			hw->IWM0_SIZE |= (w<<5);
			hw->IWM0_SIZE &= ~(0x3ff<<11);
			hw->IWM0_SIZE |= ((w*num)<<11);
			hw->IWM0_SIZE &= ~(0xff<<21);
			hw->IWM0_SIZE |= (h<<21);
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_IDX:	
			if(param2 > 23)
				return ret_val;
			
			if(param2 < 8){
				hw->IWM0_IDX0 &= ~(0xf<<(param2*4));
				hw->IWM0_IDX0 |= (param1<<(param2*4));
			}
			else if(param2 < 16){
				hw->IWM0_IDX1 &= ~(0xf<<((param2-8)*4));
				hw->IWM0_IDX1 |= (param1<<((param2-8)*4));
			
			}else{
				hw->IWM0_IDX2 &= ~(0xf<<((param2-16)*4));
				hw->IWM0_IDX2 |= (param1<<((param2-16)*4));
			}
		break;
			
		case VPP_IOCTL_CMD_SET_WATERMARK1_PHOTO_SIZE:
			w = param1&0xff;
			h = ((param1&0xff00)>>8)&0xff;	
		
			hw->IWM1_SIZE &= ~(0xff<<11);
			hw->IWM1_SIZE |= (w<<11);
			hw->IWM1_SIZE &= ~(0xff<<21);
			hw->IWM1_SIZE |= (h<<21);
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_MODE:
			if(param1)
				hw->IWM0_CON |= BIT(1);
			else
				hw->IWM0_CON &= ~BIT(1);	
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK1_MODE:
			if(param1)
				hw->IWM1_CON |= BIT(1);
			else
				hw->IWM1_CON &= ~BIT(1);	
		break;		

		case VPP_IOCTL_CMD_SET_WATERMARK0_EN:
			if(param1)
				hw->IWM0_CON |= BIT(0);
			else
				hw->IWM0_CON &= ~BIT(0);	
		break;		

		case VPP_IOCTL_CMD_SET_WATERMARK1_EN:
			if(param1)
				hw->IWM1_CON |= BIT(0);
			else
				hw->IWM1_CON &= ~BIT(0);	
		break;		
			
		case VPP_IOCTL_CMD_SET_MOTION_DET_EN:
			if(param1){
				hw->MD_CON &= ~(0x1f<<18);
				hw->MD_CON |= (BIT(23));				
				hw->MD_CON |= BIT(0);
			}
			else{
				hw->MD_CON &= ~BIT(0);	
			}
		break;
			
		case VPP_IOCTL_CMD_SET_MOTION_CALBUF:
			hw->MD_BASE_ADDR = param1;
		break;

		case VPP_IOCTL_CMD_SET_MOTION_RANGE:
			x_s = param1&0xffff;
			y_s = ((param1&0xffff0000)>>16)&0xffff; 
			x_e = param2&0xffff;
			y_e = ((param2&0xffff0000)>>16)&0xffff; 		
			//hw->MD_WIN_CON0 = param1;
			//hw->MD_WIN_CON1 = param1;	
			hw->MD_WIN_CON0 = (x_s | (y_s<<16));
			hw->MD_WIN_CON1 = ((x_e-1) | ((y_e-1)<<16));			
		break;

		
		case VPP_IOCTL_CMD_SET_MOTION_BLK_THRESHOLD:
			param1 = param1&0xff;
			hw->MD_CON &= ~(0xff<<1);
			hw->MD_CON |= (param1<<1);
		break;
		
		case VPP_IOCTL_CMD_SET_MOTION_FRAME_THRESHOLD:
			param1 = param1&0x1ff;
			hw->MD_CON &= ~(0x1ff<<9);
			hw->MD_CON |= (param1<<9);
		break;

		case VPP_IOCTL_CMD_SET_IFP_ADDR:
			hw->IPF_SADR = param1;
		break;
		
		case VPP_IOCTL_CMD_SET_IFP_EN:
			if(param1)
				hw->CON |= BIT(28);
			else
				hw->CON &= ~BIT(28);	
		break;	
			
		case VPP_IOCTL_CMD_SET_MODE:
			if(param1)
				hw->CON |= BIT(1);
			else
				hw->CON &= ~BIT(1);	
		break;	
			
		case VPP_IOCTL_CMD_SET_SIZE:
			w = param1;
			h = param2;	
		
			hw->SIZE = 0;
			hw->SIZE |= (w<<0);
			hw->SIZE |= (h<<11);
		break;
		
		case VPP_IOCTL_CMD_SET_YCBCR:
			if(param1 > 3){
				os_printf("set ycbcr error:%d\r\n",param1);
			}
			hw->CON &= ~(BIT(2)|BIT(3));
			hw->CON |= param1<<2;
		break;
		
		case VPP_IOCTL_CMD_BUF0_CNT:
			if(param1 > 16){
				os_printf("set buf0 error:%d\r\n",param1);
			}
			hw->CON &= ~(0x0f<<7);
			hw->CON |= (param1<<7);
		break;

		case VPP_IOCTL_CMD_BUF1_CNT:
			if(param1 > 16){
				os_printf("set buf1 error:%d\r\n",param1);
			}
			hw->CON &= ~(0x0f<<13);
			hw->CON |= (param1<<13);
		break;		
			
		case VPP_IOCTL_CMD_BUF1_EN:
			if(param1)
				hw->CON |= BIT(11);
			else
				hw->CON &= ~BIT(11);	
		break;	
			
		case VPP_IOCTL_CMD_BUF1_SHRINK:
			if(param1)
				hw->CON &= ~BIT(12);			// 1/2
			else
				hw->CON |= BIT(12);	            // 1/3
		break;	
			
		case VPP_IOCTL_CMD_INPUT_INTERFACE:
			if(param1)
				hw->CON |= BIT(5);
			else
				hw->CON &= ~BIT(5);	
		break;

		case VPP_IOCTL_CMD_BUF0_Y_ADDR:
			hw->DMA_YADR = param1;
		break;

		case VPP_IOCTL_CMD_BUF0_U_ADDR:
			hw->DMA_UADR = param1;
		break;

		case VPP_IOCTL_CMD_BUF0_V_ADDR:
			hw->DMA_VADR = param1;
		break;

		case VPP_IOCTL_CMD_BUF1_Y_ADDR:
			hw->DMA_YADR1 = param1;
		break;

		case VPP_IOCTL_CMD_BUF1_U_ADDR:
			hw->DMA_UADR1 = param1;
		break;

		case VPP_IOCTL_CMD_BUF1_V_ADDR:
			hw->DMA_VADR1 = param1;
		break;

		case VPP_IOCTL_CMD_ITP_Y_ADDR:
			hw->ITP_YADR = param1;
		break;

		case VPP_IOCTL_CMD_ITP_U_ADDR:
			hw->ITP_UADR = param1;
		break;

		case VPP_IOCTL_CMD_ITP_V_ADDR:
			hw->ITP_VADR = param1;
		break;

		case VPP_IOCTL_CMD_ITP_EN:
			if(param1)
				hw->CON |= BIT(29);
			else
				hw->CON &= ~BIT(29);
		break;

		default:
			os_printf("NO VPP IOCTL:%d\r\n",ioctl_cmd);
            ret_val = -ENOTSUPP;
        break;
	}


	return ret_val;
}


void VPP_IRQHandler_action(void *p_vpp)
{
	uint32 sta = 0;
	uint8 loop;
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp; 
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	sta = hw->STA;
	for(loop = 0;loop < VPP_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->STA = BIT(loop);
			if(vppirq_vector_table[loop] != NULL)
				vppirq_vector_table[loop] (loop,vppirq_dev_table[loop],0);
		}
	}
}


void irq_vpp_enable(struct vpp_device *p_vpp,uint8 mode,uint8 irq){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp; 
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	if(mode){
		hw->CON |= BIT(irq+20);
	}else{
		hw->CON &= ~BIT(irq+20);
	}
}


int32 vppirq_register(struct vpp_device *p_vpp,uint32 irq, vpp_irq_hdl isr, uint32 dev_id){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp; 	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	request_irq(vpp_hw->irq_num, VPP_IRQHandler_action, p_vpp);
	
	irq_vpp_enable(p_vpp, 1, irq);
	vppirq_vector_table[irq] = isr;
	vppirq_dev_table[irq] = dev_id;
	hw->STA |= BIT(irq);
	os_printf("vppirq_register:%d %x  %x\r\n",irq,(uint32)vppirq_vector_table[irq],(uint32)isr);
	return 0;
}


int32 vppirq_unregister(struct vpp_device *p_vpp,uint32 irq){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp;	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	irq_vpp_enable(p_vpp, 0, irq);
	vppirq_vector_table[irq] = NULL;
	vppirq_dev_table[irq] = 0;
	hw->STA |= BIT(irq);
	return 0;
}


static int32 hgvpp_open(struct vpp_device *p_vpp){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp;	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	hw->CON |= BIT(0);
	vpp_hw->opened = 1;
	vpp_hw->dsleep = 0;
	irq_enable(vpp_hw->irq_num);
	return 0;
}

static int32 hgvpp_close(struct vpp_device *p_vpp){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp; 	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	hw->CON &= ~BIT(0);
	vpp_hw->opened = 0;
	vpp_hw->dsleep = 0;
	irq_disable(vpp_hw->irq_num);
	return 0;
}

int32 hgvpp_suspend(struct dev_obj *obj){
	struct hgvpp *vpp_hw = (struct hgvpp*)obj;
	struct hgvpp_hw *hw;
	struct hgvpp_hw *hw_cfg;
	//确保已经被打开并且休眠过,直接返回
	if(!vpp_hw->opened || vpp_hw->dsleep)
	{
		return RET_OK;
	}
	vpp_hw->dsleep = 1;
	vpp_hw->cfg_backup = (uint32 *)os_malloc(sizeof(struct hgvpp_hw));
	hw_cfg = (struct hgvpp_hw*)vpp_hw->cfg_backup;
	hw     = (struct hgvpp_hw*)vpp_hw->hw;
	
	hw_cfg->CON 		 = hw->CON;				
	hw_cfg->SIZE		 = hw->SIZE; 			   
	hw_cfg->DLT 		 = hw->DLT;				  
	hw_cfg->DHT 		 = hw->DHT;				 
	hw_cfg->STA 		 = hw->STA;				  
	hw_cfg->DMA_YADR	 = hw->DMA_YADR; 				
	hw_cfg->DMA_UADR	 = hw->DMA_UADR; 			 
	hw_cfg->DMA_VADR	 = hw->DMA_VADR; 			 
	hw_cfg->DMA_YADR1	 = hw->DMA_YADR1;				 
	hw_cfg->DMA_UADR1	 = hw->DMA_UADR1;			  
	hw_cfg->DMA_VADR1	 = hw->DMA_VADR1;			   
	hw_cfg->IWM0_CON	 = hw->IWM0_CON; 			 
	hw_cfg->IWM0_SIZE	 = hw->IWM0_SIZE;				  
	hw_cfg->IWM0_YUV	 = hw->IWM0_YUV; 			 
	hw_cfg->IWM0_LADR	 = hw->IWM0_LADR;			  
	hw_cfg->IWM0_IDX0	 = hw->IWM0_IDX0;				  
	hw_cfg->IWM0_IDX1	 = hw->IWM0_IDX1;			  
	hw_cfg->IWM0_IDX2	 = hw->IWM0_IDX2;				  
	hw_cfg->IWM1_CON	 = hw->IWM1_CON; 			 
	hw_cfg->IWM1_SIZE	 = hw->IWM1_SIZE;			  
	hw_cfg->IWM1_YUV	 = hw->IWM1_YUV;			 
	hw_cfg->IWM1_LADR	 = hw->IWM1_LADR;				
	hw_cfg->IPF_SADR	 = hw->IPF_SADR; 				 
	hw_cfg->MD_CON		 = hw->MD_CON;			   
	hw_cfg->MD_WIN_CON0  = hw->MD_WIN_CON0;				
	hw_cfg->MD_WIN_CON1  = hw->MD_WIN_CON1;				
	hw_cfg->MD_BASE_ADDR = hw->MD_BASE_ADDR;			 
	hw_cfg->ITP_YADR	 = hw->ITP_YADR; 			 
	hw_cfg->ITP_UADR	 = hw->ITP_UADR; 			 
	hw_cfg->ITP_VADR	 = hw->ITP_VADR;   

	
	irq_disable(vpp_hw->irq_num);
	return 0;
}

int32 hgvpp_resume(struct dev_obj *obj){
	struct hgvpp *vpp_hw = (struct hgvpp*)obj;
	struct hgvpp_hw *hw;
	struct hgvpp_hw *hw_cfg;
	//如果已经被打开并且没有休眠过,直接返回
	if(!vpp_hw->opened || !vpp_hw->dsleep)
	{
		return RET_OK;
	}
	vpp_hw->dsleep = 0;	
	hw_cfg = (struct hgvpp_hw*)vpp_hw->cfg_backup;
	hw     = (struct hgvpp_hw*)vpp_hw->hw;
	hw->CON 		 = hw_cfg->CON;				
	hw->SIZE		 = hw_cfg->SIZE; 			   
	hw->DLT 		 = hw_cfg->DLT;				  
	hw->DHT 		 = hw_cfg->DHT;				 
	hw->STA 		 = hw_cfg->STA;				  
	hw->DMA_YADR	 = hw_cfg->DMA_YADR; 				
	hw->DMA_UADR	 = hw_cfg->DMA_UADR; 			 
	hw->DMA_VADR	 = hw_cfg->DMA_VADR; 			 
	hw->DMA_YADR1	 = hw_cfg->DMA_YADR1;				 
	hw->DMA_UADR1	 = hw_cfg->DMA_UADR1;			  
	hw->DMA_VADR1	 = hw_cfg->DMA_VADR1;			   
	hw->IWM0_CON	 = hw_cfg->IWM0_CON; 			 
	hw->IWM0_SIZE	 = hw_cfg->IWM0_SIZE;				  
	hw->IWM0_YUV	 = hw_cfg->IWM0_YUV; 			 
	hw->IWM0_LADR	 = hw_cfg->IWM0_LADR;			  
	hw->IWM0_IDX0	 = hw_cfg->IWM0_IDX0;				  
	hw->IWM0_IDX1	 = hw_cfg->IWM0_IDX1;			  
	hw->IWM0_IDX2	 = hw_cfg->IWM0_IDX2;				  
	hw->IWM1_CON	 = hw_cfg->IWM1_CON; 			 
	hw->IWM1_SIZE	 = hw_cfg->IWM1_SIZE;			  
	hw->IWM1_YUV	 = hw_cfg->IWM1_YUV;			 
	hw->IWM1_LADR	 = hw_cfg->IWM1_LADR;				
	hw->IPF_SADR	 = hw_cfg->IPF_SADR; 				 
	hw->MD_CON		 = hw_cfg->MD_CON;			   
	hw->MD_WIN_CON0  = hw_cfg->MD_WIN_CON0;				
	hw->MD_WIN_CON1  = hw_cfg->MD_WIN_CON1;				
	hw->MD_BASE_ADDR = hw_cfg->MD_BASE_ADDR;			 
	hw->ITP_YADR	 = hw_cfg->ITP_YADR; 			 
	hw->ITP_UADR	 = hw_cfg->ITP_UADR; 			 
	hw->ITP_VADR	 = hw_cfg->ITP_VADR;   
	irq_enable(vpp_hw->irq_num);
	os_free(vpp_hw->cfg_backup);
	return 0;
}

static const struct vpp_hal_ops dev_ops = {
    .open        = hgvpp_open,
    .close       = hgvpp_close,
    .ioctl       = hgvpp_ioctl,
    .request_irq = vppirq_register,
    .release_irq = vppirq_unregister,
#ifdef CONFIG_SLEEP	
	.ops.suspend = hgvpp_suspend,
	.ops.resume  = hgvpp_resume,
#endif   	
};



int32 hgvpp_attach(uint32 dev_id, struct hgvpp *vpp){
    vpp->opened          = 0;
    vpp->use_dma         = 0;
    vpp->irq_hdl                   = NULL;
    //memset(dvp->irq_hdl,0,sizeof(dvp->irq_hdl));
    vpp->irq_data                  = 0;
	//memset(dvp->irq_data,0,sizeof(dvp->irq_data));
	vpp->dev.dev.ops = (const struct devobj_ops *)&dev_ops;

    irq_disable(vpp->irq_num);
    dev_register(dev_id, (struct dev_obj *)vpp);	
	return 0;
}


