/**
  ******************************************************************************
  * @file       sdk\include\chip\txw80x\io_mask.h
  * @author     HUGE-IC Application Team
  * @version    V1.0.0
  * @date       2024-01-04
  * @brief      This file contains all the GPIO functions.
  * @copyright  Copyright (c) 2016-2022 HUGE-IC
  ******************************************************************************
  * @attention
  * Only used for txw81x.
  * 
  *
  *
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __IO_MASK_H
#define __IO_MASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "typesdef.h"
#include "io_function.h"


#define IO_INDEX0(index)                  ((index&0x3)<<26)
#define IO_INDEX0_GET(io_func)            ((io_func&IO_INDEX0(3))>>26)
#define IO_INDEX1(index)                  ((index&0x3)<<28)
#define IO_INDEX1_GET(io_func)            ((io_func&IO_INDEX1(3))>>28)
#define IO_INDEX2(index)                  ((index&0x3)<<30)
#define IO_INDEX2_GET(io_func)            ((io_func&IO_INDEX2(3))>>30)

#define IO_FUNC(func)                     ((func&0xFF)<<0)

#define IO_INDEX0_BIT_SITE                (8)
#define IO_INDEX0_BIT_WIDTH               (5)
#define IO_INDEX0_VAL_SITE                (13)
#define IO_INDEX0_VAL_WIDTH               (1)
#define IO_INDEX0_SET_VAL(val)            ((val&0x01)<<IO_INDEX0_VAL_SITE)
#define IO_INDEX0_SET_BIT(bit)            ((bit&0x1F)<<IO_INDEX0_BIT_SITE)
#define IO_INDEX0_SET(mask, val, bit)     (IO_INDEX0(mask) | IO_INDEX0_SET_VAL(val) | IO_INDEX0_SET_BIT(bit))
#define IO_INDEX0_GET_VAL(io_func)        ((io_func & IO_INDEX0_SET_VAL(0x01)) >> IO_INDEX0_VAL_SITE)
#define IO_INDEX0_GET_BIT(io_func)        ((io_func & IO_INDEX0_SET_BIT(0x1F)) >> IO_INDEX0_BIT_SITE)


#define IO_INDEX1_BIT_SITE                (14)
#define IO_INDEX1_BIT_WIDTH               (5)
#define IO_INDEX1_VAL_SITE                (19)
#define IO_INDEX1_VAL_WIDTH               (1)
#define IO_INDEX1_SET_VAL(val)            ((val&0x01)<<IO_INDEX1_VAL_SITE)
#define IO_INDEX1_SET_BIT(bit)            ((bit&0x1F)<<IO_INDEX1_BIT_SITE)
#define IO_INDEX1_SET(mask, val, bit)     (IO_INDEX1(mask) | IO_INDEX1_SET_VAL(val) | IO_INDEX1_SET_BIT(bit))
#define IO_INDEX1_GET_VAL(io_func)        ((io_func & IO_INDEX1_SET_VAL(0x01)) >> IO_INDEX1_VAL_SITE)
#define IO_INDEX1_GET_BIT(io_func)        ((io_func & IO_INDEX1_SET_BIT(0x1F)) >> IO_INDEX1_BIT_SITE)


#define IO_INDEX2_BIT_SITE                (20)
#define IO_INDEX2_BIT_WIDTH               (5)
#define IO_INDEX2_VAL_SITE                (25)
#define IO_INDEX2_VAL_WIDTH               (1)
#define IO_INDEX2_SET_VAL(val)            ((val&0x01)<<IO_INDEX2_VAL_SITE)
#define IO_INDEX2_SET_BIT(bit)            ((bit&0x1F)<<IO_INDEX2_BIT_SITE)
#define IO_INDEX2_SET(mask, val, bit)     (IO_INDEX2(mask) | IO_INDEX2_SET_VAL(val) | IO_INDEX2_SET_BIT(bit))
#define IO_INDEX2_GET_VAL(io_func)        ((io_func & IO_INDEX2_SET_VAL(0x01)) >> IO_INDEX2_VAL_SITE)
#define IO_INDEX2_GET_BIT(io_func)        ((io_func & IO_INDEX2_SET_BIT(0x1F)) >> IO_INDEX2_BIT_SITE)


#define IO_MASK(set, func)                ((set) | (func))

/**
 * @A : iofuncmask0[14]
 * @B : iofuncmask0[8]
 * 
 * @(A=0) & (B=0) : comp_dout_dig0
 * @(A=1) & (B=0) : ospi_cs_io
 * @(A=0) & (B=1) : dbgpath_dbgo[8]
 */
