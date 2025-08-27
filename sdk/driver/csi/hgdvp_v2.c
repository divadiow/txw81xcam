#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "devid.h"
#include "dev/csi/hgdvp.h"
#include "osal/string.h"

struct hgdvp_hw
{
	__IO uint32 CON;
	__IO uint32 STA;
};


dvp_irq_hdl dvpirq_vector_table[DVP_IRQ_NUM];
volatile uint32 dvpirq_dev_table[DVP_IRQ_NUM];


static int32 hgdvp_ioctl(struct dvp_device *p_dvp, enum dvp_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    int32  ret_val = RET_OK;	
	//uint32 x_s,y_s,x_e,y_e;
	//uint32 *size_array;
	struct hgdvp *dvp_hw = (struct hgdvp*)p_dvp;	
	struct hgdvp_hw *hw  = (struct hgdvp_hw *)dvp_hw->hw;
	switch(ioctl_cmd){
		case DVP_IOCTL_CMD_SET_FORMAT:
			hw->CON &= ~(BIT(1));
			if(param1 > 1){
				os_printf("set FORMAT mode err\r\n");
				return RET_ERR; 
			}
			hw->CON |= param1<<1;

		break;
        case DVP_IOCTL_CMD_SET_HSYNC_VAILD:
			if(param1)
				hw->CON &= ~BIT(2);		
			else
				hw->CON |= BIT(2);			
		break;
        case DVP_IOCTL_CMD_SET_VSYNC_VAILD:
			if(param1)
				hw->CON &= ~BIT(3);		
			else
				hw->CON |= BIT(3);			
		break;

        case DVP_IOCTL_CMD_SET_DEBOUNCE:
			if(param1){
				hw->CON |= BIT(24);
				hw->CON &=~(BIT(25)|BIT(26));
				hw->CON |=(param2<<25);
			}
			else
				hw->CON &= ~BIT(29);			
		break;

		case DVP_IOCTL_CMD_EX_D5_D6:
			if(param1 == 0)
				hw->CON &= ~BIT(16);		
			else
				hw->CON |= BIT(16);				
		break;

		case DVP_IOCTL_CMD_SET_SCEN:
			if(param1 == 0)
				hw->CON &= ~BIT(4);		
			else
				hw->CON |= BIT(4);				
		break;

		default:
			os_printf("NO DVP IOCTL:%d\r\n",ioctl_cmd);
            ret_val = -ENOTSUPP;
        break;
	}


	return ret_val;
}


void DVP_IRQHandler_action(void *p_dvp)
{
	uint32 sta = 0;
	uint8 loop;
	struct hgdvp *dvp_hw = (struct hgdvp*)p_dvp; 
	struct hgdvp_hw *hw  = (struct hgdvp_hw *)dvp_hw->hw;
	sta = hw->STA;
	for(loop = 0;loop < DVP_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->STA = BIT(loop);
			if(dvpirq_vector_table[loop] != NULL)
				dvpirq_vector_table[loop] (loop,dvpirq_dev_table[loop],0);
		}
	}
}


void irq_dvp_enable(struct dvp_device *p_dvp,uint8 mode,uint8 irq){
	struct hgdvp *dvp_hw = (struct hgdvp*)p_dvp; 
	struct hgdvp_hw *hw  = (struct hgdvp_hw *)dvp_hw->hw;
	if(mode){
		hw->CON |= BIT(irq+8);
	}else{
		hw->CON &= ~BIT(irq+8);
	}
}


int32 dvpirq_register(struct dvp_device *p_dvp,uint32 irq, dvp_irq_hdl isr, uint32 dev_id){
	struct hgdvp *dvp_hw = (struct hgdvp*)p_dvp; 	
	struct hgdvp_hw *hw  = (struct hgdvp_hw *)dvp_hw->hw;
	request_irq(dvp_hw->irq_num, DVP_IRQHandler_action, p_dvp);
	
	irq_dvp_enable(p_dvp, 1, irq);
	dvpirq_vector_table[irq] = isr;
	dvpirq_dev_table[irq] = dev_id;
	hw->STA |= BIT(irq);
	os_printf("dvpirq_register:%d %x  %x\r\n",irq,(uint32)dvpirq_vector_table[irq],(uint32)isr);
	return 0;
}


int32 dvpirq_unregister(struct dvp_device *p_dvp,uint32 irq){
	struct hgdvp *dvp_hw = (struct hgdvp*)p_dvp;	
	struct hgdvp_hw *hw  = (struct hgdvp_hw *)dvp_hw->hw;
	irq_dvp_enable(p_dvp, 0, irq);
	dvpirq_vector_table[irq] = NULL;
	dvpirq_dev_table[irq] = 0;
	hw->STA |= BIT(irq);
	return 0;
}


static int32 hgdvp_open(struct dvp_device *p_dvp){
	struct hgdvp *dvp_hw = (struct hgdvp*)p_dvp;	
	struct hgdvp_hw *hw  = (struct hgdvp_hw *)dvp_hw->hw;
	hw->CON |= BIT(0);
	dvp_hw->opened = 1;
	dvp_hw->dsleep = 0;
	irq_enable(dvp_hw->irq_num);
	return 0;
}

static int32 hgdvp_close(struct dvp_device *p_dvp){
	struct hgdvp *dvp_hw = (struct hgdvp*)p_dvp; 	
	struct hgdvp_hw *hw  = (struct hgdvp_hw *)dvp_hw->hw;
	hw->CON &= ~BIT(0);
	dvp_hw->opened = 0;
	dvp_hw->dsleep = 0;
	irq_disable(dvp_hw->irq_num);
	
	_os_printf("hgdvp_close...............................................................\r\n");
	return 0;
}


