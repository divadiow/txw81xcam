#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/audac.h"


int32 audac_open(struct audac_device *audac, enum audac_sample_rate sample_rate) {

    if(audac) {
        return ((const struct audac_hal_ops *)audac->dev.ops)->open(audac, sample_rate);
    }

    return RET_ERR;
}

int32 audac_close(struct audac_device *audac) {

    if(audac) {
        return ((const struct audac_hal_ops *)audac->dev.ops)->close(audac);
    }

    return RET_ERR;
}

int32 audac_write(struct audac_device *audac, void* buf, uint32 bytes) {

    if(audac) {
        return ((const struct audac_hal_ops *)audac->dev.ops)->write(audac, buf, bytes);
    }

    return RET_ERR;
}

int32 audac_ioctl(struct audac_device *audac, enum audac_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2) {

    if(audac) {
        return ((const struct audac_hal_ops *)audac->dev.ops)->ioctl(audac, ioctl_cmd, param1, param2);
    }

    return RET_ERR;
}

int32 audac_request_irq(struct audac_device *audac, enum audac_irq_flag irq_flag, audac_irq_hdl irq_hdl, uint32 irq_data) {

    if(audac) {
        return ((const struct audac_hal_ops *)audac->dev.ops)->request_irq(audac, irq_flag, irq_hdl, irq_data);
    }

    return RET_ERR;
}

int32 audac_release_irq(struct audac_device *audac, enum audac_irq_flag irq_flag) {

    if(audac) {
        return ((const struct audac_hal_ops *)audac->dev.ops)->release_irq(audac, irq_flag);
    }

    return RET_ERR;
}

