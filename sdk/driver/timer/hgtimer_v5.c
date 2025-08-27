/**
 * @file hgtimer_v5.c
 * @author bxd
 * @brief low power led timer
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
#include "hal/timer_device.h"
#include "hal/pwm.h"
#include "dev/pwm/hgpwm_v0.h"
#include "dev/timer/hgtimer_v5.h"
#include "hgtimer_v5_hw.h"


/**********************************************************************************/
/*                           LOW LAYER FUNCTION                                   */
/**********************************************************************************/

#define LL_LED_TIMER_CHECK_COUNT_OV_INTERRUPT_ENABLE(p_timer)       (LED_TIMER0_BASE  == (uint32)p_timer) ? (SYSCTRL->SYS_CON15 & (1 <<  8)) : \
                                                                    ((LED_TIMER1_BASE == (uint32)p_timer) ? (SYSCTRL->SYS_CON15 & (1 <<  9)) : \
                                                                    ((LED_TIMER2_BASE == (uint32)p_timer) ? (SYSCTRL->SYS_CON15 & (1 << 10)) : \
                                                                    ((LED_TIMER3_BASE == (uint32)p_timer) ? (SYSCTRL->SYS_CON15 & (1 << 11)) : 0)))


static inline void hgtimer_v5_irq_config(struct hgtimer_v5 *dev, uint32 enable) {

    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 pos  = 0;
    uint32 addr = (uint32)(&hw->LED_TMR_CON0);

    //Confirm which led_timer will be enable interrupt
    if (LED_TIMER0_BASE == addr) {
        pos = 8;
    } else if (LED_TIMER1_BASE == addr) {
        pos = 9;
    } else if (LED_TIMER2_BASE == addr) {
        pos = 10;
    } else if (LED_TIMER3_BASE == addr){
        pos = 11;
    }

    sysctrl_unlock();
    if (enable) {
        SYSCTRL->SYS_CON15 |=  (1 << pos);
    } else {
        SYSCTRL->SYS_CON15 &= ~(1 << pos);
    }
    sysctrl_lock();
}


/**********************************************************************************/
/*                           PWM FUNCTION START                                   */
/**********************************************************************************/
static int32 hgtimer_v5_pwm_switch_func_cmd(enum hgpwm_v0_func_cmd param) {

    switch (param) {
        case (HGPWM_V0_FUNC_CMD_INIT):
            return HGTIMER_V5_PWM_FUNC_CMD_INIT;
            break;
        case (HGPWM_V0_FUNC_CMD_DEINIT):
            return HGTIMER_V5_PWM_FUNC_CMD_DEINIT;
            break;
        case (HGPWM_V0_FUNC_CMD_START):
            return HGTIMER_V5_PWM_FUNC_CMD_START;
            break;
        case (HGPWM_V0_FUNC_CMD_STOP):
            return HGTIMER_V5_PWM_FUNC_CMD_STOP;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY):
            return HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_SINGLE_INCREAM):
            return HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_SINGLE_INCREAM;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_INCREAM_DECREASE):
            return HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_INCREAM_DECREASE;
            break;
        case (HGPWM_V0_FUNC_CMD_REQUEST_IRQ_PERIOD):
            return HGTIMER_V5_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD;
            break;
        case (HGPWM_V0_FUNC_CMD_RELEASE_IRQ):
            return HGTIMER_V5_PWM_FUNC_CMD_RELEASE_IRQ;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PRESCALER):
            return HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_PRESCALER;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY):
            return HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY;
            break;
        default:
            return -1;
            break;
    }
}

