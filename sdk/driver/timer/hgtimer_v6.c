/**
 * @file hgtimer_v6.c
 * @author bxd
 * @brief super timer
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
#include "dev/timer/hgtimer_v6.h"
#include "hgtimer_v6_hw.h"

/**********************************************************************************/
/*                              COMM FUNCTION                                     */
/**********************************************************************************/

#define HGTIMER_V6_HW_SUPTMRCON0_BIT (\
                                      LL_STRM_CON0_SUPTMR01_SYNC_MD_SEL(0x3)   | \
                                      LL_STRM_CON0_SUPTMR23_SYNC_MD_SEL(0x3)   | \
                                      LL_STRM_CON0_SUPTMR45_SYNC_MD_SEL(0x3)   | \
                                      LL_SUPTMR_CON0_SUPTMR_GP0_EN             | \
                                      LL_SUPTMR_CON0_SUPTMR_GP1_EN             | \
                                      LL_SUPTMR_CON0_CPMC_SEL(0x1)             | \
                                      LL_SUPTMR_CON0_LOAD_SEL(0x1)             | \
                                      LL_SUPTMR_CON0_SUPTMR0_CNT_TYPE_SEL(0x1) | \
                                      LL_SUPTMR_CON0_SUPTMR1_CNT_TYPE_SEL(0x1) | \
                                      LL_SUPTMR_CON0_SUPTMR2_CNT_TYPE_SEL(0x1) | \
                                      LL_SUPTMR_CON0_SUPTMR3_CNT_TYPE_SEL(0x1) | \
                                      LL_SUPTMR_CON0_SUPTMR4_CNT_TYPE_SEL(0x1) | \
                                      LL_SUPTMR_CON0_SUPTMR5_CNT_TYPE_SEL(0x1)   \
                                     )

static int32 hgtimer_v6_switch_suptmr_index(struct hgtimer_v6 *dev)
{

    switch ((uint32)(dev->hw_suptmrx)) {
        case ((uint32)HG_SUPTMR0_BASE):
            return 0;
            break;
        case ((uint32)HG_SUPTMR1_BASE):
            return 1;
            break;
        case ((uint32)HG_SUPTMR2_BASE):
            return 2;
            break;
        case ((uint32)HG_SUPTMR3_BASE):
            return 3;
            break;
        case ((uint32)HG_SUPTMR4_BASE):
            return 4;
            break;
        case ((uint32)HG_SUPTMR5_BASE):
            return 5;
            break;
        default:
            return -1;
            break;
    }
}


/**********************************************************************************/
/*                           PWM FUNCTION START                                   */
/**********************************************************************************/
static int32 hgtimer_v6_pwm_switch_func_cmd(enum hgpwm_v0_func_cmd param)
{

    switch (param) {
        case (HGPWM_V0_FUNC_CMD_INIT):
            return HGTIMER_V6_PWM_FUNC_CMD_INIT;
            break;
        case (HGPWM_V0_FUNC_CMD_DEINIT):
            return HGTIMER_V6_PWM_FUNC_CMD_DEINIT;
            break;
        case (HGPWM_V0_FUNC_CMD_START):
            return HGTIMER_V6_PWM_FUNC_CMD_START;
            break;
        case (HGPWM_V0_FUNC_CMD_STOP):
            return HGTIMER_V6_PWM_FUNC_CMD_STOP;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY):
            return HGTIMER_V6_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY;
            break;
        case (HGPWM_V0_FUNC_CMD_REQUEST_IRQ_COMPARE):
            return HGTIMER_V6_PWM_FUNC_CMD_REQUEST_IRQ_COMPARE;
            break;
        case (HGPWM_V0_FUNC_CMD_REQUEST_IRQ_PERIOD):
            return HGTIMER_V6_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD;
            break;
        case (HGPWM_V0_FUNC_CMD_RELEASE_IRQ):
            return HGTIMER_V6_PWM_FUNC_CMD_RELEASE_IRQ;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PRESCALER):
            return HGTIMER_V6_PWM_FUNC_CMD_IOCTL_SET_PRESCALER;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY):
            return HGTIMER_V6_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY;
            break;
        default:
            return -1;
            break;
    }
}

