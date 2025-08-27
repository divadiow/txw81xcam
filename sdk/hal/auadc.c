#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/auadc.h"


int32 auadc_open(struct auadc_device *auadc, enum auadc_sample_rate sample_rate) {

    if(auadc) {
        return ((const struct auadc_hal_ops *)auadc->dev.ops)->open(auadc, sample_rate);
    }

    return RET_ERR;
}

int32 auadc_close(struct auadc_device *auadc) {

    if(auadc) {
        return ((const struct auadc_hal_ops *)auadc->dev.ops)->close(auadc);
    }

    return RET_ERR;
}

int32 auadc_read(struct auadc_device *auadc, void* buf, uint32 bytes) {

    if(auadc) {
        return ((const struct auadc_hal_ops *)auadc->dev.ops)->read(auadc, buf, bytes);
    }

    return RET_ERR;
}

int32 auadc_ioctl(struct auadc_device *auadc, enum auadc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2) {

    if(auadc) {
        return ((const struct auadc_hal_ops *)auadc->dev.ops)->ioctl(auadc, ioctl_cmd, param1, param2);
    }

    return RET_ERR;
}

int32 auadc_request_irq(struct auadc_device *auadc, enum auadc_irq_flag irq_flag, auadc_irq_hdl irq_hdl, uint32 irq_data) {

    if(auadc) {
        return ((const struct auadc_hal_ops *)auadc->dev.ops)->request_irq(auadc, irq_flag, irq_hdl, irq_data);
    }

    return RET_ERR;
}

int32 auadc_release_irq(struct auadc_device *auadc, enum auadc_irq_flag irq_flag) {

    if(auadc) {
        return ((const struct auadc_hal_ops *)auadc->dev.ops)->release_irq(auadc, irq_flag);
    }

    return RET_ERR;
}

