/**
 * @file hgadc_v0.c
 * @author bxd
 * @brief AD_KEY
 * @version 
 * TXW80X; TXW81X
 * @date 2023-08-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "hal/adc.h"
#include "dev/adc/hgadc_v0.h"
#include "hgadc_v0_hw.h"


#define ADC_CHANNEL_ENABLE  1
#define ADC_CHANNEL_SUSPEND 2
#define ADC_CHANNEL_DISABLE 3

/* ADC channel type, must start at 0 & unique value & less than 32*/
/* Table */

// I/O class
#define _ADC_CHANNEL_IO_CLASS        0

//RF sensor of temperature
#define _ADC_CHANNEL_RF_TEMPERATURE  1
#define _ADC_CHANNEL_RF_VDDI         2

//Internal voltage
#define _ADC_CHANNEL_PLL_VREF        3
#define _ADC_CHANNEL_RF_VTUNE        4
#define _ADC_CHANNEL_RF_VCO_VDD      5
#define _ADC_CHANNEL_RF_VDD_DIV      6
#define _ADC_CHANNEL_RF_VDD_PFD		 7

/**********************************************************************************/
/*                           LOW LAYER FUNCTION                                   */
/**********************************************************************************/

/* List opreation */
static int32 hgadc_v0_list_insert(adc_channel_node *head_node, adc_channel_node *new_node) {

    adc_channel_node *temp_node = head_node;

    /* find the last node */
    while (temp_node->next) {
        temp_node = temp_node->next;
    }

    temp_node->next = new_node;
    new_node->next  = NULL;

    /* channel amount */
    head_node->channel_amount++;

    os_printf("*** add success: ADC channel cnt = %d, name:%d\n\r", head_node->channel_amount, new_node->data.channel);

    return RET_OK;
}

static int32 hgadc_v0_list_delete(adc_channel_node *head_node, uint32 channel) {

    adc_channel_node *temp_node   = head_node;
    adc_channel_node *delete_node = NULL;

    /* find the node */
    while (temp_node->next) {
        if (channel == temp_node->next->data.channel) {

            delete_node = temp_node->next;
            temp_node->next = temp_node->next->next;
            os_free(delete_node);

            head_node->channel_amount--;

            os_printf("*** delete success: ADC channel cnt = %d\n\r", head_node->channel_amount);
            
            break;
        }
        temp_node = temp_node->next;
    }

    return RET_OK;
}

static int32 hgadc_v0_list_get_by_channel(adc_channel_node *head_node, uint32 channel, adc_channel_node **get_node) {

    adc_channel_node *temp_node = head_node;

    /* find the node */
    while (temp_node->next) {
        if (channel == temp_node->next->data.channel) {
            
            *get_node = temp_node->next;
        
            return RET_OK;
        }
        temp_node = temp_node->next;
    }

    return RET_ERR;
}

static int32 hgadc_v0_list_get_by_index(adc_channel_node *head_node, uint32 index, adc_channel_node **get_node) {

    adc_channel_node *temp_node = head_node;
    uint32 i = 0;

    /* find the node */
    for (i = 0; i < head_node->channel_amount; i++) {
        if ((i == index) && (temp_node->next)) {
            *get_node = temp_node->next;
            return RET_OK;
        }

        temp_node = temp_node->next;
    }

    return RET_ERR;

}

static int32 hgadc_v0_list_check_repetition(adc_channel_node *head_node, uint32 channel) {

    adc_channel_node *temp_node = head_node;
    uint32 i = 0;

    /* find the node which repeated */
    for (i = 0; i < head_node->channel_amount; i++) {
        if (temp_node->next) {
            if (channel == temp_node->next->data.channel) {
                return RET_ERR;
            }

        } 
        temp_node = temp_node->next;
    }

    return RET_OK;
}

static int32 hgadc_v0_list_delete_all(adc_channel_node *head_node, struct hgadc_v0 *dev) {

    adc_channel_node *temp_node = head_node;
    adc_channel_node *delete_node = NULL;

    while (temp_node->next) {
        delete_node = temp_node->next;
        /* disable adc channel */
        delete_node->data.func(dev, delete_node->data.channel, ADC_CHANNEL_DISABLE);
        temp_node->next = temp_node->next->next;
        os_free(delete_node);
        head_node->channel_amount--;

        os_printf("*** delete success: ADC channel cnt = %d\n\r", head_node->channel_amount);
    }

    return RET_OK;
}


static int32 hgadc_v0_list_get_channel_amount(adc_channel_node *head_node) {

    return head_node->channel_amount;
}


static int32 hgadc_v0_switch_hal_adc_ioctl_cmd(enum adc_ioctl_cmd param) {

    switch (param) {
        default:
            return -1;
            break;
    }
}

static int32 hgadc_v0_switch_hal_adc_irq_flag(enum adc_irq_flag param) {

    switch (param) {
        case (ADC_IRQ_FLAG_SAMPLE_DONE):
            return 0;
            break;
        default:
            return -1;
            break;
    }
}


static int32 hgadc_v0_switch_param_channel(uint32 channel) {

    /* I/O class under 0x101*/
    if (channel < 0x101) {
        return _ADC_CHANNEL_IO_CLASS;
    }

    switch (channel) {
        case ADC_CHANNEL_RF_TEMPERATURE:
            return _ADC_CHANNEL_RF_TEMPERATURE;
            break;
        case ADC_CHANNEL_VTUNE:
            return _ADC_CHANNEL_RF_VTUNE;
            break;
        case ADC_CHANNEL_VCO_VDD:
            return _ADC_CHANNEL_RF_VCO_VDD;
            break;
        case ADC_CHANNEL_VDD_DIV:
            return _ADC_CHANNEL_RF_VDD_DIV;
            break;
        case ADC_CHANNEL_VDDI:
            return _ADC_CHANNEL_RF_VDDI;
		case ADC_CHANNEL_VDD_PFD:
			return _ADC_CHANNEL_RF_VDD_PFD;
            break;
        default :
            return RET_ERR;
            break;
    }
}

/* channel configuration */
#ifdef TXW81X

static inline int32 hgadc_v0_adc_channel_txw81x_confirm_io(uint32 channel)
{
	/*!
	 * confirm gpio channel is correct
	 * ADKEY can't support above PC15 and the range from PB0 to PB5
	 */
	 if ( (channel > PC_15) ||
		  ( (channel >= PB_0) && (channel <= PB_5) )
		) {
			os_printf("ADKEY can't support above PC15 and the range from PB0 to PB5\r\n");
			return RET_ERR;
	 }

	return RET_OK;
}

