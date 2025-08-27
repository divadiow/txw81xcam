/**
 * @file hgi2s_v0.c
 * @author bxd
 * @brief iis
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
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "hal/i2s.h"
#include "dev/i2s/hgi2s_v0.h"
#include "hgi2s_v0_hw.h"


#define __OVER_SAMPPLE_RATE (512)

/**********************************************************************************/
/*                        I2S LOW LAYER FUNCTION                                  */
/**********************************************************************************/
static int32 hgi2s_v0_switch_hal_i2s_mode(enum i2s_mode mode) {

    switch (mode) {
        case (I2S_MODE_MASTER   ):
            return 0;
            break;
        case (I2S_MODE_SLAVE    ):
            return 1;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgi2s_v0_switch_hal_i2s_channel(enum i2s_channel channel) {

    switch (channel) {
        case (I2S_CHANNEL_MONO  ):
            return 1;
            break;
        case (I2S_CHANNEL_STEREO):
            return 0;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgi2s_v0_switch_hal_i2s_sample_bits(enum i2s_sample_bits bits) {

    switch (bits) {
        case (I2S_SAMPLE_BITS_8BITS ):
            return 8;
            break;
        case (I2S_SAMPLE_BITS_16BITS):
            return 16;
            break;
        case (I2S_SAMPLE_BITS_24BITS):
            return 24;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgi2s_v0_switch_hal_i2s_data_fmt(enum i2s_data_fmt data_fmt) {

    switch (data_fmt) {
        case (I2S_DATA_FMT_I2S):
            return 0;
            break;
        case (I2S_DATA_FMT_LSB):
            return 1;
            break;
        case (I2S_DATA_FMT_MSB):
            return 2;
            break;
        case (I2S_DATA_FMT_PCM):
            return 3;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgi2s_v0_switch_hal_i2s_sample_freq(enum i2s_sample_freq frequency) {

    switch (frequency) {
        case (I2S_SAMPLE_FREQ_8K):
                return 8000;
            break;
            case (I2S_SAMPLE_FREQ_16K):
                return 16000;
            break;
        case (I2S_SAMPLE_FREQ_44_1K):
            return 44100;
            break;
        case (I2S_SAMPLE_FREQ_48K  ):
            return 48000;
            break;
        default:
            return -1;
            break;
    }
}

static inline void hgi2s_v0_set_mclk(struct hgi2s_v0_hw *p_i2s, uint32 freq) {
    uint32 mclk_div = 0;

    //IIS0
    if (HG_IIS0_BASE == (uint32)p_i2s) {
        SYSCTRL_REG_OPT(
            SYSCTRL->SYS_CON0 = (SYSCTRL->SYS_CON0 & ~(0x03 << 10)) | (1 << 10);
        );
        
        #if 1
        /* i2s module clk use pll0 & pll1 */
        mclk_div = ((peripheral_clock_get(HG_APB0_PT_IIS0)+((freq*__OVER_SAMPPLE_RATE)/2)) / (freq*__OVER_SAMPPLE_RATE));
        #endif

        #if 0
        /* FPGA: 96MHz */
        mclk_div = (96000000 / (freq*256)); 
        #endif
    
        SYSCTRL_REG_OPT(
            SYSCTRL->CLK_CON0 = (SYSCTRL->CLK_CON0 & ~(0x7f << 16)) | ((mclk_div-1) << 16);
        );
    } 
    //IIS1
    else if(HG_IIS1_BASE == (uint32)p_i2s){
        SYSCTRL_REG_OPT(
            SYSCTRL->SYS_CON0 = (SYSCTRL->SYS_CON0 & ~(0x03 << 30)) | (1 << 30);
        );
        
        #if 1
        /* i2s module clk use pll0 & pll1 */
        mclk_div = (peripheral_clock_get(HG_APB0_PT_IIS1) / (freq*256));
        #endif

        #if 0
        /* FPGA: 96MHz */
        mclk_div = (96000000 / (freq*256)); 
        #endif
        
        SYSCTRL_REG_OPT(
            SYSCTRL->CLK_CON0 = (SYSCTRL->CLK_CON0 & ~(0x7f << 23)) | ((mclk_div-1) << 23);
        );
    }
}

static inline void hgi2s_v0_enable(struct hgi2s_v0_hw *p_i2s) {
    p_i2s->CON |= LL_I2S_CON_ENABLE(1);
}

static inline void hgi2s_v0_disable(struct hgi2s_v0_hw *p_i2s) {
    p_i2s->CON &= ~ LL_I2S_CON_ENABLE(1);
}

static inline void hgi2s_v0_set_tx(struct hgi2s_v0_hw *p_i2s) {
    p_i2s->CON |= LL_I2S_CON_WORKMODE(1);
}

static inline void hgi2s_v0_set_rx(struct hgi2s_v0_hw *p_i2s) {
    p_i2s->CON &= ~ LL_I2S_CON_WORKMODE(1);
}

static int32 hgi2s_v0_set_wsclk_pol(struct hgi2s_v0_hw *p_i2s, uint32 value) {

    hgi2s_v0_disable(p_i2s);

    if (value) {
        p_i2s->CON |=   LL_I2S_CON_WSPOL(1);
    } else {
        p_i2s->CON &= ~ LL_I2S_CON_WSPOL(1);
    }

    return RET_OK;
}

static int32 hgi2s_v0_set_sample_bits(struct hgi2s_v0_hw *p_i2s, enum i2s_sample_bits bits) {

    int32 i2s_sample_bits_to_reg = 0;

    i2s_sample_bits_to_reg = hgi2s_v0_switch_hal_i2s_sample_bits(bits);

    if ((-1) == i2s_sample_bits_to_reg) {
        return RET_ERR;
    }
    
    hgi2s_v0_disable(p_i2s);

    p_i2s->BIT_SET = i2s_sample_bits_to_reg;
    p_i2s->WS_CON  = i2s_sample_bits_to_reg;
    p_i2s->BAUD    = (64 / i2s_sample_bits_to_reg) - 1;
    
    return RET_OK;
}

static int32 hgi2s_v0_set_channel(struct hgi2s_v0_hw *p_i2s, enum i2s_channel channel) {

    int32 i2s_channel_to_reg = 0;

    i2s_channel_to_reg = hgi2s_v0_switch_hal_i2s_channel(channel);

    if ((-1) == i2s_channel_to_reg) {
        return RET_ERR;
    }

    hgi2s_v0_disable(p_i2s);

    p_i2s->CON = (p_i2s->CON &~ LL_I2S_CON_MONO(0x3)) | LL_I2S_CON_MONO(i2s_channel_to_reg);

    return RET_OK;
}

static int32 hgi2s_v0_set_data_fmt(struct hgi2s_v0_hw *p_i2s, enum i2s_data_fmt data_fmt) {

    int32 i2s_data_fmt_to_reg = 0;

    i2s_data_fmt_to_reg = hgi2s_v0_switch_hal_i2s_data_fmt(data_fmt);

    if ((-1) == i2s_data_fmt_to_reg) {
        return RET_ERR;
    }

    hgi2s_v0_disable(p_i2s);

    p_i2s->CON = (p_i2s->CON &~ LL_I2S_CON_FRMT(0x3)) | LL_I2S_CON_FRMT(i2s_data_fmt_to_reg);

    return RET_OK;
}

static int32 hgi2s_v0_set_debounce(struct hgi2s_v0_hw *p_i2s, uint32 enable) {

    hgi2s_v0_disable(p_i2s);

    if (!enable) {
        p_i2s->CON |=   LL_I2S_CON_BCLKDBSBPS(1) | LL_I2S_CON_RXDBSBPS(1) | LL_I2S_CON_WSCLKDBSBPS(1);
    } else {
        p_i2s->CON &= ~(LL_I2S_CON_BCLKDBSBPS(1) | LL_I2S_CON_RXDBSBPS(1) | LL_I2S_CON_WSCLKDBSBPS(1));
    }

    return RET_OK;
}

static int32 hgi2s_v0_set_duplex(struct hgi2s_v0 *dev, uint32 enable) {

    if (enable) {
        dev->duplex_en = 1;
    } else {
        dev->duplex_en = 0;
    }

    return RET_OK;
}

/**********************************************************************************/
/*                          I2S ATTCH FUNCTION                                    */
/**********************************************************************************/
int32 hgi2s_v0_open(struct i2s_device *i2s, enum i2s_mode mode, enum i2s_sample_freq frequency, enum i2s_sample_bits bits)
{

    struct hgi2s_v0 *dev          = (struct hgi2s_v0*)i2s;
    struct hgi2s_v0_hw *hw        = (struct hgi2s_v0_hw *)dev->hw;
    uint32 i2s_reg_con            = 0;
    int32  i2s_mode_to_reg        = 0;
    int32  i2s_channel_to_reg     = 0;
    int32  i2s_sample_bits_to_reg = 0;
    int32  i2s_data_fmt_to_reg    = 0;
    int32  i2s_sample_freq_to_reg = 0;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    /* hal enum */
    i2s_mode_to_reg        = hgi2s_v0_switch_hal_i2s_mode(mode                 );
    i2s_channel_to_reg     = hgi2s_v0_switch_hal_i2s_channel(I2S_CHANNEL_STEREO);
    i2s_data_fmt_to_reg    = hgi2s_v0_switch_hal_i2s_data_fmt(I2S_DATA_FMT_I2S );
    i2s_sample_bits_to_reg = hgi2s_v0_switch_hal_i2s_sample_bits(bits          );
    i2s_sample_freq_to_reg = hgi2s_v0_switch_hal_i2s_sample_freq(frequency     );

    if (((-1) == i2s_mode_to_reg       ) || \
        ((-1) == i2s_channel_to_reg    ) || \
        ((-1) == i2s_data_fmt_to_reg   ) || \
        ((-1) == i2s_sample_bits_to_reg) || \
        ((-1) == i2s_sample_freq_to_reg)) {
        return -EINVAL;
    }

    #if 0
        /* make sure the sys_clk */
        if (0 != ((peripheral_clock_get(HG_APB0_PT_IIS0)*2) % (i2s_sample_freq_to_reg * 256))) {
            return RET_ERR;
        }
    #endif

    /* pin config */
    if (pin_func(dev->dev.dev.dev_id , 1) != RET_OK) {
        return RET_ERR;
    }

    /*
     * open I2S clk
     */
     if (HG_IIS0_BASE == (uint32)hw) {
        sysctrl_iis0_clk_open();
     } else if (HG_IIS1_BASE == (uint32)hw) {
        sysctrl_iis1_clk_open();
     }

    /*
     * clear the regs
     */
     hw->BAUD      = 0;
     hw->BIT_SET   = 0;
     hw->CON       = 0;
     hw->DMA_LEN   = 0;
     hw->DMA_STADR = 0;
     hw->WS_CON    = 0;
     hw->STA       = 0xFFFF;

    /* reg config */
    i2s_reg_con = hw->CON;
    
    i2s_reg_con = LL_I2S_CON_MODE(i2s_mode_to_reg    ) | 
                  LL_I2S_CON_MONO(i2s_channel_to_reg ) | 
                  LL_I2S_CON_MCLK_OE(1               ) |  /* open mclk to io */ 
                  LL_I2S_CON_FRMT(i2s_data_fmt_to_reg) ;
    
    //close debounce
    i2s_reg_con |= (LL_I2S_CON_BCLKDBSBPS(1) | LL_I2S_CON_RXDBSBPS(1) | LL_I2S_CON_WSCLKDBSBPS(1));

    hw->CON     = i2s_reg_con;
    hw->WS_CON  = i2s_sample_bits_to_reg - 1;
    hw->BIT_SET = i2s_sample_bits_to_reg - 1;
    hw->BAUD    = ((__OVER_SAMPPLE_RATE/4) / i2s_sample_bits_to_reg) - 1;

    hgi2s_v0_set_mclk(hw, i2s_sample_freq_to_reg);
    
    dev->opened    = 1;
    dev->duplex_en = 0;
    dev->dsleep    = 0;

    return RET_OK;
}

int32 hgi2s_v0_close(struct i2s_device *i2s) {

    struct hgi2s_v0 *dev = (struct hgi2s_v0*)i2s;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;

    if (!dev->opened) {
        return RET_OK;
    }

    /*
     * close IIS clk
     */
    if (HG_IIS0_BASE == (uint32)hw) {
        sysctrl_iis0_clk_close();
    } else if (HG_IIS1_BASE == (uint32)hw) {
        sysctrl_iis1_clk_close();
    }


    irq_disable(dev->irq_num       );
    pin_func(dev->dev.dev.dev_id, 0);
    hgi2s_v0_disable(hw            );

    dev->opened    = 0;
    dev->duplex_en = 0;
    dev->dsleep    = 0;

    return RET_OK;
}

int32 hgi2s_v0_write(struct i2s_device *i2s, const void* buf, uint32 len) {

    struct hgi2s_v0 *dev   = (struct hgi2s_v0*)i2s;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;
    uint32 len_to_reg      = 0;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    len_to_reg = len;
    
    hw->DMA_STADR = (uint32)buf;
    hw->DMA_LEN   = len_to_reg;

    hgi2s_v0_set_tx(hw);

    if (!dev->duplex_en) {
        hgi2s_v0_enable(hw);
    }

    return RET_OK;
}

int32 hgi2s_v0_read(struct i2s_device *i2s, void* buf, uint32 len) {

    struct hgi2s_v0 *dev   = (struct hgi2s_v0*)i2s;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;
    uint32 len_to_reg      = 0;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    len_to_reg = len;

    hw->DMA_STADR = (uint32)buf;
    hw->DMA_LEN   = len_to_reg;

    hgi2s_v0_set_rx(hw);
    
    if (!dev->duplex_en) {
        hgi2s_v0_enable(hw);
    }

    return RET_OK;
}

int32 hgi2s_v0_ioctl(struct i2s_device *i2s, uint32 cmd, uint32 param) {

    int32  ret_val         = RET_OK;
    struct hgi2s_v0 *dev   = (struct hgi2s_v0*)i2s;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    switch(cmd) {
        case (I2S_IOCTL_CMD_SET_WSCLK_POL):
            ret_val = hgi2s_v0_set_wsclk_pol(hw  , param);
            break;
        case (I2S_IOCTL_CMD_SET_SAMPLE_BITS):
            ret_val = hgi2s_v0_set_sample_bits(hw, param);
            break;
        case (I2S_IOCTL_CMD_SET_CHANNEL):
            ret_val = hgi2s_v0_set_channel(hw    , param);
            break;
        case (I2S_IOCTL_CMD_SET_DATA_FMT):
            ret_val = hgi2s_v0_set_data_fmt(hw   , param);
            break;
        case (I2S_IOCTL_CMD_SET_DEBOUNCE):
            ret_val = hgi2s_v0_set_debounce(hw   , param);
            break;
        case (I2S_IOCTL_CMD_SET_DUPLEX):
            ret_val = hgi2s_v0_set_duplex(dev    , param);
            break;
        default:
            ret_val = -ENOTSUPP;
            break;
    }

    return ret_val;
}

#ifdef CONFIG_SLEEP
static int32 hgi2s_v0_suspend(struct dev_obj *obj)
{
    struct hgi2s_v0 *dev   = (struct hgi2s_v0*)obj;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_suspend_lock, 10000)) {
        return RET_ERR;
    }


    /*!
     * Close the IIS 
     */
    hw->CON &= ~ BIT(0);


    pin_func(dev->dev.dev.dev_id , 0);


    /*
     * close irq
     */
    irq_disable(dev->irq_num);

    /*
     * clear pending
     */
    hw->STA = 0xFFFFFFFF;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /* 
     * save the reglist
     */
    dev->bp_regs.con       = hw->CON;
    dev->bp_regs.bit_set   = hw->BIT_SET;
    dev->bp_regs.baud      = hw->BAUD;
    dev->bp_regs.ws_con    = hw->WS_CON;
    dev->bp_regs.dma_stadr = hw->DMA_STADR;
    dev->bp_regs.dma_len   = hw->DMA_LEN;

    /*
     * save the irq_hdl created by user
     */
    dev->bp_irq_hdl  = dev->irq_hdl;
    dev->bp_irq_data = dev->irq_data;

    /*
     * close IIS clk
     */
    if (HG_IIS0_BASE == (uint32)hw) {
        sysctrl_iis0_clk_close();
    } else if (HG_IIS1_BASE == (uint32)hw) {
        sysctrl_iis1_clk_close();
    }

    dev->dsleep = 1;

    os_mutex_unlock(&dev->bp_suspend_lock);

    return RET_OK;
}

static int32 hgi2s_v0_resume(struct dev_obj *obj)
{
    struct hgi2s_v0 *dev   = (struct hgi2s_v0*)obj;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;

    if ((!dev->opened) || (!dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_resume_lock, 10000)) {
        return RET_ERR;
    }

    /* pin config */
    if (pin_func(dev->dev.dev.dev_id , 1) != RET_OK) {
        return RET_ERR;
    }


    /*
     * open I2S clk
     */
     if (HG_IIS0_BASE == (uint32)hw) {
        sysctrl_iis0_clk_open();
     } else if (HG_IIS1_BASE == (uint32)hw) {
        sysctrl_iis1_clk_open();
     }


    /*
     * recovery the reglist from sram
     */
    hw->BIT_SET   = dev->bp_regs.bit_set;
    hw->BAUD      = dev->bp_regs.baud;
    hw->WS_CON    = dev->bp_regs.ws_con;
    hw->DMA_STADR = dev->bp_regs.dma_stadr;
    hw->DMA_LEN   = dev->bp_regs.dma_len;
    hw->CON       = dev->bp_regs.con;

    /*
     * recovery the irq handle and data
     */ 
    dev->irq_hdl  = dev->bp_irq_hdl;
    dev->irq_data = dev->bp_irq_data;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));


    /*!
     * Open the IIS 
     */
    hw->CON |= BIT(0);


    /*
     * open irq
     */
    irq_enable(dev->irq_num);

    dev->dsleep = 0;

    os_mutex_unlock(&dev->bp_resume_lock);

    return RET_OK;
}
#endif

/* interrupt handler */
static void hgi2s_v0_irq_handler(void *data) {

    struct hgi2s_v0 *dev   = (struct hgi2s_v0 *)data;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;

    if ((hw->CON & LL_I2S_CON_HF_PEND_IE(1)) && (hw->STA & LL_I2S_STA_DMA_HF_PENGING(1))) {
        hw->STA = LL_I2S_STA_DMA_HF_PENGING(1);
        if (dev->irq_hdl) {
            dev->irq_hdl(I2S_IRQ_FLAG_HALF, dev->irq_data);
        }
    }

    if ((hw->CON & LL_I2S_CON_OV_PEND_IE(1)) && (hw->STA & LL_I2S_STA_DMA_OV_PENGING(1))) {
        hw->STA = LL_I2S_STA_DMA_OV_PENGING(1);
        if (dev->irq_hdl) {
            dev->irq_hdl(I2S_IRQ_FLAG_FULL, dev->irq_data);
        }
    }
}

/* request interrupt */
int32 hgi2s_v0_request_irq(struct i2s_device *i2s, uint32 irq_flag, i2s_irq_hdl irqhdl, uint32 irq_data) {

    struct hgi2s_v0 *dev   = (struct hgi2s_v0 *)i2s;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    dev->irq_hdl  = irqhdl;
    dev->irq_data = irq_data;
    request_irq(dev->irq_num, hgi2s_v0_irq_handler, dev);


    if (irq_flag & I2S_IRQ_FLAG_HALF) {
        hw->CON |= LL_I2S_CON_HF_PEND_IE(1);
    }

    if (irq_flag & I2S_IRQ_FLAG_FULL) {
        hw->CON |= LL_I2S_CON_OV_PEND_IE(1);
    }

    irq_enable(dev->irq_num);

    return RET_OK;
}

int32 hgi2s_v0_release_irq(struct i2s_device *i2s, uint32 irq_flag) {

    struct hgi2s_v0 *dev   = (struct hgi2s_v0 *)i2s;
    struct hgi2s_v0_hw *hw = (struct hgi2s_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    if (irq_flag & I2S_IRQ_FLAG_HALF) {
        hw->CON &= ~ LL_I2S_CON_HF_PEND_IE(1);
    }

    if (irq_flag & I2S_IRQ_FLAG_FULL) {
        hw->CON &= ~ LL_I2S_CON_OV_PEND_IE(1);
    }

    return RET_OK;
}

static const struct i2s_hal_ops i2s_v0_ops = {
    .open        = hgi2s_v0_open,
    .close       = hgi2s_v0_close,
    .write       = hgi2s_v0_write,
    .read        = hgi2s_v0_read,
    .request_irq = hgi2s_v0_request_irq,
    .release_irq = hgi2s_v0_release_irq,
    .ioctl       = hgi2s_v0_ioctl,    
#ifdef CONFIG_SLEEP
    .ops.suspend = hgi2s_v0_suspend,
    .ops.resume  = hgi2s_v0_resume,
#endif
};

int32 hgi2s_v0_attach(uint32 dev_id, struct hgi2s_v0 *i2s) {

    i2s->opened          = 0;
    i2s->dsleep          = 0;
    i2s->irq_hdl         = NULL;
    i2s->irq_data        = 0;
    i2s->duplex_en       = 0;
    i2s->dev.dev.ops     = (const struct devobj_ops *)&i2s_v0_ops;
#ifdef CONFIG_SLEEP
    os_mutex_init(&i2s->bp_suspend_lock);
    os_mutex_init(&i2s->bp_resume_lock);
#endif    
    irq_disable(i2s->irq_num);
    dev_register(dev_id, (struct dev_obj *)i2s);
    return RET_OK;
}


