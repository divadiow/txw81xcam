// Revision History
// V1.0.0  06/01/2019  First Release, copy from 4001a project
// V1.0.1  07/27/2019  change uart0 to A2/A3
// V1.0.2  02/07/2020  change PIN_SPI_CS to PIN_SPI0_CS
// V1.1.0  03/02/2020  add pa/pa-vmode pin

#ifndef _PIN_NAMES_DEF_H_
#define _PIN_NAMES_DEF_H_

//#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------*/
/*----------UART PIN DEFINITION----------*/
/*---------------------------------------*/

/* UART0 */
#ifndef PIN_UART0_TX
#define PIN_UART0_TX 255
#endif

#ifndef PIN_UART0_RX
#define PIN_UART0_RX 255
#endif

#ifndef PIN_UART0_DE
#define PIN_UART0_DE 255
#endif

#ifndef PIN_UART0_RE
#define PIN_UART0_RE 255
#endif

/* UART1 */
#ifndef PIN_UART1_TX
#define PIN_UART1_TX 255
#endif

#ifndef PIN_UART1_RX
#define PIN_UART1_RX 255
#endif

/* UART4 */
#ifndef PIN_UART4_TX
#define PIN_UART4_TX 255
#endif

#ifndef PIN_UART4_RX
#define PIN_UART4_RX 255
#endif

#ifndef PIN_UART4_DE
#define PIN_UART4_DE 255
#endif

#ifndef PIN_UART4_RE
#define PIN_UART4_RE 255
#endif

/* UART5 */
#ifndef PIN_UART5_TX
#define PIN_UART5_TX 255
#endif

#ifndef PIN_UART5_RX
#define PIN_UART5_RX 255
#endif

#ifndef PIN_UART5_DE
#define PIN_UART5_DE 255
#endif

#ifndef PIN_UART5_RE
#define PIN_UART5_RE 255
#endif

/*---------------------------------------*/
/*----------IIC PIN DEFINITION-----------*/
/*---------------------------------------*/

/* IIC1 */
#ifndef PIN_IIC1_SCL
#define PIN_IIC1_SCL 255
#endif

#ifndef PIN_IIC1_SDA
#define PIN_IIC1_SDA 255
#endif

/* IIC2 */
#ifndef PIN_IIC2_SCL
#define PIN_IIC2_SCL 255
#endif

#ifndef PIN_IIC2_SDA
#define PIN_IIC2_SDA 255
#endif


/*---------------------------------------*/
/*----------IIS PIN DEFINITION-----------*/
/*---------------------------------------*/

/* IIS0 */
#ifndef PIN_IIS0_MCLK
#define PIN_IIS0_MCLK 255
#endif

#ifndef PIN_IIS0_BCLK
#define PIN_IIS0_BCLK 255
#endif

#ifndef PIN_IIS0_WCLK
#define PIN_IIS0_WCLK 255
#endif

#ifndef PIN_IIS0_DATA
#define PIN_IIS0_DATA 255
#endif

/* IIS1 */
#ifndef PIN_IIS1_MCLK
#define PIN_IIS1_MCLK 255
#endif

#ifndef PIN_IIS1_BCLK
#define PIN_IIS1_BCLK 255
#endif

#ifndef PIN_IIS1_WCLK
#define PIN_IIS1_WCLK 255
#endif

#ifndef PIN_IIS1_DATA
#define PIN_IIS1_DATA 255
#endif


/*---------------------------------------*/
/*----------PDM PIN DEFINITION-----------*/
/*---------------------------------------*/

/* PDM */
#ifndef PIN_PDM_MCLK
#define PIN_PDM_MCLK 255
#endif

#ifndef PIN_PDM_DATA
#define PIN_PDM_DATA 255
#endif


/*---------------------------------------*/
/*----------LED PIN DEFINITION-----------*/
/*---------------------------------------*/

/* LED */
#ifndef PIN_LED_SEG0
#define PIN_LED_SEG0 255
#endif

#ifndef PIN_LED_SEG1
#define PIN_LED_SEG1  255
#endif

#ifndef PIN_LED_SEG2
#define PIN_LED_SEG2  255
#endif

#ifndef PIN_LED_SEG3
#define PIN_LED_SEG3  255
#endif

#ifndef PIN_LED_SEG4
#define PIN_LED_SEG4  255
#endif

#ifndef PIN_LED_SEG5
#define PIN_LED_SEG5  255
#endif

#ifndef PIN_LED_SEG6
#define PIN_LED_SEG6  255
#endif

#ifndef PIN_LED_SEG7
#define PIN_LED_SEG7  255
#endif

#ifndef PIN_LED_SEG8
#define PIN_LED_SEG8  255
#endif

#ifndef PIN_LED_SEG9
#define PIN_LED_SEG9  255
#endif

#ifndef PIN_LED_SEG10 
#define PIN_LED_SEG10 255
#endif

#ifndef PIN_LED_SEG11
#define PIN_LED_SEG11 255
#endif

#ifndef PIN_LED_COM0
#define PIN_LED_COM0 255
#endif

#ifndef PIN_LED_COM1
#define PIN_LED_COM1 255
#endif

