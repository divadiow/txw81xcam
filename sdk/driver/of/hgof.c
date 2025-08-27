#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "devid.h"
#include "dev/of/hgof.h"
#include "osal/string.h"

struct hgof_hw
{
	__IO uint32 CTL;
	__IO uint32 ADDR0;
    __IO uint32 ADDR1;      
    __IO uint32 ACC;
    __IO uint32 INFO;
};

static int32 hgof_ioctl(struct of_device *p_of, enum of_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2){
    int32  ret_val = RET_OK;
	struct hgof *of_hw = (struct hgof*)p_of;	
	struct hgof_hw *hw  = (struct hgof_hw *)of_hw->hw;
	switch(ioctl_cmd){
		case OF_INSTR_MODE:
			hw->CTL &= ~(3<<19);
			hw->CTL |= (param1<<19);
		break;
		case CAL_PIXEL_NUM:
			hw->CTL &= ~(3<<17);
			hw->CTL |= (param1<<17);
		break;		
		case CAL_ROW_NUM:
			hw->CTL &= ~(0x1f<<12);
			hw->CTL |= (param1<<12);
		break;	
		case SET_ROW_SIZE:
			hw->CTL &= ~(0xfff<<0);
			hw->CTL |= (param1<<0);
		break;	
		case SET_OF_ADDR:
			hw->ADDR0 = param1;
			hw->ADDR1 = param2;
		break;	

		case GET_CAL_ACC:
			ret_val = hw->ACC;
		break;
		
		default:
			os_printf("NO OF IOCTL:%d\r\n",ioctl_cmd);
            ret_val = -ENOTSUPP;
        break;
	}
	return ret_val;
}

static int32 hgof_init(struct of_device *p_of){
    int32  ret_val = RET_OK;
	struct hgof *of_hw = (struct hgof*)p_of;	
	struct hgof_hw *hw  = (struct hgof_hw *)of_hw->hw;
	hw->CTL   = 0;
	hw->ADDR0 = 0;
	hw->ADDR1 = 0;
	return ret_val;
}	

static const struct of_hal_ops dev_ops = {
	.init                  = hgof_init,
	.ioctl				   = hgof_ioctl,
};

void hgof_attach(uint32 dev_id, struct hgof *of)
{
	of->dev.dev.ops = (const struct devobj_ops *)&dev_ops;
    irq_disable(of->irq_num);
    dev_register(dev_id, (struct dev_obj *)of);
}

