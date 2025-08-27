#ifndef _HGTIMER_V3_H_
#define _HGTIMER_V3_H_
#include "hal/timer_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief TIMER register structure
  * @{
  */
struct hgtimer_hw {
    __IO uint32_t TMR_CON;
    __IO uint32_t TMR_EN;
    __IO uint32_t TMR_IE;
    __IO uint32_t TMR_CNT;
    __IO uint32_t TMR_FLG;
    __IO uint32_t TMR_CLR;
    __IO uint32_t TMR_CAP1;
    __IO uint32_t TMR_CAP2;
    __IO uint32_t TMR_CAP3;
    __IO uint32_t TMR_CAP4;
    __IO uint32_t TMR_DCTL;
    __IO uint32_t TMR_DADR;
    __IO uint32_t TMR_DLEN;
    __IO uint32_t TMR_DCNT;
    __IO uint32_t TMR_IR_BCNT;
};

struct hgtimer {
    struct timer_device     dev;
    struct hgtimer_hw      *hw;
    uint32                  irq_num;
    timer_cb_hdl            cb;
    uint32                  cb_data;
    enum timer_type         mode;
    uint32                  flags;
    uint32                  opened;
};

struct hgtimer_all {
    __IO uint32_t TMR_ALLCON;
};


/**
  * @}
  */

/** @defgroup TIMER_Register_Constants TIMER Register Constants
  * @ingroup  TIMER_Driver
  * @brief    TIMER register constant table definition
  * @{
  */

/***** TIMERx CON Register *****/
/*! Timer IR selection from TIMER0, TIMER2~7
 */
#define LL_TIMER_CON_IR_FIRST_LSB                 (1UL << 29)
/*! Timer IR function logic 0 PWM polarity selection (for TIMER1 only)
 */
#define LL_TIMER_CON_IR_ZERO_PWMPOL               (1UL << 28)
/*! Timer IR function logic 1 PWM polarity selection (for TIMER1 only)
 */
#define LL_TIMER_CON_IR_ONE_PWMPOL                (1UL << 27)
/*! Timer IR function enable (for TIMER1 only)
 */
#define LL_TIMER_CON_IR_EN                        (1UL << 26)
/*! PWM polarity selection in TIMER module
 */
#define LL_TIMER_CON_PWMPOL                       (1UL << 25)
/*! The polarity of capture event 4 is selected: 0 = rising edge, 1 = falling edge.
 */
#define LL_TIMER_CON_CAP4POL                      (1UL << 24)
/*! The polarity of capture event 3 is selected: 0 = rising edge, 1 = falling edge.
 */
#define LL_TIMER_CON_CAP3POL                      (1UL << 23)
/*! The polarity of capture event 2 is selected: 0 = rising edge, 1 = falling edge.
 */
#define LL_TIMER_CON_CAP2POL                      (1UL << 22)
/*! The polarity of capture event 1 is selected: 0 = rising edge, 1 = falling edge.
 */
#define LL_TIMER_CON_CAP1POL                      (1UL << 21)
/*! When the capture event 4 occurs, the value of the CNT is automatically cleared.
 */
#define LL_TIMER_CON_CTRRST4                      (1UL << 20)
/*! When the capture event 3 occurs, the value of the CNT is automatically cleared.
 */
#define LL_TIMER_CON_CTRRST3                      (1UL << 19)
/*! When the capture event 2 occurs, the value of the CNT is automatically cleared.
 */
#define LL_TIMER_CON_CTRRST2                      (1UL << 18)
/*! When the capture event 1 occurs, the value of the CNT is automatically cleared.
 */
#define LL_TIMER_CON_CTRRST1                      (1UL << 17)
/*! The number of the Capture register:
 *  00 : Capture data store in CAP1
 *  01 : Capture data store in CAP1 CAP2
 *  10 : Capture data store in CAP1 CAP2 CAP3
 *  11 : Capture data store in CAP1 CAP2 CAP3 CAP4
 */
#define LL_TIMER_CON_CAP_CNT(n)                   (((n)&0x3) << 15)
/*! Capture selection:
 *  00 : GPIO
 *  01 : GPIO OR
 *  10 : compare0 output
 *  11 : compare1 output
 */
#define LL_TIMER_CON_CAP_SEL(n)                   (((n)&0x3) << 13)
/*! Output sync signal selection:
 *  00 : CNT value = PRD value
 *  01 : CNT value = CMP value
 *  10 : Output SYNCI value to SYNCO
 *  11 : PWM output is assigned to SYNCO
 */
#define LL_TIMER_CON_SYNCO_SEL(n)                 (((n)&0x3) << 11)
/*! Synci polarity inversion:
 *  1 : Invert
 *  0 : not reversed
 */
#define LL_TIMER_CON_SYNCI_POL                    (1UL << 10)
/*! Synci function selection
 *  00 : disable
 *  01 : kick start
 *  10 : reset
 *  11 : gating
 */
#define LL_TIMER_CON_SLAVE_MODE(n)                (((n)&0x3) << 8)
/*! Timer prescaler settings:
 *  000 : 0 frequency division
 *  001 : 2 frequency division
 *  010 : 4 frequency division
 *  011 : 8 frequency division
 *  100 : 16 frequency division
 *  101 : 32 frequency division
 *  110 : 64 frequency division
 *  111 : 128 frequency division
 */
#define LL_TIMER_CON_PSC(n)                       (((n)&0x7) << 5)
/*! Timer counter source select bits:
 *  001 : Internal high speed RC
 *  010 : Internal low speed RC
 *  011 : External crystal oscillator divided by 2 clocks
 *  100 : timer inc pin rising
 *  101 : timer inc pin falling
 *  110 : timer inc pin rising and falling
 *  Others : system clock
 */
#define LL_TIMER_CON_INC_SRC_SEL(n)               (((n)&0x7) << 2)
/*! Timer mode select bits:
 *  00 : timer counter mode
 *  01 : timer pwm mode
 *  10 : timer capture mode
 *  Others : reservation
 */
#define LL_TIMER_CON_MODE_SEL(n)                  (((n)&0x3) << 0)


/***** TIMERx EN Register *****/
/*! TMR enable signal, active high.
 */
#define LL_TIMER_EN_TMREN                         (1UL << 0)


/***** TIMERx IE Register *****/
/*! Timer IR TX word done interrupt enable (for TIMER1 only)
 */
#define LL_TIMER_IE_IR_TX_WORD_DONE_IE            (1UL << 11)
/*! Timer IR TX done interrupt enable (for TIMER1 only)
 */
#define LL_TIMER_IE_IR_TX_DONE_IE                 (1UL << 10)
/*! Dma buffer full interrupt enable.
 */
#define LL_TIMER_IE_DMA_FL_IE                     (1UL << 9)
/*! Dma buffer half full interrupt enable.
 */
#define LL_TIMER_IE_DMA_HF_IE                     (1UL << 8)
/*! The slave mode trigger mode or reset mode interrupt enable.
 */
#define LL_TIMER_IE_SLAVE_IE                      (1UL << 7)
/*! When the CNT value is equal to the CMP value, the interrupt is enabled and is valid only in pwm mode.
 */
#define LL_TIMER_IE_CMP_IE                        (1UL << 6)
/*! When the CNT value is equal to the PRD value, the interrupt is enabled and is valid only in the counter mode/PWM mode.
 */
#define LL_TIMER_IE_PRD_IE                        (1UL << 5)
/*! When the CNT value overflows (16'hffff), the interrupt is enabled.
 */
#define LL_TIMER_IE_OVF_IE                        (1UL << 4)
/*! When the capture event 4 occurs, the interrupt is enabled.
 */
#define LL_TIMER_IE_CAP4_IE                       (1UL << 3)
/*! When the capture event 3 occurs, the interrupt is enabled.
 */
#define LL_TIMER_IE_CAP3_IE                       (1UL << 2)
/*! When the capture event 2 occurs, the interrupt is enabled.
 */
#define LL_TIMER_IE_CAP2_IE                       (1UL << 1)
/*! When the capture event 1 occurs, the interrupt is enabled.
 */
#define LL_TIMER_IE_CAP1_IE                       (1UL << 0)


/***** TIMERx CNT Register *****/
/*! Count register.
 */
#define LL_TIMER_CNT(n)                           (((n)&0xFFFFFFFF) << 0)


/***** TIMERx FLG Register *****/
/*! Timer IR TX word done interrupt flag (for TIMER1 only)
 */
#define LL_TIMER_IE_IR_TX_WORD_DONE_FLG           (1UL << 11)
/*! Timer IR TX done interrupt flag (for TIMER1 only)
 */
#define LL_TIMER_IE_IR_TX_DONE_FLG                (1UL << 10)
/*! Dma buffer full sign.
 */
#define LL_TIMER_IE_DMA_FL_FLG                    (1UL << 9)
/*! Dma buffer half full sign.
 */
#define LL_TIMER_IE_DMA_HF_FLG                    (1UL << 8)
/*! The slave mode flag (reset or trigger only).
 */
#define LL_TIMER_IE_SLAVE_FLG                     (1UL << 7)
/*! The CNT value is equal to the CMP value flag and is valid only in pwm mode.
 */
#define LL_TIMER_IE_CMP_FLG                       (1UL << 6)
/*! The CNT value is equal to the PRD value flag and is valid only in counter mode/PWM mode.
 */
#define LL_TIMER_IE_PRD_FLG                       (1UL << 5)
/*! CNT value overflow (16'hffff) flag.
 */
#define LL_TIMER_IE_OVF_FLG                       (1UL << 4)
/*! Capture event 4 occurs.
 */
#define LL_TIMER_IE_CAP4_FLG                      (1UL << 3)
/*! Capture event 3 occurs.
 */
#define LL_TIMER_IE_CAP3_FLG                      (1UL << 2)
/*! Capture event 2 occurs.
 */
#define LL_TIMER_IE_CAP2_FLG                      (1UL << 1)
/*! Capture event 1 occurs.
 */
#define LL_TIMER_IE_CAP1_FLG                      (1UL << 0)


/***** TIMERx CLR Register *****/
/*! Timer IR TX word done interrupt clear (for TIMER1 only)
 */
#define LL_TIMER_IE_IR_TX_WORD_DONE_CLR           (1UL << 11)
/*! Timer IR TX done interrupt clear (for TIMER1 only)
 */
#define LL_TIMER_IE_IR_TX_DONE_CLR                (1UL << 10)
/*! Dma buffer full sign clear.
 */
#define LL_TIMER_IE_DMA_FL_CLR                    (1UL << 9)
/*! Dma buffer half full sign clear.
 */
#define LL_TIMER_IE_DMA_HF_CLR                    (1UL << 8)
/*! The slave mode flag (reset or trigger only) clear.
 */
#define LL_TIMER_IE_SLAVE_CLR                     (1UL << 7)
/*! The CNT value is equal to the CMP value flag clear.
 */
#define LL_TIMER_IE_CMP_CLR                       (1UL << 6)
/*! The CNT value is equal to the PRD value flag clear.
 */
#define LL_TIMER_IE_PRD_CLR                       (1UL << 5)
/*! The CNT value overflow (16'hffff) flag is cleared.
 */
#define LL_TIMER_IE_OVF_CLR                       (1UL << 4)
/*! Capture event 4 occurs flag clear.
 */
#define LL_TIMER_IE_CAP4_CLR                      (1UL << 3)
/*! Capture event 3 occurs flag clear.
 */
#define LL_TIMER_IE_CAP3_CLR                      (1UL << 2)
/*! Capture event 2 occurs flag clear.
 */
#define LL_TIMER_IE_CAP2_CLR                      (1UL << 1)
/*! Capture event 1 occurs flag clear.
 */
#define LL_TIMER_IE_CAP1_CLR                      (1UL << 0)


/***** TIMERx CAP1/PR Register *****/
/*! Capture mode        : capture register 1
 *  Timing mode/PWM mode: Count period register
 */
#define LL_TIMER_CMP1_PR(n)                       (((n)&0xFFFFFFFF) << 0)


/***** TIMERx CAP2/CMP Register *****/
/*! Capture mode        : capture register 2
 *  Timing mode/PWM mode: compare register
 */
#define LL_TIMER_CMP2_CMP(n)                      (((n)&0xFFFFFFFF) << 0)


/***** TIMERx CAP3/PR_SD Register *****/
/*! Capture mode        : capture register 3
 *  Timing mode/PWM mode: counting period shadow register
 */
#define LL_TIMER_CMP3_PR_SD(n)                    (((n)&0xFFFFFFFF) << 0)


/***** TIMERx CAP4/CMP_SD Register *****/
/*! Capture mode        : capture register 4
 *  Timing mode/PWM mode: Compare shadow registers
 */
#define LL_TIMER_CMP4_CMP_SD(n)                   (((n)&0xFFFFFFFF) << 0)


/***** TIMERx DCCTL Register *****/
/*! Dma mode selection
 *  0 : Single mode, after the specified dma length is completed, disable TMR.
 *  1 : Loop mode, after the dma length is specified, restart from the start address.
 */
#define LL_TIMER_DMA_LPBK                         (1UL << 1)
/*! Dma enabled.
 */
#define LL_TIMER_DMA_EN                           (1UL << 0)


/***** TIMERx DADR Register *****/
/*! Dma starting address.
 */
#define LL_TIMER_DADR_STADR(n)                    (((n)&0xFFFF) << 0)


/***** TIMERx DLEN Register *****/
/*! Dma buffer length (32bit), if the buffer is n, the configuration is n-1;
 */
#define LL_TIMER_DLEN_LEN(n)                      (((n)&0xFFFF) << 0)


/***** TIMERx DCNT Register *****/
/*! The number of dma data is valid.
 */
#define LL_TIMER_DCNT_CNT(n)                      (((n)&0xFFFF) << 0)


/***** TIMER ALLCON Register *****/
/*! The timer sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR5_SYNC                 (1UL << 13)
/*! The timer3 sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR3_SYNC                 (1UL << 11)
/*! The timer2 sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR2_SYNC                 (1UL << 10)
/*! The timer1 sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR1_SYNC                 (1UL << 9)
/*! The timer0 sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR0_SYNC                 (1UL << 8)
/*! Timer5 starts counting.
 */
#define LL_TIMER_ALLCON_TMR5_KICK                 (1UL << 5)
/*! Timer3 starts counting.
 */
#define LL_TIMER_ALLCON_TMR3_KICK                 (1UL << 3)
/*! Timer2 starts counting.
 */
#define LL_TIMER_ALLCON_TMR2_KICK                 (1UL << 2)
/*! Timer1 starts counting.
 */
#define LL_TIMER_ALLCON_TMR1_KICK                 (1UL << 1)
/*! Timer0 starts counting.
 */
#define LL_TIMER_ALLCON_TMR0_KICK                 (1UL << 0)
/*! Configure the trigger path for the sync count of TIMER.
 */
#define LL_TIMER_ALLCON_SYNC_COUNT_ALL(n)         (((n)&0x3F) << 8)
/*! Configure the synchronous trigger path of TIMER.
 */
#define LL_TIMER_ALLCON_KICK_ALL(n)               (((n)&0x3F) << 0)


/**
  * @}
  */

/** @defgroup TIMER_Exported_Constants TIMER Exported Constants
  * @ingroup  TIMER_Driver
  * @brief    TIMER external constant definition
  * @{
  */

/**
  * @brief Enumeration constant for low layer TIMER Prescaler for timer
  */
enum hgtimer_psc {
    /*! Timer clock source 0 prescaler
     */
    HGTIMER_PSC_0 = 0,
    /*! Timer clock source 2 prescaler
     */
    HGTIMER_PSC_2,
    /*! Timer clock source 4 prescaler
     */
    HGTIMER_PSC_4,
    /*! Timer clock source 8 prescaler
     */
    HGTIMER_PSC_8,
    /*! Timer clock source 16 prescaler
     */
    HGTIMER_PSC_16,
    /*! Timer clock source 32 prescaler
     */
    HGTIMER_PSC_32,
    /*! Timer clock source 64 prescaler
     */
    HGTIMER_PSC_64,
    /*! Timer clock source 128 prescaler
     */
    HGTIMER_PSC_128,
};

/**
  * @brief Enumeration constant for low layer TIMER clock source
  */
enum hgtimer_src_sel {
    /*! Select the internal high speed RC clock.
     */
    HGTIMER_SOURCE_INTER_HIGH_SPEED_RC = 0x1,
    /*! Select the internal low speed RC clock.
     */
    HGTIMER_SOURCE_INTER_LOW_SPEED_RC,
    /*! Select the external crystal and divide by 2.
     */
    HGTIMER_SOURCE_EXTER_CRYSTAL_DIV_2,
    /*! Select the pin input from the outside and on the rising edge.
     */
    HGTIMER_SOURCE_INC_PIN_RISING,
    /*! Select the pin input from the outside and on the falling edge.
     */
    HGTIMER_SOURCE_INC_PIN_FALLING,
    /*! Select the pin input from the outside and on both the rising edge and the falling edge.
     */
    HGTIMER_SOURCE_INC_PIN_RISING_FALLING,
    /*! Select the system clock.
     */
    HGTIMER_SOURCE_SYS_CLK,
};

/**
  * @brief Enumeration constant for low layer TIMER the pattern of the timer
  */
enum hgtimer_mode {
    /*! Timer mode selection: counter mode
     */
    HGTIMER_MODE_COUNTER,
    /*! Timer mode selection: PWM mode
     */
    HGTIMER_MODE_PWM,
    /*! Timer mode selection: capture mode
     */
    HGTIMER_MODE_CAPTURE,
};

/**
  * @brief Enum constants, Mode selection in slave mode in TIMER.
  */
typedef enum {
    /*! Disable.
     */
    HGTIMER_SLAVE_MODE_DISABLE = 0,
    /*! Select the trigger mode.
     */
    HGTIMER_SLAVE_MODE_KICK_START,
    /*! Select the reset mode.
     */
    HGTIMER_SLAVE_MODE_RESET,
    /*! Select the gate mode.
     */
    HGTIMER_SLAVE_MODE_GATING,
} TYEP_ENUM_LL_TIMER_SLAVE_MODE;

/**
  * @}
  */

int32 hgtimer_attach(uint32 dev_id, struct hgtimer *timer);

#ifdef __cplusplus
}
#endif

#endif /* _HGTIMER_V3_H_ */