#ifndef PIN_LED_COM2
#define PIN_LED_COM2 255
#endif

#ifndef PIN_LED_COM3
#define PIN_LED_COM3 255
#endif

#ifndef PIN_LED_COM4
#define PIN_LED_COM4 255
#endif

#ifndef PIN_LED_COM5
#define PIN_LED_COM5 255
#endif

#ifndef PIN_LED_COM6
#define PIN_LED_COM6 255
#endif

#ifndef PIN_LED_COM7
#define PIN_LED_COM7 255
#endif


/*---------------------------------------*/
/*---------GMAC PIN DEFINITION-----------*/
/*---------------------------------------*/

/* GMAC */

#ifndef PIN_GMAC_RMII_REF_CLKIN
#define PIN_GMAC_RMII_REF_CLKIN PB_14
#endif

#ifndef PIN_GMAC_RMII_RXD0
#define PIN_GMAC_RMII_RXD0      PB_12
#endif

#ifndef PIN_GMAC_RMII_RXD1
#define PIN_GMAC_RMII_RXD1      PB_13
#endif

#ifndef PIN_GMAC_RMII_TXD0
#define PIN_GMAC_RMII_TXD0      PB_15
#endif

#ifndef PIN_GMAC_RMII_TXD1
#define PIN_GMAC_RMII_TXD1      PC_0
#endif

#ifndef PIN_GMAC_RMII_CRS_DV
#define PIN_GMAC_RMII_CRS_DV    PC_2
#endif

#ifndef PIN_GMAC_RMII_TX_EN
#define PIN_GMAC_RMII_TX_EN     PC_1
#endif

#ifndef PIN_GMAC_RMII_MDIO
#define PIN_GMAC_RMII_MDIO      PC_4
#endif

#ifndef PIN_GMAC_RMII_MDC
#define PIN_GMAC_RMII_MDC       PC_5
#endif



/*---------------------------------------*/
/*---------SDIO PIN DEFINITION-----------*/
/*---------------------------------------*/

/* SDIO */
#ifndef PIN_SDCLK
#define PIN_SDCLK  255
#endif

#ifndef PIN_SDCMD
#define PIN_SDCMD  255
#endif

#ifndef PIN_SDDAT0
#define PIN_SDDAT0 255
#endif

#ifndef PIN_SDDAT1
#define PIN_SDDAT1 255
#endif

#ifndef PIN_SDDAT2
#define PIN_SDDAT2 255
#endif

#ifndef PIN_SDDAT3
#define PIN_SDDAT3 255
#endif


/*---------------------------------------*/
/*---------QSPI PIN DEFINITION-----------*/
/*---------------------------------------*/

/* QSPI */
#ifndef PIN_QSPI_CS
#define PIN_QSPI_CS  PB_4
#endif

#ifndef PIN_QSPI_CLK
#define PIN_QSPI_CLK PB_1
#endif

#ifndef PIN_QSPI_IO0
#define PIN_QSPI_IO0 PB_0
#endif

#ifndef PIN_QSPI_IO1
#define PIN_QSPI_IO1 PB_5
#endif

#ifndef PIN_QSPI_IO2
#define PIN_QSPI_IO2 PB_3
#endif

#ifndef PIN_QSPI_IO3
#define PIN_QSPI_IO3 PB_2
#endif

#ifndef PIN_QSPI_CS1
#define PIN_QSPI_CS1 PB_6
#endif


/*---------------------------------------*/
/*---------SPI PIN DEFINITION------------*/
/*---------------------------------------*/

/* SPI0 */
#ifndef PIN_SPI0_CS
#define PIN_SPI0_CS  255
#endif

#ifndef PIN_SPI0_CLK
#define PIN_SPI0_CLK 255
#endif

#ifndef PIN_SPI0_IO0
#define PIN_SPI0_IO0 255
#endif

#ifndef PIN_SPI0_IO1
#define PIN_SPI0_IO1 255
#endif

#ifndef PIN_SPI0_IO2
#define PIN_SPI0_IO2 255
#endif

#ifndef PIN_SPI0_IO3
#define PIN_SPI0_IO3 255
#endif


/* SPI1 */
#ifndef PIN_SPI1_CS
#define PIN_SPI1_CS  255
#endif

#ifndef PIN_SPI1_CLK
#define PIN_SPI1_CLK 255
#endif

#ifndef PIN_SPI1_IO0
#define PIN_SPI1_IO0 255
#endif

#ifndef PIN_SPI1_IO1
#define PIN_SPI1_IO1 255
#endif

#ifndef PIN_SPI1_IO2
#define PIN_SPI1_IO2 255
#endif

#ifndef PIN_SPI1_IO3
#define PIN_SPI1_IO3 255
#endif

/* SPI5 */
#ifndef PIN_SPI5_CLK
#define PIN_SPI5_CLK 255
#endif

#ifndef PIN_SPI5_IO0
#define PIN_SPI5_IO0 255
#endif

#ifndef PIN_SPI5_IO1
#define PIN_SPI5_IO1 255
#endif

/* SPI6 */
#ifndef PIN_SPI6_CLK
#define PIN_SPI6_CLK 255
#endif