static int32 hgadc_v0_adc_channel_txw81x_io_class(struct hgadc_v0 *dev, uint32 channel, uint32 enable) {

    //BIT(26)  enable channel
    //BIT(27)  suspend channel
    //BIT(28)  disable channel

    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    int32 ret = 0;

    if (ADC_CHANNEL_ENABLE == enable) {

		ret = hgadc_v0_adc_channel_txw81x_confirm_io(channel);
        if (ret == RET_ERR) {
            return RET_ERR;
        }
	
        /* pin config */
        ret = pin_func(dev->dev.dev.dev_id , BIT(26) | channel);
        
        if (ret == RET_ERR) {
            return RET_ERR;
        }


		/*!
		 * ADKEY_CON.bit4 (PA0-PA7、PA15)
		 * N port
		 */
		 if (((channel>=PA_0)&&(channel<=PA_7))||\
		 	 (channel==PA_15)) {
		 	 
			 hw->ADKEY_CON &= ~(0x7F << 4);
			 //N port && enable channel
			 hw->ADKEY_CON |= (1<<24) | (1<<4);

		 } 
		
		 /*!
		  * ADKEY_CON.bit5 (PA8-PA14)
		  * N port
		  */
		 else if (((channel>=PA_8)&&(channel<=PA_14))) {
		 	
			 hw->ADKEY_CON &= ~(0x7F << 4);
			 //N port && enable channel
			 hw->ADKEY_CON |= (1<<24) | (1<<5);
		 }
		 
		/*!
		 * ADKEY_CON.bit6 (PB6-PB15)
		 * N port
		 */
		else if (((channel>=PB_6)&&(channel<=PB_15))) {
			
			hw->ADKEY_CON &= ~(0x7F << 4);
			//N port && enable channel
			hw->ADKEY_CON |= (1<<24) | (1<<6);
		}
		
		/*!
		 * ADKEY_CON.bit8 (PC0-PC5)
		 * P port
		 */
		else if (((channel>=PC_0)&&(channel<=PC_5))) {
			
			hw->ADKEY_CON &= ~((0x7F << 4) | ((1<<24)));
			//P port && enable channel
			hw->ADKEY_CON |= (1<<8);			
		}
		
		/*!
		 * ADKEY_CON.bit9 (PC8-PC15)
		 * P port
		 */
		else if (((channel>=PC_8)&&(channel<=PC_15))) {
			
			hw->ADKEY_CON &= ~((0x7F << 4) | ((1<<24)));
			//P port && enable channel
			hw->ADKEY_CON |= (1<<9);				
		}


        /* Wait stable */
        __NOP();__NOP();__NOP();__NOP();
    } else if (ADC_CHANNEL_SUSPEND == enable) {
        /* pin config */
        ret = pin_func(dev->dev.dev.dev_id , BIT(27) | channel);

        /* disable current channel  */
		/* turn to P port */
		hw->ADKEY_CON &= ~((0x7F << 4) | ((1<<24)));
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);
        
        if (ret == RET_ERR) {
            return RET_ERR;
        }
        
    } else if (ADC_CHANNEL_DISABLE == enable) {
        /* pin config */
        ret = pin_func(dev->dev.dev.dev_id , BIT(28) | channel);

        /* disable current channel */
		/* turn to P port */
		hw->ADKEY_CON &= ~((0x7F << 4) | ((1<<24)));
        
        if (ret == RET_ERR) {
            return RET_ERR;
        }
    }


    return RET_OK;
}
#endif

#ifdef TXW80X
static int32 hgadc_v0_adc_channel_txw80x_io_class(struct hgadc_v0 *dev, uint32 channel, uint32 enable) {

    //BIT(26)  enable channel
    //BIT(27)  suspend channel
    //BIT(28)  disable channel

    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    int32 ret = 0;

    if (ADC_CHANNEL_ENABLE == enable) {
        /* pin config, return gpiox addr */
        ret = pin_func(dev->dev.dev.dev_id , BIT(26) | channel);
        
        if (ret == RET_ERR) {
            return RET_ERR;
        }

        /* GPIO0_EN/GPIO1_EN */
        if (BIT(25) == ret) {
            if (ADKEY1_BASE == dev->hw) {
                //GPIOA
                hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) ) | (0x1 << 8);
            } else {
                //GPIOA
                hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) ) | (0x2 << 8);
            }
        } else {
            //GPIOB / 
            hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) ) | (0x1 << 8);
        }

        /* Wait stable */
        __NOP();__NOP();__NOP();__NOP();
    } else if (ADC_CHANNEL_SUSPEND == enable) {
        /* pin config, return gpiox addr */
        ret = pin_func(dev->dev.dev.dev_id , BIT(27) | channel);

        /* disable current channel */
        hw->ADKEY_CON &= ~ (0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);
        
        if (ret == RET_ERR) {
            return RET_ERR;
        }
        
    } else if (ADC_CHANNEL_DISABLE == enable) {
        /* pin config, return gpiox addr */
        ret = pin_func(dev->dev.dev.dev_id , BIT(28) | channel);

        /* disable current channel */
        hw->ADKEY_CON &= ~ (0xF << 8);
        
        if (ret == RET_ERR) {
            return RET_ERR;
        }
    }


    return RET_OK;
}
#endif

static int32 hgadc_v0_adc_channel_pll_vref(struct hgadc_v0 *dev, uint32 channel, uint32 enable) {

    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 pmu_con5 = 0;

    if (ADC_CHANNEL_ENABLE == enable) {
        /* open pll_vref */
        pmu_con5 = PMU->PMUCON5;
        pmu_con5 = ( pmu_con5 &~ (0xF << 15) ) | (5 << 15);
        pmu_reg_write((uint32)&PMU->PMUCON5, pmu_con5);
        
        /* ATOUT EN */
        hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) ) | (0x8 << 8);
    } else if (ADC_CHANNEL_SUSPEND == enable) {
        /* close pll_vref */
        pmu_con5 = PMU->PMUCON5;
        pmu_con5 &= ~(0xF << 15);
        pmu_reg_write((uint32)&PMU->PMUCON5, pmu_con5);

        /* ATOUT DISABLE */
        hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) );
    } else if (ADC_CHANNEL_DISABLE == enable) {
        /* close pll_vref */
        pmu_con5 = PMU->PMUCON5;
        pmu_con5 &= ~(0xF << 15);
        pmu_reg_write((uint32)&PMU->PMUCON5, pmu_con5);
        
        /* ATOUT DISABLE */
        hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) );
    }

    return RET_OK;

}

