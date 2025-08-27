#ifndef _TXW81X_DEF_H_
#define _TXW81X_DEF_H_

#ifdef __cplusplus
 extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "typesdef.h"

/* -------------------------  Interrupt Number Definition  ------------------------ */

enum IRQn
{
/******  Peripheral Interrupt Numbers *********************************************************/
    USB20DMA_IRQn               = 0,
    USB20MC_IRQn                = 1,
    UART0_IRQn                  = 2,
    UART1_IRQn                  = 3,
    LCD_IRQn                    = 4,
    
    QSPI_IRQn                   = 5,
    SPI0_IRQn                   = 6,
    SPI1_IRQn                   = 7,
    SPI2_IRQn                   = 8,
    OSPI_IRQn                   = 9,
    
    TIM0_IRQn                   = 10,
    TIM1_IRQn                   = 11,
    TIM2_IRQn                   = 12,
    TIM3_IRQn                   = 13,
    SCALE1_IRQn                 = 14,
    
    AUDIO_SUBSYS0_IRQn          = 15, //ALAW & HS & VAD 
    AUDIO_SUBSYS1_IRQn          = 16, //ADC
    AUDIO_SUBSYS2_IRQn          = 17, //DAC
    SDIO_IRQn                   = 18,
    SDIO_RST_IRQn               = 19,
    
    SDHOST_IRQn                 = 20,
    LMAC_IRQn                   = 21,
    GMAC_IRQn                   = 22,
    M2M0_IRQn                   = 23,
    M2M1_IRQn                   = 24,
    
    CORET_IRQn                  = 25, //CPU TIMER
    SPACC_PKA_IRQn              = 26, //SHA/SYS_AES 
    CRC_IRQn                    = 27,
    ADKEY01_IRQn                = 28,
    PD_TMR_IRQn                 = 29,
    
    WKPND_IRQn                  = 30,
    PDWKPND_IRQn                = 31,
    LVD_IRQn                    = 32,
    WDT_IRQn                    = 33,
    SYS_ERR_IRQn                = 34,
    
    IIS0_IRQn                   = 35,
    IIS1_IRQn                   = 36,
    GPIOABCE_IRQn               = 37,
    //GPIODE_IRQn                 = 38,
    DVP_IRQn                    = 39,
    
    MJPEG01_IRQn                = 40,
    //CSI2_IRQn                   = 41,
    VPP_IRQn                    = 42,
    PRC_IRQn                    = 43,
    STMR012345_IRQn             = 44,
    
    PDM_IRQn                    = 45,
    LED_TMR_IRQn                = 46,
    SCALE2_IRQn                 = 47,
    GFSK_IRQn                   = 48,
    CMPOUT01_IRQn               = 49,
    
    UART45_IRQn                 = 50,
    SCALE3_IRQn                 = 51,
    //SPI5_IRQn                   = 52,
    //SPI6_IRQn                   = 53,
    //LED_IRQn                    = 54, 
    
    //SUPTMR3_IRQn                = 55,
    //SUPTMR4_IRQn                = 56,
    //SUPTMR5_IRQn                = 57,
    USB20PHY_RTC_IRQn           = 58,
    //TK_IRQn                     = 59,
    //CPU0_SPINLOCK_IRQn          = 60,
    //CPU0_SEND_MAIL_IRQn         = 61,
    //CPU0_RECV_MAIL_IRQn         = 62,
    //AUDIO_SUBSYS_IRQn           = 63,
    SHA_VIRTUAL_IRQn             = 64,
    AES_VIRTUAL_IRQn             = 65,
    UART4_IRQn                   = 66,
    UART5_IRQn                   = 67,
    GPIOA_IRQn					         = 68,
    GPIOB_IRQn					         = 69,
    GPIOC_IRQn					         = 70,
    GPIOE_IRQn					         = 71,
    AUVAD_IRQn                   = 72,
    AUHS_IRQn                    = 73,
    AUALAW_IRQn                  = 74,
    IRQ_NUM,
};

/* ================================================================================ */
/* ================      Processor and Core Peripheral Section     ================ */
/* ================================================================================ */

