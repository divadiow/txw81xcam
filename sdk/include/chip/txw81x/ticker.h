#ifndef _TICKER_H_
#define _TICKER_H_

#ifdef __cplusplus
extern "C" {
#endif

//#define TIMER0_BASE             (APB1_BASE + 0x5000)
#define TIMER0                  ((struct hgtimer_v2_hw *) TIMER0_BASE)


#define HW_TIMER_PSC        7//HGTIMER_V2_TIMER_PSC_128

/**
  * @brief  Time(us) to timer tick
  * @param  n : Time in us
  * @retval Time(us) to timer tick
  */
#define HW_TIMER_T2C(n)         (((n)*(system_clock_get()/1000000)) >> (HW_TIMER_PSC))
/**
  * @brief  timer tick to Time(us)
  * @param  n : timer tick
  * @retval timer tick to Time(us)
  */
#define HW_TIMER_C2T(n)         ((n)*(1 << HW_TIMER_PSC)/(system_clock_get()/1000000))

/**
  * @brief  The tick timer is initialized.
  * @param  None
  * @retval None
  * @note   Use the lowest priority interrupt.
  */
//void hw_timer_init(void);

/**
  * @brief  get the tick of the timer.
  * @param  None
  * @retval None
  */
//uint32 hw_timer_get_tick(uint8 start);

/**
  * @brief  tick timer interrupt handler.
  * @param  None
  * @retval None
  */
//void hw_timer_irq_handler(void);

#ifdef __cplusplus
}
#endif
#endif