#ifdef TXW81X
static int32 hgadc_v0_adc_channel_txw81x_rf_temperature(struct hgadc_v0 *dev, uint32 channel, uint32 enable) {

    #define GPIOC(offset)   (*((uint32 *)(0x40020C00+offset)))
    #define IO_NUM          (8)
    
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

        /* Close the interrupt to protect the RF register opreation */
        mask = disable_irq();


    if (ADC_CHANNEL_ENABLE == enable) {

        /* connect to rf_temperature channel */
        *((uint32 *)(0x40019000 + 0x18))  = ( *((uint32 *)(0x40019000 + 0x18)) & ~(0xf<<27) ) | 0x8<<27;
        //*((uint32 *)(0x40019000 + 0x1C)) |= 0x1;
        

        #if 0
        //IO MODE = analog
        GPIOC(0x00) |=  (3 << (IO_NUM *2));
        
        //IO MODE = analog
        GPIOC(0x50) |=  (1 << (IO_NUM *1));
        
        /*!
         * only PC0-PC5 by hw->ADKEY_CON.BIT8
         * **/
        //RF_TOUT & IO_OUT
        hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0x7F << 4) ) | ((0x1<<10) | (0x1<<9));//PC8 
        #endif

        /* RF_TOUT */
        hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0x7F << 4) ) | (0x1 << 10);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

		/*!
	     * 关闭通路
	     */
		*((uint32 *)(0x40019000 + 0x18))  &= ~(0xf<<27);
		
        hw->ADKEY_CON &= ~(0x7F << 4);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);
    } else if (ADC_CHANNEL_DISABLE == enable) {
		
		/*!
	     * 关闭通路
	     */
		*((uint32 *)(0x40019000 + 0x18))  &= ~(0xf<<27);
		
        hw->ADKEY_CON &= ~(0x7F << 4);
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}

#endif

#ifdef TXW80X
static int32 hgadc_v0_adc_channel_txw80x_rf_temperature(struct hgadc_v0 *dev, uint32 channel, uint32 enable) {

    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

        /* Close the interrupt to protect the RF register opreation */
        mask = disable_irq();


    if (ADC_CHANNEL_ENABLE == enable) {

        /* connect to rf_temperature channel */
        *((uint32 *)(0x40019000 + 0x18))  = ( *((uint32 *)(0x40019000 + 0x18)) & ~(0xf<<27) ) | 0x8<<27;
        //*((uint32 *)(0x40019000 + 0x1C)) |= 0x1; 

        /* RF_TOUT */
        hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

		/*!
	     * 关闭通路
	     */
		*((uint32 *)(0x40019000 + 0x18))  &= ~(0xf<<27);
		
        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);
    } else if (ADC_CHANNEL_DISABLE == enable) {

		/*!
	     * 关闭通路
	     */
		*((uint32 *)(0x40019000 + 0x18))  &= ~(0xf<<27);

        hw->ADKEY_CON &= ~(0xF << 8);
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}
#endif

/*
 * 将VDDI从PB8放出
 */
static void lo_dc_test(struct hgadc_v0 *dev)
{
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;

    #define RFSYS_REG7      (*((uint32 *)0x4001901C))
    #define RFSYS_REG6      (*((uint32 *)0x40019018))
    #define GPIOB(offset)   (*((uint32 *)(0x40020B00+offset)))


//    uint32 vddi_trim_bit       = 0;
//    uint32 tmp1                = 0;
//    
//    uint32 trim_ok = 0;
//    uint32 trim_cmd = 0;
//    
//    uint32 efuse_ts_dat = 0;
//    uint32 efuse_vddi_dat = 0;
//    float vddi_dec_dat = 0;
    

    pmu_reg_write(PMU->PMUCON11, PMU->PMUCON11 | (BIT(9)|BIT(10)));
    pmu_reg_write(PMU->PMUCON11, PMU->PMUCON11 | (BIT(14)));

    //enable mac & rf
    SYSCTRL->SYS_CON1 |= 1<<21;
    SYSCTRL->SYS_CON3 |= 1<<3 | 1<<5;
    //enable rf power
//    RFDIGCAL->SOFT_RFIF_CON |= 0x1;     //软件使能RF_EN
    
	//ADCEN = 1; DAOUTEN = 1 & open interrupt
    hw->ADKEY_CON |= (1 << 0) | (1 << 2);
    //software kict 
    hw->ADKEY_CON &= ~(0xF << 15);
    //clear the DATA, config baud
    hw->ADKEY_DATA = ( hw->ADKEY_DATA &~ (0xFFFF << 16) ) | (0xB3 << 16);

    GPIOB(0x0000) = (0x3 << (8)*2);
    GPIOB(0x0074) = 0;
    GPIOB(0x0078) = 0;
    GPIOB(0x007C) = 0;
    GPIOB(0x0080) = 0;
    GPIOB(0x0010) = 0;
    GPIOB(0x0014) = 0;
    GPIOB(0x0008) = 0;
    GPIOB(0x000C) = 0;
    GPIOB(0x0050)&= ~(1<<8);
    GPIOB(0x0068) = (1<<8);

    RFSYS_REG7 |= 0x1;
    hw->ADKEY_CON = (hw->ADKEY_CON &~ (0xF << 8)) | (5 << 8);


//PB8 output VDDI
    //RF_TOUT to PB8
    RFSYS_REG6 = (RFSYS_REG6 & ~(0xf<<27)) | 0x9<<27 | 0x1<<31; //select VDDI to PB8

}


struct _rf_pmu_dc_for_adc {
    uint8 rf_vref    : 4,
          rf_lo_vref : 4;
    uint8 rf_ibpt    : 4,
          rf_ibct    : 4;
};

#define EFUSE_RF_PMU_SIZE_FOR_ADC    3
#define EFUSE_RF_PMU_OFFSET_FOR_ADC  52

static int32 rf_pmu_dc_efuse_read_for_adc(struct _rf_pmu_dc_for_adc *p_pmu)
{
    uint8 efuse_buf[EFUSE_RF_PMU_SIZE_FOR_ADC];

    sysctrl_efuse_config_and_read(EFUSE_RF_PMU_OFFSET_FOR_ADC, (void *)efuse_buf, EFUSE_RF_PMU_SIZE_FOR_ADC);

    p_pmu->rf_lo_vref = efuse_buf[2] & 0x0F;
    p_pmu->rf_ibpt    = efuse_buf[1] & 0x0F;
    p_pmu->rf_ibct    = (efuse_buf[1]>>4) & 0x0F;
    p_pmu->rf_vref    = (efuse_buf[0]>>4) & 0x0F;
    
    if(efuse_buf[0]) {
        return RET_OK;
    }

    return RET_ERR;
}