/* --------  Configuration of the CK803 Processor and Core Peripherals  ------- */
#define __CK803_REV               0x0000U   /* Core revision r0p0 */
#define __MPU_PRESENT             0         /* MGU present or not */
#define __VIC_PRIO_BITS           3         /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0         /* Set to 1 if different SysTick Config is used */

#include "csi_core.h"                     /* Processor and core peripherals */
#include "stdint.h"

////////

/** @addtogroup Peripheral_registers_structures
  * @{
  */

/**
  * @}
  */

/** @addtogroup Peripheral_memory_map
  * @{
  */
#define FLASH_BASE                  ((uint32_t)0x08000000) /*!< FLASH base address in the alias region */
#define SRAM_BASE                   ((uint32_t)0x20000000) /*!< SRAM base address in the alias region */
#define SRAM_END_ADDR               ((uint32_t)0x20044000) /*!< SRAM end address in the alias region */
#define PSRAM_BASE                  ((uint32_t)0x38000000) /*!< SRAM base address in the alias region */
#define PSRAM_END_ADDR              (PSRAM_BASE + 16*1024*1024) /*!< SRAM end address in the alias region */
#define PERIPH_BASE                 ((uint32_t)0x40000000) /*!< Peripheral base address in the alias region */

/*!< Peripheral memory map */
#define APB0_BASE                PERIPH_BASE
#define APB1_BASE               (PERIPH_BASE + 0x10000)
#define AHB_BASE                (PERIPH_BASE + 0x20000)

#define QSPI_BASE               (APB0_BASE + 0x0000)
#define HG_OSPI_BASE            (APB0_BASE + 0x1000)
#define HG_SCHED_BASE           (APB0_BASE + 0x2000)
#define HG_UART0_BASE           (APB0_BASE + 0x4000)
#define HG_UART1_BASE           (APB0_BASE + 0x4100)
#define HG_UART2_BASE           (APB0_BASE + 0x4200)
#define HG_UART3_BASE           (APB0_BASE + 0x4300)
#define HG_UART4_BASE           (APB0_BASE + 0x4b70)
#define HG_UART5_BASE           (APB0_BASE + 0x4b90)
#define SPI0_BASE               (APB0_BASE + 0x4400)
#define SPI1_BASE               (APB0_BASE + 0x4500)
#define SPI2_BASE               (APB0_BASE + 0x4600)
#define SPI3_BASE               (APB0_BASE + 0x4700)
#define SPI4_BASE               (APB0_BASE + 0x4800)
#define SPI5_BASE               (APB0_BASE + 0x4b20)
#define SPI6_BASE               (APB0_BASE + 0x4b38)
#define HG_LED_BASE             (APB0_BASE + 0x4b50)
#define IIC1_BASE               (APB0_BASE + 0x4500)
#define IIC2_BASE               (APB0_BASE + 0x4600)
#define IIC3_BASE               (APB0_BASE + 0x4700)
#define IIC4_BASE               (APB0_BASE + 0x4800)
#define HG_IIS0_BASE            (APB0_BASE + 0x80C0)
#define HG_IIS1_BASE            (APB0_BASE + 0x80E0)
#define HG_PDM_BASE             (APB0_BASE + 0x8110)
#define PRC_BASE                (APB0_BASE + 0x5000)
#define OF_BASE                 (APB0_BASE + 0x5020)
#define DVP_BASE                (APB0_BASE + 0x5040)
#define VPP_BASE                (APB0_BASE + 0x5080)
#define JPG0_BASE               (APB0_BASE + 0x5100)
#define JPG1_BASE               (APB0_BASE + 0x5200)
#define MJPEG0_TAB_BASE        (APB0_BASE + 0x5180)
#define MJPEG1_TAB_BASE        (APB0_BASE + 0x5280)
#define CSI2_HOST_BASE          (APB0_BASE + 0x5700)

#define TK_BASE                 (APB0_BASE + 0x6000)