#ifndef PIN_SPI6_IO0
#define PIN_SPI6_IO0 255
#endif

#ifndef PIN_SPI6_IO1
#define PIN_SPI6_IO1 255
#endif


/*---------------------------------------*/
/*---------ADC PIN DEFINITION------------*/
/*---------------------------------------*/

/* ADC */
#ifndef PIN_ADC
#define PIN_ADC 255
#endif


/*---------------------------------------*/
/*---------DVP PIN DEFINITION------------*/
/*---------------------------------------*/

/* DVP */
#ifndef PIN_DVP_HSYNC
#define PIN_DVP_HSYNC  PB_7
#endif

#ifndef PIN_DVP_VSYNC
#define PIN_DVP_VSYNC  PB_6
#endif

#ifndef PIN_DVP_PCLK
#define PIN_DVP_PCLK   PB_12
#endif

#ifndef PIN_DVP_MCLK
#define PIN_DVP_MCLK   PB_9
#endif

#ifndef PIN_DVP_DATA0
#define PIN_DVP_DATA0  PB_14
#endif

#ifndef PIN_DVP_DATA1
#define PIN_DVP_DATA1  PC_0
#endif

#ifndef PIN_DVP_DATA2
#define PIN_DVP_DATA2  PC_1
#endif

#ifndef PIN_DVP_DATA3
#define PIN_DVP_DATA3  PB_15
#endif

#ifndef PIN_DVP_DATA4
#define PIN_DVP_DATA4  PB_13
#endif

#ifndef PIN_DVP_DATA5
#define PIN_DVP_DATA5  PB_11
#endif

#ifndef PIN_DVP_DATA6
#define PIN_DVP_DATA6  PB_10
#endif

#ifndef PIN_DVP_DATA7
#define PIN_DVP_DATA7  PB_8
#endif

#ifndef PIN_DVP_RESET
#define PIN_DVP_RESET  255
#endif

#ifndef PIN_DVP_PDN
#define PIN_DVP_PDN    255
#endif


/*---------------------------------------*/
/*---------SDH PIN DEFINITION------------*/
/*---------------------------------------*/

/* SDH */
#ifndef PIN_SDH_CLK
#define PIN_SDH_CLK  255
#endif

#ifndef PIN_SDH_CMD
#define PIN_SDH_CMD  255
#endif

#ifndef PIN_SDH_DAT0
#define PIN_SDH_DAT0 255
#endif

#ifndef PIN_SDH_DAT1
#define PIN_SDH_DAT1 255
#endif

#ifndef PIN_SDH_DAT2
#define PIN_SDH_DAT2 255
#endif

#ifndef PIN_SDH_DAT3
#define PIN_SDH_DAT3 255
#endif

/*---------------------------------------*/
/*---------TIMER PIN DEFINITION----------*/
/*---------------------------------------*/

/* NORMAL TIMER0 */
#ifndef PIN_PWM_CHANNEL_0
#define PIN_PWM_CHANNEL_0 255
#endif

#ifndef PIN_CAPTURE_CHANNEL_0
#define PIN_CAPTURE_CHANNEL_0 255
#endif

/* NORMAL TIMER1 */
#ifndef PIN_PWM_CHANNEL_1
#define PIN_PWM_CHANNEL_1 255
#endif

#ifndef PIN_CAPTURE_CHANNEL_1
#define PIN_CAPTURE_CHANNEL_1 255
#endif

/* LED_TIMER0 */
#ifndef PIN_PWM_CHANNEL_2
#define PIN_PWM_CHANNEL_2 255
#endif

/* SUPTMR0 */
#ifndef PIN_PWM_CHANNEL_3
#define PIN_PWM_CHANNEL_3 255
#endif

#ifndef PIN_PWM_CHANNEL_4
#define PIN_PWM_CHANNEL_4 255
#endif

#ifndef LCD_TE
#define LCD_TE 255
#endif

enum pin_name {

    PA_0  =  0,
    PA_1,
    PA_2,
    PA_3,
    PA_4,
    PA_5,
    PA_6,
    PA_7,
    PA_8,
    PA_9,
    PA_10,
    PA_11,
    PA_12,
    PA_13,
    PA_14,
    PA_15,
 
    PB_0,
    PB_1,
    PB_2,
    PB_3,
    PB_4,
    PB_5,
    PB_6,
    PB_7,
    PB_8,
    PB_9,
    PB_10,
    PB_11,
    PB_12,
    PB_13,
    PB_14,
    PB_15,

    PC_0,
    PC_1,
    PC_2,
    PC_3,
    PC_4,
    PC_5,
    PC_6,
    PC_7,
    PC_8,
    PC_9,
    PC_10,
    PC_11,
    PC_12,
    PC_13,
    PC_14,
    PC_15,

    PE_0,
    PE_1,
    PE_2,
    PE_3,
    PE_4,
    PE_5,
    PE_6,
    PE_7,
    PE_8,
    PE_9,
    PE_10,
    PE_11,
    PE_12,
    PE_13,
    PE_14,
    PE_15,    
};

#ifdef __cplusplus
}
#endif
#endif