static int32  hgtimer_v5_pwm_switch_period_duty(uint32 period, uint32 duty, uint32 *p_period, uint32 *p_duty) {

    uint32 pmu_con0_tmp     = 0;
    uint32 period_us_to_reg = 0;
    uint32 duty_us_to_reg   = 0;

    pmu_con0_tmp = PMU->PMUCON0;
    
    /* make sure the period */
    switch ((pmu_con0_tmp & (0x3 << 25)) >> 25) {
        /* rc128k_pre */
        case (0):
            period_us_to_reg = ((128000*period) / 1000000);
            duty_us_to_reg   = ((128000*duty  ) / 1000000);

            if (period_us_to_reg && duty_us_to_reg) {
                *p_period = period_us_to_reg;
                *p_duty   = duty_us_to_reg;
                return 0;
            } else {
                return -1;
            }
            break;
        /* x32m_div_clk */
        case (1):
            period_us_to_reg = ((32000000*period) / 1000000);
            duty_us_to_reg   = ((32000000*duty  ) / 1000000);

            if (period_us_to_reg && duty_us_to_reg) {
                *p_period = period_us_to_reg;
                *p_duty   = duty_us_to_reg;
                return 1;
            } else {
                return -1;
            }
            break;
        /* rc10m_clk */
        case (2):
            period_us_to_reg = ((10000000*period) / 1000000);
            duty_us_to_reg   = ((10000000*duty  ) / 1000000);

            if (period_us_to_reg && duty_us_to_reg) {
                *p_period = period_us_to_reg;
                *p_duty   = duty_us_to_reg;
                return 2;
            } else {
                return -1;
            }
            break;
        /* lxosc32k_clk */
        case (3):
            period_us_to_reg = ((32000*period) / 1000000);
            duty_us_to_reg   = ((32000*duty  ) / 1000000);

            if (period_us_to_reg && duty_us_to_reg) {
                *p_period = period_us_to_reg;
                *p_duty   = duty_us_to_reg;
                return 3;
            } else {
                return -1;
            }
            break;
        default:
            return -EINVAL;
            break;
    }
}

static inline int32 hgtimer_v5_pwm_func_init(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    uint32 period_to_reg      = 0;
    uint32 duty_to_reg        = 0;
    uint32 led_timer_con0_tmp = 0;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;

    if (dev->opened) {
        return -EBUSY;
    }

    if (pin_func(dev->dev.dev.dev_id , 1) != RET_OK) {
        return RET_ERR;
    }

    /* make sure the period & duty */
    hgtimer_v5_pwm_switch_period_duty(p_config->period, p_config->duty, &period_to_reg, &duty_to_reg);

    hgtimer_v5_irq_config(dev, 0);
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, 0x00000000);

    /* config reg */
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_MODE(0xFFF)) | LL_LED_TIMER_CON0_TMR_MODE(1          );
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PSR(0x00F )) | LL_LED_TIMER_CON0_TMR_PSR(0           );
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PR(0xFFF  )) | LL_LED_TIMER_CON0_TMR_PR(period_to_reg);
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_STEP(0xFFF)) | LL_LED_TIMER_CON0_TMR_STEP(duty_to_reg);

    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

    dev->opened = 1;
    dev->pwm_en = 1;

    return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_deinit(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;

    if ((!dev->opened) || (!dev->pwm_en)) {
        return RET_OK;
    }

    irq_disable(dev->irq_num);
    pin_func(dev->dev.dev.dev_id , 0);
    hgtimer_v5_irq_config(dev, 0);
    
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, 0x00000000);
    dev->opened  = 0;
    dev->pwm_en  = 0;
    
    return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_start(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = hw->LED_TMR_CON0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }
    
    led_timer_con0_tmp |= LL_LED_TIMER_CON0_TMR_EN(1);
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

    return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_stop(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = hw->LED_TMR_CON0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }
    
    led_timer_con0_tmp &= ~ LL_LED_TIMER_CON0_TMR_EN(1);
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

    return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_ioctl_set_period_duty(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    uint32 period_to_reg  = 0;
    uint32 duty_to_reg    = 0;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = hw->LED_TMR_CON0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    if ((p_config->duty < 0) || ((p_config->duty) > (p_config->period))) {
        return RET_ERR;
    }

    /* make sure the period & duty */
    if ((-1) == hgtimer_v5_pwm_switch_period_duty(p_config->period, p_config->duty, &period_to_reg, &duty_to_reg)) {
        return -EINVAL;
    };

    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PR(0xFFF  )) | LL_LED_TIMER_CON0_TMR_PR(period_to_reg);
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_STEP(0xFFF)) | LL_LED_TIMER_CON0_TMR_STEP(duty_to_reg);

    /* led timer must stop to config period & duty */
    hgtimer_v5_pwm_func_stop(dev, p_config);

    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

    hgtimer_v5_pwm_func_start(dev, p_config);

    return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_ioctl_set_single_incream(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    uint32 period_to_reg      = 0;
    uint32 none               = 50;    /* just for through the "func_switch_period_duty", no use */
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = hw->LED_TMR_CON0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    /* make sure the pwm step > 0 */
    if (!p_config->duty) {
        return -EINVAL;
    }

    /* make sure the period & duty */
    if ((-1) == hgtimer_v5_pwm_switch_period_duty(p_config->period, p_config->duty, &period_to_reg, &none)) {
        return -EINVAL;
    }

    /* config reg */
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_MODE(0xFFF)) | LL_LED_TIMER_CON0_TMR_MODE(2);
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PSR(0x00F )) | LL_LED_TIMER_CON0_TMR_PSR(0);
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PR(0xFFF  )) | LL_LED_TIMER_CON0_TMR_PR(period_to_reg);
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_STEP(0xFFF)) | LL_LED_TIMER_CON0_TMR_STEP(p_config->duty);

    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

    return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_ioctl_set_incream_decrease(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    uint32 period_to_reg      = 0;
    uint32 none               = 50;    /* just for through the "func_switch_period_duty", no use */
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = hw->LED_TMR_CON0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    /* make sure the pwm step > 0 */
    if (!p_config->duty) {
        return -EINVAL;
    }

    /* make sure the period & duty */
    if ((-1) == hgtimer_v5_pwm_switch_period_duty(p_config->period, p_config->duty, &period_to_reg, &none)) {
        return -EINVAL;
    }

    /* config reg */
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_MODE(0xFFF)) | LL_LED_TIMER_CON0_TMR_MODE(3);
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PSR(0x00F )) | LL_LED_TIMER_CON0_TMR_PSR(0);
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PR(0xFFF  )) | LL_LED_TIMER_CON0_TMR_PR(period_to_reg);
    led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_STEP(0xFFF)) | LL_LED_TIMER_CON0_TMR_STEP(p_config->duty);

    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

    return RET_OK;
}