///EFUSE RF相关的偏移
#define EFUSE_PACK_OFFSET_FOR_ADC       257

static inline uint8 get_chip_pack_for_adc(void)
{
    uint8 pack = 0;

    sysctrl_efuse_config_and_read(EFUSE_PACK_OFFSET_FOR_ADC, &pack, 1);

    return pack;
}


static int32 hgadc_v0_adc_channel_rf_vddi_config(struct hgadc_v0 *dev)
{
    #define SOFT_RFIF_CON   (*((uint32 *)0x4001d0cc))
    #define RFSYS_REG7      (*((uint32 *)0x4001901C))
    #define RFSYS_REG4      (*((uint32 *)0x40019010))
    #define RFIDLEDIS0      (*((uint32 *)0x40019054))

    struct _rf_pmu_dc_for_adc p_pmu;


    //没开VDDI，则要开VDDI
    if ((0==( (SOFT_RFIF_CON) & BIT(0) ) ) && (dev->rf_vddi_en==0) ) {
        os_printf("Open VDDI!\r\n");
        sysctrl_unlock();

        SYSCTRL->SYS_CON3 &= ~(1 << 3); //RF_POR=0 to reset RFDIG register
        SYSCTRL->SYS_CON3 |= (1 << 3);  //RF_POR=1 to wakeup RFDIG

        (SOFT_RFIF_CON) |= 0x7f<<7; //software control //这里是将RF的关键控制信号切换成软件控制
        (SOFT_RFIF_CON) |= BIT(0); //RF_EN为1

        (SOFT_RFIF_CON) |= BIT(25); // bbgclk is always generated


        if(rf_pmu_dc_efuse_read_for_adc(&p_pmu)==RET_OK) {
            if(get_chip_pack_for_adc() == 0) {   //for QFN58 RFDC config
                p_pmu.rf_vref    = 8;
                p_pmu.rf_ibpt    = 0xa;
                p_pmu.rf_ibct    = 0xa;
                p_pmu.rf_lo_vref = 0x8;
            }

            (RFSYS_REG7) &= ~((0xf<<9)|(0xf<<5)|(0xf<<1));
            (RFSYS_REG7) |= (p_pmu.rf_vref<<9)|(p_pmu.rf_ibpt<<5)|(p_pmu.rf_ibct<<1);

            (RFSYS_REG4) = 0x2a6f7c3c; //LO_VREFCP_VDD=10, LO_VREFLO_VDD=11
            (RFSYS_REG4) &= ~(0xf<<11);
            (RFSYS_REG4) |= (p_pmu.rf_lo_vref<<11);
        }
        else {
            (RFSYS_REG7) = 0x13099f10; //RF_VREF=15
        }

        (RFIDLEDIS0) = 0x02000803;
        //disable status
        (SOFT_RFIF_CON) &= ~ BIT(0); //RF_EN为0

        //enable mac & rf
        SYSCTRL->SYS_CON3 |= 1<<3 | 1<<5;

        dev->rf_vddi_en = 1;
    }

#if 0
    lo_dc_test(dev);
#endif

    return RET_OK;
}


static int32 hgadc_v0_adc_channel_rf_vddi(struct hgadc_v0 *dev, uint32 channel, uint32 enable) {

    #define RFSYS_REG7      (*((uint32 *)0x4001901C))
    #define RFSYS_REG6      (*((uint32 *)0x40019018))


    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();


    if (ADC_CHANNEL_ENABLE == enable) {

        /*
         * 判断VDDI是否已经开启
         */
        hgadc_v0_adc_channel_rf_vddi_config(dev);

        //os_printf("ADC module info: vddi gears: %d\r\n", ((*((uint32 *)(0x40019000 + 0x1C))) & (0xF << 9) ) >> 9);

        /* connect to rf_vddi channel */
        RFSYS_REG6  = ( RFSYS_REG6 & ~(0xf<<27) ) | 0x9<<27 | 0x1<<31;
        
        //vddi to PC0
        //RFSYS_REG7 |= 0x1;
    
        /* RF_TOUT */
        hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

		/*!
	     * 关闭通路
	     */
		RFSYS_REG6  &= ~((0xf<<27) | (0x1<<31));

        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

		/*!
	     * 关闭通路
	     */
		RFSYS_REG6  &= ~((0xf<<27) | (0x1<<31));

        
        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;
}


static int32 hgadc_v0_adc_channel_rf_vtune(struct hgadc_v0 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();

    if (ADC_CHANNEL_ENABLE == enable) {
        
        //LO的模拟测试使能信号开启  select vtune to RF_TOUT (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7)) | 0x1<<7 | 0x4<<8;

        //AUXPEN选择  RF_TOUT放到ADKEY0
        hw->ADKEY_CON = (hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));
        
        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));

        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}

static int32 hgadc_v0_adc_channel_rf_vco_vdd(struct hgadc_v0 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();

    if (ADC_CHANNEL_ENABLE == enable) {
        
        /*ADKEY0采样vco_vdd电压*/	
        (*(uint32 *)(0x40019000)) = ((*(uint32 *)(0x40019000)) & ~(15<< 7)) | 0x1<<7 | 0x0<<8;

        //AUXPEN选择  RF_TOUT放到ADKEY0
        hw->ADKEY_CON = (hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));
        
        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));

        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}


static int32 hgadc_v0_adc_channel_rf_vdd_div(struct hgadc_v0 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();

    if (ADC_CHANNEL_ENABLE == enable) {
        
        /*ADKEY0采样vco_vdd电压*/	
        (*(uint32 *)(0x40019000)) = ((*(uint32 *)(0x40019000)) & ~(15<< 7)) | 0x1<<7 | 0x2<<8;

        //AUXPEN选择  RF_TOUT放到ADKEY0
        hw->ADKEY_CON = (hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));

        
        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));


        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}

static int32 hgadc_v0_adc_channel_rf_vdd_pfd(struct hgadc_v0 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();

    if (ADC_CHANNEL_ENABLE == enable) {
        
        /*ADKEY0采样pfd电压*/	
        (*(uint32 *)(0x40019000)) = ((*(uint32 *)(0x40019000)) & ~(15<< 7)) | 0x0<<7 | 0x1<<8;

        //AUXPEN选择  RF_TOUT放到ADKEY0
        hw->ADKEY_CON = (hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));

        
        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));


        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}



