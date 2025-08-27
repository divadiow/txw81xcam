#ifndef _HGLED_V0_HW_H
#define _HGLED_V0_HW_H

#ifdef __cplusplus
    extern "C" {
#endif

/* **** **** LED_CON **** **** */

/* Seg enable
 */
#define LL_LED_CON_SEG_EN(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 20))

/* Com enable
 */
#define LL_LED_CON_COM_EN(n)                                    ((uint32)(((uint32)(n)&0xFF) << 12))

/* LED interrupt pending clear
 */
#define LL_LED_CON_PENDING_CLR                                  ((uint32)(1UL << 5))

/* LED interrupt pending
 */
#define LL_LED_CON_PENDING                                      ((uint32)(1UL << 4))

/* LED interrupt enable
 */
#define LL_LED_CON_IE                                           ((uint32)(1UL << 3))

/* LED load data from shadow register
 */
#define LL_LED_CON_LOAD_DATA_EN                                 ((uint32)(1UL << 2))

/* LED scan type selcect
 */
#define LL_LED_CON_SCAN_SEL(n)                                  ((uint32)(((uint32)(n)&0x1) << 1))

/* LEN enable
 */
#define LL_LED_CON_EN                                           ((uint32)(1UL << 0))


/* **** **** LED_TIME **** **** */

/* LED clock prescale
 */
#define LL_LED_TIME_CLK_PSC(n)                                  ((uint32)(((uint32)(n)&0x0FFF) << 16))

/* LED gray control
 */
#define LL_LED_TIME_GRAY(n)                                     ((uint32)(((uint32)(n)&0xFF) << 8))

/* LED time length
 */
#define LL_LED_TIME_LEN(n)                                      ((uint32)(((uint32)(n)&0xFF) << 0))


/* **** **** LED_DATA0 **** **** */
#define LL_LED_DATA0_COM0(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 0))
#define LL_LED_DATA0_COM1(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 16))


/* **** **** LED_DATA1 **** **** */
#define LL_LED_DATA1_COM2(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 0))
#define LL_LED_DATA1_COM3(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 16))


/* **** **** LED_DATA2 **** **** */
#define LL_LED_DATA2_COM4(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 0))
#define LL_LED_DATA2_COM5(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 16))


/* **** **** LED_DATA3 **** **** */
#define LL_LED_DATA3_COM6(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 0))
#define LL_LED_DATA3_COM7(n)                                    ((uint32)(((uint32)(n)&0x0FFF) << 16))


struct hgled_v0_hw {
    __IO uint32_t CON;
    __IO uint32_t TIME;
    __IO uint32_t DATA0;
    __IO uint32_t DATA1;
    __IO uint32_t DATA2;
    __IO uint32_t DATA3;
};






#ifdef __cplusplus
}
#endif

#endif /* _HGLED_V0_HW_H */