static inline int32 hgtimer_v5_pwm_func_cmd_ioctl_set_prescaler(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v5_hw *hw  = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = hw->LED_TMR_CON0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }
    
    led_timer_con0_tmp = (led_timer_con0_tmp &~ (LL_LED_TIMER_CON0_TMR_PSR(0xF))) | (LL_LED_TIMER_CON0_TMR_PSR(p_config->param1));
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

	return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_cmd_ioctl_set_period_duty_immediately(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config)
{
	return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_request_irq_period(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    dev->_pwm_irq_hdl = p_config->irq_hdl;
    dev->irq_data     = p_config->irq_data;
    hgtimer_v5_irq_config(dev, 1);
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v5_pwm_func_release_irq(struct hgtimer_v5 *dev, struct hgpwm_v0_config *p_config) {

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    irq_disable(dev->irq_num);
    dev->_pwm_irq_hdl = NULL;
    dev->irq_data     = 0;
    hgtimer_v5_irq_config(dev, 0);

    return RET_OK;
}

static int32 hgtimer_v5_pwm_config(struct hgtimer_v5 *dev, uint32 config, uint32 param) {

    struct hgpwm_v0_config *p_config;
    int32  hgtimer_v5_pwm_func_cmd = 0;
    int32  ret_val = RET_OK;
    
    /* Make sure the config struct pointer */
    if (!config) {
        return -EINVAL;
    }

    p_config = (struct hgpwm_v0_config*)config;
    hgtimer_v5_pwm_func_cmd = hgtimer_v5_pwm_switch_func_cmd(p_config->func_cmd);
    if ((-1) == hgtimer_v5_pwm_func_cmd) {
        return -EINVAL;
    }

    switch (hgtimer_v5_pwm_func_cmd) {
        case (HGTIMER_V5_PWM_FUNC_CMD_INIT):
            ret_val = hgtimer_v5_pwm_func_init(dev, p_config);
            break;
        case (HGTIMER_V5_PWM_FUNC_CMD_DEINIT):
            ret_val = hgtimer_v5_pwm_func_deinit(dev, p_config);
            break;
        case (HGTIMER_V5_PWM_FUNC_CMD_START):
            ret_val = hgtimer_v5_pwm_func_start(dev, p_config);
            break;
        case (HGTIMER_V5_PWM_FUNC_CMD_STOP):
            ret_val = hgtimer_v5_pwm_func_stop(dev, p_config);
            break;
        case (HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY):
            ret_val = hgtimer_v5_pwm_func_ioctl_set_period_duty(dev, p_config);
            break;
        case (HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_SINGLE_INCREAM):
            ret_val = hgtimer_v5_pwm_func_ioctl_set_single_incream(dev, p_config);
            break;
        case (HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_INCREAM_DECREASE):
            ret_val = hgtimer_v5_pwm_func_ioctl_set_incream_decrease(dev, p_config);
            break;
        case (HGTIMER_V5_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD):
            ret_val = hgtimer_v5_pwm_func_request_irq_period(dev, p_config);
            break;
        case (HGTIMER_V5_PWM_FUNC_CMD_RELEASE_IRQ):
            ret_val = hgtimer_v5_pwm_func_release_irq(dev, p_config);
            break;
		case (HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_PRESCALER):
			ret_val = hgtimer_v5_pwm_func_cmd_ioctl_set_prescaler(dev, p_config);
			break;
		case (HGTIMER_V5_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY):
			ret_val = hgtimer_v5_pwm_func_cmd_ioctl_set_period_duty_immediately(dev, p_config);
			break;
        default:
            ret_val = -ENOTSUPP;
            break;
    }

    return ret_val;
}
/**********************************************************************************/
/*                           PWM FUNCTION END                                     */
/**********************************************************************************/


/**********************************************************************************/
/*                           COUNTER FUNCTION START                               */
/**********************************************************************************/
static int32 hgtimer_v5_counter_switch_hal_type(struct hgtimer_v5 *dev, enum timer_type param) {

    switch (param) {
        case (TIMER_TYPE_ONCE):
            dev->type = HGTIMER_V5_TYPE_ONCE;
            return 0;
            break;
        case (TIMER_TYPE_PERIODIC):
            dev->type = HGTIMER_V5_TYPE_PERIODIC;
            return 1;
            break;
        default:
            return -1;
            break;
    }
}

static int32  hgtimer_v5_counter_switch_tmo_us(uint32 tmo_us) {

    uint32 pmu_con0_tmp  = 0;
    uint32 tmo_us_to_reg = 0;

    pmu_con0_tmp = PMU->PMUCON0;
    
    /* make sure the period */
    switch ((pmu_con0_tmp & (0x3 << 25)) >> 25) {
        /* rc128k_pre */
        case (0):
            tmo_us_to_reg = ((128000*tmo_us) / 1000000);
            if (tmo_us_to_reg) {
                return tmo_us_to_reg;
            } else {
                return -1;
            }
            break;
        /* x32m_div_clk */
        case (1):
            tmo_us_to_reg = ((32000000*tmo_us) / 1000000);
            if (tmo_us_to_reg) {
                return tmo_us_to_reg;
            } else {
                return -1;
            }
            break;
        /* rc10m_clk */
        case (2):
            tmo_us_to_reg = ((10000000*tmo_us) / 1000000);
            if (tmo_us_to_reg) {
                return tmo_us_to_reg;
            } else {
                return -1;
            }
            break;
        /* lxosc32k_clk */
        case (3):
            tmo_us_to_reg = ((32000*tmo_us) / 1000000);
            if (tmo_us_to_reg) {
                return tmo_us_to_reg;
            } else {
                return -1;
            }
            break;
        default:
            return -EINVAL;
            break;
    }
}

static int32 hgtimer_v5_counter_set_psc(struct hgtimer_v5 *timer, uint32 psc)
{
    struct hgtimer_v5 *dev = (struct hgtimer_v5 *)timer;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = 0;

    if (!dev->counter_en) {
        return RET_ERR;
    }

    led_timer_con0_tmp = hw->LED_TMR_CON0;

    led_timer_con0_tmp = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PSR(0x00F)) | LL_LED_TIMER_CON0_TMR_PSR(psc);
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

    return RET_OK;
}

static int32 hgtimer_v5_counter_func_open(struct timer_device *timer, enum timer_type type, uint32 flags) {

    struct hgtimer_v5 *dev = (struct hgtimer_v5 *)timer;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = 0;

    if (dev->opened) {
        return -EBUSY;
    }

    if ((-1) == hgtimer_v5_counter_switch_hal_type(dev, type)) {
        return RET_ERR;
    }
    
    irq_enable(dev->irq_num);
    hgtimer_v5_irq_config(dev, 0);
    
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, 0x00000000);

    switch (dev->type) {
        /* TIMER_TYPE_ONCE */
        case (HGTIMER_V5_TYPE_ONCE):
            
        /* TIMER_TYPE_PERIODIC */
        case (HGTIMER_V5_TYPE_PERIODIC):
            
        /* TIMER_TYPE_COUNTER */
        case (HGTIMER_V5_TYPE_COUNTER):
            //Config reg
            led_timer_con0_tmp = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_MODE(0xFFF)) | LL_LED_TIMER_CON0_TMR_MODE(0);
            led_timer_con0_tmp = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PSR(0x00F )) | LL_LED_TIMER_CON0_TMR_PSR(0 );
            pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);
            break;
        default:
            return RET_ERR;
            break;
    }

    dev->opened     = 1;
    dev->counter_en = 1;

    return RET_OK;
}