static int32 hgadc_v0_txw80x_raw_data_handle(struct hgadc_v0 *dev, uint32 channel, uint32 *adc_data) {

    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 delay_cnt = 0, cnt = 0;
    //volatile float data_temp1 = 0.0;

    int32  data_temp = 0;
    uint64 __time    = 0;

    /* Sample the pll_vref to verify the ADC data, beacause the VCC is shaking */
    hgadc_v0_adc_channel_rf_vddi(dev, _ADC_CHANNEL_RF_VDDI, ADC_CHANNEL_ENABLE);

    /* Close the interrupt */
    hw->ADKEY_CON &= ~(1 << 20);

    /* Read the div of adc clk to delay after sample done */
    delay_cnt = (hw->ADKEY_DATA >> 16);

    /* Clear the last "done" pending */
    hw->ADKEY_DATA |= (1 << 12);

    //kick start to sample
    hw->ADKEY_CON  |= (1 << 19);

    __time = os_jiffies();
    while(!(hw->ADKEY_CON & (0x1 <<31))) {
        /* 100ms超时时间 */
        if ((os_jiffies() - __time) > 100) {
            /* 清除此时错误状态下的PENDING */
            hw->ADKEY_DATA |= (1 << 12);
            os_printf("*** adc module info: ADC sample err2 !!!!\r\n");
            goto __adc_err;
        }
    }
    hw->ADKEY_DATA |= (1 << 12);
    
    /* Waitting for the ADC circuit ready for the next sample */
    for (cnt  = 0; cnt < delay_cnt; cnt++) {
        __NOP();
    }

    data_temp = (hw->ADKEY_DATA & 0xFFF);
    
    /* 防止采集VDDI值为0，导致下面除数为0，引发CPU异常中断 */
    if (!data_temp) {
        data_temp = dev->refer_vddi_adc_data;
    }
    
    //os_printf("1--->%d\r\n", data_temp);
    //os_printf("2--->%d\r\n", *adc_data);

    if (ADC_CHANNEL_RF_TEMPERATURE == channel) {

        //os_printf("--Debug---->1 :%d\r\n", dev->refer_vddi);
        //os_printf("--Debug---->2 :%d\r\n", (*adc_data << 17) / data_temp);
        //os_printf("--Debug---->3 :%d\r\n", (dev->refer_tsensor << 17) / 4096);
    
        data_temp = ( ( (*adc_data << 12) / data_temp ) - (dev->refer_tsensor) ) * dev->refer_vddi;
    
        //os_printf("3--->%d\r\n", data_temp);
        //os_printf("--Debug---->4 :%d\r\n", ((data_temp * 1000) / 4) >> 25);
        data_temp = ((data_temp * 250 ) >> 20) + 25;

        //data_temp1 = (hw->ADKEY_DATA & 0xFFF);
        //data_temp1 *= 2;
        
        //os_printf("Debug---->1 :%.3f\r\n", (float)dev->refer_vddi / (float)128);
        //os_printf("Debug---->2 :%.3f\r\n", (float)(*adc_data) / data_temp1);
        //os_printf("Debug---->3 :%.3f\r\n", (float)(dev->refer_tsensor / 2) / (float)4096);

        //os_printf("vddi gears: %d\r\n", ((*((uint32 *)(0x40019000 + 0x1C))) & (0xF << 9) ) >> 9);
        //data_temp1 = ( ( (float)(*adc_data) / data_temp1) - ( (float)(dev->refer_tsensor / 2) / (float)4096) ) * (float)((float)dev->refer_vddi / (float)128);
        //os_printf("4--->%.3f\r\n", data_temp1);
        //data_temp1 = ((data_temp1) / (float)(0.004)) + 25;
        //os_printf("temp data: %d\r\n", data_temp);
        //os_printf("temp1 data :%f\r\n", data_temp1);

        
        *adc_data = data_temp;

  }else if ((ADC_CHANNEL_VCO_VDD == channel) || (ADC_CHANNEL_VDD_DIV == channel)) {
        //os_printf("1----> = %d\r\n", data_temp);  //A'
        //os_printf("2----> = %d\r\n", *adc_data);  //B'

        //返回的是：电压值*256*256
        data_temp = ((((dev->refer_vddi) * (*adc_data)) * 256) / data_temp);

        *adc_data = data_temp;
        
        //os_printf("3----> = %drn", data_temp);
  }else {
        //data_temp = ( ( (*adc_data) * 1464 ) << 15) / data_temp;

        //os_printf("1----> = %d\r\n", data_temp);  //A'
        //os_printf("2----> = %d\r\n", *adc_data);  //B'

        
        data_temp = ( ( (*adc_data) * dev->refer_vddi_adc_data) << 9) / data_temp;
        //os_printf("3----> = %d\r\n", data_temp);

        *adc_data = data_temp >> 9;

        if (*adc_data >= 4095) {
            *adc_data = 4095;
        }
        
        //os_printf("4----> = %d\r\n", *adc_data);
   }

__adc_err:

    hgadc_v0_adc_channel_rf_vddi(dev, _ADC_CHANNEL_RF_VDDI, ADC_CHANNEL_DISABLE);

    /* Open the interrupt */
    hw->ADKEY_CON |= (1 << 20);

    return RET_OK;

}