int32 hgdvp_suspend(struct dev_obj *obj){
	struct hgdvp *dvp_hw = (struct hgdvp*)obj;
	struct hgdvp_hw *hw;
	struct hgdvp_hw *hw_cfg;
	//确保已经被打开并且休眠过,直接返回
	if(!dvp_hw->opened || dvp_hw->dsleep)
	{
		return RET_OK;
	}
	dvp_hw->dsleep = 1;
	dvp_hw->cfg_backup = (uint32 *)os_malloc(sizeof(struct hgdvp_hw));
	//memcpy((uint8 *)p_jpg->cfg_backup,(uint8 *)jpg_hw->hw,sizeof(struct hgjpg_hw));
	hw_cfg = (struct hgdvp_hw*)dvp_hw->cfg_backup;
	hw     = (struct hgdvp_hw*)dvp_hw->hw;
	//hw_cfg->CSR0 	= hw->CSR0;
	hw_cfg->CON 	= hw->CON;
	hw_cfg->STA 	= hw->STA;
	
	irq_disable(dvp_hw->irq_num);
	return 0;
}

int32 hgdvp_resume(struct dev_obj *obj){
	struct hgdvp *dvp_hw = (struct hgdvp*)obj;
	struct hgdvp_hw *hw;
	struct hgdvp_hw *hw_cfg;
	//如果已经被打开并且没有休眠过,直接返回
	if(!dvp_hw->opened || !dvp_hw->dsleep)
	{
		return RET_OK;
	}
	dvp_hw->dsleep = 0;
	hw_cfg = (struct hgdvp_hw*)dvp_hw->cfg_backup;
	hw     = (struct hgdvp_hw*)dvp_hw->hw;
	//hw->CSR0 	= hw_cfg->CSR0;
	hw->CON 	= hw_cfg->CON;
	hw->STA 	= hw_cfg->STA;
	
	irq_enable(dvp_hw->irq_num);
	os_free(dvp_hw->cfg_backup);
	return 0;
}


static int32 hgdvp_init(struct dvp_device *p_dvp){
//	struct hgdvp *dvp_hw = (struct hgdvp*)p_dvp; 
#ifdef FPGA_SUPPORT
	SYSCTRL->CLK_CON2 |= BIT(31);
	SYSCTRL->SYS_CON1 |= BIT(28);
	SYSCTRL->CLK_CON1 &= ~(BIT(27)|BIT(28));
	SYSCTRL->CLK_CON1 |= BIT(28);		
#else
	SYSCTRL->CLK_CON2 |= BIT(31);
	SYSCTRL->SYS_CON1 |= BIT(28); 
	SYSCTRL->CLK_CON1 &= ~(BIT(27)|BIT(28));
	SYSCTRL->CLK_CON1 |= BIT(28);	
#endif
	pin_func(HG_DVP_DEVID,1);

	return 0;
}

static int32 hgdvp_set_baudrate(struct dvp_device *p_dvp,uint32 baudrate){
	//struct hgdvp_hw *hw  = (struct hgdvp_hw *)dvp_hw->hw;
	uint8 m_cfg = 0;
#if defined(FPGA_SUPPORT)
	m_cfg = 240000000/baudrate - 1;
#else
	if(baudrate != 0)
		m_cfg = (peripheral_clock_get(HG_APB0_PT_DVP))/baudrate - 1;
	else
		m_cfg = 0x3f;
#endif
	
	_os_printf("%s:clock:%d\n",__FUNCTION__,peripheral_clock_get(HG_APB0_PT_DVP));
	SYSCTRL->SYS_KEY = 0x3fac87e4;
	SYSCTRL->CLK_CON3 &= ~(0x3f<<26);		
	SYSCTRL->CLK_CON3 |= ((m_cfg&0x3f)<<26);		
	if(m_cfg&0x40){
		SYSCTRL->SYS_CON3 |= BIT(26);
	}else
	{
		SYSCTRL->SYS_CON3 &= ~BIT(26);
	}

	if(m_cfg&0x80){
		SYSCTRL->SYS_CON3 |= BIT(2);
	}else
	{
		SYSCTRL->SYS_CON3 &= ~BIT(2);
	}
	

	SYSCTRL->SYS_KEY = 0;

		
	//vsync低有效 hsync高有效 
	//hw->CON = BIT(11);				//	


	return 0;
}


static const struct dvp_hal_ops dev_ops = {
    .init        = hgdvp_init,
    .baudrate    = hgdvp_set_baudrate,
    .open        = hgdvp_open,
    .close       = hgdvp_close,
    .ioctl       = hgdvp_ioctl,
    .request_irq = dvpirq_register,
    .release_irq = dvpirq_unregister,
#ifdef CONFIG_SLEEP	
	.ops.suspend = hgdvp_suspend,
	.ops.resume  = hgdvp_resume,
#endif    
};



int32 hgdvp_attach(uint32 dev_id, struct hgdvp *dvp){
    dvp->opened          = 0;
    dvp->use_dma         = 0;
    dvp->irq_hdl                   = NULL;
    //memset(dvp->irq_hdl,0,sizeof(dvp->irq_hdl));
    dvp->irq_data                  = 0;
	//memset(dvp->irq_data,0,sizeof(dvp->irq_data));
	dvp->dev.dev.ops = (const struct devobj_ops *)&dev_ops;

    irq_disable(dvp->irq_num);
    dev_register(dev_id, (struct dev_obj *)dvp);	
	return 0;
}