static int32 hgtimer_v5_counter_func_close(struct timer_device *timer) {

    struct hgtimer_v5 *dev = (struct hgtimer_v5 *)timer;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;

    if ((!dev->opened) || (!dev->counter_en)) {
        return RET_OK;
    }

    irq_disable(dev->irq_num);
    
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, 0x00000000);
    dev->opened      = 0;
    dev->counter_en  = 0;
    
    return RET_OK;
}

static int32 hgtimer_v5_counter_func_start(struct timer_device *timer, uint32 period_clkpd_cnt, timer_cb_hdl cb, uint32 cb_data) {

    struct hgtimer_v5 *dev   = (struct hgtimer_v5 *)timer;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    int32 tmo_us_to_reg      = 0;
    uint32 led_timer_con0_tmp   = hw->LED_TMR_CON0;

    if (!dev->counter_en) {
        return RET_ERR;
    }

    /* make sure the period */
//    tmo_us_to_reg = hgtimer_v5_counter_switch_tmo_us(tmo_us);
//    if ((-1) == tmo_us_to_reg) {
//       return -EINVAL;
//    }
    tmo_us_to_reg = period_clkpd_cnt;
    if (!tmo_us_to_reg || (tmo_us_to_reg > 0xFFF)) {
        return RET_ERR;
    }

    switch (dev->type) {
        
        /* TIMER_TYPE_ONCE */
        case (HGTIMER_V5_TYPE_ONCE):
            
            dev->counter_once_en = 1;
    
            /* config reg */
            led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PR(0xFFF)) | LL_LED_TIMER_CON0_TMR_PR(tmo_us_to_reg);
            led_timer_con0_tmp |= LL_LED_TIMER_CON0_TMR_EN(1);

            if (cb) {
                dev->_counter_irq_hdl = cb;
                dev->irq_data         = cb_data;
                hgtimer_v5_irq_config(dev, 1);
            }
           
            pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);
            break;
            
        /* TIMER_TYPE_PERIODIC */
        case (HGTIMER_V5_TYPE_PERIODIC):

        /* TIMER_TYPE_COUNTER */
        case (HGTIMER_V5_TYPE_COUNTER):
            
            dev->counter_period_en = 1;
    
            /* config reg */
            led_timer_con0_tmp  = (led_timer_con0_tmp &~ LL_LED_TIMER_CON0_TMR_PR(0xFFF)) | LL_LED_TIMER_CON0_TMR_PR(tmo_us_to_reg);
            led_timer_con0_tmp |= LL_LED_TIMER_CON0_TMR_EN(1);

            if (cb) {
                dev->_counter_irq_hdl = cb;
                dev->irq_data         = cb_data;
                hgtimer_v5_irq_config(dev, 1);
            }
           
            pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);
            break;
        default:
            return RET_ERR;
            break;
    }
    
    return RET_OK;
}

