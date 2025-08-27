#ifndef _HGTIMER_V6_HW_H
#define _HGTIMER_V6_HW_H

#ifdef __cplusplus
extern "C" {
#endif

/* **** SUPTMR_CON0 **** */

/* Clear SUPTMR5 counter
 */
#define LL_SUPTMR_CON0_SUPTMR5_CNT_CLR                          ((uint32)(1UL << 29))

/* Clear SUPTMR4 counter
 */
#define LL_SUPTMR_CON0_SUPTMR4_CNT_CLR                          ((uint32)(1UL << 28))

/* Clear SUPTMR3 counter
 */
#define LL_SUPTMR_CON0_SUPTMR3_CNT_CLR                          ((uint32)(1UL << 27))

/* Clear SUPTMR2 counter
 */
#define LL_SUPTMR_CON0_SUPTMR2_CNT_CLR                          ((uint32)(1UL << 26))

/* Clear SUPTMR1 counter
 */
#define LL_SUPTMR_CON0_SUPTMR1_CNT_CLR                          ((uint32)(1UL << 25))

/* Clear SUPTMR0 counter
 */
#define LL_SUPTMR_CON0_SUPTMR0_CNT_CLR                          ((uint32)(1UL << 24))

/* SUPTMR5 count type select
 */
#define LL_SUPTMR_CON0_SUPTMR5_CNT_TYPE_SEL(n)                  ((uint32)(((uint32)(n)&0x1) << 21))

/* SUPTMR4 count type select
 */
#define LL_SUPTMR_CON0_SUPTMR4_CNT_TYPE_SEL(n)                  ((uint32)(((uint32)(n)&0x1) << 20))

/* SUPTMR3 count type select
 */
#define LL_SUPTMR_CON0_SUPTMR3_CNT_TYPE_SEL(n)                  ((uint32)(((uint32)(n)&0x1) << 19))

/* SUPTMR2 count type select
 */
#define LL_SUPTMR_CON0_SUPTMR2_CNT_TYPE_SEL(n)                  ((uint32)(((uint32)(n)&0x1) << 18))

/* SUPTMR1 count type select
 */
#define LL_SUPTMR_CON0_SUPTMR1_CNT_TYPE_SEL(n)                  ((uint32)(((uint32)(n)&0x1) << 17))

/* SUPTMR0 count type select
 */
#define LL_SUPTMR_CON0_SUPTMR0_CNT_TYPE_SEL(n)                  ((uint32)(((uint32)(n)&0x1) << 16))

/* Auto load point select
 */
#define LL_SUPTMR_CON0_LOAD_SEL(n)                              ((uint32)(((uint32)(n)&0x1) << 15))

/* CMPC flag point select
 */
#define LL_SUPTMR_CON0_CPMC_SEL(n)                              ((uint32)(((uint32)(n)&0x1) << 14))

/* SUPTMR5 count mode select
 */
#define LL_SUPTMR_CON0_SUPTMR5_CNT_MOD_SEL(n)                   ((uint32)(((uint32)(n)&0x1) << 13))

/* SUPTMR4 count mode select
 */
#define LL_SUPTMR_CON0_SUPTMR4_CNT_MOD_SEL(n)                   ((uint32)(((uint32)(n)&0x1) << 12))

/* SUPTMR3 count mode select
 */
#define LL_SUPTMR_CON0_SUPTMR3_CNT_MOD_SEL(n)                   ((uint32)(((uint32)(n)&0x1) << 11))

/* SUPTMR2 count mode select
 */
#define LL_SUPTMR_CON0_SUPTMR2_CNT_MOD_SEL(n)                   ((uint32)(((uint32)(n)&0x1) << 10))

/* SUPTMR1 count mode select
 */
#define LL_SUPTMR_CON0_SUPTMR1_CNT_MOD_SEL(n)                   ((uint32)(((uint32)(n)&0x1) << 9))

/* SUPTMR0 count mode select
 */
#define LL_SUPTMR_CON0_SUPTMR0_CNT_MOD_SEL(n)                   ((uint32)(((uint32)(n)&0x1) << 8))

/* Enable SUPTMR1 SUPTMR3 and SUPTMR5 group function
 */
#define LL_SUPTMR_CON0_SUPTMR_GP1_EN                            ((uint32)(1UL << 7))

/* Enable SUPTMR0 SUPTMR2 and SUPTMR4 group function
 */
#define LL_SUPTMR_CON0_SUPTMR_GP0_EN                            ((uint32)(1UL << 6))

/* SUPTMR4 SUPTMR5 sync mode select
 */
#define LL_STRM_CON0_SUPTMR45_SYNC_MD_SEL(n)                    ((uint32)(((uint32)(n)&0x3) << 4))

/* SUPTMR2 SUPTMR3 sync mode select
 */
#define LL_STRM_CON0_SUPTMR23_SYNC_MD_SEL(n)                    ((uint32)(((uint32)(n)&0x3) << 2))

/* SUPTMR0 SUPTMR1 sync mode select
 */
#define LL_STRM_CON0_SUPTMR01_SYNC_MD_SEL(n)                    ((uint32)(((uint32)(n)&0x3) << 0))



/* **** SUPTMR_CON1 **** */

/* Set SUPTMR CMPC value
 */
#define LL_SUPTMR_CON1_SUPTMR_CMPC(n)                           ((uint32)(((uint32)(n)&0xFFFF) << 16))

/* Enable SUPTMR5 auto load
 */
#define LL_SUPTMR_CON1_SUPTMR5_LOAD_EN(x)                       ((uint32)((x&0x1) << 13))

/* Enable SUPTMR4 auto load
 */
#define LL_SUPTMR_CON1_SUPTMR4_LOAD_EN(x)                       ((uint32)((x&0x1) << 12))

/* Enable SUPTMR3 auto load
 */
#define LL_SUPTMR_CON1_SUPTMR3_LOAD_EN(x)                       ((uint32)((x&0x1) << 11))

/* Enable SUPTMR2 auto load
 */
#define LL_SUPTMR_CON1_SUPTMR2_LOAD_EN(x)                       ((uint32)((x&0x1) << 10))

/* Enable SUPTMR1 auto load
 */
#define LL_SUPTMR_CON1_SUPTMR1_LOAD_EN(x)                       ((uint32)((x&0x1) << 9))

/* Enable SUPTMR0 auto load
 */
#define LL_SUPTMR_CON1_SUPTMR0_LOAD_EN(x)                       ((uint32)((x&0x1) << 8))

/* Enable all SUPTMR auto load
 */
#define LL_STRM_CON1_ALL_SUPTMR_LOAD_EN                         ((uint32)(0x3F << 8))

/* Enable SUPTMR5 counter
 */
#define LL_SUPTMR_CON1_SUPTMR5_CNT_EN(x)                        ((uint32)((x&0x1) << 5))

/* Enable SUPTMR4 counter
 */
#define LL_SUPTMR_CON1_SUPTMR4_CNT_EN(x)                        ((uint32)((x&0x1) << 4))

/* Enable SUPTMR3 counter
 */
#define LL_SUPTMR_CON1_SUPTMR3_CNT_EN(x)                        ((uint32)((x&0x1) << 3))

/* Enable SUPTMR2 counter
 */
#define LL_SUPTMR_CON1_SUPTMR2_CNT_EN(x)                        ((uint32)((x&0x1) << 2))

/* Enable SUPTMR1 counter
 */
#define LL_SUPTMR_CON1_SUPTMR1_CNT_EN(x)                        ((uint32)((x&0x1) << 1))

/* Enable SUPTMR0 counter
 */
#define LL_SUPTMR_CON1_SUPTMR0_CNT_EN(x)                        ((uint32)((x&0x1) << 0))



/* **** SUPTMRx_CON **** */

/* SUPTMRx BRAKE flag clear
 */
#define LL_SUPTMRx_CON_BRK_IF_CLR                               ((uint32)(1UL << 31))

/* SUPTMRx counter equal to CMPB flag clear
 */
#define LL_SUPTMRx_CON_CMPB_IF_CLR                              ((uint32)(1UL << 30))

/* SUPTMRx counter equal to CMPA flag clear
 */
#define LL_SUPTMRx_CON_CMPA_IF_CLR                              ((uint32)(1UL << 29))

/* SUPTMRx counter equal to zero flag clear
 */
#define LL_SUPTMRx_CON_UD_IF_CLR                                ((uint32)(1UL << 28))

/* SUPTMRx counter equal to period flag clear
 */
#define LL_SUPTMRx_CON_OV_IF_CLR                                ((uint32)(1UL << 27))

/* SUPTMRx BRAKE flag
 */
#define LL_SUPTMRx_CON_BRK_IF                                   ((uint32)(1UL << 26))

/* SUPTMRx counter equal to CMPB flag
 */
#define LL_SUPTMRx_CON_CMPB_IF                                  ((uint32)(1UL << 25))

/* SUPTMRx counter equal to CMPA flag
 */
#define LL_SUPTMRx_CON_CMPA_IF                                  ((uint32)(1UL << 24))

/* SUPTMRx counter equal to zero flag
 */
#define LL_SUPTMRx_CON_UD_IF                                    ((uint32)(1UL << 23))

/* SUPTMRx counter equal to period flag
 */
#define LL_SUPTMRx_CON_OV_IF                                    ((uint32)(1UL << 22))

/* Enable SUPTMRx BRAKE interrupt
 */
#define LL_SUPTMRx_CON_BRAKE_IE                                 ((uint32)(1UL << 21))

/* Enable SUPTMRx counter equal to CMPB interrupt
 */
#define LL_SUPTMRx_CON_CMPB_IE                                  ((uint32)(1UL << 20))

/* Enable SUPTMRx counter equal to CMPA interrupt
 */
#define LL_SUPTMRx_CON_CMPA_IE                                  ((uint32)(1UL << 19))

/* Enable SUPTMRx counter equal to zero interrupt
 */
#define LL_SUPTMRx_CON_UD_IE                                    ((uint32)(1UL << 18))

/* Enable SUPTMRx counter equal to period interrupt
 */
#define LL_SUPTMRx_CON_OV_IE                                    ((uint32)(1UL << 17))

/* Set SUPTMRx clock prescale
 */
#define LL_SUPTMRx_CON_PSC(n)                                   ((uint32)(((uint32)(n)&0x3FF) << 7))



/* **** SUPTMR_DT **** */

/* Set SUPTMR4 SUPTMR5 dead time
 */
#define LL_SUPTMR_DT_SUPTMR45_DT(n)                             ((uint32)(((uint32)(n)&0x3FF) << 20))

/* Set SUPTMR2 SUPTMR3 dead time
 */
#define LL_SUPTMR_DT_SUPTMR23_DT(n)                             ((uint32)(((uint32)(n)&0x3FF) << 10))

/* Set SUPTMR4 SUPTMR5 dead time
 */
#define LL_SUPTMR_DT_SUPTMR01_DT(n)                             ((uint32)(((uint32)(n)&0x3FF) << 0))



/* **** SUPTMR_DTCON **** */

/* Set SUPTMR5 output in dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR5_DT_DAT(n)                       ((uint32)(((uint32)(n)&0x1) << 23))

/* Set SUPTMR4 output in dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR4_DT_DAT(n)                       ((uint32)(((uint32)(n)&0x1) << 22))

/* Set SUPTMR3 output in dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR3_DT_DAT(n)                       ((uint32)(((uint32)(n)&0x1) << 21))

/* Set SUPTMR2 output in dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR2_DT_DAT(n)                       ((uint32)(((uint32)(n)&0x1) << 20))

/* Set SUPTMR1 output in dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR1_DT_DAT(n)                       ((uint32)(((uint32)(n)&0x1) << 19))

/* Set SUPTMR0 output in dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR0_DT_DAT(n)                       ((uint32)(((uint32)(n)&0x1) << 18))


/* SUPTMR5 dead time edge select
 */
#define LL_SUPTMR_DTCON_SUPTMR5_EDGE_SEL(n)                     ((uint32)(((uint32)(n)&0x1) << 17))

/* SUPTMR4 dead time edge select
 */
#define LL_SUPTMR_DTCON_SUPTMR4_EDGE_SEL(n)                     ((uint32)(((uint32)(n)&0x1) << 16))

/* SUPTMR3 dead time edge select
 */
#define LL_SUPTMR_DTCON_SUPTMR3_EDGE_SEL(n)                     ((uint32)(((uint32)(n)&0x1) << 15))

/* SUPTMR2 dead time edge select
 */
#define LL_SUPTMR_DTCON_SUPTMR2_EDGE_SEL(n)                     ((uint32)(((uint32)(n)&0x1) << 14))

/* SUPTMR1 dead time edge select
 */
#define LL_SUPTMR_DTCON_SUPTMR1_EDGE_SEL(n)                     ((uint32)(((uint32)(n)&0x1) << 13))

/* SUPTMR0 dead time edge select
 */
#define LL_SUPTMR_DTCON_SUPTMR0_EDGE_SEL(n)                     ((uint32)(((uint32)(n)&0x1) << 12))

/* SUPTMR4 SUPTMR5 dead time type select
 */
#define LL_SUPTMR_DTCON_SUPTMR23_DT_TYPE_SEL(n)                 ((uint32)(((uint32)(n)&0x7) << 9))

/* SUPTMR2 SUPTMR3 dead time type select
 */
#define LL_SUPTMR_DTCON_SUPTMR01_DT_TYPE_SEL(n)                 ((uint32)(((uint32)(n)&0x7) << 6))

/* SUPTMR0 SUPTMR1 dead time type select
 */
#define LL_SUPTMR_DTCON_SUPTMR45_DT_TYPE_SEL(n)                 ((uint32)(((uint32)(n)&0x7) << 3))

/* Enable SUPTMR4 SUPTMR5 dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR45_DT_EN                          ((uint32)(1UL << 2))

/* Enable SUPTMR2 SUPTMR3 dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR23_DT_EN                          ((uint32)(1UL << 1))

/* Enable SUPTMR0 SUPTMR1 dead time
 */
#define LL_SUPTMR_DTCON_SUPTMR01_DT_EN                          ((uint32)(1UL << 0))



/* **** SUPTMR_PWMCON **** */

/* Enable SUPTMR5 CMPB pwm output
 */
#define LL_SUPTMR_PWMCON_SUPTMR5_PWMB_EN                        ((uint32)(1UL << 23))

/* Enable SUPTMR4 CMPB pwm output
 */
#define LL_SUPTMR_PWMCON_SUPTMR4_PWMB_EN                        ((uint32)(1UL << 22))

/* Enable SUPTMR3 CMPB pwm output
 */
#define LL_SUPTMR_PWMCON_SUPTMR3_PWMB_EN                        ((uint32)(1UL << 21))

/* Enable SUPTMR2 CMPB pwm output
 */
#define LL_SUPTMR_PWMCON_SUPTMR2_PWMB_EN                        ((uint32)(1UL << 20))

/* Enable SUPTMR1 CMPB pwm output
 */
#define LL_SUPTMR_PWMCON_SUPTMR1_PWMB_EN                        ((uint32)(1UL << 19))

/* Enable SUPTMR0 CMPB pwm output
 */
#define LL_SUPTMR_PWMCON_SUPTMR0_PWMB_EN                        ((uint32)(1UL << 18))


/* SUPTMR5 PWMB output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR5_PWMB_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 17))

/* SUPTMR4 PWMB output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR4_PWMB_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 16))

/* SUPTMR3 PWMB output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR3_PWMB_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 15))

/* SUPTMR2 PWMB output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR2_PWMB_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 14))

/* SUPTMR1 PWMB output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR1_PWMB_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 13))

/* SUPTMR0 PWMB output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR0_PWMB_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 12))

/* SUPTMR5 PWMA output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR5_PWMA_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 11))

/* SUPTMR4 PWMA output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR4_PWMA_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 10))

/* SUPTMR3 PWMA output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR3_PWMA_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 9))

/* SUPTMR2 PWMA output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR2_PWMA_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 8))

/* SUPTMR1 PWMA output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR1_PWMA_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 7))

/* SUPTMR0 PWMA output action select
 */
#define LL_SUPTMR_PWMCON_SUPTMR0_PWMA_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 6))

/* Enable SUPTMR5 PWM output
 */
#define LL_SUPTMR_PWMCON_SUPTMR5_PWM_EN                         ((uint32)(1UL << 5))

/* Enable SUPTMR4 PWM output
 */
#define LL_SUPTMR_PWMCON_SUPTMR4_PWM_EN                         ((uint32)(1UL << 4))

/* Enable SUPTMR3 PWM output
 */
#define LL_SUPTMR_PWMCON_SUPTMR3_PWM_EN                         ((uint32)(1UL << 3))

/* Enable SUPTMR2 PWM output
 */
#define LL_SUPTMR_PWMCON_SUPTMR2_PWM_EN                         ((uint32)(1UL << 2))

/* Enable SUPTMR1 PWM output
 */
#define LL_SUPTMR_PWMCON_SUPTMR1_PWM_EN                         ((uint32)(1UL << 1))

/* Enable SUPTMR0 PWM output
 */
#define LL_SUPTMR_PWMCON_SUPTMR0_PWM_EN                         ((uint32)(1UL << 0))



/* **** SUPTMR_PWMMSK **** */

/* SUPTMR5 PWM mask output data select
 */
#define LL_SUPTMR_PWMMSK_SUPTMR5_PWM_MSK_DAT_SEL(n)             ((uint32)(((uint32)(n)&0x1) << 13))

/* SUPTMR4 PWM mask output data select
 */
#define LL_SUPTMR_PWMMSK_SUPTMR4_PWM_MSK_DAT_SEL(n)             ((uint32)(((uint32)(n)&0x1) << 12))

/* SUPTMR3 PWM mask output data select
 */
#define LL_SUPTMR_PWMMSK_SUPTMR3_PWM_MSK_DAT_SEL(n)             ((uint32)(((uint32)(n)&0x1) << 11))

/* SUPTMR2 PWM mask output data select
 */
#define LL_SUPTMR_PWMMSK_SUPTMR2_PWM_MSK_DAT_SEL(n)             ((uint32)(((uint32)(n)&0x1) << 10))

/* SUPTMR1 PWM mask output data select
 */
#define LL_SUPTMR_PWMMSK_SUPTMR1_PWM_MSK_DAT_SEL(n)             ((uint32)(((uint32)(n)&0x1) << 9))

/* SUPTMR0 PWM mask output data select
 */
#define LL_SUPTMR_PWMMSK_SUPTMR0_PWM_MSK_DAT_SEL(n)             ((uint32)(((uint32)(n)&0x1) << 8))

/* Enable SUPTMR5 mask output
 */
#define LL_SUPTMR_PWMMSK_SUPTMR5_PWM_MSK_EN                     ((uint32)(1UL << 5))

/* Enable SUPTMR4 mask output
 */
#define LL_SUPTMR_PWMMSK_SUPTMR4_PWM_MSK_EN                     ((uint32)(1UL << 4))

/* Enable SUPTMR3 mask output
 */
#define LL_SUPTMR_PWMMSK_SUPTMR3_PWM_MSK_EN                     ((uint32)(1UL << 3))

/* Enable SUPTMR2 mask output
 */
#define LL_SUPTMR_PWMMSK_SUPTMR2_PWM_MSK_EN                     ((uint32)(1UL << 2))

/* Enable SUPTMR1 mask output
 */
#define LL_SUPTMR_PWMMSK_SUPTMR1_PWM_MSK_EN                     ((uint32)(1UL << 1))

/* Enable SUPTMR0 mask output
 */
#define LL_SUPTMR_PWMMSK_SUPTMR0_PWM_MSK_EN                     ((uint32)(1UL << 0))



/* **** SUPTMR_BRKCON **** */

/* Set SUPTMR input filter length
 */
#define LL_SUPTMR_BRKCON_FILNUM(n)                              ((uint32)(((uint32)(n)&0x1f) << 23))

/* Set SUPTMR5 output in brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR5_BRK_DAT(n)                     ((uint32)(((uint32)(n)&0x1) << 22))

/* Set SUPTMR4 output in brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR4_BRK_DAT(n)                     ((uint32)(((uint32)(n)&0x1) << 21))

/* Set SUPTMR3 output in brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR3_BRK_DAT(n)                     ((uint32)(((uint32)(n)&0x1) << 20))

/* Set SUPTMR2 output in brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR2_BRK_DAT(n)                     ((uint32)(((uint32)(n)&0x1) << 19))

/* Set SUPTMR1 output in brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR1_BRK_DAT(n)                     ((uint32)(((uint32)(n)&0x1) << 18))

/* Set SUPTMR0 output in brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR0_BRK_DAT(n)                     ((uint32)(((uint32)(n)&0x1) << 17))

/* SUPTMR brake fb pin polarity select
 */
#define LL_SUPTMR_BRKCON_BRK_FB_POL_SEL(n)                      ((uint32)(((uint32)(n)&0x1) << 16))

/* SUPTMR brake comp polarity select
 */
#define LL_SUPTMR_BRKCON_BRK_COMP_POL_SEL(n)                    ((uint32)(((uint32)(n)&0x1) << 15))

/* Enable clear counter function when brake
 */
#define LL_SUPTMR_BRKCON_BRK_CLR_CNT_EN                         ((uint32)(1UL << 13))

/* SUPTMR soft brake
 */
#define LL_SUPTMR_BRKCON_BRK_SOFT                               ((uint32)(1UL << 12))

/* Enable SUPTMR brake source filter function
 */
#define LL_SUPTMR_BRKCON_BRK_FILT_EN                            ((uint32)(1UL << 11))

/* SUPTMR COMP brake select
 */
#define LL_SUPTMR_BRKCON_COMP_SEL(n)                            ((uint32)(((uint32)(n)&0x1) << 10))

/* Disable SUPTMR counter when brake
 */
#define LL_SUPTMR_BRKCON_CNT_DIS                                ((uint32)(1UL << 9))

/* Enable SUPTMR FB pin brake
 */
#define LL_SUPTMR_BRKCON_FB_EN                                  ((uint32)(1UL << 7))

/* Enable SUPTMR COMP brake
 */
#define LL_SUPTMR_BRKCON_COMP_EN                                ((uint32)(1UL << 6))

/* Enable SUPTMR5 brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR5_BRK_EN                         ((uint32)(1UL << 5))

/* Enable SUPTMR4 brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR4_BRK_EN                         ((uint32)(1UL << 4))

/* Enable SUPTMR3 brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR3_BRK_EN                         ((uint32)(1UL << 3))

/* Enable SUPTMR2 brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR2_BRK_EN                         ((uint32)(1UL << 2))

/* Enable SUPTMR1 brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR1_BRK_EN                         ((uint32)(1UL << 1))

/* Enable SUPTMR0 brake
 */
#define LL_SUPTMR_BRKCON_SUPTMR0_BRK_EN                         ((uint32)(1UL << 0))



/* **** SUPTMRx_PR **** */

/* Set SUPTMRx period value
*/
#define LL_SUPTMRx_PR(n)                                        ((uint32)(((uint32)(n)&0xFFFF) << 0))



/* **** SUPTMRx_CMP **** */

/* Set SUPTMRx CMPB value
 */
#define LL_SUPTMRx_CMP_CMPB(n)                                  ((uint32)(((uint32)(n)&0xFFFF) << 16))

/* Set SUPTMRx CMPA value
 */
#define LL_SUPTMRx_CMP_CMPA(n)                                  ((uint32)(((uint32)(n)&0xFFFF) << 0))



/* **** SUPTMRx_CNT **** */

/* Set SUPTMRx counter value
 */
#define LL_SUPTMRx_CNT(n)                                       ((uint32)(((uint32)(n)&0xFFFF) << 0))



struct hgtimer_v6_hw_comm {
    __IO uint32_t SUPTMR_CON0;
    __IO uint32_t SUPTMR_CON1;
    __IO uint32_t SUPTMR_DT;
    __IO uint32_t SUPTMR_DTCON;
    __IO uint32_t SUPTMR_PWMCON;
    __IO uint32_t SUPTMR_PWMMSK;
    __IO uint32_t SUPTMR_BRKCON;
};

struct hgtimer_v6_hw_suptmrx {
    __IO uint32_t CON;
    __IO uint32_t PR;
    __IO uint32_t CMP;
    __IO uint32_t CNT;
};






#ifdef __cplusplus
}
#endif


#endif
