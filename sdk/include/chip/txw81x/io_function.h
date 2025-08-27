/**
  ******************************************************************************
  * @file       sdk\include\chip\txw81x\io_function.h
  * @author     HUGE-IC Application Team
  * @version    V1.0.0
  * @date       2022-01-11
  * @brief      This file contains all the GPIO functions.
  * @copyright  Copyright (c) 2016-2024 HUGE-IC
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
#ifndef __IO_FUNCTION_H
#define __IO_FUNCTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "typesdef.h"

/** @addtogroup Docxygenid_GPIO_enum
  * @{
  */

/**
  * @brief Enumeration constant for GPIO command.
  * @note
  *       Enum number start from 0x101.
  */
enum gpio_cmd {
    /*! GPIO cmd afio set
     */
    GPIO_CMD_AFIO_SET = 0x101,

    /*! GPIO cmd iomap output
     */
    GPIO_CMD_IOMAP_OUT_FUNC,

    /*! GPIO cmd iomap input
     */
    GPIO_CMD_IOMAP_IN_FUNC,

    /*! GPIO cmd iomap inout
     */
    GPIO_CMD_IOMAP_INOUT_FUNC,

    /*! GPIO pin driver strength config
     */
    GPIO_CMD_DRIVER_STRENGTH,

};


/**
  * @brief Enumeration constant for GPIO afio set.
  */
enum gpio_afio_set{
    /*! gpio cmd afio 0
     */
    GPIO_AF_0  = 0,
    
    /*! gpio cmd afio 1
     */
    GPIO_AF_1 ,

    /*! gpio cmd afio 2
     */
    GPIO_AF_2 ,

    /*! gpio cmd afio 3
     */
    GPIO_AF_3 ,
};

/**
  * @breif : Enumeration constant for GPIO pull level.
  */
enum gpio_pull_level {
    /*! gpio pull level : NONE
     */
    GPIO_PULL_LEVEL_NONE = 0,
    
    /*! gpio pull level : 4.7K
     */
    GPIO_PULL_LEVEL_4_7K ,
    
    /*! gpio pull level : 100k
     */
    GPIO_PULL_LEVEL_100K ,
};

/**
  * @breif : Enumeration constant for GPIO driver strength
  */
enum pin_driver_strength {
    /*! Compatible version: V4
     */
    GPIO_DS_4MA ,
    
    /*! Compatible version: V4
     */
    GPIO_DS_12MA,
    
    /*! Compatible version: V4
     */
    GPIO_DS_20MA,
    
    /*! Compatible version: V4
     */
    GPIO_DS_28MA,
} ;

/** @defgroup Enumeration constant for GPIO IO map output function selection
  * @{
  */
enum gpio_iomap_out_func{
    GPIO_IOMAP_OUTPUT,

    /**
     * @A : iofuncmask0[14]
     * @B : iofuncmask0[8]
     * 
     * @(A=0) & (B=0) : comp_dout_dig0
     * @(A=1) & (B=0) : ospi_cs_io
     * @(A=0) & (B=1) : dbgpath_dbgo[8]
     */
    GPIO_IOMAP_OUT_COMP_DOUT_DIG0_VAL,

    /**
     * @A : iofuncmask0[20]
     * @B : iofuncmask0[9]
     * 
     * @(A=0) & (B=0) : comp_dout_dig1
     * @(A=1) & (B=0) : lcd_data_o[0]
     * @(A=0) & (B=1) : dbgpath_dbgo[9]
     */                               
    GPIO_IOMAP_OUT_COMP_DOUT_DIG1_VAL,

    /**
     * @A : iofuncmask0[16]
     * @B : iofuncmask0[15]
     * 
     * @(A=0) & (B=0) : antenna_sel
     * @(A=1) & (B=0) : lcd_de_or_erd
     * @(A=0) & (B=1) : grant_ble
     */      
    GPIO_IOMAP_OUT_ANTENNA_SEL_VAL,

    /**
     * @A : iofuncmask0[17]
     * @B : iofuncmask0[15]
     * 
     * @(A=0) & (B=0) : pa_en
     * @(A=1) & (B=0) : lcd_hsync_or_dc
     * @(A=0) & (B=1) : grant_ble_switch_o
     */  
    GPIO_IOMAP_OUT_PA_EN_VAL,

    /**
     * @A : iofuncmask0[18]
     * @B : iofuncmask0[15]
     * 
     * @(A=0) & (B=0) : rf_ext_lna_en
     * @(A=1) & (B=0) : lcd_vsync_or_cs
     * @(A=0) & (B=1) : grant_wifi_switch_o
     */ 
    GPIO_IOMAP_OUT_RF_EXT_LNA_EN_VAL,