static inline int32 hgtimer_v6_pwm_func_init(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;
    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;

    int32  index = 0;

    if (dev->opened) {
        return -EBUSY;
    }

    if (pin_func(dev->dev.dev.dev_id, 1) != RET_OK) {
        return RET_ERR;
    }

    if ((p_config->duty < 0) || ((p_config->duty) > (p_config->period))) {
        return RET_ERR;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    /* config regs */
    switch ((uint32)(dev->hw_suptmrx)) {
        case ((uint32)HG_SUPTMR0_BASE):
            hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR0_CNT_MOD_SEL(1);
            hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
            break;
        case ((uint32)HG_SUPTMR1_BASE):
            hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR1_CNT_MOD_SEL(1);
            hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
            break;
        case ((uint32)HG_SUPTMR2_BASE):
            hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR2_CNT_MOD_SEL(1);
            hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
            break;
        case ((uint32)HG_SUPTMR3_BASE):
            hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR3_CNT_MOD_SEL(1);
            hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
            break;
        case ((uint32)HG_SUPTMR4_BASE):
            hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR4_CNT_MOD_SEL(1);
            hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
            break;
        case ((uint32)HG_SUPTMR5_BASE):
            hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR5_CNT_MOD_SEL(1);
            hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
            break;
        default:
            return RET_ERR;
            break;
    }

    /* config PWMCON: PWMVA; PWMEN */
    hw_comm->SUPTMR_PWMCON &= ~(((1) << (6 + index)) | ((0) << (0 + index)));

    /* config period */
    hw_suptmrx->PR  = p_config->period;

    /* config compare */
    hw_suptmrx->CMP = p_config->duty;

    /* clear current cnt */
    hw_suptmrx->CNT = 0;

    dev->opened = 1;
    dev->pwm_en = 1;


    return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_deinit(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;
    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;

    int32  index = 0;

    if ((!dev->opened) || (!dev->pwm_en)) {
        return RET_OK;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    irq_disable(dev->irq_num);
    pin_func(dev->dev.dev.dev_id, 0);

    /* comm reg CON1: CNTEN; */
    hw_comm->SUPTMR_CON1   &= ~((1) << (0 + index));
    /* comm reg PWMCON: PWMEN; */
    hw_comm->SUPTMR_PWMCON &= ~((1) << (0 + index));

    hw_suptmrx->CON         = 0;

    /* clear current cnt */
    hw_suptmrx->CNT = 0;

    dev->opened  = 0;
    dev->pwm_en  = 0;

    return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_start(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;
    int32  index = 0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    /* comm reg PWMCON: PWMEN*/
    hw_comm->SUPTMR_PWMCON |= (1) << (0 + index);
    /* comm reg CON1: CNTEN */
    hw_comm->SUPTMR_CON1   |= (1) << (0 + index);

    return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_stop(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;

    int32  index = 0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    /* comm reg PWMCON: PWMEN*/
    hw_comm->SUPTMR_PWMCON &= ~((1) << (0 + index));
    /* comm reg CON1: CNTEN */
    hw_comm->SUPTMR_CON1   &= ~((1) << (0 + index));

    return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_ioctl_set_period_duty(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;
    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;
    int32  index = 0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    if ((p_config->duty < 0) || ((p_config->duty) > (p_config->period))) {
        return RET_ERR;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    /* config period */
    hw_suptmrx->PR  = p_config->period;

    /* config compare */
    hw_suptmrx->CMP = p_config->duty;

    /* clear current cnt */
    hw_suptmrx->CNT = 0;

    /* comm reg CON1: LOADEN */
    hw_comm->SUPTMR_CON1 |= (1) << (8 + index);

    return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_cmd_ioctl_set_prescaler(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{
	struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

	hw_suptmrx->CON = (hw_suptmrx->CON &~ (LL_SUPTMRx_CON_PSC(0x3FF))) | (LL_SUPTMRx_CON_PSC(p_config->param1));

	return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_cmd_ioctl_set_period_duty_immediately(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{
	struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;
    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;
    int32  index = 0;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    /* comm reg PWMCON: PWMEN*/
    hw_comm->SUPTMR_PWMCON &= ~((1) << (0 + index));
    /* comm reg CON1: CNTEN */
    hw_comm->SUPTMR_CON1   &= ~((1) << (0 + index));

	hw_suptmrx->CNT = 0;
	hw_suptmrx->PR  = p_config->period;
	hw_suptmrx->CMP = p_config->duty;

	
    /* comm reg PWMCON: PWMEN*/
    hw_comm->SUPTMR_PWMCON |= (1) << (0 + index);
    /* comm reg CON1: CNTEN */
    hw_comm->SUPTMR_CON1   |= (1) << (0 + index);

    return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_request_irq_compare(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    dev->_pwm_irq_hdl = p_config->irq_hdl;
    dev->irq_data     = p_config->irq_data;
    hw_suptmrx->CON  |= LL_SUPTMRx_CON_CMPA_IE;
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_request_irq_period(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    dev->_pwm_irq_hdl = p_config->irq_hdl;
    dev->irq_data     = p_config->irq_data;
    hw_suptmrx->CON  |= LL_SUPTMRx_CON_UD_IE;
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v6_pwm_func_release_irq(struct hgtimer_v6 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;

    if (!dev->pwm_en) {
        return RET_ERR;
    }

    irq_disable(dev->irq_num);
    dev->_pwm_irq_hdl = NULL;
    dev->irq_data     = 0;
    hw_suptmrx->CON  &= ~(LL_SUPTMRx_CON_UD_IE | LL_SUPTMRx_CON_UD_IE);

    return RET_OK;
}

static int32 hgtimer_v6_pwm_config(struct hgtimer_v6 *dev, uint32 config, uint32 param)
{

    struct hgpwm_v0_config *p_config;
    int32  hgtimer_v6_pwm_func_cmd = 0;
    int32  ret_val = RET_OK;

    /* Make sure the config struct pointer */
    if (!config) {
        return -EINVAL;
    }

    p_config = (struct hgpwm_v0_config *)config;
    hgtimer_v6_pwm_func_cmd = hgtimer_v6_pwm_switch_func_cmd(p_config->func_cmd);
    if ((-1) == hgtimer_v6_pwm_func_cmd) {
        return -EINVAL;
    }

    switch (hgtimer_v6_pwm_func_cmd) {
        case (HGTIMER_V6_PWM_FUNC_CMD_INIT):
            ret_val = hgtimer_v6_pwm_func_init(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_DEINIT):
            ret_val = hgtimer_v6_pwm_func_deinit(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_START):
            ret_val = hgtimer_v6_pwm_func_start(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_STOP):
            ret_val = hgtimer_v6_pwm_func_stop(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY):
            ret_val = hgtimer_v6_pwm_func_ioctl_set_period_duty(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_REQUEST_IRQ_COMPARE):
            ret_val = hgtimer_v6_pwm_func_request_irq_compare(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD):
            ret_val = hgtimer_v6_pwm_func_request_irq_period(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_RELEASE_IRQ):
            ret_val = hgtimer_v6_pwm_func_release_irq(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_IOCTL_SET_PRESCALER):
            ret_val = hgtimer_v6_pwm_func_cmd_ioctl_set_prescaler(dev, p_config);
            break;
        case (HGTIMER_V6_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY):
            ret_val = hgtimer_v6_pwm_func_cmd_ioctl_set_period_duty_immediately(dev, p_config);
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
/*                         COUNTER FUNCTION START                                 */
/**********************************************************************************/
static int32 hgtimer_v6_counter_switch_hal_type(struct hgtimer_v6 *dev, enum timer_type param)
{

    switch (param) {
        case (TIMER_TYPE_ONCE):
            dev->type = HGTIMER_V6_TYPE_ONCE;
            return 0;
            break;
        case (TIMER_TYPE_PERIODIC):
            dev->type = HGTIMER_V6_TYPE_PERIODIC;
            return 1;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgtimer_v6_counter_set_psc(struct hgtimer_v6 *timer, uint32 psc)
{
    struct hgtimer_v6 *dev = (struct hgtimer_v6 *)timer;
    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;

    if (!dev->counter_en) {
        return RET_ERR;
    }

    hw_suptmrx->CON = (hw_suptmrx->CON & ~ LL_SUPTMRx_CON_PSC(0x3FF)) | LL_SUPTMRx_CON_PSC(psc);

    return RET_OK;
}

static int32 hgtimer_v6_counter_func_open(struct timer_device *timer, enum timer_type type, uint32 flags)
{

    struct hgtimer_v6 *dev = (struct hgtimer_v6 *)timer;
    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;
    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;

    if (dev->opened) {
        return -EBUSY;
    }

    if ((-1) == hgtimer_v6_counter_switch_hal_type(dev, type)) {
        return RET_ERR;
    }

    irq_enable(dev->irq_num);

    switch (dev->type) {
        /* TIMER_TYPE_ONCE */
        case (HGTIMER_V6_TYPE_ONCE):

        /* TIMER_TYPE_PERIODIC */
        case (HGTIMER_V6_TYPE_PERIODIC):

        /* TIMER_TYPE_COUNTER */
        case (HGTIMER_V6_TYPE_COUNTER):

            /* config regs */
            switch ((uint32)(dev->hw_suptmrx)) {
                case ((uint32)HG_SUPTMR0_BASE):
                    hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR0_CNT_MOD_SEL(1);
                    hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
                    break;
                case ((uint32)HG_SUPTMR1_BASE):
                    hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR1_CNT_MOD_SEL(1);
                    hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
                    break;
                case ((uint32)HG_SUPTMR2_BASE):
                    hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR2_CNT_MOD_SEL(1);
                    hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
                    break;
                case ((uint32)HG_SUPTMR3_BASE):
                    hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR3_CNT_MOD_SEL(1);
                    hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
                    break;
                case ((uint32)HG_SUPTMR4_BASE):
                    hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR4_CNT_MOD_SEL(1);
                    hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
                    break;
                case ((uint32)HG_SUPTMR5_BASE):
                    hw_comm->SUPTMR_CON0 = (hw_comm->SUPTMR_CON0 & ~(HGTIMER_V6_HW_SUPTMRCON0_BIT)) | LL_SUPTMR_CON0_SUPTMR5_CNT_MOD_SEL(1);
                    hw_suptmrx->CON      = LL_SUPTMRx_CON_PSC(0);    /* default psc: 1 */
                    break;
                default:
                    return RET_ERR;
                    break;
            }
            break;
        default:
            return RET_ERR;
            break;
    }

    /* clear current cnt */
    hw_suptmrx->CNT = 0;

    dev->opened     = 1;
    dev->counter_en = 1;

    return RET_OK;
}

static int32 hgtimer_v6_counter_func_close(struct timer_device *timer)
{

    struct hgtimer_v6 *dev = (struct hgtimer_v6 *)timer;
    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;
    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;
    int32  index = 0;

    if ((!dev->opened) || (!dev->counter_en)) {
        return RET_OK;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    irq_disable(dev->irq_num);

    /* config regs */
    hw_comm->SUPTMR_CON1 &= ~(1) << (0 + index);
    hw_suptmrx->CON       = 0;
    dev->opened           = 0;
    dev->counter_en       = 0;

    return RET_OK;
}

static int32 hgtimer_v6_counter_func_start(struct timer_device *timer, uint32 period_sysclkpd_cnt, timer_cb_hdl cb, uint32 cb_data)
{

    struct hgtimer_v6 *dev = (struct hgtimer_v6 *)timer;
    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;
    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;
    int32  index = 0;

    if (!dev->counter_en) {
        return RET_ERR;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    switch (dev->type) {

        /* TIMER_TYPE_ONCE */
        case (HGTIMER_V6_TYPE_ONCE):

            dev->counter_once_en = 1;

            /* config period */
            hw_suptmrx->PR = period_sysclkpd_cnt;

            /* clear current cnt */
            hw_suptmrx->CNT = 0;

            if (cb) {
                dev->_counter_irq_hdl = cb;
                dev->irq_data         = cb_data;
                hw_suptmrx->CON      |= LL_SUPTMRx_CON_OV_IE;
            }

            hw_comm->SUPTMR_CON1 |= (1) << (0 + index);
            break;

        /* TIMER_TYPE_PERIODIC */
        case (HGTIMER_V6_TYPE_PERIODIC):

        /* TIMER_TYPE_COUNTER */
        case (HGTIMER_V6_TYPE_COUNTER):

            dev->counter_period_en = 1;

            /* config period */
            hw_suptmrx->PR = period_sysclkpd_cnt;

            /* clear current cnt */
            hw_suptmrx->CNT = 0;

            if (cb) {
                dev->_counter_irq_hdl = cb;
                dev->irq_data         = cb_data;
                hw_suptmrx->CON      |= LL_SUPTMRx_CON_UD_IE;
            }

            hw_comm->SUPTMR_CON1 |= (1) << (0 + index);
            break;
        default:
            return RET_ERR;
            break;
    }

    return RET_OK;
}

static int32 hgtimer_v6_counter_func_stop(struct timer_device *timer)
{

    struct hgtimer_v6 *dev = (struct hgtimer_v6 *)timer;
    struct hgtimer_v6_hw_comm    *hw_comm    = (struct hgtimer_v6_hw_comm *)dev->hw_comm;
    int32  index = 0;

    if (!dev->counter_en) {
        return RET_ERR;
    }

    /* confirm which suptmr */
    index = hgtimer_v6_switch_suptmr_index(dev);
    if ((-1) == index) {
        return RET_ERR;
    }

    hw_comm->SUPTMR_CON1 &= ~(1) << (0 + index);

    return RET_OK;
}

static int32 hgtimer_v6_counter_func_ioctl(struct timer_device *timer, uint32 cmd, uint32 param1, uint32 param2)
{

    int32  ret_val = RET_OK;
    struct hgtimer_v6 *dev = (struct hgtimer_v6 *)timer;

    switch (cmd) {
        case (HGPWM_V0_FUNC_CMD_MASK):
            ret_val = hgtimer_v6_pwm_config(dev, param1, 0);
            break;
        case (TIMER_SET_CLK_PSC):
            ret_val = hgtimer_v6_counter_set_psc(dev, param1);
            break;
        default:
            ret_val = RET_ERR;
            break;
    }

    return ret_val;
}

/**********************************************************************************/
/*                          COUNTER FUNCTION END                                  */
/**********************************************************************************/

static void hgtimer_v6_irq_handler(void *data)
{

    struct hgtimer_v6 *dev = (struct hgtimer_v6 *)data;
    struct hgtimer_v6_hw_suptmrx *hw_suptmrx = (struct hgtimer_v6_hw_suptmrx *)dev->hw_suptmrx;


    if ((hw_suptmrx->CON & LL_SUPTMRx_CON_UD_IF) &&
        (hw_suptmrx->CON & LL_SUPTMRx_CON_UD_IE)) {
        /* clear the pending */
        hw_suptmrx->CON |= LL_SUPTMRx_CON_UD_IF_CLR;

        /* pwm mode irq */
        if (dev->opened && dev->pwm_en && dev->_pwm_irq_hdl) {
            dev->_pwm_irq_hdl(PWM_IRQ_FLAG_PERIOD, dev->irq_data);
        }

        /* counter mode irq */
        if (dev->opened && dev->counter_en) {
            if (dev->counter_once_en) {
                dev->counter_once_en = 0;

                /* close timer & interrupt */
                hw_suptmrx->CON &= ~ LL_SUPTMRx_CON_UD_IE;
            }
            if (dev->_counter_irq_hdl) {
                dev->_counter_irq_hdl(dev->irq_data, TIMER_INTR_PERIOD);
            }
        }
    }


    if ((hw_suptmrx->CON & LL_SUPTMRx_CON_CMPA_IF) &&
        (hw_suptmrx->CON & LL_SUPTMRx_CON_CMPA_IE)) {
        /* clear the pending */
        hw_suptmrx->CON |= LL_SUPTMRx_CON_CMPA_IF_CLR;

        /* pwm mode irq */
        if (dev->opened && dev->pwm_en && dev->_pwm_irq_hdl) {
            dev->_pwm_irq_hdl(PWM_IRQ_FLAG_PERIOD, dev->irq_data);
        }
    }
}

static const struct timer_hal_ops timer_v6_ops = {
    .open         = hgtimer_v6_counter_func_open,
    .close        = hgtimer_v6_counter_func_close,
    .start        = hgtimer_v6_counter_func_start,
    .stop         = hgtimer_v6_counter_func_stop,
    .ioctl        = hgtimer_v6_counter_func_ioctl,
};

int32 hgtimer_v6_attach(uint32 dev_id, struct hgtimer_v6 *timer)
{

    timer->opened           = 0;
    timer->_counter_irq_hdl = NULL;
    timer->_pwm_irq_hdl     = NULL;
    timer->irq_data         = 0;
    timer->dev.dev.ops      = (const struct devobj_ops *)&timer_v6_ops;
    request_irq(timer->irq_num, hgtimer_v6_irq_handler, timer);
    dev_register(dev_id, (struct dev_obj *)timer);
    return RET_OK;
}


