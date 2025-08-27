/**
  ******************************************************************************
  * @file    Libraries/Driver/include/LL/tx_peg_ll_qspi.h
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    01-07-2019
  * @brief   This file contains all the QSPI LL firmware functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2019 HUGE-IC</center></h2>
  *
  *
  *
  ******************************************************************************
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _HG_XSPI_PSRAM_H_
#define _HG_XSPI_PSRAM_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

     
/** @addtogroup TX_PEG_StdPeriph_Driver CanesVenatici Driver
  * @{
  */
     
/** @addtogroup qspi_interface_gr QSPI Driver
  * @ingroup  TX_PEG_StdPeriph_Driver
  * @{
  */ 

/** @addtogroup QSPI_LL_Driver QSPI LL Driver
  * @ingroup  qspi_interface_gr
  * @brief Mainly the driver part of the QSPI module, which includes \b QSPI \b Register 
  * \b Constants, \b QSPI \b Exported \b Constants, \b QSPI \b Exported \b Struct, \b QSPI
  * \b Data \b transfers \b functions, \b QSPI \b Initialization \b and \b QSPI \b Configuration 
  * \b And \b Interrupt \b Handle \b function.
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/


struct psram_attr {
    uint32 read_mem_opc     : 8,
           write_mem_opc    : 8,
           read_reg_opc     : 8,
           write_reg_opc    : 8;
    
    uint32 read_lc      : 4,
           write_lc     : 4,
           lc_type      : 1,
           driver       : 2,
           rbx_en       : 1,
           slow_refresh_en : 1,
           /* 0/1/2/3 for sigle\dual\quad\octal */
           wire_mode    : 3,
           ddr_mode     : 1,
           reserved1    : 7;

    uint32 burst_type   : 1,
           burst_len    : 15,
           density_mbyte: 16;
    
    uint8  psram_mr[8];
    
    /* read-only attr */
    uint32 vid          : 5,
           did          : 2,
           is_good_die  : 1,

           /* 0 ：1.8V, 1:3.3V */
           voltage_range: 1,
           refresh_range: 3,
           /* 0 ：
            * 1 ：half-sleep
            * 2 ：deep sleep
            */
           ulp_mode     : 4,
           
           /* 0 ：slow, 1:fast */
           self_refresh_speed : 1,
           ulp_half_sleep_support : 1,
           rbx_support  : 1,
           reserved3    : 1,
           voltage_gear : 4,
           
           reserved2    : 8;
    
    uint32 page_size : 16,
           clk_mhz   : 16;
};


struct stig_req {
    uint32  is_read : 1,
            addr_len: 3,
            data_len: 4,
            cmd     : 8,
            dummy   : 8,
            reserved: 8;
    uint32  addr;
    void   *p_buf;
};

enum psram_type {
    APS1604M_3SQR,
    APS1604M_DQRA,
    APS3208K_OKUD,
    APS6408L_OBMx,
    APS1604M_3SQR_TXW817_812,
    SCKW18X128800,
    SCKM18X032800,
    PSRAM_TYPE_END,
};

  
/***** DRIVER API *****/



/***** LL API *****/


  
/***** LL API AND DRIVER API *****/

uint8_t psram_init(enum psram_type pt, uint32 clk_hz,uint32 check_size);
int psram_auto_init();
void psram_deinit(enum psram_type pt) ;





#ifdef __cplusplus
}
#endif

/**
  * @}
  */

/**
  * @}
  */

#endif 

/*************************** (C) COPYRIGHT 2023 HUGE-IC ***** END OF FILE *****/
