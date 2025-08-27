
// @file    pin_function.c
// @author  wangying
// @brief   This file contains all the mars pin functions.

// Revision History
// V1.0.0  06/01/2019  First Release, copy from 4001a project
// V1.0.1  07/05/2019  add lmac pin-func
// V1.0.2  07/06/2019  add sdio pull-up regs config
// V1.0.3  07/09/2019  add agc/rx-bw/lo-freq-idx gpio control
// V1.0.4  07/18/2019  change gpio-agc default index to 5
// V1.0.5  07/19/2019  uart1 only init tx
// V1.0.6  07/23/2019  add uart-rx pull-up resistor config
// V1.0.7  07/24/2019  switch-en1 disable; delete fdd/tdd macro-def switch
// V1.0.8  07/26/2019  not use pb17 for mac debug for it is used to reset ext-rf
// V1.0.9  07/29/2019  add function dbg_pin_func()
// V1.1.0  02/11/2020  add spi-pin-function
// V1.1.1  02/27/2020  fix uart1 pin-function
// V1.2.0  03/02/2020  add uart0-pin code and rf-pin code
// V1.2.1  03/26/2020  fix vmode pin
// V1.2.2  04/14/2020  use pa2 as lmac debug1:rx_req

#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "lib/lcd/lcd.h"


__weak void user_pin_func(int dev_id, int request) {};


void gpio_iomap_output_iomask(uint32 io_func)
{

    #define __INDEX_NUM (3)

    uint32 i = 0;
    uint32 mask_bit = 0;
    uint32 mask_val = 0;
    uint32 mask = 0;

    for (i=0; i<__INDEX_NUM; i++) {
        switch (i) {
            case (0):
                mask     = IO_INDEX0_GET(io_func);
                mask_val = IO_INDEX0_GET_VAL(io_func);
                mask_bit = IO_INDEX0_GET_BIT(io_func);
                break;
            case (1):
                mask     = IO_INDEX1_GET(io_func);
                mask_val = IO_INDEX1_GET_VAL(io_func);
                mask_bit = IO_INDEX1_GET_BIT(io_func);
                break;
            case (2):
                mask     = IO_INDEX2_GET(io_func);
                mask_val = IO_INDEX2_GET_VAL(io_func);
                mask_bit = IO_INDEX2_GET_BIT(io_func);
                break;
        }

        switch (mask) {
            //mask0
            case (0):
                if (mask_val) {
                    SYSCTRL_REG_OPT(SYSCTRL->IOFUNCMASK0 |= BIT(mask_bit));
                } else {
                    SYSCTRL_REG_OPT(SYSCTRL->IOFUNCMASK0 &=~ BIT(mask_bit));
                }
                break;
            //mask1
            case (1):
                if (mask_val) {
                    SYSCTRL_REG_OPT(SYSCTRL->IOFUNCMASK1 |= BIT(mask_bit));
                } else {
                    SYSCTRL_REG_OPT(SYSCTRL->IOFUNCMASK1 &=~ BIT(mask_bit));
                }
                break;
        }
    }
}


/** 
  * @brief  Configure the GPIO pin driver strength.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @strength         : Driver strength to configure the GPIO pin, reference @ref gpio_private_pin_driver_strength.
  * @return
  *         - RET_OK  : Configure the GPIO pin driver strength successfully.
  *         - RET_ERR : Configure the GPIO pin driver strength unsuccessfully.
  */
int32 gpio_driver_strength(uint32 pin, enum pin_driver_strength strength)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_DRIVER_STRENGTH, strength, 0);
    }
    return RET_ERR;
}


/** 
  * @brief  Configure the GPIO module AFIO.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @afio             : AFIO value, reference @ref gpio_private_afio_set.
  * @return
  *         - RET_OK  : GPIO module configure AFIO successfully.
  *         - RET_ERR : GPIO module configure AFIO unsuccessfully.
  */
int32 gpio_set_altnt_func(uint32 pin, enum gpio_afio_set afio)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_AFIO_SET, afio, 0);
    }
    return RET_ERR;
}


/** 
  * @brief  Configure the GPIO module IOMAP_OUTPUT.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @func_sel         : IOMAP_OUTPUT function value, reference @ref gpio_private_iomap_out_func.
  * @return
  *         - RET_OK  : GPIO module configure IOMAP_OUTPUT successfully.
  *         - RET_ERR : GPIO module configure IOMAP_OUTPUT unsuccessfully.
  */
int32 gpio_iomap_output(uint32 pin, enum gpio_iomap_out_func func_sel)
{
    struct gpio_device *gpio = gpio_get(pin);
    
    if (( func_sel &~ IO_FUNC(0xFF)       ) && 
        ( (IO_INDEX0_GET_VAL(func_sel)) ||
          (IO_INDEX1_GET_VAL(func_sel)) ||
          (IO_INDEX2_GET_VAL(func_sel))   ) ) {
        gpio_iomap_output_iomask(func_sel);
        
        if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
            return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_IOMAP_OUT_FUNC, IO_FUNC(func_sel), 0);
        }
    } else {
        if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
            return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_IOMAP_OUT_FUNC, func_sel, 0);
        }
    }

    return RET_ERR;
}


/** 
  * @brief  Configure the GPIO module IOMAP_INPUT.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @func_sel         : IOMAP_INPUT function value, reference @ref gpio_private_iomap_in_func.
  * @return
  *         - RET_OK  : GPIO module configure IOMAP_INPUT successfully.
  *         - RET_ERR : GPIO module configure IOMAP_INPUT unsuccessfully.
  */
int32 gpio_iomap_input(uint32 pin, enum gpio_iomap_in_func func_sel)
{

    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_IOMAP_IN_FUNC, func_sel, 0);
    }

    return RET_ERR;
}


/** 
  * @brief  Configure the GPIO module IOMAP_INOUT.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @in_func_sel      : IOMAP_INPUT function value, reference @ref gpio_private_iomap_in_func.
  * @out_func_sel     : IOMAP_OUTPUT function value, reference @ref gpio_private_iomap_out_func.
  * @return
  *         - RET_OK  : GPIO module configure IOMAP_INOUT successfully.
  *         - RET_ERR : GPIO module configure IOMAP_INOUT unsuccessfully.
  */
