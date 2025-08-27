
/******************************************************************************
 * @file     isr.c
 * @brief    source file for the interrupt server route
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include "sys_config.h"
//#include <drv_common.h>
#include <csi_config.h>

#include "soc.h"
#ifndef CONFIG_KERNEL_NONE
#include <csi_kernel.h>
#endif
#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "osal/string.h"

extern void ck_usart_irqhandler(int32_t idx);
extern void dw_timer_irqhandler(int32_t idx);
extern void dw_gpio_irqhandler(int32_t idx);
extern void systick_handler(void);
extern void xPortSysTickHandler(void);
extern void OSTimeTick(void);
static struct sys_hwirq sys_irqs[IRQ_NUM];

#if defined(CONFIG_SUPPORT_TSPEND) || defined(CONFIG_KERNEL_NONE)
#define  ATTRIBUTE_ISR __attribute__((isr))
#else
#define  ATTRIBUTE_ISR
#endif

#define readl(addr) ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#if defined(CONFIG_KERNEL_RHINO)
#define SYSTICK_HANDLER systick_handler
#elif defined(CONFIG_KERNEL_FREERTOS)
#define SYSTICK_HANDLER xPortSysTickHandler
#elif defined(CONFIG_KERNEL_UCOS)
#define SYSTICK_HANDLER OSTimeTick
#endif

#if !defined(CONFIG_KERNEL_FREERTOS) && !defined(CONFIG_KERNEL_NONE)
#define  CSI_INTRPT_ENTER() csi_kernel_intrpt_enter()
#define  CSI_INTRPT_EXIT()  csi_kernel_intrpt_exit()
#else
#define  CSI_INTRPT_ENTER()
#define  CSI_INTRPT_EXIT()
#endif

#ifdef SYS_IRQ_STAT
#define SYS_IRQ_STATE_ST(irqn)  uint32 _t1_, _t2_; _t1_ = csi_coret_get_value();
#define SYS_IRQ_STATE_END(irqn) do{ \
                _t2_ = csi_coret_get_value();\
                _t1_ = ((_t1_>_t2_)?(_t1_-_t2_):(csi_coret_get_load()-_t2_+_t1_));\
                sys_irqs[irqn].tot_cycle += _t1_;\
                sys_irqs[irqn].trig_cnt++;\
                if (_t1_ > sys_irqs[irqn].max_cycle) {\
                    sys_irqs[irqn].max_cycle = _t1_;\
                }\
            }while(0)
#else
#define SYS_IRQ_STATE_ST(irqn)
#define SYS_IRQ_STATE_END(irqn)
#endif

#define SYSTEM_IRQ_HANDLE_FUNC(func_name,irqn)\
    ATTRIBUTE_ISR void func_name(void)\
    {\
        SYS_IRQ_STATE_ST(irqn);\
        CSI_INTRPT_ENTER();\
        if (sys_irqs[irqn].handle) {\
            sys_irqs[irqn].handle(sys_irqs[irqn].data);\
        }\
        CSI_INTRPT_EXIT();\
        SYS_IRQ_STATE_END(irqn);\
    }


int32 request_irq(uint32 irq_num, irq_handle handle, void *data)
{
    if (irq_num < IRQ_NUM) {
        sys_irqs[irq_num].data = data;
        sys_irqs[irq_num].handle = handle;
        return RET_OK;
    } else {
        return -EINVAL;
    }
}

int32 release_irq(uint32 irq_num)
{
    if (irq_num < IRQ_NUM) {
        irq_disable(irq_num);
        sys_irqs[irq_num].handle = NULL;
        sys_irqs[irq_num].data   = NULL;
        return RET_OK;
    } else {
        return -EINVAL;
    }
}

uint32 irq_status(void)
{
    uint32 tot_time = 0;
#ifdef SYS_IRQ_STAT
    int i;
    uint32_t v1, v2, v3;
    os_printf("SYS IRQ:\r\n");
    for (i = 0; i < IRQ_NUM; i++) {
        if (sys_irqs[i].trig_cnt) {
            v1 = sys_irqs[i].trig_cnt;
            v2 = sys_irqs[i].tot_cycle/(sys_get_sysclk()/1000000);//us
            v3 = sys_irqs[i].max_cycle/(sys_get_sysclk()/1000000);//us
            tot_time += v2; //us
            sys_irqs[i].trig_cnt = 0;
            sys_irqs[i].tot_cycle = 0;
            sys_irqs[i].max_cycle = 0;
            os_printf("  IRQ%-2d: trig:%-8d tot_time:%-8dus max:%-8dus\r\n", i, v1, v2, v3);
        }
    }
#endif
    return tot_time;
}

ATTRIBUTE_ISR void CORET_IRQHandler(void)
{
    SYS_IRQ_STATE_ST(CORET_IRQn);
    CSI_INTRPT_ENTER();
    readl(0xE000E010);
    SYSTICK_HANDLER();
    CSI_INTRPT_EXIT();
    SYS_IRQ_STATE_END(CORET_IRQn);
}


#define AES_INTERUPT_ON_OFF  ((*(unsigned int *)0x40012040) & BIT(3))
#define AES_INETRUPT_PENDING ((*(unsigned int *)0x40012044))
#define SHA_INTERUPT_ON_OFF  (((*(unsigned int *)0x40014008) & BIT(0)))
#define SHA_INTERUPT_PENDING ((*(unsigned int *)0x4001400C))
ATTRIBUTE_ISR void SYSAES_IRQHandler(void)
{
        SYS_IRQ_STATE_ST(SHA_VIRTUAL_IRQn);
        CSI_INTRPT_ENTER();
        if (sys_irqs[SPACC_PKA_IRQn].handle && AES_INTERUPT_ON_OFF && AES_INETRUPT_PENDING ) {
            sys_irqs[SPACC_PKA_IRQn].handle(sys_irqs[SPACC_PKA_IRQn].data);
        }
        if (sys_irqs[SHA_VIRTUAL_IRQn].handle && SHA_INTERUPT_ON_OFF && SHA_INTERUPT_PENDING ) {
            sys_irqs[SHA_VIRTUAL_IRQn].handle(sys_irqs[SHA_VIRTUAL_IRQn].data);
        }
        CSI_INTRPT_EXIT();
        SYS_IRQ_STATE_END(SHA_VIRTUAL_IRQn);
}


#define UART4_IE_ENABLE ((*(unsigned int *)0x40004b70) & 0x80000383)
#define UART5_IE_ENABLE ((*(unsigned int *)0x40004b90) & 0x80000383)
ATTRIBUTE_ISR void UART45_IRQHandler(void)
{
        CSI_INTRPT_ENTER();
        if (sys_irqs[UART4_IRQn].handle && UART4_IE_ENABLE ) {
            sys_irqs[UART4_IRQn].handle(sys_irqs[UART4_IRQn].data);
        }
        if (sys_irqs[UART5_IRQn].handle && UART5_IE_ENABLE ) {
            sys_irqs[UART5_IRQn].handle(sys_irqs[UART5_IRQn].data);
        }
        CSI_INTRPT_EXIT();
}

#define GPIOA_IE_ENABLE ((*(unsigned int *)0x40020A3C) & 0x0000FFFF)
#define GPIOB_IE_ENABLE ((*(unsigned int *)0x40020B3C) & 0x0000FFFF)
#define GPIOC_IE_ENABLE ((*(unsigned int *)0x40020C3C) & 0x0000FFFF)
#define GPIOE_IE_ENABLE ((*(unsigned int *)0x40020E3C) & 0x0000FFFF)
ATTRIBUTE_ISR void GPIO_IRQHandler(void)
{
        CSI_INTRPT_ENTER();
        if (sys_irqs[GPIOA_IRQn].handle && GPIOA_IE_ENABLE ) {
            sys_irqs[GPIOA_IRQn].handle(sys_irqs[GPIOA_IRQn].data);
        }

		if (sys_irqs[GPIOB_IRQn].handle && GPIOB_IE_ENABLE ) {
            sys_irqs[GPIOB_IRQn].handle(sys_irqs[GPIOB_IRQn].data);
        }

		if (sys_irqs[GPIOC_IRQn].handle && GPIOC_IE_ENABLE ) {
            sys_irqs[GPIOC_IRQn].handle(sys_irqs[GPIOC_IRQn].data);
        }

		if (sys_irqs[GPIOE_IRQn].handle && GPIOE_IE_ENABLE ) {
            sys_irqs[GPIOE_IRQn].handle(sys_irqs[GPIOE_IRQn].data);
        }
        CSI_INTRPT_EXIT();
}

#define AUDIO_VAD_IRQ_ENABLE  ((*(unsigned int *)0x40008028) & 0x00001FC0)
#define AUDIO_HS_IRQ_ENABLE   ((*(unsigned int *)0x40008028) & 0x00018000)
#define AUDIO_ALAW_IRQ_ENABLE ((*(unsigned int *)0x40008028) & 0x00006000)
ATTRIBUTE_ISR void AUDIO_VAD_HS_ALAW_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
    if (sys_irqs[AUVAD_IRQn].handle && AUDIO_VAD_IRQ_ENABLE ) {
        sys_irqs[AUVAD_IRQn].handle(sys_irqs[AUVAD_IRQn].data);
    }

    if (sys_irqs[AUHS_IRQn].handle && AUDIO_HS_IRQ_ENABLE ) {
        sys_irqs[AUHS_IRQn].handle(sys_irqs[AUHS_IRQn].data);
    }

    if (sys_irqs[AUALAW_IRQn].handle && AUDIO_ALAW_IRQ_ENABLE ) {
        sys_irqs[AUALAW_IRQn].handle(sys_irqs[AUALAW_IRQn].data);
    }
    CSI_INTRPT_EXIT();
}


//0
SYSTEM_IRQ_HANDLE_FUNC(USB20DMA_IRQHandler, USB20DMA_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(USB20MC_IRQHandler, USB20MC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(UART0_IRQHandler, UART0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(UART1_IRQHandler, UART1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LCD_IRQHandler, LCD_IRQn)

//5
SYSTEM_IRQ_HANDLE_FUNC(QSPI_IRQHandler, QSPI_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SPI0_IRQHandler, SPI0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SPI1_IRQHandler, SPI1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SPI2_IRQHandler, SPI2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(OSPI_IRQHandler, OSPI_IRQn)

//10
SYSTEM_IRQ_HANDLE_FUNC(TIM0_IRQHandler, TIM0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(TIM1_IRQHandler, TIM1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(TIM2_IRQHandler, TIM2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(TIM3_IRQHandler, TIM3_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SCALE1_IRQHandler, SCALE1_IRQn)

//15
//SYSTEM_IRQ_HANDLE_FUNC(AUDIO_VAD_HS_ALAW_IRQHandler, AUDIO_SUBSYS0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(AUDIO_ADC_IRQHandler, AUDIO_SUBSYS1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(AUDIO_DAC_IRQHandler, AUDIO_SUBSYS2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SDIO_IRQHandler, SDIO_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SDIO_RST_IRQHandler, SDIO_RST_IRQn)

//20
SYSTEM_IRQ_HANDLE_FUNC(SDHOST_IRQHandler, SDHOST_IRQn)
__ram SYSTEM_IRQ_HANDLE_FUNC(LMAC_IRQHandler, LMAC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GMAC_IRQHandler, GMAC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(M2M0_IRQHandler, M2M0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(M2M1_IRQHandler, M2M1_IRQn)

//25
//SYSTEM_IRQ_HANDLE_FUNC(CORET_IRQHandler, CORET_IRQn)
//SYSTEM_IRQ_HANDLE_FUNC(SYSAES_IRQHandler, SPACC_PKA_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CRC_IRQHandler, CRC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(ADKEY_IRQHandler, ADKEY01_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(PD_TMR_IRQHandler, PD_TMR_IRQn)

//30
SYSTEM_IRQ_HANDLE_FUNC(WKPND_IRQHandler, WKPND_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(PDWKPND_IRQHandler, PDWKPND_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LVD_IRQHandler, LVD_IRQn)
//SYSTEM_IRQ_HANDLE_FUNC(WDT_IRQHandler, WDT_IRQn)
ATTRIBUTE_ISR void WDT_IRQHandler()
{
    //喂狗并清除中断pending
    *(volatile unsigned int *)(0x40015000 + 0x4) = 0xAAAA;
    if (sys_irqs[WDT_IRQn].handle ) {
        sys_irqs[WDT_IRQn].handle(sys_irqs[WDT_IRQn].data);
    }
}
SYSTEM_IRQ_HANDLE_FUNC(SYS_ERR_IRQHandler, SYS_ERR_IRQn)

//35
SYSTEM_IRQ_HANDLE_FUNC(IIS0_IRQHandler, IIS0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(IIS1_IRQHandler, IIS1_IRQn)
//SYSTEM_IRQ_HANDLE_FUNC(GPIO_IRQHandler, GPIOABCE_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(DVP_IRQHandler, DVP_IRQn)
//RES: default_handler

//40
SYSTEM_IRQ_HANDLE_FUNC(JPG_IRQHandler, MJPEG01_IRQn)
//RES: default_handler
SYSTEM_IRQ_HANDLE_FUNC(VPP_IRQHandler, VPP_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(PRC_IRQHandler, PRC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(STMR_IRQHandler, STMR012345_IRQn)

//45
SYSTEM_IRQ_HANDLE_FUNC(PDM_IRQHandler, PDM_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LED_TMR_IRQHandler, LED_TMR_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SCALE2_IRQHandler, SCALE2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GFSK_IRQHandler, GFSK_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CMP_IRQHandler, CMPOUT01_IRQn)

//50
//SYSTEM_IRQ_HANDLE_FUNC(UART45_IRQHandler, UART45_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SCALE3_IRQHandler, SCALE3_IRQn)
//RES: default_handler
//RES: default_handler
//RES: default_handler


//50
//RES: default_handler
//RES: default_handler
//RES: default_handler
SYSTEM_IRQ_HANDLE_FUNC(USB20PHY_RTC_IRQHandler, USB20PHY_RTC_IRQn)






