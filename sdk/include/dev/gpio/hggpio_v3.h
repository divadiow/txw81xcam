#ifndef _HUGEIC_GPIO_V3_H_
#define _HUGEIC_GPIO_V3_H_
#include "hal/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HGGPIO_V3_MAX_PINS            (16)

struct hggpio_v3 {
    struct gpio_device dev;
    uint32 hw;
    int32 pin_id[HGGPIO_V3_MAX_PINS];
    gpio_irq_hdl irq_hdl[HGGPIO_V3_MAX_PINS];
    uint32 pin_num[2];
    uint8 irq_num;
};

int32 hggpio_v3_attach(uint32 dev_id, struct hggpio_v3 *gpio);

#ifdef __cplusplus
}
#endif


#endif