void hgadc_v0_txw81x_raw_data_handle(struct hgadc_v0 *dev, uint32 channel, uint32 *adc_data)
{
    #define RAW_DATA_DBG    (0)
    
    volatile int32  data_temp = 0;
      

      if (ADC_CHANNEL_RF_TEMPERATURE == channel) {


	  	/*!
	  	 * formula:
	  	 * { {2.7/4096} * diff } / 0.004
	  	 */
            
        #if RAW_DATA_DBG
            os_printf("tsensor  ldo-> = %d\r\n", dev->refer_adda_vref);
            os_printf("tsensor  tsd-> = %d\r\n", dev->refer_tsensor);
            os_printf("tsensor  raw-> = %d\r\n", *adc_data);
        #endif

        //diff = tsensor - (effuse_tsensor)
        data_temp = *adc_data - (dev->refer_tsensor);
        #if RAW_DATA_DBG
            os_printf("tsensor diff-> = %d\r\n", data_temp);
        #endif

        //{(2.7*1024)*diff*1000}
        data_temp = dev->refer_adda_vref * data_temp * 1000;
        #if RAW_DATA_DBG
            os_printf("tsensor 1----> = %d\r\n", data_temp);
        #endif

        // {(2.7*1024)*diff*1000} / {4096*4}
        data_temp = data_temp >> 14;
        #if RAW_DATA_DBG
            os_printf("tsensor 2----> = %d\r\n", data_temp);
        #endif

        // {(2.7*1024)*diff*1000} / {4096*4} / {1024}
        data_temp = data_temp >> 10;
        #if RAW_DATA_DBG
            os_printf("tsensor 3----> = %d\r\n", data_temp);
        #endif

        //室温25度 + 5度（待定）
        data_temp = data_temp + 30;
        #if RAW_DATA_DBG
            os_printf("tsensor 4----> = %d\r\n", data_temp);
        #endif
        
        *adc_data = data_temp;
    
    }else if ((ADC_CHANNEL_VCO_VDD == channel) || (ADC_CHANNEL_VDD_DIV == channel)) {
          //os_printf("1----> = %d\r\n", data_temp);  //A'
          //os_printf("2----> = %d\r\n", *adc_data);  //B'
    
          //返回的是：电压值*256*256
//          data_temp = ((((dev->refer_vddi) * (*adc_data)) * 256) / data_temp);
//    
//          *adc_data = data_temp;
          
          //os_printf("3----> = %drn", data_temp);
    } else if ((ADC_CHANNEL_VTUNE == channel) || (ADC_CHANNEL_VDD_PFD == channel)) {
		/*!
	  	 * formula:
	  	 * { {2.7*65536} * adc_vtune } / 4096
	  	 */
		
	
   		*adc_data = (((*adc_data) * (dev->refer_adda_vref) * (64))/4096);	
    } else {

		/*!
		 * fromulation: (adda_ref / 2.7) * raw_data           
		 */

		//(adda_ref*10 / 27)
		data_temp = ((dev->refer_adda_vref * 10) / (27));

		//(adda_ref*10 / 27) * raw_data
		data_temp = data_temp * (*adc_data);

		//((adda_ref*10 / 27) * raw_data) / 1024
		*adc_data = (data_temp) >> 10;

        if (*adc_data >= 4095) {
            *adc_data = 4095;
        }

	}


}


void hgadc_v0_txw80x_open_data_handler(struct hgadc_v0 *dev)
{
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 i = 0;
    uint32 _refer_vddi    = 0;
    uint32 _refer_tsensor = 0;
    uint32 _vddi_gears    = 0;
    uint32 _rfsys_reg7    = 0;


    _vddi_gears     = sysctrl_efuse_vddi_gears_get();
    _refer_vddi     = sysctrl_efuse_vddi_get();
    _refer_tsensor  = sysctrl_efuse_tsensor_get();

    /* calibrate vddi to 1.18v by efuse vaule of vddi gears */
    if (_vddi_gears) {
        os_printf("*** ADC module info: vddi gears value = 0x%x\r\n", _vddi_gears);
        _rfsys_reg7 = *((uint32 *)(0x40019000 + 0x1C));
        _rfsys_reg7 = ( _rfsys_reg7 &~ (0xF << 9) ) | (_vddi_gears << 9);
        *((uint32 *)(0x40019000 + 0x1C)) = _rfsys_reg7;
    }

    if (0 == _refer_vddi) {
        os_printf("*** ADC module info: vddi don't calibrate!!, vddi will use default: 1.27v adc dat=1580.\r\n");
        //(1+0.27)*256
        dev->refer_vddi = (256 + 69);

        dev->refer_vddi_adc_data = 1580;
    } else {
        //(1+vddi from efuse) * 256, \"vddi from efuse\" already multiply by 256
        os_printf("*** ADC module info: ideal:1464, (1+.)*256, note: .*256\r\n");
        os_printf("*** ADC module info: vddi calibrated value = 0x%x\r\n", _refer_vddi);
        dev->refer_vddi = 256 + _refer_vddi;

        dev->refer_vddi_adc_data = ( ( (dev->refer_vddi) * 10) / 33 ) * 4095;
        dev->refer_vddi_adc_data = dev->refer_vddi_adc_data >> 8;
        os_printf("*** ADC module info: vddi adc_data = %d(D)\r\n", dev->refer_vddi_adc_data);
    }

    if (0 == _refer_tsensor) {
        //efuse: ( (vptat_adc_data/vddi_adc_data) / 2 ) * 4096
        os_printf("*** ADC module info: ideal:2238+-, ( (vptat_adc_data/vddi_adc_data) / 2 ) * 4096\r\n");
        os_printf("*** ADC module info: tsensor don't calibrate!!, tsensor will use default: 4012\r\n");
        dev->refer_tsensor = 4012;
    } else {
        //efuse: ( (vptat_adc_data/vddi_adc_data) / 2 ) * 4096"
        os_printf("*** ADC module info: ideal:2238+-, ( (vptat_adc_data/vddi_adc_data) / 2 ) * 4096\r\n");
        os_printf("*** ADC module info: tsensor calibrated value = 0x%x\r\n", _refer_tsensor);
        dev->refer_tsensor = _refer_tsensor * 2;
    }


    //ADCEN = 1; DAOUTEN = 1 & open interrupt
    hw->ADKEY_CON |= (1 << 0) | (1 << 2) | BIT(20);
    
    //software kict 
    hw->ADKEY_CON &= ~(0xF << 15);

    //clear the DATA, config baud
    hw->ADKEY_DATA = ( hw->ADKEY_DATA &~ (0xFFFF << 16) ) | (0xB3 << 16);

    /* Wait for ADC init done */
    for (i = 0; i < 200; i++) {
        __NOP();
    };

    /* 硬件要求：开启就kick一下和重新开关一下，防止意外复位动作将ADC搞挂 */
    hw->ADKEY_CON |= (1 << 19);

    //等30个ADC时钟
    for (i = 0; i < ((hw->ADKEY_DATA >> 16) * 30); i++) {
        __NOP();
    };

    hw->ADKEY_CON &= ~ BIT(0);
    hw->ADKEY_CON |= BIT(0);

    /* Wait for ADC init done */
    for (i = 0; i < 200; i++) {
        __NOP();
    };


}

void hgadc_v0_txw81x_open_data_handler(struct hgadc_v0 *dev)
{
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    uint32 i = 0;
    uint32 div = 0;

    dev->refer_adda_vref = ((2*1024) + sysctrl_efuse_adda_vref_get());
    dev->refer_tsensor   = sysctrl_efuse_tsensor_get();

    //open power for ldo 2.7v
    pmu_reg_write((uint32)&PMU->PMUCON11, PMU->PMUCON11 | (BIT(27)));


    //ADCEN = 1; DAOUTEN = 1 & open interrupt
    hw->ADKEY_CON |= (1 << 0) | (1 << 2) | BIT(20);

    //reference choose LDO 2.7v
    hw->ADKEY_CON |= BIT(23);

    //open FILTER
    //hw->ADKEY_CON |= BIT(25);
    
    //software kict 
    hw->ADKEY_CON &= ~(0xF << 15);

    //clear the DATA, config baud

    div = (peripheral_clock_get(HG_APB1_PT_ADKEY)/ (1000000)) - 1;
    if (div<2) {
        div = 2;
    }
    os_printf("ADKEY baud:%d\r\n", div);
    hw->ADKEY_DATA = ( hw->ADKEY_DATA &~ (0xFFFF << 16) ) | (div << 16);


    hw->ADKEY_CON &= ~ BIT(0);
    hw->ADKEY_CON |= BIT(0);

    /* Wait for ADC init done */
    for (i = 0; i < 200; i++) {
        __NOP();
    };

}