#define SCALE0_BASE             (APB0_BASE + 0x7000)
#define SCALE1_BASE             (APB0_BASE + 0x7100)
#define LCDC_BASE               (APB0_BASE + 0x7000)
#define SCALE2_BASE             (APB0_BASE + 0x71A0)
#define SCALE3_BASE             (APB0_BASE + 0x7200)
#define AUDIO_BASE              (APB0_BASE + 0x8000)

#define COMP_BASE               (APB0_BASE + 0x9000)
#define EFUSE_BASE              (APB0_BASE + 0xa000)

#define HG_SYSAES_BASE          (APB1_BASE + 0x2000)
//#define RFADCDIG_BASE           (APB1_BASE + 0x4000)
#define HG_SHA_BASE             (APB1_BASE + 0x4000)
#define WDT_BASE                (APB1_BASE + 0x5000)
#define TIMER_ALL_BASE          (APB1_BASE + 0x5128)
#define TIMER0_BASE             (APB1_BASE + 0x5100)
#define TIMER1_BASE             (APB1_BASE + 0x5200)
#define TIMER2_BASE             (APB1_BASE + 0x5300)
#define TIMER3_BASE             (APB1_BASE + 0x5400)
#define TIMER4_BASE             (APB1_BASE + 0x5500)
//#define TIMER5_BASE             (APB1_BASE + 0x5600)
//#define TIMER6_BASE             (APB1_BASE + 0x5700)
//#define TIMER7_BASE             (APB1_BASE + 0x5800)
#define SUPTMR_BASE             (APB1_BASE + 0x5600)
#define SIMPLE_TIMER0_BASE      (APB1_BASE + 0x5B00)
#define SIMPLE_TIMER1_BASE      (APB1_BASE + 0x5B10)
#define SIMPLE_TIMER2_BASE      (APB1_BASE + 0x5B20)
#define SIMPLE_TIMER3_BASE      (APB1_BASE + 0x5B30)
#define SIMPLE_TIMER4_BASE      (APB1_BASE + 0x5B40)
#define SIMPLE_TIMER5_BASE      (APB1_BASE + 0x5B50)



//#define HG_RTC_BASE             (APB1_BASE + 0x8050)
#define HG_CRC32_BASE           (APB1_BASE + 0x7000)
#define PMU_BASE                (APB1_BASE + 0x8000)
#define LVD_BASE                (APB1_BASE + 0x8000)
#define RFDIG_BASE              (APB1_BASE + 0x9000)
#define ADKEY_BASE              (APB1_BASE + 0xa000)
#define ADKEY1_BASE             (APB1_BASE + 0xa00c)
#define ZH_RFSPI_BASE           (APB1_BASE + 0xb000)
#define RFDIGCAL_BASE           (APB1_BASE + 0xd000)

#define HG_SYSCTRL_BASE         (AHB_BASE + 0x0000)
#define HG_GPIOA_BASE           (AHB_BASE + 0x0a00)
#define HG_GPIOB_BASE           (AHB_BASE + 0x0b00)
#define HG_GPIOC_BASE           (AHB_BASE + 0x0c00)
#define HG_GPIOD_BASE           (AHB_BASE + 0x0d00)
#define HG_GPIOE_BASE           (AHB_BASE + 0x0e00)
#define HG_GPIOF_BASE           (AHB_BASE + 0x0f00)
#define HG_GPIOG_BASE           (AHB_BASE + 0x1000)
#define HG_GPIOH_BASE           (AHB_BASE + 0x1100)

#define M2M_DMA_BASE            (AHB_BASE + 0x1200)
#define M2M_DMA0_BASE           (AHB_BASE + 0x1200)
#define M2M_DMA1_BASE           (AHB_BASE + 0x121C)
#define SYS_MNT_BASE            (AHB_BASE + 0x1300)
#define SDHOST_BASE             (AHB_BASE + 0x1400)
#define LED_TIMER0_BASE         (PMU_BASE + 0x000d0)
#define LED_TIMER1_BASE         (PMU_BASE + 0x000d4)
#define LED_TIMER2_BASE         (PMU_BASE + 0x000d8)
#define LED_TIMER3_BASE         (PMU_BASE + 0x000dc)