int32 gpio_iomap_inout(uint32 pin, enum gpio_iomap_in_func in_func_sel, enum gpio_iomap_out_func out_func_sel)
{

    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_IOMAP_INOUT_FUNC, in_func_sel, out_func_sel);
    }

    return RET_ERR;
}

static int uart_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_UART0_DEVID:
            if (request) {
                gpio_iomap_output(PIN_UART0_TX, GPIO_IOMAP_OUT_UART0_TX);
                gpio_iomap_input(PIN_UART0_RX, GPIO_IOMAP_IN_UART0_IN);
                gpio_set_mode(PIN_UART0_RX, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);

                if(PIN_UART0_RE != 255) {
                    gpio_iomap_output(PIN_UART0_RE, GPIO_IOMAP_OUT_UART0_RTS_RE_O);
                }

                if(PIN_UART0_DE != 255) {
                    gpio_iomap_output(PIN_UART0_DE, GPIO_IOMAP_OUT_UART0_CTS_DE_OUT);
                }
            } else {
                gpio_set_dir(PIN_UART0_TX, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_UART0_RX, GPIO_DIR_INPUT);
            }
            break;
        case (HG_UART4_DEVID):
            if (request) {
                SYSCTRL_REG_OPT(SYSCTRL->IOFUNCMASK0 |= BIT(13));
                gpio_iomap_output(PIN_UART4_TX, GPIO_IOMAP_OUT_CLK_TO_IO);
                gpio_iomap_input(PIN_UART4_RX, GPIO_IOMAP_IN_UART4_IN);

                if(PIN_UART4_RE != 255) {
                    gpio_iomap_output(PIN_UART4_RE, GPIO_DIR_OUTPUT);
                    gpio_set_dir(PIN_UART4_RE, GPIO_DIR_OUTPUT);
                }

                if(PIN_UART4_DE != 255) {
                    gpio_iomap_output(PIN_UART4_DE, GPIO_DIR_OUTPUT);
                    gpio_set_dir(PIN_UART4_DE, GPIO_DIR_OUTPUT);
                }
            } else {
                gpio_set_dir(PIN_UART4_TX, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_UART4_RX, GPIO_DIR_INPUT);
            }
            break;
        case (HG_UART5_DEVID):
            if (request) {
                SYSCTRL_REG_OPT(SYSCTRL->IOFUNCMASK0 |= BIT(31));
                gpio_iomap_output(PIN_UART5_TX, GPIO_IOMAP_OUT_IIS1_MCLK_OUT);
                gpio_iomap_input(PIN_UART5_RX, GPIO_IOMAP_IN_IIS1_MCLK_IN__UART5_IN__LCD_D22_IN_MASK2_9);
                if(PIN_UART5_RE != 255) {
                    gpio_iomap_output(PIN_UART5_RE, GPIO_DIR_OUTPUT);
                    gpio_set_dir(PIN_UART5_RE, GPIO_DIR_OUTPUT);
                }

                if(PIN_UART5_DE != 255) {
                    gpio_iomap_output(PIN_UART5_DE, GPIO_DIR_OUTPUT);
                    gpio_set_dir(PIN_UART5_DE, GPIO_DIR_OUTPUT);
                }
            } else {
                gpio_set_dir(PIN_UART5_TX, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_UART5_RX, GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int gmac_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_GMAC_DEVID:
            if (request) {
#ifdef GMAC_RMII_IO_ALTERNATIVE
                gpio_set_altnt_func(PIN_GMAC_RMII_REF_CLKIN, 2);
                gpio_set_altnt_func(PIN_GMAC_RMII_RXD0, 2);
                gpio_set_altnt_func(PIN_GMAC_RMII_RXD1, 2);
                gpio_set_altnt_func(PIN_GMAC_RMII_TXD0, 2);
                gpio_set_altnt_func(PIN_GMAC_RMII_TXD1, 2);
                gpio_set_altnt_func(PIN_GMAC_RMII_CRS_DV, 2);
                gpio_set_altnt_func(PIN_GMAC_RMII_TX_EN, 2);
#else
                gpio_set_altnt_func(PIN_GMAC_RMII_REF_CLKIN, 0);
                gpio_set_altnt_func(PIN_GMAC_RMII_RXD0, 0);
                gpio_set_altnt_func(PIN_GMAC_RMII_RXD1, 0);
                gpio_set_altnt_func(PIN_GMAC_RMII_TXD0, 0);
                gpio_set_altnt_func(PIN_GMAC_RMII_TXD1, 0);
                gpio_set_altnt_func(PIN_GMAC_RMII_CRS_DV, 0);
                gpio_set_altnt_func(PIN_GMAC_RMII_TX_EN, 0);
#endif
                gpio_set_dir(PIN_GMAC_RMII_MDIO, GPIO_DIR_OUTPUT);
                gpio_set_dir(PIN_GMAC_RMII_MDC, GPIO_DIR_OUTPUT);
            } else {
                gpio_set_dir(PIN_GMAC_RMII_REF_CLKIN, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_GMAC_RMII_RXD0, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_GMAC_RMII_RXD1, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_GMAC_RMII_TXD0, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_GMAC_RMII_TXD1, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_GMAC_RMII_CRS_DV, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_GMAC_RMII_TX_EN, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_GMAC_RMII_MDIO, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_GMAC_RMII_MDC, GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int sdio_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_SDIOSLAVE_DEVID:
            if (request) {
                gpio_set_altnt_func(PIN_SDCLK, 0);
                gpio_set_altnt_func(PIN_SDCMD, 0);
                gpio_set_mode(PIN_SDCMD, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PIN_SDCLK, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_altnt_func(PIN_SDDAT0, 0);
                gpio_set_mode(PIN_SDDAT0, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_altnt_func(PIN_SDDAT1, 0);
                gpio_set_mode(PIN_SDDAT1, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_altnt_func(PIN_SDDAT2, 0);
                gpio_set_mode(PIN_SDDAT2, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_altnt_func(PIN_SDDAT3, 0);
                gpio_set_mode(PIN_SDDAT3, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);

                gpio_ioctl(PIN_SDCMD, GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(PIN_SDCLK, GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(PIN_SDDAT0, GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(PIN_SDDAT1, GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(PIN_SDDAT2, GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(PIN_SDDAT3, GPIO_INPUT_DELAY_ON_OFF, 1, 0);
            } else {
                gpio_set_dir(PIN_SDCLK, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SDCMD, GPIO_DIR_INPUT);
                gpio_set_mode(PIN_SDCMD, GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_set_dir(PIN_SDDAT0, GPIO_DIR_INPUT);
                gpio_set_mode(PIN_SDDAT0, GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_set_dir(PIN_SDDAT1, GPIO_DIR_INPUT);
                gpio_set_mode(PIN_SDDAT1, GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_set_dir(PIN_SDDAT2, GPIO_DIR_INPUT);
                gpio_set_mode(PIN_SDDAT2, GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_set_dir(PIN_SDDAT3, GPIO_DIR_INPUT);
                gpio_set_mode(PIN_SDDAT3, GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_ioctl(PIN_SDCMD, GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(PIN_SDCLK, GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(PIN_SDDAT0, GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(PIN_SDDAT1, GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(PIN_SDDAT2, GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(PIN_SDDAT3, GPIO_INPUT_DELAY_ON_OFF, 0, 0);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int qspi_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_QSPI_DEVID:
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int xspi_pin_func(int dev_id, int request)
{
#define OSPI_MAP1_D4(n)         ((n & 0xF)<<0)
#define OSPI_MAP1_D5(n)         ((n & 0xF)<<4)
#define OSPI_MAP1_D6(n)         ((n & 0xF)<<8)
#define OSPI_MAP1_D7(n)         ((n & 0xF)<<12)
    
#define OSPI_MAP0_CLK(n)        ((n & 0xF)<<0)
#define OSPI_MAP0_CS(n)         ((n & 0xF)<<4)
#define OSPI_MAP0_DQS(n)        ((n & 0xF)<<8)
#define OSPI_MAP0_DM(n)         ((n & 0xF)<<12)
#define OSPI_MAP0_D0(n)         ((n & 0xF)<<16)
#define OSPI_MAP0_D1(n)         ((n & 0xF)<<20)
#define OSPI_MAP0_D2(n)         ((n & 0xF)<<24)
#define OSPI_MAP0_D3(n)         ((n & 0xF)<<28)

#define GPIO_A  (0x40020A00)
#define GPIO_B  (0x40020B00)
#define GPIO_C  (0x40020C00)
#define GPIO_D  (0x40020D00)
#define GPIO_E  (0x40020E00)

#define REG_GPIO_DIR_INPUT(port, n)      (*(volatile unsigned int*)(port+0x00)) &= ~(3<<(n<<1))
#define REG_GPIO_PDL_100(port, n)        (*(volatile unsigned int*)(port+0x18)) |= BIT(((n-0)<<2))
#define REG_GPIO_PDH_100(port, n)        (*(volatile unsigned int*)(port+0x1C)) |= BIT(((n-8)<<2))

    int ret = RET_OK;
    int switch_off = request & 0x80;
    int psram_type = request & 0x7F;

    if (HG_XSPI_DEVID != dev_id) {
        return EINVAL;
    }
    
    switch (psram_type) {
        case 0: //APS1604M_3SQR,
            if (!switch_off) {
                //这里不能随意修改,外置psram配置,并且只能开机调用一次(特殊处理)
                *((volatile uint32_t*)0x40020180) = 0x00004000; //IOFUNCMASK0
                *((volatile uint32_t*)0x40020b00) = 0x00001000;
                *((volatile uint32_t*)0x40020b10) = 0x02111111;
                *((volatile uint32_t*)0x40020b78) = 0x00010000;
                *((volatile uint32_t*)0x40020a00) = 0x2a820000;
                *((volatile uint32_t*)0x40020a14) = 0x02222112;
                *((volatile uint32_t*)0x40020a34) = 0x02222002;
            } else {
                REG_GPIO_DIR_INPUT(GPIO_A, 7);
                REG_GPIO_DIR_INPUT(GPIO_A, 8);
                REG_GPIO_DIR_INPUT(GPIO_A, 11);
                REG_GPIO_DIR_INPUT(GPIO_A, 12);
                REG_GPIO_DIR_INPUT(GPIO_A, 13);
                REG_GPIO_DIR_INPUT(GPIO_A, 14);
            }
            break;
        case 1: // APS1604M_DQRA,
            /* not support */
            break;
        case 2: //APS3208K_OKUD,
            if (!switch_off) {
                REG_GPIO_PDL_100(GPIO_E, 4);

                SYSCTRL_REG_OPT( 
                    SYSCTRL->OSPI_MAP_CTL0 = OSPI_MAP0_CLK(8) | OSPI_MAP0_DQS(4) | OSPI_MAP0_DM(15) | OSPI_MAP0_CS(10) |
                                             OSPI_MAP0_D0(14) | OSPI_MAP0_D1(13) | OSPI_MAP0_D2(12) | OSPI_MAP0_D3(11);
                    SYSCTRL->OSPI_MAP_CTL1 = 0xFFFF0000 | OSPI_MAP1_D4(9) | OSPI_MAP1_D5(6) | OSPI_MAP1_D6(7) | OSPI_MAP1_D7(5);
                );

                /**
                 * PE16_IE  [0]       PE16_OE  [1]      PE16_DRV  [2]    PE16_PD  [3]     PE16_PU  [4]      PE_OUT_DATA  [5]     PE_CLKn[6]
                 */
                //while(1) {SYSCTRL->PE16CON = (0x1<<1) | (0x1<<5); SYSCTRL->PE16CON = (0x1<<1) | (0x1<<1);}
                SYSCTRL_REG_OPT( SYSCTRL->PE16CON = (0x1<<1) | (0x1<<6); );// clkn
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<14)) | (1<<14); );
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<15)) | (1<<15); );
            } else {
                SYSCTRL_REG_OPT( 
                    SYSCTRL->OSPI_MAP_CTL1 = 0;
                ); 
            }
            break;
        case 3: //APS6408L_OBMx,
            if (!switch_off) {
                REG_GPIO_PDL_100(GPIO_E, 4);

                SYSCTRL->OSPI_MAP_CTL0 = OSPI_MAP0_CLK(9) | OSPI_MAP0_DQS(14) | OSPI_MAP0_DM(15) | OSPI_MAP0_CS(8) |
                                         OSPI_MAP0_D0(4) | OSPI_MAP0_D1(5) | OSPI_MAP0_D2(6) | OSPI_MAP0_D3(7);
                SYSCTRL->OSPI_MAP_CTL1 = 0xFFFF0000 | OSPI_MAP1_D4(10) | OSPI_MAP1_D5(11) | OSPI_MAP1_D6(12) | OSPI_MAP1_D7(13);
                /**
                 * PE16_IE  [0]       PE16_OE  [1]      PE16_DRV  [2]    PE16_PD  [3]     PE16_PU  [4]      PE_OUT_DATA  [5]     PE_CLKn[6]
                 */
                //SYSCTRL_REG_OPT( SYSCTRL->PE16CON = (0x1<<1) | (0x1<<5); );// rst  = 1
                SYSCTRL_REG_OPT( SYSCTRL->PE16CON = 0x40000101; );// rst  = 1
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<14)) | (1<<14); );
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<15)) | (1<<15); );
            } else {
                SYSCTRL_REG_OPT( 
                    SYSCTRL->OSPI_MAP_CTL1 = 0;
                ); 
            }
            break;
        case 4:
            if (!switch_off) {
                //PA0 CS function mask
                *((volatile uint32_t*)0x40020180) = 0x00004000; //IOFUNCMASK0 14bit
                *((volatile uint32_t*)0x40020a74) = 0x00000001; //IOFUNCOUTCON0 PA0   
                
                *((volatile uint32_t*)0x40020a00) = 0x01000001;
                *((volatile uint32_t*)0x40020a10) = 0x00000001;

                //PC8 PC10 PC14
                *((volatile uint32_t*)0x40020C00) = 0x20220000;
                *((volatile uint32_t*)0x40020C14) = 0x01000101;
                *((volatile uint32_t*)0x40020C34) = 0x01000101;

                //PE0 PE2
                *((volatile uint32_t*)0x40020E00) = 0x00000022;
                *((volatile uint32_t*)0x40020E10) = 0x00000101;
                *((volatile uint32_t*)0x40020E30) = 0x00000101;
            } else {
                REG_GPIO_DIR_INPUT(GPIO_C, 8);
                REG_GPIO_DIR_INPUT(GPIO_C, 10);
                REG_GPIO_DIR_INPUT(GPIO_C, 14);
                REG_GPIO_DIR_INPUT(GPIO_E, 0);
                REG_GPIO_DIR_INPUT(GPIO_E, 2);
                REG_GPIO_DIR_INPUT(GPIO_A, 0);
            }
            break;
        case 5://SCKW18X128800
            if (!switch_off) {
                REG_GPIO_PDL_100(GPIO_E, 6);
                SYSCTRL_REG_OPT(
                    SYSCTRL->OSPI_MAP_CTL0 = OSPI_MAP0_CLK(5) | OSPI_MAP0_DQS(6) | OSPI_MAP0_DM(6) | OSPI_MAP0_CS(4) |
                                             OSPI_MAP0_D0(14) | OSPI_MAP0_D1(13) | OSPI_MAP0_D2(12) | OSPI_MAP0_D3(11);
                    SYSCTRL->OSPI_MAP_CTL1 = 0xFFFF0000 | OSPI_MAP1_D4(10) | OSPI_MAP1_D5(9) | OSPI_MAP1_D6(8) | OSPI_MAP1_D7(7);
                );
                /**
                 * PE16_IE  [0]       PE16_OE  [1]      PE16_DRV  [2]    PE16_PD  [3]     PE16_PU  [4]      PE_OUT_DATA  [5]     PE_CLKn[6]
                 */
                //SYSCTRL_REG_OPT( SYSCTRL->PE16CON = (0x1<<1) | (0x1<<5); );// rst  = 1
                SYSCTRL_REG_OPT( SYSCTRL->PE16CON = 0x40000101; );// rst  = 1
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<14)) | (1<<14); );
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<15)) | (1<<15); );
            } else {
                SYSCTRL_REG_OPT( 
                    SYSCTRL->OSPI_MAP_CTL1 = 0;
                ); 
            }
            break;
        case 6://SCKM18X032800
            if (!switch_off) {
                REG_GPIO_PDL_100(GPIO_E, 4);
                SYSCTRL_REG_OPT(
                    SYSCTRL->OSPI_MAP_CTL0 = OSPI_MAP0_CLK(8) | OSPI_MAP0_DQS(4) | OSPI_MAP0_DM(4) | OSPI_MAP0_CS(10) |
                                             OSPI_MAP0_D0(14) | OSPI_MAP0_D1(13) | OSPI_MAP0_D2(12) | OSPI_MAP0_D3(11);
                    SYSCTRL->OSPI_MAP_CTL1 = 0x7FFF0000 | OSPI_MAP1_D4(9) | OSPI_MAP1_D5(6) | OSPI_MAP1_D6(7) | OSPI_MAP1_D7(5);
                );
                /**
                 * PE16_IE  [0]       PE16_OE  [1]      PE16_DRV  [2]    PE16_PD  [3]     PE16_PU  [4]      PE_OUT_DATA  [5]     PE_CLKn[6]
                 */
                //SYSCTRL_REG_OPT( SYSCTRL->PE16CON = (0x1<<1) | (0x1<<5); );// rst  = 1
                SYSCTRL_REG_OPT( SYSCTRL->PE16CON = 0x40000101; );// rst  = 1
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<14)) | (1<<14); );
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<15)) | (1<<15); );
            } else {
                SYSCTRL_REG_OPT( 
                    SYSCTRL->OSPI_MAP_CTL1 = 0;
                ); 
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int spi_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_SPI0_DEVID:
            if (request) {
                gpio_iomap_inout(PIN_SPI0_CS, GPIO_IOMAP_IN_SPI0_NSS_IN, GPIO_IOMAP_OUT_SPI0_NSS_OUT);
                gpio_iomap_inout(PIN_SPI0_CLK, GPIO_IOMAP_IN_SPI0_SCK_IN, GPIO_IOMAP_OUT_SPI0_SCK_OUT);
                gpio_iomap_inout(PIN_SPI0_IO0, GPIO_IOMAP_IN_SPI0_IO0_IN, GPIO_IOMAP_OUT_SPI0_IO0_OUT);
                gpio_iomap_inout(PIN_SPI0_IO1, GPIO_IOMAP_IN_SPI0_IO1_IN, GPIO_IOMAP_OUT_SPI0_IO1_OUT);
                gpio_iomap_inout(PIN_SPI0_IO2, GPIO_IOMAP_IN_SPI0_IO2_IN, GPIO_IOMAP_OUT_SPI0_IO2_OUT);
                gpio_iomap_inout(PIN_SPI0_IO3, GPIO_IOMAP_IN_SPI0_IO3_IN, GPIO_IOMAP_OUT_SPI0_IO3_OUT);
            } else {
                gpio_set_dir(PIN_SPI0_CS, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI0_CLK, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI0_IO0, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI0_IO1, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI0_IO2, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI0_IO3, GPIO_DIR_INPUT);
            }
            break;
        case HG_SPI1_DEVID:
            if (request) {
                gpio_iomap_inout(PIN_SPI1_CS, GPIO_IOMAP_IN_SPI1_NSS_IN__LCD_D11_IN_MASK1_30, GPIO_IOMAP_OUT_SPI1_NSS_OUT);
                gpio_iomap_inout(PIN_SPI1_CLK, GPIO_IOMAP_IN_SPI1_SCK_IN, GPIO_IOMAP_OUT_SPI1_SCK_OUT);
                gpio_iomap_inout(PIN_SPI1_IO0, GPIO_IOMAP_IN_SPI1_IO0_IN, GPIO_IOMAP_OUT_SPI1_IO0_OUT);
                gpio_iomap_inout(PIN_SPI1_IO1, GPIO_IOMAP_IN_SPI1_IO1_IN__LCD_D12_IN_MASK1_31, GPIO_IOMAP_OUT_SPI1_IO1_OUT);
                gpio_iomap_input(PIN_SPI1_IO2,GPIO_IOMAP_IN_SPI1_IO2_IN__IIS1_BCLK_IN);
                gpio_iomap_output(PIN_SPI1_IO2,GPIO_IOMAP_OUT_SPI1_IO2_OUT);
                gpio_iomap_input(PIN_SPI1_IO3,GPIO_IOMAP_IN_SPI1_IO3_IN__IIS1_DAT_IN);
                gpio_iomap_output(PIN_SPI1_IO3,GPIO_IOMAP_OUT_SPI1_IO3_OUT);
            } else {
                gpio_set_dir(PIN_SPI1_CS, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI1_CLK, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI1_IO0, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI1_IO1, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI1_IO2, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SPI1_IO3, GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int iis_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
//        case HG_IIS0_DEVID:
//            if (request) {
//                gpio_iomap_inout(PIN_IIS0_MCLK, GPIO_IOMAP_IN_IIS0_MCLK_IN, GPIO_IOMAP_OUT_IIS0_MCLK_OUT);
//                gpio_iomap_inout(PIN_IIS0_BCLK, GPIO_IOMAP_IN_IIS0_BCLK_IN, GPIO_IOMAP_OUT_IIS0_BCLK_OUT);
//                gpio_iomap_inout(PIN_IIS0_WCLK, GPIO_IOMAP_IN_IIS0_WSCLK_IN, GPIO_IOMAP_OUT_IIS0_WSCLK_OUT);
//                gpio_iomap_inout(PIN_IIS0_DATA, GPIO_IOMAP_IN_IIS0_DI, GPIO_IOMAP_OUT_IIS0_DO);
//            } else {
//                gpio_set_dir(PIN_IIS0_MCLK, GPIO_DIR_INPUT);
//                gpio_set_dir(PIN_IIS0_BCLK, GPIO_DIR_INPUT);
//                gpio_set_dir(PIN_IIS0_WCLK, GPIO_DIR_INPUT);
//                gpio_set_dir(PIN_IIS0_DATA, GPIO_DIR_INPUT);
//            }
//            break;
//        case HG_IIS1_DEVID:
//            if (request) {
//                gpio_iomap_inout(PIN_IIS1_MCLK, GPIO_IOMAP_IN_IIS1_MCLK_IN, GPIO_IOMAP_OUT_IIS1_MCLK_OUT);
//                gpio_iomap_inout(PIN_IIS1_BCLK, GPIO_IOMAP_IN_IIS1_BCLK_IN, GPIO_IOMAP_OUT_IIS1_BCLK_OUT);
//                gpio_iomap_inout(PIN_IIS1_WCLK, GPIO_IOMAP_IN_IIS1_WSCLK_IN, GPIO_IOMAP_OUT_IIS1_WSCLK_OUT);
//                gpio_iomap_inout(PIN_IIS1_DATA, GPIO_IOMAP_IN_IIS1_DI, GPIO_IOMAP_OUT_IIS1_DO);
//            } else {
//                gpio_set_dir(PIN_IIS1_MCLK, GPIO_DIR_INPUT);
//                gpio_set_dir(PIN_IIS1_BCLK, GPIO_DIR_INPUT);
//                gpio_set_dir(PIN_IIS1_WCLK, GPIO_DIR_INPUT);
//                gpio_set_dir(PIN_IIS1_DATA, GPIO_DIR_INPUT);
//            }
//            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int pdm_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
//        case HG_PDM_DEVID:
//            if (request) {
//                gpio_iomap_output(PIN_PDM_MCLK, GPIO_IOMAP_OUT_PDM_MCLK);
//                gpio_iomap_input(PIN_PDM_DATA,  GPIO_IOMAP_IN_PDM_DATA);
//            } else {
//                gpio_set_dir(PIN_PDM_MCLK, GPIO_DIR_INPUT);
//                gpio_set_dir(PIN_PDM_DATA, GPIO_DIR_INPUT);
//            }
//            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int iic_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {
        case HG_I2C1_DEVID:
            if (request) {
                gpio_iomap_inout(PIN_IIC1_SCL, GPIO_IOMAP_IN_SPI1_SCK_IN, GPIO_IOMAP_OUT_SPI1_SCK_OUT);
                gpio_iomap_inout(PIN_IIC1_SDA, GPIO_IOMAP_IN_SPI1_IO0_IN, GPIO_IOMAP_OUT_SPI1_IO0_OUT);
                gpio_set_mode(PIN_IIC1_SCL, GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
                gpio_set_mode(PIN_IIC1_SDA, GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
            } else {
                gpio_set_dir(PIN_IIC1_SCL, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_IIC1_SDA, GPIO_DIR_INPUT);
            }
            break;
        case HG_I2C2_DEVID:
            if (request) {
                gpio_iomap_inout(PIN_IIC2_SCL, GPIO_IOMAP_IN_SPI2_SCK_IN, GPIO_IOMAP_OUT_SPI2_SCK_OUT);
                gpio_iomap_inout(PIN_IIC2_SDA, GPIO_IOMAP_IN_SPI2_IO0_IN, GPIO_IOMAP_OUT_SPI2_IO0_OUT);
                gpio_driver_strength(PIN_IIC2_SCL, GPIO_DS_28MA);
                gpio_driver_strength(PIN_IIC2_SDA, GPIO_DS_28MA);           
                gpio_set_mode(PIN_IIC2_SCL, GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
                gpio_set_mode(PIN_IIC2_SDA, GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
            } else {
                gpio_set_dir(PIN_IIC2_SCL, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_IIC2_SDA, GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int adc_pin_func(int dev_id, int request) {

    #define ADC_FLAG_REQUEST BIT(26)
    #define ADC_FLAG_SUSPEND BIT(27)
    #define ADC_FLAG_RELEASE BIT(28)

    //struct gpio_device *ptr = NULL;
    uint32 flag = request & (0xFF << 24);
    uint32 adc_pin = (request&0xFFFFFF);

    switch (dev_id) {
        case (HG_ADC0_DEVID):   
            switch (flag) {
                /* release */
                case (ADC_FLAG_RELEASE):
                    gpio_ioctl(adc_pin, GPIO_GENERAL_ANALOG, 0, 0);
                    gpio_set_dir(adc_pin, GPIO_DIR_INPUT);
                    break;
                /* suspend */
                case (ADC_FLAG_SUSPEND):
                    gpio_ioctl(adc_pin, GPIO_GENERAL_ANALOG, 0, 0);
                    break;
                case (ADC_FLAG_REQUEST):
                    gpio_ioctl(adc_pin, GPIO_GENERAL_ANALOG, 1, 0);
                    break;
                default:
                    return RET_ERR;
                    break;
            }
            break;
    }

    return RET_OK;
}

static int timer_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_TIMER0_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(PIN_PWM_CHANNEL_0, GPIO_DIR_INPUT);
                    gpio_set_dir(PIN_CAPTURE_CHANNEL_0, GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(PIN_PWM_CHANNEL_0, GPIO_IOMAP_OUT_TMR0_PWM_OUT);
                    break;
                /* 2: capture */
                case (2):
                    gpio_iomap_input(PIN_CAPTURE_CHANNEL_0, GPIO_IOMAP_IN_TMR0_CAP_IN);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        case HG_TIMER1_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(PIN_PWM_CHANNEL_1, GPIO_DIR_INPUT);
                    gpio_set_dir(PIN_CAPTURE_CHANNEL_1, GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(PIN_PWM_CHANNEL_1, GPIO_IOMAP_OUT_TMR1_PWM_OUT);
                    break;
                /* 2: capture */
                case (2):
                    gpio_iomap_input(PIN_CAPTURE_CHANNEL_1, GPIO_IOMAP_IN_TMR1_CAP_IN);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        case HG_TIMER2_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(PIN_PWM_CHANNEL_2, GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(PIN_PWM_CHANNEL_2, GPIO_IOMAP_OUT_TMR2_PWM_OUT);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;          
        case HG_TIMER3_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(PIN_PWM_CHANNEL_3, GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(PIN_PWM_CHANNEL_3, GPIO_IOMAP_OUT_TMR3_PWM_OUT);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        case HG_LED_TIMER0_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(PIN_PWM_CHANNEL_2, GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(PIN_PWM_CHANNEL_2, GPIO_IOMAP_OUT_LED_TMR0_PWM_OUT);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        case HG_SIMTMR0_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(PIN_PWM_CHANNEL_4, GPIO_DIR_INPUT);
                    //gpio_set_dir(PIN_CAPTURE_CHANNEL_3, GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    SYSCTRL->IOFUNCMASK0 &= ~BIT(27);
                    gpio_iomap_output(PIN_PWM_CHANNEL_4, GPIO_IOMAP_OUT_STMR0_PWM_OUT);
                    break;
                /* 2: capture */
                case (2):
                    //gpio_iomap_input(PIN_CAPTURE_CHANNEL_3, GPIO_IOMAP_IN_STMR0_CAP_IN);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int dvp_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {   
        case HG_DVP_DEVID:
            gpio_set_altnt_func(PIN_DVP_HSYNC, 1);
            gpio_set_altnt_func(PIN_DVP_VSYNC, 1);
            gpio_set_altnt_func(PIN_DVP_PCLK , 1);
            gpio_set_altnt_func(PIN_DVP_MCLK , 1);
            gpio_set_altnt_func(PIN_DVP_DATA0, 1);
            gpio_set_altnt_func(PIN_DVP_DATA1, 1);
            gpio_set_altnt_func(PIN_DVP_DATA2, 1);
            gpio_set_altnt_func(PIN_DVP_DATA3, 1);
            gpio_set_altnt_func(PIN_DVP_DATA4, 1);
            gpio_set_altnt_func(PIN_DVP_DATA5, 1);
            gpio_set_altnt_func(PIN_DVP_DATA6, 1);
            gpio_set_altnt_func(PIN_DVP_DATA7, 1);
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int lcdc_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {
        case HG_LCDC_DEVID:
#if LCD_EN
            if(VS_CS != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(18);
                //ll_gpio_iomap_output_config(GPIOC, (8), 2);                   //vsync
                gpio_iomap_output(VS_CS, 5); 
                gpio_driver_strength(VS_CS, GPIO_DS_4MA);
            }


            if(HS_DC != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(17);
                //ll_gpio_iomap_output_confi g(GPIOC, (8), 2);                  //hsync
                gpio_iomap_output(HS_DC, 4); 
                gpio_driver_strength(HS_DC, GPIO_DS_4MA);
            }
        
            if(LCD_TE != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(10);
                gpio_iomap_input(LCD_TE,GPIO_IOMAP_IN_PORT_WKUP_IN3__LCD_TE_MASK0_10);

            }

            if(DE_ERD != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(16);
                gpio_iomap_output(DE_ERD, 3); 
                gpio_driver_strength(DE_ERD, GPIO_DS_4MA);  
            }
        

            //fix confict if lcd using FEM
            if(DOTCLK_RWR != 255){
                SYSCTRL->IOFUNCMASK2 |= BIT(18);
                gpio_iomap_output(DOTCLK_RWR, 56); 
                gpio_driver_strength(DOTCLK_RWR, GPIO_DS_4MA);  
            }



            if(LCD_D0 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(20);
                SYSCTRL->IOFUNCMASK1 |= BIT(19);
                //ll_gpio_iomap_output_config(GPIOC, (8), 2);                   //D0
                gpio_iomap_inout(LCD_D0, GPIO_IOMAP_IN_LCD_D0_IN_MASK1_19,2); 
                gpio_driver_strength(LCD_D0, GPIO_DS_4MA);
            }

            if(LCD_D1 != 255){
                //ll_gpio_iomap_output_config(GPIOC, (9), 8);                   //D1
                SYSCTRL->IOFUNCMASK1 |= BIT(20);
                gpio_iomap_inout(LCD_D1, GPIO_IOMAP_IN_LCD_D1_IN_MASK1_20,8);
                gpio_driver_strength(LCD_D1, GPIO_DS_4MA);
            }

            if(LCD_D2 != 255){
                
                //ll_gpio_iomap_output_config(GPIOC, (10), 9);              //D2
                SYSCTRL->IOFUNCMASK1 |= BIT(21);
                gpio_iomap_inout(LCD_D2, GPIO_IOMAP_IN_LCD_D2_IN_MASK1_21,9);
                gpio_driver_strength(LCD_D2, GPIO_DS_4MA);
            }

            if(LCD_D3 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(21);
                SYSCTRL->IOFUNCMASK1 |= BIT(22);
                //ll_gpio_iomap_output_config(GPIOC, (11), 10);             //D3
                gpio_iomap_inout(LCD_D3, GPIO_IOMAP_IN_STMR0_CAP_IN__LCD_D3_IN_MASK1_22,10);
                gpio_driver_strength(LCD_D3, GPIO_DS_4MA);
            }       

            if(LCD_D4 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(22);
                SYSCTRL->IOFUNCMASK1 |= BIT(23);
                //ll_gpio_iomap_output_config(GPIOC, (12), 11);             //D4
                gpio_iomap_inout(LCD_D4, GPIO_IOMAP_IN_STMR1_CAP_IN__LCD_D4_IN_MASK1_23,11);
                gpio_driver_strength(LCD_D4, GPIO_DS_4MA);
            }

            
            if(LCD_D5 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(23);
                SYSCTRL->IOFUNCMASK1 |= BIT(24);
                gpio_iomap_inout(LCD_D5, GPIO_IOMAP_IN_STMR2_CAP_IN__LCD_D5_IN_MASK1_24,12);                //D5
                gpio_driver_strength(LCD_D5, GPIO_DS_4MA);
            }

            if(LCD_D6 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(24);
                SYSCTRL->IOFUNCMASK1 |= BIT(25);
                gpio_iomap_inout(LCD_D6, GPIO_IOMAP_IN_STMR3_CAP_IN__LCD_D6_IN_MASK1_25,16);                //D6
                gpio_driver_strength(LCD_D6, GPIO_DS_4MA);
            }

            if(LCD_D7 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(25);
                SYSCTRL->IOFUNCMASK1 |= BIT(26);
                gpio_iomap_inout(LCD_D7, GPIO_IOMAP_IN_PORT_WKUP_IN1__LCD_D7_IN_MASK1_26,17);               //D7
                gpio_driver_strength(LCD_D7, GPIO_DS_4MA);
            }

            if(LCD_D8 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(26);
                gpio_iomap_output(LCD_D8, 18);              //D8
                gpio_driver_strength(LCD_D8, GPIO_DS_4MA);
            }   

            if(LCD_D9 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(27);
                gpio_iomap_output(LCD_D9, 19);              //D9
                gpio_driver_strength(LCD_D9, GPIO_DS_4MA);
            }

            if(LCD_D10 != 255){
                SYSCTRL->IOFUNCMASK0 |= BIT(28);
                gpio_iomap_output(LCD_D10, 20);             //D10
                gpio_driver_strength(LCD_D10, GPIO_DS_4MA);
            }

            if(LCD_D11 != 255){
                SYSCTRL->IOFUNCMASK1 |= BIT(8);
                //ll_gpio_iomap_output_config(GPIOA, (7), 21);              //D11
                gpio_iomap_output(LCD_D11, 21);
                gpio_driver_strength(LCD_D11, GPIO_DS_4MA);
            }

            if(LCD_D12 != 255){
                SYSCTRL->IOFUNCMASK1 |= BIT(9);
                //ll_gpio_iomap_output_config(GPIOA, (11), 26);             //D12
                gpio_iomap_output(LCD_D12, 26);
                gpio_driver_strength(LCD_D12, GPIO_DS_4MA);
            }

            if(LCD_D13 != 255){
                SYSCTRL->IOFUNCMASK1 |= BIT(10);
                //ll_gpio_iomap_output_config(GPIOA, (12), 28);             //D13
                gpio_iomap_output(LCD_D13, 28);
                gpio_driver_strength(LCD_D13, GPIO_DS_4MA);
            }

            if(LCD_D14 != 255){
                SYSCTRL->IOFUNCMASK1 |= BIT(11);
                //ll_gpio_iomap_output_config(GPIOA, (13), 29);             //D14
                gpio_iomap_output(LCD_D14, 29);
                gpio_driver_strength(LCD_D14, GPIO_DS_4MA);
            }

            if(LCD_D15 != 255){
                SYSCTRL->IOFUNCMASK1 |= BIT(12);
                //ll_gpio_iomap_output_config(GPIOA, (14), 30);             //D15
                gpio_iomap_output(LCD_D15, 30);
                gpio_driver_strength(LCD_D15, GPIO_DS_4MA);
            }

            if(LCD_D16 != 255){
                //ll_gpio_iomap_output_config(GPIOC, (9), 8);                   //D16
                gpio_iomap_output(LCD_D16, 43);
                gpio_driver_strength(LCD_D16, GPIO_DS_4MA);
            }

            if(LCD_D17 != 255){
                //ll_gpio_iomap_output_config(GPIOC, (9), 8);                   //D17
                gpio_iomap_output(LCD_D17, 44);
                gpio_driver_strength(LCD_D17, GPIO_DS_4MA); 
            }
#endif
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;

}


static int sdh_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_SDIOHOST_DEVID:
            if (request) {
                gpio_set_mode(PIN_SDH_CLK, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PIN_SDH_CMD, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PIN_SDH_DAT0, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PIN_SDH_DAT1, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                if (request == 4) {
                    gpio_set_mode(PIN_SDH_DAT2, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                    gpio_set_mode(PIN_SDH_DAT3, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                }




                gpio_iomap_output(PIN_SDH_CLK, GPIO_IOMAP_OUT_SDHOST_SCLK_O);
                gpio_iomap_inout(PIN_SDH_CMD, GPIO_IOMAP_IN_SDHOST_CMD_IN, GPIO_IOMAP_OUT_SDHOST_CMD_OUT);
                gpio_iomap_inout(PIN_SDH_DAT0, GPIO_IOMAP_IN_SDHOST_DAT0_IN, GPIO_IOMAP_OUT_SDHOST_DAT0_OUT);
                gpio_iomap_inout(PIN_SDH_DAT1, GPIO_IOMAP_IN_SDHOST_DAT1_IN, GPIO_IOMAP_OUT_SDHOST_DAT1_OUT);
                if (request == 4) {
                    gpio_iomap_inout(PIN_SDH_DAT2, GPIO_IOMAP_IN_SDHOST_DAT2_IN, GPIO_IOMAP_OUT_SDHOST_DAT2_OUT);
                    gpio_iomap_inout(PIN_SDH_DAT3, GPIO_IOMAP_IN_SDHOST_DAT3_IN, GPIO_IOMAP_OUT_SDHOST_DAT3_OUT);
                }
            } else {
                gpio_set_dir(PIN_SDH_CLK, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SDH_CMD, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SDH_DAT0, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SDH_DAT1, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SDH_DAT2, GPIO_DIR_INPUT);
                gpio_set_dir(PIN_SDH_DAT3, GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


int pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    sysctrl_unlock();

    switch (dev_id) {
        case HG_UART0_DEVID:
        case HG_UART1_DEVID:
        case HG_UART4_DEVID:
        case HG_UART5_DEVID:
            ret = uart_pin_func(dev_id, request);
            break;
        case HG_GMAC_DEVID:
            ret = gmac_pin_func(dev_id, request);
            break;
        case HG_SDIOSLAVE_DEVID:
            ret = sdio_pin_func(dev_id, request);
            break;
        case HG_SPI0_DEVID:
        case HG_SPI2_DEVID:
        case HG_SPI1_DEVID:
        case HG_SPI3_DEVID:
        case HG_SPI5_DEVID:
        case HG_SPI6_DEVID:
            ret = spi_pin_func(dev_id, request);
            break;
        case HG_I2C0_DEVID:
        case HG_I2C1_DEVID:
        case HG_I2C2_DEVID:
        case HG_I2C3_DEVID:
            ret = iic_pin_func(dev_id, request);
            break;
        case HG_DVP_DEVID:
            ret = dvp_pin_func(dev_id, request);
            break;
        case HG_QSPI_DEVID:
            ret = qspi_pin_func(dev_id, request);
            break;
        case HG_SDIOHOST_DEVID:
            ret = sdh_pin_func(dev_id, request);
            break;
        case HG_IIS0_DEVID:
        case HG_IIS1_DEVID:
            ret = iis_pin_func(dev_id, request);
            break;
        case HG_PDM0_DEVID:
            ret = pdm_pin_func(dev_id, request);
            break;
        case HG_TIMER0_DEVID:
        case HG_TIMER1_DEVID:
        case HG_TIMER2_DEVID:
        case HG_TIMER3_DEVID:
        case HG_LED_TIMER0_DEVID:
        case HG_LED_TIMER1_DEVID:
        case HG_LED_TIMER2_DEVID:
        case HG_LED_TIMER3_DEVID:
        case HG_SIMTMR0_DEVID:
        case HG_SIMTMR1_DEVID:
        case HG_SIMTMR2_DEVID:
        case HG_SIMTMR3_DEVID:
        case HG_SIMTMR4_DEVID:
        case HG_SIMTMR5_DEVID:
            ret = timer_pin_func(dev_id, request);
            break;
        case HG_ADC0_DEVID:
        case HG_ADC1_DEVID:
            ret = adc_pin_func(dev_id, request);
            break;
        case HG_XSPI_DEVID:
            ret = xspi_pin_func(dev_id, request);
            break;
        case HG_LCDC_DEVID:
            ret = lcdc_pin_func(dev_id, request);
            break;
        default:
            break;
    }

    user_pin_func(dev_id, request);
    sysctrl_lock();
    return ret;
}