/**********************************************************************************/
/*                             ATTCH FUNCTION                                     */
/**********************************************************************************/

static int32 hgadc_v0_open(struct adc_device *adc) {

    struct hgadc_v0    *dev = (struct hgadc_v0 *)adc;
    uint32 mask = 0;

    mask = disable_irq();

    if (dev->opened) {
        /* Enable interrupt */
        enable_irq(mask);
        return -EBUSY;
    }

#if TXW80X
    hgadc_v0_txw80x_open_data_handler(dev);
#endif

#if TXW81X
    hgadc_v0_txw81x_open_data_handler(dev);
#endif

    irq_enable(dev->irq_num);

    /* init head node */
    dev->head_node.channel_amount = 0;
    dev->head_node.data.channel   = -1;
    dev->head_node.data.func      = NULL;
    dev->head_node.next           = NULL;

    dev->opened     = 1;
    dev->irq_en     = 0;
    dev->rf_vddi_en = 0;
    os_printf("*** open ADC success!\n\r");

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;
}

static int32 hgadc_v0_close(struct adc_device *adc) {

    uint32 mask = 0;
    struct hgadc_v0    *dev = (struct hgadc_v0 *)adc;
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;

    if (!dev->opened) {
        return RET_OK;
    }

    /* Close the interrupt to protect the list opreation */
    mask = disable_irq();

    /* keep ADC open, when channel is still in use */
    if (hgadc_v0_list_get_channel_amount(&dev->head_node)) {
        
        /* Enable interrupt */
        enable_irq(mask);

        return RET_OK;
    }

    hgadc_v0_list_delete_all(&dev->head_node, dev);

    /* Enable interrupt */
    enable_irq(mask);

    irq_disable(dev->irq_num);
    dev->head_node.channel_amount = 0;
    dev->head_node.next           = NULL;
    dev->head_node.data.channel   = -1;
    dev->head_node.data.func      = NULL;
    hw->ADKEY_CON      = 0;
    hw->ADKEY_DATA     = 0;
    dev->refer_tsensor = 0;
    dev->refer_vddi    = 0;
    dev->irq_en        = 0;
    dev->opened        = 0;
    dev->rf_vddi_en    = 0;
    dev->refer_vddi_adc_data = 0;

    return RET_OK;
}


static int32 hgadc_v0_add_channel(struct adc_device *adc, uint32 channel) {

    int32  _class = 0;
    uint32 mask = 0;
    struct hgadc_v0 *dev = (struct hgadc_v0 *)adc;
    adc_channel_node *new_node = NULL;

    if (!dev->opened) {
        return RET_ERR;
    }

    /* ADKEY1 P channel cant't support current adc sample channel, limited by hardware */
    if ((ADKEY1_BASE == dev->hw) && (PA_15 < channel) && (channel < 0x100)) {
        os_printf("*** ADC module info: ADKEY1 can't support the %d channel!!!\r\n", channel);
        return RET_ERR;
    }

    /* Close the interrupt to protect the list opreation */
    mask = disable_irq();

    /* Check for the channel which repeated  */
    if (RET_ERR == hgadc_v0_list_check_repetition(&dev->head_node, channel)) {
        os_printf("*** ADC module info: ADC channel repeat!!!\n\r");
        enable_irq(mask);
        return RET_OK;
    }

    _class = hgadc_v0_switch_param_channel(channel);

    if (RET_ERR == _class) {
        enable_irq(mask);
        return RET_ERR;
    }

    /* save the adkey configuration by channel */
    switch (_class) {
        case _ADC_CHANNEL_IO_CLASS:	
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }

			#ifdef TXW80X
            new_node->data.func    = hgadc_v0_adc_channel_txw80x_io_class;
			#endif

			#ifdef TXW81X
			new_node->data.func    = hgadc_v0_adc_channel_txw81x_io_class;
			#endif
			
            new_node->data.channel = channel;
            new_node->next         = NULL;
            hgadc_v0_list_insert(&dev->head_node, new_node);
            break;
        case _ADC_CHANNEL_RF_TEMPERATURE:
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }

			#ifdef TXW80X 
			new_node->data.func    = hgadc_v0_adc_channel_txw80x_rf_temperature;
			#endif

			#ifdef TXW81X 
			new_node->data.func    = hgadc_v0_adc_channel_txw81x_rf_temperature;
			#endif
            
            new_node->data.channel = channel;
            new_node->next         = NULL;
            hgadc_v0_list_insert(&dev->head_node, new_node);
            break;
         case _ADC_CHANNEL_RF_VTUNE:
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = hgadc_v0_adc_channel_rf_vtune;
            new_node->data.channel = channel;
            new_node->next         = NULL;
            hgadc_v0_list_insert(&dev->head_node, new_node);
            break;
         case _ADC_CHANNEL_RF_VCO_VDD:
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = hgadc_v0_adc_channel_rf_vco_vdd;
            new_node->data.channel = channel;
            new_node->next         = NULL;
            hgadc_v0_list_insert(&dev->head_node, new_node);
            break;
         case _ADC_CHANNEL_RF_VDD_DIV:
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = hgadc_v0_adc_channel_rf_vdd_div;
            new_node->data.channel = channel;
            new_node->next         = NULL;
            hgadc_v0_list_insert(&dev->head_node, new_node);
            break;
		 case _ADC_CHANNEL_RF_VDDI:
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = hgadc_v0_adc_channel_rf_vddi;
            new_node->data.channel = channel;
            new_node->next         = NULL;
            hgadc_v0_list_insert(&dev->head_node, new_node);
            break;
		case _ADC_CHANNEL_RF_VDD_PFD:
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = hgadc_v0_adc_channel_rf_vdd_pfd;
            new_node->data.channel = channel;
            new_node->next         = NULL;
            hgadc_v0_list_insert(&dev->head_node, new_node);
            break;
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}