static int32 hgtimer_v5_counter_func_stop(struct timer_device *timer) {

    struct hgtimer_v5 *dev = (struct hgtimer_v5 *)timer;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 led_timer_con0_tmp = hw->LED_TMR_CON0;

    if (!dev->counter_en) {
        return RET_ERR;
    }

    led_timer_con0_tmp &= ~ LL_LED_TIMER_CON0_TMR_EN(1);
    pmu_reg_write((uint32)&hw->LED_TMR_CON0, led_timer_con0_tmp);

    return RET_OK;
}

static int32 hgtimer_v5_counter_func_ioctl(struct timer_device *timer, uint32 cmd, uint32 param1, uint32 param2) {

    int32  ret_val = RET_OK;
    struct hgtimer_v5 *dev = (struct hgtimer_v5 *)timer;

    switch (cmd) {
        case (HGPWM_V0_FUNC_CMD_MASK):
            ret_val = hgtimer_v5_pwm_config(dev, param1, 0);
            break;
        case (TIMER_SET_CLK_PSC):
            ret_val = hgtimer_v5_counter_set_psc(dev, param1);
            break;
        default:
            ret_val = RET_ERR;
            break;
    }

    return ret_val;
}

/**********************************************************************************/
/*                           COUNTER FUNCTION END                                 */
/**********************************************************************************/
static inline void hgtimer_v5_clear_count_ov_pending(struct hgtimer_v5 *dev) {

    uint32 pndclr_temp          = PMU->PNDCLR;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    uint32 addr                 = (uint32)(&hw->LED_TMR_CON0);
    
    //Confirm which led timer pending will be clear
    if (LED_TIMER0_BASE == addr) {
        pndclr_temp |= (1 << 25);
    } else if (LED_TIMER1_BASE == addr) {
        pndclr_temp |= (1 << 26);
    } else if (LED_TIMER2_BASE == addr) {
        pndclr_temp |= (1 << 27);
    } else if (LED_TIMER3_BASE == addr) {
        pndclr_temp |= (1 << 28);
    }

    pmu_reg_write((uint32)&PMU->PNDCLR, pndclr_temp);
}

