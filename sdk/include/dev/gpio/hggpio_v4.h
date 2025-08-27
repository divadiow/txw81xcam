#ifndef _HUGEIC_GPIO_V4_H_
#define _HUGEIC_GPIO_V4_H_
#include "hal/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HGGPIO_V4_MAX_PINS            (16)

struct gpio_bk{
    __IO uint32_t MODE;
    __IO uint32_t OTYPE;
    __IO uint32_t OSPEEDL;
    __IO uint32_t OSPEEDH;
    __IO uint32_t PUPL;
    __IO uint32_t PUPH;
    __IO uint32_t PUDL;
    __IO uint32_t PUDH;
    __IO uint32_t ODAT;
    __IO uint32_t AFRL;
    __IO uint32_t AFRH;
    __IO uint32_t IMK;
    __IO uint32_t DEBEN;
    __IO uint32_t AIOEN;
    __IO uint32_t TRG0;
    __IO uint32_t IEEN;
    __IO uint32_t IOFUNCOUTCON0;
    __IO uint32_t IOFUNCOUTCON1;
    __IO uint32_t IOFUNCOUTCON2;
    __IO uint32_t IOFUNCOUTCON3;
};

struct hggpio_v4 {
    struct gpio_device dev;
    uint32 hw;
	uint32 comm_irq_num;
    int32 pin_id[HGGPIO_V4_MAX_PINS];
    gpio_irq_hdl irq_hdl[HGGPIO_V4_MAX_PINS];
    uint32 pin_num[2];
    uint8 irq_num;
    uint32_t flag;
#ifdef CONFIG_SLEEP
    struct gpio_bk       bk;
#endif    
};


int32 hggpio_v4_attach(uint32 dev_id, struct hggpio_v4 *gpio);

#ifdef __cplusplus
}
#endif


#endif

