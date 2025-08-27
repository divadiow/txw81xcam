#ifndef _HGTIMER5_HW_H
#define _HGTIMER5_HW_H

#ifdef __cplusplus
extern "C" {
#endif


/***** LED_TIMER TIMERx CON0 Register *****/
/*! LED_TIMER Period registers. Timer count always start at 0.
 */
#define LL_LED_TIMER_CON0_TMR_PR(n)               ((n & 0xFFF) << 20)
/*! LED_TIMER PWM registers. Write this always configure same data to PWM register.
 */
#define LL_LED_TIMER_CON0_TMR_STEP(n)             ((n & 0xFFF) <<  8)
/*! LED_TIMER Prescaler registers.
 */
#define LL_LED_TIMER_CON0_TMR_PSR(n)              ((n & 0x00F) <<  4)
/*! LED_TIMER pending.
 */
#define LL_LED_TIMER_CON0_TMR_PENDING(n)          ((n & 0x001) <<  3)
/*! LED_TIMER Mode registers.
 */
#define LL_LED_TIMER_CON0_TMR_MODE(n)             ((n & 0x003) <<  1)
/*! LED_TIMER EN registers.
 */
#define LL_LED_TIMER_CON0_TMR_EN(n)               ((n & 0x001) <<  0)


struct hgtimer_v5_hw {
    __IO uint32_t LED_TMR_CON0;
};



#ifdef __cplusplus
}
#endif

#endif /*  */