/**
 * @file hgled_v0.c
 * @author bxd
 * @brief led
 * @version
 * TXW80X
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
#include "hal/led.h"
#include "dev/led/hgled_v0.h"
#include "hgled_v0_hw.h"

/**********************************************************************************/
/*                           LOW LAYER FUNCTION                                   */
/**********************************************************************************/

static int32 hgled_v0_switch_hal_led_work_mode(enum led_work_mode param) {

    switch (param) {
        case (LED_WORK_MODE_COMMON_ANODE):
            return 1;
            break;
        case (LED_WORK_MODE_COMMON_CATHODE):
            return 0;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgled_v0_switch_hal_led_seg_cnt(enum led_seg_cnt param) {

    switch (param) {
        case (LED_SEG_CNT_1):
            return 0x10001;
            break;
        case (LED_SEG_CNT_2):
            return 0x20003;
            break;
        case (LED_SEG_CNT_3):
            return 0x30007;
            break;
        case (LED_SEG_CNT_4):
            return 0x4000f;
            break;
        case (LED_SEG_CNT_5):
            return 0x5001f;
            break;
        case (LED_SEG_CNT_6):
            return 0x6003f;
            break;
        case (LED_SEG_CNT_7):
            return 0x7007f;
            break;
        case (LED_SEG_CNT_8):
            return 0x800ff;
            break;
        case (LED_SEG_CNT_9):
            return 0x901ff;
            break;
        case (LED_SEG_CNT_10):
            return 0xa03ff;
            break;
        case (LED_SEG_CNT_11):
            return 0xb07ff;
            break;
        case (LED_SEG_CNT_12):
            return 0xc0fff;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgled_v0_switch_hal_led_com_cnt(enum led_com_cnt param) {

    switch (param) {
        case (LED_COM_CNT_1):
            return 0x10001;
            break;
        case (LED_COM_CNT_2):
            return 0x20003;
            break;
        case (LED_COM_CNT_3):
            return 0x30007;
            break;
        case (LED_COM_CNT_4):
            return 0x4000f;
            break;
        case (LED_COM_CNT_5):
            return 0x5001f;
            break;
        case (LED_COM_CNT_6):
            return 0x6003f;
            break;
        case (LED_COM_CNT_7):
            return 0x7007f;
            break;
        case (LED_COM_CNT_8):
            return 0x800ff;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgled_v0_switch_hal_led_irq_flag(enum led_irq_flag param) {

    switch (param) {
        case (LED_IRQ_FLAG_INT):
            return 0;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgled_v0_switch_hal_led_ioctl_cmd(enum led_ioctl_cmd param) {

    switch (param) {
        case (LED_IOCTL_CMD_SET_SCAN_FREQ):
            return 0;
            break;
        default:
            return -1;
            break;
    }
}

static inline void hgled_v0_enable(struct hgled_v0_hw *p_led) {
    (p_led)->CON |= LL_LED_CON_EN;
}

static inline void hgled_v0_disable(struct hgled_v0_hw *p_led) {
    (p_led)->CON &= ~LL_LED_CON_EN;
}

static inline void hgled_v0_clear_interrupt_pending(struct hgled_v0_hw *p_led) {
    (p_led)->CON |= LL_LED_CON_PENDING_CLR;
}

static inline int32 hgled_v0_set_scan_freq(struct hgled_v0_hw *p_led, uint32 param1, uint32 param2) {
    (p_led)->TIME = ((p_led)->TIME &~ LL_LED_TIME_CLK_PSC(0xFFF)) | LL_LED_TIME_CLK_PSC(param1);
    (p_led)->TIME = ((p_led)->TIME &~ LL_LED_TIME_LEN(0xFF     )) | LL_LED_TIME_LEN(param2    );

    return RET_OK;
}

/**********************************************************************************/
/*                             ATTCH FUNCTION                                     */
/**********************************************************************************/

static int32 hgled_v0_open(struct led_device *led, enum led_work_mode work_mode, enum led_seg_cnt seg_cnt, enum led_com_cnt com_cnt) {

    struct hgled_v0 *dev    = (struct hgled_v0 *)led;
    struct hgled_v0_hw *hw  = (struct hgled_v0_hw *)dev->hw;
    int32  work_mode_to_reg = 0;
    int32  seg_cnt_to_reg   = 0;
    int32  com_cnt_to_reg   = 0;
       

    if (dev->opened) {
        return -EBUSY;
    }

    /* hal enum */
    work_mode_to_reg = hgled_v0_switch_hal_led_work_mode(work_mode);
    seg_cnt_to_reg   = hgled_v0_switch_hal_led_seg_cnt(seg_cnt    );
    com_cnt_to_reg   = hgled_v0_switch_hal_led_com_cnt(com_cnt    );

    if (((-1) == work_mode_to_reg) ||
        ((-1) == seg_cnt_to_reg   ) ||
        ((-1) == com_cnt_to_reg   ) ) {
            return RET_ERR;
    }

    /* pin config */
    if (pin_func(dev->dev.dev.dev_id , 1) != RET_OK) {
        return RET_ERR;
    }

    #ifdef TXW80X
    sysctrl_tk_clk_en();
    #endif


    /* reg config */
    hw->CON = LL_LED_CON_SEG_EN(seg_cnt_to_reg & 0xFFFF) | \
              LL_LED_CON_COM_EN(com_cnt_to_reg & 0xFFFF) | \
              LL_LED_CON_SCAN_SEL(work_mode_to_reg     ) | \
              LL_LED_CON_PENDING_CLR;

    hw->TIME = LL_LED_TIME_CLK_PSC(59) |     /* default clk psc */
               LL_LED_TIME_GRAY(0)     | 
               LL_LED_TIME_LEN(99);           /* default time len */

    hw->DATA0 = 0x0000;
    hw->DATA1 = 0x0000;
    hw->DATA2 = 0x0000;
    hw->DATA3 = 0x0000;
    hw->CON  |= LL_LED_CON_LOAD_DATA_EN;
    hw->CON  |= LL_LED_CON_EN;

    dev->opened  = 1;

    dev->com_cnt   = com_cnt_to_reg >> 16;
    dev->seg_cnt   = seg_cnt_to_reg >> 16;
    dev->tube_type = work_mode;

   return RET_OK;
}

static int32 hgled_v0_on(struct led_device *led, uint32 num, enum led_com_cnt pos) {

    struct hgled_v0 *dev = (struct hgled_v0 *)led;
    struct hgled_v0_hw *hw = (struct hgled_v0_hw *)dev->hw;
    const uint16 num_tbl[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

    int32  i = 0;
    uint32 e = 0;
    uint32 *led_ptr  = (uint32 *)(&(hw->DATA0));
    uint32 data_temp = 0;

    i = hgled_v0_switch_hal_led_com_cnt(pos);
    
    if ((-1) == i) {
        return RET_ERR;
    }

    i = (i >> 16);

    if (i > dev->com_cnt) {
        return RET_ERR;
    }

    i -= 1;

    e = num % 10;
    
    //update pointer by pos, 16 bits for seg
    led_ptr = led_ptr + (i/2);

    data_temp = *(led_ptr);

    data_temp = (data_temp &~ (0xFF << (16*(i%2)))) | (num_tbl[e] << (16*(i%2)));

    //write data to show
    *(led_ptr) = data_temp;

    return RET_OK;
}

static int32 hgled_v0_off(struct led_device *led, enum led_com_cnt pos) {

    struct hgled_v0 *dev = (struct hgled_v0 *)led;
    struct hgled_v0_hw *hw = (struct hgled_v0_hw *)dev->hw;

    int32  i = 0;
    uint32 *led_ptr  = (uint32 *)(&(hw->DATA0));
    uint32 data_temp = 0;

    i = hgled_v0_switch_hal_led_com_cnt(pos);
    
    if ((-1) == i) {
        return RET_ERR;
    }

    i = (i >> 16);

    if (i > dev->com_cnt) {
        return RET_ERR;
    }

    i -= 1;

    //update pointer by pos, 16 bits for seg
    led_ptr = led_ptr + (i/2);

    data_temp = *(led_ptr);

    //write 0 to data register for turn off the tube
    data_temp = (data_temp &~ (0xFF << (16*(i%2))));

    *(led_ptr) = data_temp;

    return RET_OK;
}


static int32 hgled_v0_close(struct led_device *led) {

    struct hgled_v0 *dev = (struct hgled_v0 *)led;
    struct hgled_v0_hw *hw = (struct hgled_v0_hw *)dev->hw;

    if (!dev->opened) {
        return RET_OK;
    }

    hgled_v0_disable(hw       );
    irq_disable(dev->irq_num       );
    pin_func(dev->dev.dev.dev_id, 0);

    #ifdef TXW80X
    sysctrl_tk_clk_dis();
    #endif
    
    dev->opened    = 0;
    dev->tube_type = 0;
    dev->seg_cnt   = 0;
    dev->com_cnt   = 0;

    return RET_OK;
}

static int32 hgled_v0_ioctl(struct led_device *led, enum led_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2) {

    int32  ret_val = RET_OK;
    struct hgled_v0 *dev = (struct hgled_v0 *)led;
    struct hgled_v0_hw *hw = (struct hgled_v0_hw *)dev->hw;

    switch (ioctl_cmd) {
        case (LED_IOCTL_CMD_SET_SCAN_FREQ):
            ret_val = hgled_v0_set_scan_freq(hw, param1, param2);
            break;
        default:
            ret_val = -ENOTSUPP;
            break;
    }

    return ret_val;
}

static void hgled_v0_irq_handler(void *data) {

    struct hgled_v0 *dev = (struct hgled_v0 *)data;
    struct hgled_v0_hw *hw = (struct hgled_v0_hw *)dev->hw;

    if((hw->CON & LL_LED_CON_PENDING) && (hw->CON & LL_LED_CON_IE)) {
        hgled_v0_clear_interrupt_pending(hw);

        if (dev->irq_hdl) {
            dev->irq_hdl(LED_IRQ_FLAG_INT, dev->irq_data);
        }
    }
}

static int32 hgled_v0_request_irq(struct led_device *led, enum led_irq_flag irq_flag, led_irq_hdl irq_hdl, uint32 irq_data) {

    struct hgled_v0 *dev = (struct hgled_v0 *)led;
    struct hgled_v0_hw *hw = (struct hgled_v0_hw *)dev->hw;

    dev->irq_hdl  = irq_hdl;
    dev->irq_data = irq_data;
    request_irq(dev->irq_num, hgled_v0_irq_handler, dev);

    if (irq_flag & LED_IRQ_FLAG_INT) {
        hw->CON |= LL_LED_CON_IE;
    }

    irq_enable(dev->irq_num);

    return RET_OK;
}

static int32 hgled_v0_release_irq(struct led_device *led, enum led_irq_flag irq_flag) {

    struct hgled_v0 *dev = (struct hgled_v0 *)led;
    struct hgled_v0_hw *hw = (struct hgled_v0_hw *)dev->hw;

    irq_disable(dev->irq_num);
    hw->CON &= ~ LL_LED_CON_IE;

    return RET_OK;
}

static const struct led_hal_ops led_ops = {
    .open        = hgled_v0_open,
    .close       = hgled_v0_close,
    .on          = hgled_v0_on,
    .off         = hgled_v0_off,
    .ioctl       = hgled_v0_ioctl,
    .request_irq = hgled_v0_request_irq,
    .release_irq = hgled_v0_release_irq,
};

int32 hgled_v0_attach(uint32 dev_id, struct hgled_v0 *led) {

    led->opened          = 0;
    led->seg_cnt         = 0;
    led->com_cnt         = 0;
    led->tube_type       = 0;
    led->irq_hdl         = NULL;
    led->irq_data        = 0;
    led->dev.dev.ops     =(const struct devobj_ops *)&led_ops;
    irq_disable(led->irq_num);
    dev_register(dev_id, (struct dev_obj *)led);
    return RET_OK;
}