    /**
     * @A : iofuncmask0[19]
     * @B : iofuncmask0[15]
     * 
     * @(A=0) & (B=0) : rf_tx_en_fem
     * @(A=1) & (B=0) : lcd_dotclk_or_rwr
     * @(A=0) & (B=1) : rf_switch_en1
     */ 
    GPIO_IOMAP_OUT_RF_TX_EN_FEM_VAL,

    /**
     * @A : iofuncmask0[15]
     * 
     * @(A=0): rf_rx_en_fem
     * @(A=1): rf_switch_en0
     */ 
    GPIO_IOMAP_OUT_RF_RX_EN_FEM_VAL,

    /**
     * @NONE
     * 
     */ 
    GPIO_IOMAP_OUT_LCD_DATA_O_1,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_LCD_DATA_O_2,

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
    GPIO_IOMAP_OUT_UART1_RTS_RE_O_VAL,

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
    GPIO_IOMAP_OUT_UART1_CTS_DE_OUT_VAL,

    /**
     * @A : iofuncmask0[23]
     * 
     * @(A=0): uart1_out
     * @(A=1): lcd_data_o[5]
     */
    GPIO_IOMAP_OUT_UART1_OUT_VAL,

    /**
     * @A : iofuncmask0[2]
     * 
     * @(A=0): uart0_rts_re_o
     * @(A=1): dbgpath_dbgo[2]
     */
    GPIO_IOMAP_OUT_UART0_RTS_RE_O,

    /**
     * @A : iofuncmask0[3]
     * 
     * @(A=0): uart0_cts_de_out
     * @(A=1): dbgpath_dbgo[3]
     */
    GPIO_IOMAP_OUT_UART0_CTS_DE_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_UART0_TX,

    /**
     * @A : Iofuncmask0[24]
     * 
     * @(A=0): stmr3_pwm_out
     * @(A=1): lcd_data_o[6]
     */
    GPIO_IOMAP_OUT_STMR3_PWM_OUT_VAL,

    /**
     * @A : Iofuncmask0[25]
     * 
     * @(A=0): stmr2_pwm_out
     * @(A=1): lcd_data_o[7]
     */
    GPIO_IOMAP_OUT_STMR2_PWM_OUT_VAL,

    /**
     * @A : Iofuncmask0[26]
     * 
     * @(A=0): stmr1_pwm_out
     * @(A=1): lcd_data_o[8]
     */
    GPIO_IOMAP_OUT_STMR1_PWM_OUT_VAL,

    /**
     * @A : Iofuncmask0[27]
     * 
     * @(A=0): stmr0_pwm_out
     * @(A=1): lcd_data_o[9]
     */
    GPIO_IOMAP_OUT_STMR0_PWM_OUT_VAL,

    /**
     * @A : Iofuncmask0[28]
     * 
     * @(A=0): stmr4_pwm_out
     * @(A=1): lcd_data_o[10]
     */
    GPIO_IOMAP_OUT_STMR4_PWM_OUT_VAL,

    /**
     * @A : iofuncmask1[8]
     * @B : iofuncmask1[6]
     * 
     * @(A=0) & (B=0) : stmr5_pwm_out
     * @(A=1) & (B=0) : lcd_data_o[11]
     * @(A=0) & (B=1) : qspi_ospi_mnt_o[6]
     */ 
    GPIO_IOMAP_OUT_STMR5_PWM_OUT_VAL,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SDHOST_SCLK_O,

    /**
     * @A : mjpeg_dma_dbg[3]
     * 
     * @(A=0): tmr3_pwm_out
     * @(A=1): mjpeg_dma_dbg[0]
     */
    GPIO_IOMAP_OUT_TMR3_PWM_OUT,

    /**
     * @A : usb20_phy_cfg3[22]
     * @B : mjpeg_dma_dbg[3]
     * 
     * @(A=0) & (B=0) : tmr2_pwm_out
     * @(A=1) & (B=0) : comp_dis_dbg
     * @(A=0) & (B=1) : mjpeg_dma_dbg[1]
     */ 
    GPIO_IOMAP_OUT_TMR2_PWM_OUT,

    /**
     * @A : usb20_phy_cfg3[22]
     * @B : mjpeg_dma_dbg[3]
     * 
     * @(A=0) & (B=0) : tmr1_pwm_out
     * @(A=1) & (B=0) : comp_clk_dbg
     * @(A=0) & (B=1) : mjpeg_dma_dbg[2]
     */ 
    GPIO_IOMAP_OUT_TMR1_PWM_OUT,