enum IOMASK_GPIO_IOMAP_OUT_COMP_DOUT_DIG0 {
    GPIO_IOMAP_OUT_COMP_DOUT_DIG0         = IO_MASK(IO_INDEX0_SET(0, 0, 14) |\
                                                    IO_INDEX1_SET(0, 0,  8) |\
                                                    IO_INDEX2_SET(0, 0,  8) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_COMP_DOUT_DIG0_VAL)),
    GPIO_IOMAP_OUT_OSPI_CS_IO             = IO_MASK(IO_INDEX0_SET(0, 1, 14) |\
                                                    IO_INDEX1_SET(0, 0,  8) |\
                                                    IO_INDEX2_SET(0, 0,  8) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_COMP_DOUT_DIG0_VAL)),
};

/**
 * @A : iofuncmask0[20]
 * @B : iofuncmask0[9]
 * 
 * @(A=0) & (B=0) : comp_dout_dig1
 * @(A=1) & (B=0) : lcd_data_o[0]
 * @(A=0) & (B=1) : dbgpath_dbgo[9]
 */  
enum IOMASK_GPIO_IOMAP_OUT_COMP_DOUT_DIG1 {
    GPIO_IOMAP_OUT_COMP_DOUT_DIG1         = IO_MASK(IO_INDEX0_SET(0, 0, 20) |\
                                                    IO_INDEX1_SET(0, 0,  9) |\
                                                    IO_INDEX2_SET(0, 0,  9) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_COMP_DOUT_DIG1_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_0           = IO_MASK(IO_INDEX0_SET(0, 1, 20) |\
                                                    IO_INDEX1_SET(0, 0,  9) |\
                                                    IO_INDEX2_SET(0, 0,  9) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_COMP_DOUT_DIG1_VAL)),
};

/**
 * @A : iofuncmask0[16]
 * @B : iofuncmask0[15]
 * 
 * @(A=0) & (B=0) : antenna_sel
 * @(A=1) & (B=0) : lcd_de_or_erd
 * @(A=0) & (B=1) : grant_ble
 */   
enum IOMASK_GPIO_IOMAP_OUT_ANTENNA_SEL {
    GPIO_IOMAP_OUT_ANTENNA_SEL            = IO_MASK(IO_INDEX0_SET(0, 0, 16) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_ANTENNA_SEL_VAL)),
    GPIO_IOMAP_OUT_LCD_DE_OR_ERD          = IO_MASK(IO_INDEX0_SET(0, 1, 16) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_ANTENNA_SEL_VAL)),
    GPIO_IOMAP_OUT_GRANT_BLE              = IO_MASK(IO_INDEX0_SET(0, 0, 16) |\
                                                    IO_INDEX1_SET(0, 1, 15) |\
                                                    IO_INDEX2_SET(0, 1, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_ANTENNA_SEL_VAL)),
};

/**
 * @A : iofuncmask0[17]
 * @B : iofuncmask0[15]
 * 
 * @(A=0) & (B=0) : pa_en
 * @(A=1) & (B=0) : lcd_hsync_or_dc
 * @(A=0) & (B=1) : grant_ble_switch_o
 */  