static int32 hgadc_v0_delete_channel(struct adc_device *adc, uint32 channel) {

    uint32 mask = 0;
    adc_channel_node *get_node = NULL;
    struct hgadc_v0 *dev = (struct hgadc_v0 *)adc;

    if (!dev->opened) {
        return RET_ERR;
    }

    /* Close the interrupt to protect the list opreation */
    mask = disable_irq();

    if (RET_ERR == hgadc_v0_list_get_by_channel(&dev->head_node, channel, &get_node)) {
        os_printf("*** ADC module info: Delete func: No this ADC channel!!!\n\r");
        enable_irq(mask);
        return RET_ERR;
    }

    get_node->data.func(dev, channel, ADC_CHANNEL_DISABLE);

    hgadc_v0_list_delete(&dev->head_node, channel);

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;
}

static int32 hgadc_v0_get_value(struct adc_device *adc, uint32 channel, uint32 *raw_data) {

    struct hgadc_v0    *dev = (struct hgadc_v0 *)adc;
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;
    adc_channel_node *get_node = NULL;
    uint32 mask = 0;
    uint32 delay_cnt = 0, cnt = 0;

    if (!dev->opened) {
        return RET_ERR;
    }

    os_mutex_lock(&dev->adc_lock, osWaitForever);

    if (RET_ERR == hgadc_v0_list_get_by_channel(&dev->head_node, channel, &get_node)) {
        os_printf("*** ADC module info: get_value func: No this ADC channel!!!\n\r");
        os_mutex_unlock(&dev->adc_lock);
        return RET_ERR;
    }

    /* Read the div of adc clk to delay after sample done */
    delay_cnt = (hw->ADKEY_DATA >> 16);

    /* config current channel */
    if (get_node->data.func) {
        get_node->data.func(dev, channel, ADC_CHANNEL_ENABLE);
    } else {
        os_mutex_unlock(&dev->adc_lock);
        return RET_ERR;
    }

    //防止kick的时候，没有done。但系统复位了，导致adc重新open无法工作
    mask = disable_irq();

    /* Clear the last "done" pending */
    LL_ADKEY_CLEAR_DONE_PENDING(hw);

    //kick start to sample
    LL_ADKEY_SOTF_KICK(hw);

    /* Waitting for the ADC circuit ready for the next sample */
    for (cnt  = 0; cnt < (delay_cnt + 300); cnt++) {
        __NOP();
    }

    enable_irq(mask);

    /* 超时5s */
    if (os_sema_down(&dev->adc_done, 5000) <= 0){
        /* Clear the last "done" pending */
        LL_ADKEY_CLEAR_DONE_PENDING(hw);
        os_printf("*** adc module info: ADC sample err1 !!!!");
    }
    //os_printf("***  down!!\n\r");

    *raw_data = LL_ADKEY_GET_DATA(hw);

    //os_printf("** %d channel raw_data: %d\n\r", get_node->data.channel, *raw_data);

    get_node->data.func(dev, channel, ADC_CHANNEL_SUSPEND);

#if TXW80X
    hgadc_v0_txw80x_raw_data_handle(dev, channel, raw_data);
#endif

#if TXW81X
    hgadc_v0_txw81x_raw_data_handle(dev, channel, raw_data);
#endif

    if (dev->irq_en && dev->irq_hdl) {
        dev->irq_hdl(ADC_IRQ_FLAG_SAMPLE_DONE, get_node->data.channel, *raw_data);
    }

    os_mutex_unlock(&dev->adc_lock);

    return RET_OK;
}

static int32 hgadc_v0_ioctl(struct adc_device *adc, enum adc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2) {

    return RET_OK;
}

static void hgadc_v0_irq_handler(void *data) {

    struct hgadc_v0    *dev = (struct hgadc_v0 *)data;
    struct hgadc_v0_hw *hw  = (struct hgadc_v0_hw *)dev->hw;

    //os_printf("***  interrupt!!\n\r");

    if ((LL_ADKEY_GET_IRQ_EN_SAMPLE_DONE(hw)) && (LL_ADKEY_GET_IRQ_EN_SAMPLE_DONE(hw))) {
        LL_ADKEY_CLEAR_DONE_PENDING(hw);

        os_sema_up(&dev->adc_done);
        //os_printf("***  up!!\n\r");
    }
}

static int32 hgadc_v0_request_irq(struct adc_device *adc, enum adc_irq_flag irq_flag, adc_irq_hdl irq_hdl, uint32 irq_data) {

    struct hgadc_v0    *dev = (struct hgadc_v0 *)adc;
    
    dev->irq_hdl  = irq_hdl;
    dev->irq_data = irq_data;

    if (irq_flag & ADC_IRQ_FLAG_SAMPLE_DONE) {
         dev->irq_en = 1;
    }

    return RET_OK;
}

static int32 hgadc_v0_release_irq(struct adc_device *adc, enum adc_irq_flag irq_flag) {

    struct hgadc_v0 *dev = (struct hgadc_v0 *)adc;

    if (irq_flag & ADC_IRQ_FLAG_SAMPLE_DONE) {
        dev->irq_en = 0;
    }

    return RET_OK;
}

static const struct adc_hal_ops adcops = {
    .open           = hgadc_v0_open,
    .close          = hgadc_v0_close,
    .add_channel    = hgadc_v0_add_channel,
    .delete_channel = hgadc_v0_delete_channel,
    .get_value      = hgadc_v0_get_value,
    .ioctl          = hgadc_v0_ioctl,
    .request_irq    = hgadc_v0_request_irq,
    .release_irq    = hgadc_v0_release_irq,
};

int32 hgadc_v0_attach(uint32 dev_id, struct hgadc_v0 *adc) {

    struct hgadc_v0_hw *hw = (struct hgadc_v0_hw *)adc->hw;

    adc->opened             = 0;
    adc->irq_en             = 0;
    adc->refer_vddi         = 0;
    adc->refer_tsensor      = 0;
    adc->refer_adda_vref    = 0;
    adc->refer_vddi_adc_data= 0;
    adc->rf_vddi_en         = 0;
    adc->irq_hdl            = NULL;
    adc->irq_data           = 0;
    adc->dev.dev.ops        = (const struct devobj_ops *)&adcops;

    os_mutex_init(&adc->adc_lock);
    os_sema_init(&adc->adc_done, 0);

    request_irq(adc->irq_num, hgadc_v0_irq_handler, adc);
    hw->ADKEY_CON |= BIT(20);
    irq_enable(adc->irq_num);
    dev_register(dev_id, (struct dev_obj *)adc);
    
    return RET_OK;
}