    /**
     * @A : mjpeg_dma_dbg[3]
     * 
     * @(A=0): tmr0_pwm_out
     * @(A=1): lcd_data_o[12]
     */
    GPIO_IOMAP_OUT_TMR0_PWM_OUT,

    /**
     * @A : iofuncmask0[4]
     * 
     * @(A=0): led_tmr0_pwm_out
     * @(A=1): dbgpath_dbgo[4]
     */
    GPIO_IOMAP_OUT_LED_TMR0_PWM_OUT,

    /**
     * @A : iofuncmask1[10]
     * @B : iofuncmask0[5]
     * 
     * @(A=0) & (B=0) : led_tmr1_pwm_out
     * @(A=1) & (B=0) : lcd_data_o[13]
     * @(A=0) & (B=1) : dbgpath_dbgo[5]
     */ 
    GPIO_IOMAP_OUT_LED_TMR1_PWM_OUT_VAL,

    /**
     * @A : iofuncmask1[11]
     * @B : iofuncmask0[6]
     * 
     * @(A=0) & (B=0) : led_tmr2_pwm_out
     * @(A=1) & (B=0) : lcd_data_o[14]
     * @(A=0) & (B=1) : dbgpath_dbgo[6]
     */ 
    GPIO_IOMAP_OUT_LED_TMR2_PWM_OUT_VAL,

    /**
     * @A : iofuncmask1[12]
     * @B : iofuncmask0[7]
     * 
     * @(A=0) & (B=0) : led_tmr3_pwm_out
     * @(A=1) & (B=0) : lcd_data_o[15]
     * @(A=0) & (B=1) : dbgpath_dbgo[7]
     */   
    GPIO_IOMAP_OUT_LED_TMR3_PWM_OUT_VAL,

    /**
     * @NONE
     * 
     */    
    GPIO_IOMAP_OUT_PDM_MCLK,

    /**
     * @A : iofuncmask0[11]
     * 
     * @(A=0): qspi_nss1_out
     * @(A=1): ospi_cs_io
     */   
    GPIO_IOMAP_OUT_QSPI_NSS1_OUT_VAL,

    /**
     * @A : iofuncmask1[0]
     * 
     * @(A=0): spi0_nss_out
     * @(A=1): qspi_ospi_mnt_o[0]
     */ 
    GPIO_IOMAP_OUT_SPI0_NSS_OUT,

    /**
     * @A : iofuncmask1[1]
     * 
     * @(A=0): spi0_sck_out
     * @(A=1): qspi_ospi_mnt_o[1]
     */ 
    GPIO_IOMAP_OUT_SPI0_SCK_OUT,

    /**
     * @A : iofuncmask1[2]
     * 
     * @(A=0): spi0_io0_out
     * @(A=1): qspi_ospi_mnt_o[2]
     */    
    GPIO_IOMAP_OUT_SPI0_IO0_OUT,

    /**
     * @A : iofuncmask1[3]
     * 
     * @(A=0): spi0_io1_out
     * @(A=1): qspi_ospi_mnt_o[3]
     */    
    GPIO_IOMAP_OUT_SPI0_IO1_OUT,

    /**
     * @A : iofuncmask1[4]
     * 
     * @(A=0): spi0_io2_out
     * @(A=1): qspi_ospi_mnt_o[4]
     */ 
    GPIO_IOMAP_OUT_SPI0_IO2_OUT,

    /**
     * @A : iofuncmask1[5]
     * 
     * @(A=0): spi0_io3_out
     * @(A=1): qspi_ospi_mnt_o[5]
     */ 
    GPIO_IOMAP_OUT_SPI0_IO3_OUT,

    /**
     * @NONE
     * 
     */      
    GPIO_IOMAP_OUT_SPI1_NSS_OUT,

    /**
     * @NONE
     * 
     */  
    GPIO_IOMAP_OUT_SPI1_SCK_OUT,

    /**
     * @NONE
     * 
     */  
    GPIO_IOMAP_OUT_SPI1_IO0_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SPI1_IO1_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_LCD_DATA_O_16,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_LCD_DATA_O_17,

    /**
     * @A : iofuncmask1[13]
     * 
     * @(A=0): spi2_nss_out
     * @(A=1): lcd_data_o[18]
     */ 
    GPIO_IOMAP_OUT_SPI2_NSS_OUT_VAL,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SPI2_SCK_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SPI2_IO0_OUT,