#define HG_SUPTMR_COMM_BASE     (SUPTMR_BASE       )
#define HG_SUPTMR0_BASE         (APB1_BASE + 0x5B00)
#define HG_SUPTMR1_BASE         (APB1_BASE + 0x5B10)
#define HG_SUPTMR2_BASE         (APB1_BASE + 0x5B20)
#define HG_SUPTMR3_BASE         (APB1_BASE + 0x5B30)
#define HG_SUPTMR4_BASE         (APB1_BASE + 0x5B40)
#define HG_SUPTMR5_BASE         (APB1_BASE + 0x5B50)

#define HG_SDIO20_SLAVE_BASE    (AHB_BASE + 0x20000)
#define MODEM_BASE              (AHB_BASE + 0x40000)
#define LMAC_BASE               (AHB_BASE + 0x42000)
#define HG_GMAC_BASE            (AHB_BASE + 0x51000)
#define HG_USB20_DEVICE_BASE    (AHB_BASE + 0xd0000)


typedef enum {
    HG_AHB_PT_ALL,
    HG_AHB_PT_GPIOA_DEBUNCE,
    HG_AHB_PT_GPIOB_DEBUNCE,
    HG_AHB_PT_GPIOC_DEBUNCE,
    HG_AHB_PT_M2M_DMA,
    HG_AHB_PT_SYS_MNT,
    HG_AHB_PT_SDMMC,
    HG_AHB_PT_SDIO,
    HG_AHB_PT_MODEM,
    HG_AHB_PT_LMAC,
    HG_AHB_PT_GMAC,
    HG_AHB_PT_USB20,

    HG_APB0_PT_ALL,
    HG_APB0_PT_QSPI,
    HG_APB0_PT_OSPI,    
    HG_APB0_PT_UART0,
    HG_APB0_PT_UART1,
    HG_APB0_PT_UART4,
    HG_APB0_PT_UART5,
    HG_APB0_PT_SPI0,
    HG_APB0_PT_SPI1,
    HG_APB0_PT_SPI2,
    HG_APB0_PT_SPI3,
    HG_APB0_PT_SPI5,
    HG_APB0_PT_SPI6,
    HG_APB0_PT_LED,
    HG_APB0_PT_IIC0,
    HG_APB0_PT_IIC1,
    HG_APB0_PT_IIC2,
    HG_APB0_PT_IIC3,
    HG_APB0_PT_IIS0,
    HG_APB0_PT_IIS1,
    HG_APB0_PT_PDM,
    HG_APB0_PT_JPEG,
    HG_APB0_PT_DVP,
    HG_APB0_PT_CMP,
    HG_APB0_PT_EFUSE,

    HG_APB1_PT_ALL,
    HG_APB1_PT_AES,
    HG_APB1_PT_WDT,
    HG_APB1_PT_TMR0,
    HG_APB1_PT_TMR1,
    HG_APB1_PT_TMR2,
    HG_APB1_PT_TMR3,
    HG_APB1_PT_STMR,
    HG_APB1_PT_SUPTMR,
    HG_APB1_PT_CRC,
    HG_APB1_PT_PMU,
    HG_APB1_PT_LVD,
    HG_APB1_PT_ADKEY,
    

    
} HG_Peripheral_Type;
    

/**
  * @}
  */

enum phy_rf_mode {
    INT_PHY_EXT_RF_MODE = 1, //for ext-rf case
    PHY_RF_NORMAL_MODE  = 2, //tx4001 ic normal case
    INT_RF_EXT_PHY_MODE = 3, //for transceiver case
};

#define EFUSE_OFFSET       3

#define OSC_CLK     32000000UL

#define M2M_DMA

#define IS_DCACHE_ADDR(addr) ((uint32)(addr) >= 0x38000000 && (uint32)(addr) < 0x38f00000) //psram
#define IS_PSRAM_ADDR(addr)  ((uint32)(addr) >= 0x38000000 && (uint32)(addr) < 0x38f00000) //psram

#ifdef __cplusplus
}
#endif

#endif