enum IOMASK_GPIO_IOMAP_OUT_PA_EN {
    GPIO_IOMAP_OUT_PA_EN                  = IO_MASK(IO_INDEX0_SET(0, 0, 17) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_PA_EN_VAL)),
    GPIO_IOMAP_OUT_LCD_HSYNC_OR_DC        = IO_MASK(IO_INDEX0_SET(0, 1, 17) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_PA_EN_VAL)),
    GPIO_IOMAP_OUT_GRANT_BLE_SWITCH_O     = IO_MASK(IO_INDEX0_SET(0, 0, 17) |\
                                                    IO_INDEX1_SET(0, 1, 15) |\
                                                    IO_INDEX2_SET(0, 1, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_PA_EN_VAL)),
};

/**
 * @A : iofuncmask0[18]
 * @B : iofuncmask0[15]
 * 
 * @(A=0) & (B=0) : rf_ext_lna_en
 * @(A=1) & (B=0) : lcd_vsync_or_cs
 * @(A=0) & (B=1) : grant_wifi_switch_o
 */ 
enum IOMASK_GPIO_IOMAP_OUT_RF_EXT_LNA_EN {
    GPIO_IOMAP_OUT_RF_EXT_LNA_EN          = IO_MASK(IO_INDEX0_SET(0, 0, 18) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_RF_EXT_LNA_EN_VAL)),
    GPIO_IOMAP_OUT_LCD_VSYNC_OR_CS        = IO_MASK(IO_INDEX0_SET(0, 1, 18) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_RF_EXT_LNA_EN_VAL)),
    GPIO_IOMAP_OUT_GRANT_WIFI_SWITCH_O    = IO_MASK(IO_INDEX0_SET(0, 0, 18) |\
                                                    IO_INDEX1_SET(0, 1, 15) |\
                                                    IO_INDEX2_SET(0, 1, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_RF_EXT_LNA_EN_VAL)),
};

/**
 * @A : iofuncmask0[19]
 * @B : iofuncmask0[15]
 * 
 * @(A=0) & (B=0) : rf_tx_en_fem
 * @(A=1) & (B=0) : lcd_dotclk_or_rwr
 * @(A=0) & (B=1) : rf_switch_en1
 */ 
enum IOMASK_GPIO_IOMAP_OUT_RF_TX_EN_FEM {
    GPIO_IOMAP_OUT_RF_TX_EN_FEM           = IO_MASK(IO_INDEX0_SET(0, 0, 19) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_RF_TX_EN_FEM_VAL)),
    GPIO_IOMAP_OUT_LCD_DOTCLK_OR_RWR      = IO_MASK(IO_INDEX0_SET(0, 1, 19) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_RF_TX_EN_FEM_VAL)),
    GPIO_IOMAP_OUT_RF_SWITCH_EN1          = IO_MASK(IO_INDEX0_SET(0, 0, 19) |\
                                                    IO_INDEX1_SET(0, 1, 15) |\
                                                    IO_INDEX2_SET(0, 1, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_RF_TX_EN_FEM_VAL)),
};

/**
 * @A : iofuncmask0[15]
 * 
 * @(A=0): rf_rx_en_fem
 * @(A=1): rf_switch_en0
 */ 
enum IOMASK_GPIO_IOMAP_OUT_RF_RX_EN_FEM {
    GPIO_IOMAP_OUT_RF_RX_EN_FEM           = IO_MASK(IO_INDEX0_SET(0, 0, 15) |\
                                                    IO_INDEX1_SET(0, 0, 15) |\
                                                    IO_INDEX2_SET(0, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_RF_RX_EN_FEM_VAL)),
    GPIO_IOMAP_OUT_RF_SWITCH_EN0          = IO_MASK(IO_INDEX0_SET(0, 1, 15) |\
                                                    IO_INDEX1_SET(0, 1, 15) |\
                                                    IO_INDEX2_SET(0, 1, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_RF_RX_EN_FEM_VAL)),
};