    /**
     * @A : iofuncmask1[14]
     * 
     * @(A=0): spi2_io1_out
     * @(A=1): lcd_data_o[19]
     */ 
    GPIO_IOMAP_OUT_SPI2_IO1_OUT_VAL,

    /**
     * @A : iofuncmask1[15]
     * 
     * @(A=0): spi2_io2_out
     * @(A=1): lcd_data_o[20]
     */ 
    GPIO_IOMAP_OUT_SPI2_IO2_OUT_VAL,

    /**
     * @A : iofuncmask1[16]
     * 
     * @(A=0): spi2_io3_out
     * @(A=1): lcd_data_o[21]
     */ 
    GPIO_IOMAP_OUT_SPI2_IO3_OUT_VAL,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SDHOST_CMD_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SDHOST_DAT0_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SDHOST_DAT1_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SDHOST_DAT2_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_SDHOST_DAT3_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_IIS0_MCLK_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_IIS0_WSCLK_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_IIS0_BCLK_OUT,

    /**
     * @NONE
     * 
     */
    GPIO_IOMAP_OUT_IIS0_DO,

    /**
     * @A : iofuncmask0[31]
     * @B : iofuncmask1[17]
     * 
     * @(A=0) & (B=0) : iis1_mclk_out
     * @(A=1) & (B=0) : uart5_tx
     * @(A=0) & (B=1) : lcd_data_o[22]
     */   
    GPIO_IOMAP_OUT_IIS1_MCLK_OUT_VAL,

    /**
     * @A : iofuncmask1[18]
     * 
     * @(A=0): iis1_wsclk_out
     * @(A=1): lcd_data_o[23]
     */ 
    GPIO_IOMAP_OUT_IIS1_WSCLK_OUT_VAL,

    /**
     * @A : iofuncmask0[30]
     * 
     * @(A=0): iis1_bclk_out
     * @(A=1): spi1_io2_out
     */
    GPIO_IOMAP_OUT_IIS1_BCLK_OUT_VAL,

    /**
     * @A : iofuncmask0[29]
     * 
     * @(A=0): iis1_do
     * @(A=1): spi1_io3_out
     */
    GPIO_IOMAP_OUT_IIS1_DO_VAL,

    /**
     * @A : iofuncmask0[13]
     * 
     * @(A=0): clk_to_io
     * @(A=1): uart4_tx
     */
    GPIO_IOMAP_OUT_CLK_TO_IO_VAL,
};


/** @defgroup Enumeration constant for GPIO IO map input function selection
  * @{
  */