static void hgtimer_v5_irq_handler(void *data) {

    struct hgtimer_v5 *dev = (struct hgtimer_v5 *)data;
    struct hgtimer_v5_hw *hw = (struct hgtimer_v5_hw *)dev->hw;
    
    if ((hw->LED_TMR_CON0 & LL_LED_TIMER_CON0_TMR_PENDING(1)) && 
         LL_LED_TIMER_CHECK_COUNT_OV_INTERRUPT_ENABLE(&(hw->LED_TMR_CON0))) {
         
        hgtimer_v5_clear_count_ov_pending(dev);

        /* pwm mode irq */
        if (dev->opened && dev->pwm_en && dev->_pwm_irq_hdl) {
            dev->_pwm_irq_hdl(PWM_IRQ_FLAG_PERIOD, dev->irq_data);
        }

        /* counter mode irq */
        if (dev->opened && dev->counter_en) {
            if (dev->counter_once_en) {
                dev->counter_once_en = 0;
            
                /* close timer & interrupt */
                hgtimer_v5_irq_config(dev, 0);
            }
            if (dev->_counter_irq_hdl) {
                dev->_counter_irq_hdl(dev->irq_data, TIMER_INTR_PERIOD);
            }
        }
    }
}

static const struct timer_hal_ops timer_v5_ops = {
    .open         = hgtimer_v5_counter_func_open,
    .close        = hgtimer_v5_counter_func_close,
    .start        = hgtimer_v5_counter_func_start,
    .stop         = hgtimer_v5_counter_func_stop,
    .ioctl        = hgtimer_v5_counter_func_ioctl,
};

int32 hgtimer_v5_attach(uint32 dev_id, struct hgtimer_v5 *timer) {

    timer->opened           = 0;
    timer->_counter_irq_hdl = NULL;
    timer->_pwm_irq_hdl     = NULL;
    timer->irq_data         = 0;
    timer->dev.dev.ops      = (const struct devobj_ops *)&timer_v5_ops;
    request_irq(timer->irq_num, hgtimer_v5_irq_handler, timer);
    dev_register(dev_id, (struct dev_obj *)timer);
    return RET_OK;
}