/**
 * @A : iofuncmask0[21]
 * @B : iofuncmask0[0]
 * @C : rtc_calib_oe
 * 
 * @(A=0) & (B=0) & (C=0): uart1_rts_re_o
 * @(A=1) & (B=0) & (C=0): lcd_data_o[3]
 * @(A=0) & (B=1) & (C=0): dbgpath_dbgo[0]
 * @(A=0) & (B=0) & (C=1): rtc_calib
 */ 
enum IOMASK_GPIO_IOMAP_OUT_UART1_RTS_RE_O {
    GPIO_IOMAP_OUT_UART1_RTS_RE_O         = IO_MASK(IO_INDEX0_SET(0, 0, 21) |\
                                                    IO_INDEX1_SET(0, 0,  0) |\
                                                    IO_INDEX2_SET(0, 0,  0) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_UART1_RTS_RE_O_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_3           = IO_MASK(IO_INDEX0_SET(0, 1, 21) |\
                                                    IO_INDEX1_SET(0, 0,  0) |\
                                                    IO_INDEX2_SET(0, 0,  0) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_UART1_RTS_RE_O_VAL)),
};

/**
 * @A : iofuncmask0[22]
 * @B : iofuncmask0[1]
 * @C : rtc_calib_oe
 * 
 * @(A=0) & (B=0) & (C=0): uart1_cts_de_out
 * @(A=1) & (B=0) & (C=0): lcd_data_o[4]
 * @(A=0) & (B=1) & (C=0): dbgpath_dbgo[1]
 * @(A=0) & (B=0) & (C=1): rtc_calib_pul
 */ 
enum IOMASK_GPIO_IOMAP_OUT_UART1_CTS_DE_OUT {
    GPIO_IOMAP_OUT_UART1_CTS_DE_OUT       = IO_MASK(IO_INDEX0_SET(0, 0, 22) |\
                                                    IO_INDEX1_SET(0, 0,  1) |\
                                                    IO_INDEX2_SET(0, 0,  1) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_UART1_CTS_DE_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_4           = IO_MASK(IO_INDEX0_SET(0, 1, 22) |\
                                                    IO_INDEX1_SET(0, 0,  1) |\
                                                    IO_INDEX2_SET(0, 0,  1) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_UART1_CTS_DE_OUT_VAL)),
};

/**
 * @A : iofuncmask0[23]
 * 
 * @(A=0): uart1_out
 * @(A=1): lcd_data_o[5]
 */
enum IOMASK_GPIO_IOMAP_OUT_UART1_OUT {
    GPIO_IOMAP_OUT_UART1_OUT              = IO_MASK(IO_INDEX0_SET(0, 0, 23) |\
                                                    IO_INDEX1_SET(0, 0, 23) |\
                                                    IO_INDEX2_SET(0, 0, 23) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_UART1_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_5           = IO_MASK(IO_INDEX0_SET(0, 1, 23) |\
                                                    IO_INDEX1_SET(0, 1, 23) |\
                                                    IO_INDEX2_SET(0, 1, 23) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_UART1_OUT_VAL)),
};

/**
 * @A : Iofuncmask0[24]
 * 
 * @(A=0): stmr3_pwm_out
 * @(A=1): lcd_data_o[6]
 */
enum IOMASK_GPIO_IOMAP_OUT_STMR3_PWM_OUT {
    GPIO_IOMAP_OUT_STMR3_PWM_OUT          = IO_MASK(IO_INDEX0_SET(0, 0, 24) |\
                                                    IO_INDEX1_SET(0, 0, 24) |\
                                                    IO_INDEX2_SET(0, 0, 24) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR3_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_6           = IO_MASK(IO_INDEX0_SET(0, 1, 24) |\
                                                    IO_INDEX1_SET(0, 1, 24) |\
                                                    IO_INDEX2_SET(0, 1, 24) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR3_PWM_OUT_VAL)),
};

/**
 * @A : Iofuncmask0[25]
 * 
 * @(A=0): stmr2_pwm_out
 * @(A=1): lcd_data_o[7]
 */