enum gpio_iomap_in_func{
    GPIO_IOMAP_INPUT                                        = 0xF000,
    GPIO_IOMAP_IN_TMR0_CAP_IN                               = 0x0,
    GPIO_IOMAP_IN_TMR0_SYNC_IN__EXT_RFSWITCH_EN0_IN         ,
    GPIO_IOMAP_IN_TMR1_CAP_IN                               ,
    GPIO_IOMAP_IN_TMR2_CAP_IN                               ,
    GPIO_IOMAP_IN_TMR3_CAP_IN                               ,
    GPIO_IOMAP_IN_PDM_DATA_IN                               ,
    GPIO_IOMAP_IN_PTA_REQ_IN                                ,
    GPIO_IOMAP_IN_PTA_PRI_IN                                ,
    GPIO_IOMAP_IN_FREQ_IND_IN                               ,
    GPIO_IOMAP_IN_STMR0_CAP_IN__LCD_D3_IN_MASK1_22          ,
    GPIO_IOMAP_IN_STMR1_CAP_IN__LCD_D4_IN_MASK1_23          ,
    GPIO_IOMAP_IN_STMR2_CAP_IN__LCD_D5_IN_MASK1_24          ,
    GPIO_IOMAP_IN_STMR3_CAP_IN__LCD_D6_IN_MASK1_25          ,
    GPIO_IOMAP_IN_PORT_WKUP_IN0                             ,
    GPIO_IOMAP_IN_PORT_WKUP_IN1__LCD_D7_IN_MASK1_26         ,
    GPIO_IOMAP_IN_PORT_WKUP_IN2__LCD_D8_IN_MASK1_27         ,
    GPIO_IOMAP_IN_PORT_WKUP_IN3__LCD_TE_MASK0_10            ,
    GPIO_IOMAP_IN_UART0_IN                                  ,
    GPIO_IOMAP_IN_UART0_CTS_DE_IN                           ,
    GPIO_IOMAP_IN_UART1_IN__LCD_D9_IN_MASK1_28              ,
    GPIO_IOMAP_IN_UART1_CTS_DE_IN__LCD_D10_IN_MASK1_29      ,
    GPIO_IOMAP_IN_FB_IN__EXT_PA_EN__SYS_NMI                 ,
    GPIO_IOMAP_IN_UART4_IN                                  ,
    GPIO_IOMAP_IN_LCD_D0_IN_MASK1_19                        ,
    GPIO_IOMAP_IN_SPI0_NSS_IN                               ,
    GPIO_IOMAP_IN_SPI0_SCK_IN                               ,
    GPIO_IOMAP_IN_SPI0_IO0_IN                               ,
    GPIO_IOMAP_IN_SPI0_IO1_IN                               ,
    GPIO_IOMAP_IN_SPI0_IO2_IN                               ,
    GPIO_IOMAP_IN_SPI0_IO3_IN                               ,
    GPIO_IOMAP_IN_SPI1_NSS_IN__LCD_D11_IN_MASK1_30          ,
    GPIO_IOMAP_IN_SPI1_SCK_IN                               ,
    GPIO_IOMAP_IN_SPI1_IO0_IN                               ,
    GPIO_IOMAP_IN_SPI1_IO1_IN__LCD_D12_IN_MASK1_31          ,
    GPIO_IOMAP_IN_LCD_D1_IN_MASK1_20                        ,
    GPIO_IOMAP_IN_LCD_D2_IN_MASK1_21                        ,
    GPIO_IOMAP_IN_SPI2_NSS_IN__LCD_D13_IN_MASK2_0           ,
    GPIO_IOMAP_IN_SPI2_SCK_IN                               ,
    GPIO_IOMAP_IN_SPI2_IO0_IN                               ,
    GPIO_IOMAP_IN_SPI2_IO1_IN__LCD_D14_IN_MASK2_1           ,
    GPIO_IOMAP_IN_SPI2_IO2_IN__LCD_D15_IN_MASK2_2           ,
    GPIO_IOMAP_IN_SPI2_IO3_IN__LCD_D16_IN_MASK2_3           ,
    GPIO_IOMAP_IN_STMR4_CAP_IN__LCD_D17_IN_MASK2_4          ,
    GPIO_IOMAP_IN_STMR5_CAP_IN__LCD_D18_IN_MASK2_5          ,
    GPIO_IOMAP_IN_SDHOST_CMD_IN                             ,
    GPIO_IOMAP_IN_SDHOST_DAT0_IN                            ,
    GPIO_IOMAP_IN_SDHOST_DAT1_IN                            ,
    GPIO_IOMAP_IN_SDHOST_DAT2_IN                            ,
    GPIO_IOMAP_IN_SDHOST_DAT3_IN                            ,
    GPIO_IOMAP_IN_IIS0_MCLK_IN__LCD_D19_IN_MASK2_6          ,
    GPIO_IOMAP_IN_IIS0_WSCLK_IN__LCD_D20_IN_MASK2_7         ,
    GPIO_IOMAP_IN_IIS0_BCLK_IN__LCD_D21_IN_MASK2_8          ,
    GPIO_IOMAP_IN_IIS0_DAT_IN                               ,
    GPIO_IOMAP_IN_IIS1_MCLK_IN__UART5_IN__LCD_D22_IN_MASK2_9,
    GPIO_IOMAP_IN_IIS1_WSCLK_IN__LCD_D23_IN_MASK2_10        ,
    GPIO_IOMAP_IN_SPI1_IO2_IN__IIS1_BCLK_IN                 ,
    GPIO_IOMAP_IN_SPI1_IO3_IN__IIS1_DAT_IN                  ,
}; 


/** @} Docxygenid_IO_function_enum*/

int32 gpio_driver_strength(uint32 pin, enum pin_driver_strength strength);
int32 gpio_set_altnt_func(uint32 pin, enum gpio_afio_set afio); 
int32 gpio_iomap_output(uint32 pin, enum gpio_iomap_out_func func_sel);
int32 gpio_iomap_input(uint32 pin, enum gpio_iomap_in_func func_sel);
int32 gpio_iomap_inout(uint32 pin, enum gpio_iomap_in_func in_func_sel, enum gpio_iomap_out_func out_func_sel);


#ifdef __cplusplus
}
#endif
#endif

/*************************** (C) COPYRIGHT 2016-2022 HUGE-IC ***** END OF FILE *****/

