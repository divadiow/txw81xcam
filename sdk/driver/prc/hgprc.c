#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "devid.h"
#include "dev/prc/hgprc.h"
#include "osal/string.h"

struct hgprc_hw
{
	__IO uint32 CON;
	__IO uint32 STA;
    __IO uint32 SADR;      
    __IO uint32 YADR;
    __IO uint32 UADR;
    __IO uint32 VADR;  	
};


prc_irq_hdl prcirq_vector_table[PRC_IRQ_NUM];
volatile uint32 prcirq_dev_table[PRC_IRQ_NUM];



static int32 hgprc_ioctl(struct prc_device *p_prc, enum prc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2){
    int32  ret_val = RET_OK;
	struct hgprc *prc_hw = (struct hgprc*)p_prc;	
	struct hgprc_hw *hw  = (struct hgprc_hw *)prc_hw->hw;

	switch(ioctl_cmd){
		case PRC_IOCTL_CMD_SET_WIDTH:
			hw->CON &= ~((0x7ff)<<16);
			hw->CON |= ((param1)<<16);
		break;
		
		case PRC_IOCTL_CMD_SET_YUV_MODE:
			hw->CON &= ~((0x3)<<2);
			hw->CON |= ((param1)<<2);
		break;
		
		case PRC_IOCTL_CMD_SET_Y_ADDR:
			hw->YADR = param1;
		break;

		case PRC_IOCTL_CMD_SET_U_ADDR:
			hw->UADR = param1;
		break;

		case PRC_IOCTL_CMD_SET_V_ADDR:
			hw->VADR = param1;
		break;

		case PRC_IOCTL_CMD_SET_SRC_ADDR:
			hw->SADR = param1;
		break;
		
		case PRC_IOCTL_CMD_KICK:
			hw->CON |= BIT(1);				//enable prc
			hw->CON |= BIT(0);
		break;

		default:
			os_printf("NO PRC IOCTL:%d\r\n",ioctl_cmd);
            ret_val = -ENOTSUPP;
        break;		
	}
	return ret_val;
}


void PRC_IRQHandler_action(void *p_prc)
{
	uint32 sta = 0;
	uint8 loop;
	struct hgprc *prc_hw = (struct hgprc*)p_prc; 
	struct hgprc_hw *hw  = (struct hgprc_hw *)prc_hw->hw;
	sta = hw->STA;
	for(loop = 0;loop < PRC_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->STA = BIT(loop);
			if(prcirq_vector_table[loop] != NULL)
				prcirq_vector_table[loop] (loop,prcirq_dev_table[loop],0);
		}
	}
}



void irq_prc_enable(struct prc_device *p_prc,uint8 mode,uint8 irq){
	struct hgprc *prc_hw = (struct hgprc*)p_prc; 
	struct hgprc_hw *hw  = (struct hgprc_hw *)prc_hw->hw;
	if(mode){
		hw->CON |= BIT(irq+8);
	}else{
		hw->CON &= ~BIT(irq+8);
	}
}


int32 prcirq_register(struct prc_device *p_prc,uint32 irq, prc_irq_hdl isr, uint32 dev_id){
	struct hgprc *prc_hw = (struct hgprc*)p_prc; 	
	struct hgprc_hw *hw  = (struct hgprc_hw *)prc_hw->hw;
	request_irq(prc_hw->irq_num, PRC_IRQHandler_action, p_prc);
	
	irq_prc_enable(p_prc, 1, irq);
	irq_enable(prc_hw->irq_num);
	prcirq_vector_table[irq] = isr;
	prcirq_dev_table[irq] = dev_id;
	hw->STA |= BIT(irq);
	return 0;
}


int32 prcirq_unregister(struct prc_device *p_prc,uint32 irq){
	struct hgprc *prc_hw = (struct hgprc*)p_prc;	
	struct hgprc_hw *hw  = (struct hgprc_hw *)prc_hw->hw;
	irq_prc_enable(p_prc, 0, irq);
	prcirq_vector_table[irq] = NULL;
	prcirq_dev_table[irq] = 0;
	hw->STA |= BIT(irq);
	return 0;
}

static int32 hgprc_init(struct prc_device *p_prc){
	int32  ret_val = RET_OK;
	struct hgprc *prc_hw = (struct hgprc*)p_prc;	
	struct hgprc_hw *hw  = (struct hgprc_hw *)prc_hw->hw;
	prc_hw->opened = 1;
	prc_hw->dsleep = 0;
	hw->CON  = 0;
	hw->SADR = 0;
	hw->YADR = 0;
	hw->UADR = 0;
	hw->VADR = 0;

	return ret_val;
}


int32 hgprc_suspend(struct dev_obj *obj){
	struct hgprc *prc_hw = (struct hgprc*)obj;
	struct hgprc_hw *hw;
	struct hgprc_hw *hw_cfg;
	//确保已经被打开并且休眠过,直接返回
	if(!prc_hw->opened || prc_hw->dsleep)
	{
		return RET_OK;
	}
	prc_hw->dsleep = 1;
	prc_hw->cfg_backup = (uint32 *)os_malloc(sizeof(struct hgprc_hw));
	hw_cfg = (struct hgprc_hw*)prc_hw->cfg_backup;
	hw     = (struct hgprc_hw*)prc_hw->hw;
	hw_cfg->CON 	= hw->CON;
	hw_cfg->STA 	= hw->STA;
	hw_cfg->SADR 	= hw->SADR;
	hw_cfg->YADR 	= hw->YADR;
	hw_cfg->UADR 	= hw->UADR;
	hw_cfg->VADR 	= hw->VADR;
	
	irq_disable(prc_hw->irq_num);
	return 0;
}

int32 hgprc_resume(struct dev_obj *obj){
	struct hgprc *prc_hw = (struct hgprc*)obj;
	struct hgprc_hw *hw;
	struct hgprc_hw *hw_cfg;
	//如果已经被打开并且没有休眠过,直接返回
	if(!prc_hw->opened || !prc_hw->dsleep)
	{
		return RET_OK;
	}
	prc_hw->dsleep = 0;	
	hw_cfg = (struct hgprc_hw*)prc_hw->cfg_backup;
	hw     = (struct hgprc_hw*)prc_hw->hw;
	//hw->CSR0 	= hw_cfg->CSR0;
	hw->CON 	= hw_cfg->CON;
	hw->STA 	= hw_cfg->STA;
	hw->SADR 	= hw_cfg->SADR;
	hw->YADR 	= hw_cfg->YADR;
	hw->UADR 	= hw_cfg->UADR;
	hw->VADR 	= hw_cfg->VADR;
	
	irq_enable(prc_hw->irq_num);
	os_free(prc_hw->cfg_backup);
	return 0;
}


static const struct prc_hal_ops dev_ops = {
	.init                  = hgprc_init,
	.ioctl				   = hgprc_ioctl,
    .request_irq           = prcirq_register,
    .release_irq           = prcirq_unregister,
#ifdef CONFIG_SLEEP	
	.ops.suspend = hgprc_suspend,
	.ops.resume  = hgprc_resume,
#endif     
};



void hgprc_attach(uint32 dev_id, struct hgprc *prc)
{
	prc->dev.dev.ops = (const struct devobj_ops *)&dev_ops;
    irq_disable(prc->irq_num);
    dev_register(dev_id, (struct dev_obj *)prc);
}