enum IOMASK_GPIO_IOMAP_OUT_STMR2_PWM_OUT {
    GPIO_IOMAP_OUT_STMR2_PWM_OUT          = IO_MASK(IO_INDEX0_SET(0, 0, 25) |\
                                                    IO_INDEX1_SET(0, 0, 25) |\
                                                    IO_INDEX2_SET(0, 0, 25) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR2_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_7           = IO_MASK(IO_INDEX0_SET(0, 1, 25) |\
                                                    IO_INDEX1_SET(0, 1, 25) |\
                                                    IO_INDEX2_SET(0, 1, 25) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR2_PWM_OUT_VAL)),
};

/**
 * @A : Iofuncmask0[26]
 * 
 * @(A=0): stmr1_pwm_out
 * @(A=1): lcd_data_o[8]
 */
enum IOMASK_GPIO_IOMAP_OUT_STMR1_PWM_OUT {
    GPIO_IOMAP_OUT_STMR1_PWM_OUT          = IO_MASK(IO_INDEX0_SET(0, 0, 26) |\
                                                    IO_INDEX1_SET(0, 0, 26) |\
                                                    IO_INDEX2_SET(0, 0, 26) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR1_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_8           = IO_MASK(IO_INDEX0_SET(0, 1, 26) |\
                                                    IO_INDEX1_SET(0, 1, 26) |\
                                                    IO_INDEX2_SET(0, 1, 26) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR1_PWM_OUT_VAL)),
};

/**
 * @A : Iofuncmask0[27]
 * 
 * @(A=0): stmr0_pwm_out
 * @(A=1): lcd_data_o[9]
 */
enum IOMASK_GPIO_IOMAP_OUT_STMR0_PWM_OUT {
    GPIO_IOMAP_OUT_STMR0_PWM_OUT          = IO_MASK(IO_INDEX0_SET(0, 0, 27) |\
                                                    IO_INDEX1_SET(0, 0, 27) |\
                                                    IO_INDEX2_SET(0, 0, 27) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR0_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_9           = IO_MASK(IO_INDEX0_SET(0, 1, 27) |\
                                                    IO_INDEX1_SET(0, 1, 27) |\
                                                    IO_INDEX2_SET(0, 1, 27) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR0_PWM_OUT_VAL)),
};

/**
 * @A : Iofuncmask0[28]
 * 
 * @(A=0): stmr4_pwm_out
 * @(A=1): lcd_data_o[10]
 */
enum IOMASK_GPIO_IOMAP_OUT_STMR4_PWM_OUT {
    GPIO_IOMAP_OUT_STMR4_PWM_OUT          = IO_MASK(IO_INDEX0_SET(0, 0, 28) |\
                                                    IO_INDEX1_SET(0, 0, 28) |\
                                                    IO_INDEX2_SET(0, 0, 28) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR4_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_10          = IO_MASK(IO_INDEX0_SET(0, 1, 28) |\
                                                    IO_INDEX1_SET(0, 1, 28) |\
                                                    IO_INDEX2_SET(0, 1, 28) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR4_PWM_OUT_VAL)),
};

/**
 * @A : iofuncmask1[8]
 * @B : iofuncmask1[6]
 * 
 * @(A=0) & (B=0) : stmr5_pwm_out
 * @(A=1) & (B=0) : lcd_data_o[11]
 * @(A=0) & (B=1) : qspi_ospi_mnt_o[6]
 */ 
enum IOMASK_GPIO_IOMAP_OUT_STMR5_PWM_OUT {
    GPIO_IOMAP_OUT_STMR5_PWM_OUT          = IO_MASK(IO_INDEX0_SET(1, 0,  8) |\
                                                    IO_INDEX1_SET(1, 0,  6) |\
                                                    IO_INDEX2_SET(1, 0,  6) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR5_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_11          = IO_MASK(IO_INDEX0_SET(1, 1,  8) |\
                                                    IO_INDEX1_SET(1, 0,  6) |\
                                                    IO_INDEX2_SET(1, 0,  6) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_STMR5_PWM_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask1[10]
 * @B : iofuncmask0[5]
 * 
 * @(A=0) & (B=0) : led_tmr1_pwm_out
 * @(A=1) & (B=0) : lcd_data_o[13]
 * @(A=0) & (B=1) : dbgpath_dbgo[5]
 */ 
enum IOMASK_GPIO_IOMAP_OUT_LED_TMR1_PWM_OUT {
    GPIO_IOMAP_OUT_LED_TMR1_PWM_OUT       = IO_MASK(IO_INDEX0_SET(1, 0, 10) |\
                                                    IO_INDEX1_SET(0, 0,  5) |\
                                                    IO_INDEX2_SET(0, 0,  5) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_LED_TMR1_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_13          = IO_MASK(IO_INDEX0_SET(1, 1, 10) |\
                                                    IO_INDEX1_SET(0, 0,  5) |\
                                                    IO_INDEX2_SET(0, 0,  5) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_LED_TMR1_PWM_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask1[11]
 * @B : iofuncmask0[6]
 * 
 * @(A=0) & (B=0) : led_tmr2_pwm_out
 * @(A=1) & (B=0) : lcd_data_o[14]
 * @(A=0) & (B=1) : dbgpath_dbgo[6]
 */ 
enum IOMASK_GPIO_IOMAP_OUT_LED_TMR2_PWM_OUT {
    GPIO_IOMAP_OUT_LED_TMR2_PWM_OUT       = IO_MASK(IO_INDEX0_SET(1, 0, 11) |\
                                                    IO_INDEX1_SET(0, 0,  6) |\
                                                    IO_INDEX2_SET(0, 0,  6) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_LED_TMR2_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_14          = IO_MASK(IO_INDEX0_SET(1, 1, 11) |\
                                                    IO_INDEX1_SET(0, 0,  6) |\
                                                    IO_INDEX2_SET(0, 0,  6) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_LED_TMR2_PWM_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask1[12]
 * @B : iofuncmask0[7]
 * 
 * @(A=0) & (B=0) : led_tmr3_pwm_out
 * @(A=1) & (B=0) : lcd_data_o[15]
 * @(A=0) & (B=1) : dbgpath_dbgo[7]
 */
enum IOMASK_GPIO_IOMAP_OUT_LED_TMR3_PWM_OUT {
    GPIO_IOMAP_OUT_LED_TMR3_PWM_OUT       = IO_MASK(IO_INDEX0_SET(1, 0, 12) |\
                                                    IO_INDEX1_SET(0, 0,  7) |\
                                                    IO_INDEX2_SET(0, 0,  7) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_LED_TMR3_PWM_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_15          = IO_MASK(IO_INDEX0_SET(1, 1, 12) |\
                                                    IO_INDEX1_SET(0, 0,  7) |\
                                                    IO_INDEX2_SET(0, 0,  7) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_LED_TMR3_PWM_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask0[11]
 * 
 * @(A=0): qspi_nss1_out
 * @(A=1): ospi_cs_io
 */ 
enum IOMASK_GPIO_IOMAP_OUT_QSPI_NSS1_OUT {
    GPIO_IOMAP_OUT_QSPI_NSS1_OUT          = IO_MASK(IO_INDEX0_SET(0, 0, 11) |\
                                                    IO_INDEX1_SET(0, 0, 11) |\
                                                    IO_INDEX2_SET(0, 0, 11) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_QSPI_NSS1_OUT_VAL)),
    GPIO_IOMAP_OUT_OSPI_CS_IO_            = IO_MASK(IO_INDEX0_SET(0, 1, 11) |\
                                                    IO_INDEX1_SET(0, 1, 11) |\
                                                    IO_INDEX2_SET(0, 1, 11) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_QSPI_NSS1_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask1[13]
 * 
 * @(A=0): spi2_nss_out
 * @(A=1): lcd_data_o[18]
 */ 
enum IOMASK_GPIO_IOMAP_OUT_SPI2_NSS_OUT {
    GPIO_IOMAP_OUT_SPI2_NSS_OUT           = IO_MASK(IO_INDEX0_SET(1, 0, 13) |\
                                                    IO_INDEX1_SET(1, 0, 13) |\
                                                    IO_INDEX2_SET(1, 0, 13) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_SPI2_NSS_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_18          = IO_MASK(IO_INDEX0_SET(1, 1, 13) |\
                                                    IO_INDEX1_SET(1, 1, 13) |\
                                                    IO_INDEX2_SET(1, 1, 13) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_SPI2_NSS_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask1[14]
 * 
 * @(A=0): spi2_io1_out
 * @(A=1): lcd_data_o[19]
 */ 
enum IOMASK_GPIO_IOMAP_OUT_SPI2_IO1_OUT {
    GPIO_IOMAP_OUT_SPI2_IO1_OUT           = IO_MASK(IO_INDEX0_SET(1, 0, 14) |\
                                                    IO_INDEX1_SET(1, 0, 14) |\
                                                    IO_INDEX2_SET(1, 0, 14) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_SPI2_IO1_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_19          = IO_MASK(IO_INDEX0_SET(1, 1, 14) |\
                                                    IO_INDEX1_SET(1, 1, 14) |\
                                                    IO_INDEX2_SET(1, 1, 14) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_SPI2_IO1_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask1[15]
 * 
 * @(A=0): spi2_io2_out
 * @(A=1): lcd_data_o[20]
 */ 
enum IOMASK_GPIO_IOMAP_OUT_SPI2_IO2_OUT {
    GPIO_IOMAP_OUT_SPI2_IO2_OUT           = IO_MASK(IO_INDEX0_SET(1, 0, 15) |\
                                                    IO_INDEX1_SET(1, 0, 15) |\
                                                    IO_INDEX2_SET(1, 0, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_SPI2_IO2_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_20          = IO_MASK(IO_INDEX0_SET(1, 1, 15) |\
                                                    IO_INDEX1_SET(1, 1, 15) |\
                                                    IO_INDEX2_SET(1, 1, 15) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_SPI2_IO2_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask1[16]
 * 
 * @(A=0): spi2_io3_out
 * @(A=1): lcd_data_o[21]
 */ 
enum IOMASK_GPIO_IOMAP_OUT_SPI2_IO3_OUT {
    GPIO_IOMAP_OUT_SPI2_IO3_OUT           = IO_MASK(IO_INDEX0_SET(1, 0, 16) |\
                                                    IO_INDEX1_SET(1, 0, 16) |\
                                                    IO_INDEX2_SET(1, 0, 16) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_SPI2_IO3_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_21          = IO_MASK(IO_INDEX0_SET(1, 1, 16) |\
                                                    IO_INDEX1_SET(1, 1, 16) |\
                                                    IO_INDEX2_SET(1, 1, 16) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_SPI2_IO3_OUT_VAL)),                                         
};

/**
 * @A : iofuncmask0[31]
 * @B : iofuncmask1[17]
 * 
 * @(A=0) & (B=0) : iis1_mclk_out
 * @(A=1) & (B=0) : uart5_tx
 * @(A=0) & (B=1) : lcd_data_o[22]
 */   
enum IOMASK_GPIO_IOMAP_OUT_IIS1_MCLK_OUT {
    GPIO_IOMAP_OUT_IIS1_MCLK_OUT          = IO_MASK(IO_INDEX0_SET(0, 0, 31) |\
                                                    IO_INDEX1_SET(1, 0, 17) |\
                                                    IO_INDEX2_SET(1, 0, 17) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_MCLK_OUT_VAL)),
    GPIO_IOMAP_OUT_UART5_TX               = IO_MASK(IO_INDEX0_SET(0, 1, 31) |\
                                                    IO_INDEX1_SET(1, 0, 17) |\
                                                    IO_INDEX2_SET(1, 0, 17) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_MCLK_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_22          = IO_MASK(IO_INDEX0_SET(0, 0, 31) |\
                                                    IO_INDEX1_SET(1, 1, 17) |\
                                                    IO_INDEX2_SET(1, 1, 17) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_MCLK_OUT_VAL)),                                                                                             
};

/**
 * @A : iofuncmask1[18]
 * 
 * @(A=0): iis1_wsclk_out
 * @(A=1): lcd_data_o[23]
 */ 
enum IOMASK_GPIO_IOMAP_OUT_IIS1_WSCLK_OUT {
    GPIO_IOMAP_OUT_IIS1_WSCLK_OUT         = IO_MASK(IO_INDEX0_SET(1, 0, 18) |\
                                                    IO_INDEX1_SET(1, 0, 18) |\
                                                    IO_INDEX2_SET(1, 0, 18) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_WSCLK_OUT_VAL)),
    GPIO_IOMAP_OUT_LCD_DATA_O_23          = IO_MASK(IO_INDEX0_SET(1, 1, 18) |\
                                                    IO_INDEX1_SET(1, 1, 18) |\
                                                    IO_INDEX2_SET(1, 1, 18) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_WSCLK_OUT_VAL)),                                                                                             
};

/**
 * @A : iofuncmask0[30]
 * 
 * @(A=0): iis1_bclk_out
 * @(A=1): spi1_io2_out
 */
enum IOMASK_GPIO_IOMAP_OUT_IIS1_BCLK_OUT {
    GPIO_IOMAP_OUT_IIS1_BCLK_OUT          = IO_MASK(IO_INDEX0_SET(0, 0, 30) |\
                                                    IO_INDEX1_SET(0, 0, 30) |\
                                                    IO_INDEX2_SET(0, 0, 30) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_BCLK_OUT_VAL)),
    GPIO_IOMAP_OUT_SPI1_IO2_OUT           = IO_MASK(IO_INDEX0_SET(0, 1, 30) |\
                                                    IO_INDEX1_SET(0, 1, 30) |\
                                                    IO_INDEX2_SET(0, 1, 30) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_BCLK_OUT_VAL)),                                                                                             
};

/**
 * @A : iofuncmask0[29]
 * 
 * @(A=0): iis1_do
 * @(A=1): spi1_io3_out
 */
enum IOMASK_GPIO_IOMAP_OUT_IIS1_DO {
    GPIO_IOMAP_OUT_IIS1_DO                = IO_MASK(IO_INDEX0_SET(0, 0, 29) |\
                                                    IO_INDEX1_SET(0, 0, 29) |\
                                                    IO_INDEX2_SET(0, 0, 29) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_DO_VAL)),
    GPIO_IOMAP_OUT_SPI1_IO3_OUT           = IO_MASK(IO_INDEX0_SET(0, 1, 29) |\
                                                    IO_INDEX1_SET(0, 1, 29) |\
                                                    IO_INDEX2_SET(0, 1, 29) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_IIS1_DO_VAL)),                                                                                             
};

/**
 * @A : iofuncmask0[13]
 * 
 * @(A=0): clk_to_io
 * @(A=1): uart4_tx
 */
enum IOMASK_GPIO_IOMAP_OUT_CLK_TO_IO {
    GPIO_IOMAP_OUT_CLK_TO_IO              = IO_MASK(IO_INDEX0_SET(0, 0, 13) |\
                                                    IO_INDEX1_SET(0, 0, 13) |\
                                                    IO_INDEX2_SET(0, 0, 13) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_CLK_TO_IO_VAL)),
    GPIO_IOMAP_OUT_UART4_TX               = IO_MASK(IO_INDEX0_SET(0, 1, 13) |\
                                                    IO_INDEX1_SET(0, 1, 13) |\
                                                    IO_INDEX2_SET(0, 1, 13) ,
                                                    IO_FUNC(GPIO_IOMAP_OUT_CLK_TO_IO_VAL)),                                                                                             
};

#ifdef __cplusplus
}
#endif
#endif

/*************************** (C) COPYRIGHT 2016-2022 HUGE-IC ***** END OF FILE *****/