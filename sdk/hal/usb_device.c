#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/usb_device.h"

int32 usb_device_open(struct usb_device *p_usb_d, struct usb_device_cfg *p_usbdev_cfg)
{
    if (p_usb_d && ((const struct usb_hal_ops *)p_usb_d->dev.ops)->open) {
        return ((const struct usb_hal_ops *)p_usb_d->dev.ops)->open(p_usb_d, p_usbdev_cfg);
    }
    return RET_ERR;
}

int32 usb_device_close(struct usb_device *p_usb_d)
{
    if (p_usb_d && ((const struct usb_hal_ops *)p_usb_d->dev.ops)->close) {
        return ((const struct usb_hal_ops *)p_usb_d->dev.ops)->close(p_usb_d);
    }
    return RET_ERR;
}

int32 usb_device_write(struct usb_device *p_usb_d, uint8 ep, uint8 *buff, uint32 len, uint8 sync)
{
    if (p_usb_d && ((const struct usb_hal_ops *)p_usb_d->dev.ops)->write) {
        return ((const struct usb_hal_ops *)p_usb_d->dev.ops)->write(p_usb_d, ep, buff, len, sync);
    }
    return RET_ERR;
}
int32 usb_device_write_scatter(struct usb_device *p_usb_d, uint8 ep, scatter_data *data, uint32 count, uint8 sync)
{
    if (p_usb_d && ((const struct usb_hal_ops *)p_usb_d->dev.ops)->write_scatter) {
        return ((const struct usb_hal_ops *)p_usb_d->dev.ops)->write_scatter(p_usb_d, ep, data, count, sync);
    }
    return RET_ERR;
}

int32 usb_device_read(struct usb_device *p_usb_d, uint8 ep, uint8 *buff, uint32 len, uint8 sync)
{
    if (p_usb_d && ((const struct usb_hal_ops *)p_usb_d->dev.ops)->read) {
        return ((const struct usb_hal_ops *)p_usb_d->dev.ops)->read(p_usb_d, ep, buff, len, sync);
    }
    return RET_ERR;
}

int32 usb_device_ioctl(struct usb_device *p_usb_d, uint32 cmd, uint32 param1, uint32 param2)
{
    if (p_usb_d && ((const struct usb_hal_ops *)p_usb_d->dev.ops)->ioctl) {
        return ((const struct usb_hal_ops *)p_usb_d->dev.ops)->ioctl(p_usb_d, cmd, param1, param2);
    }
    return RET_ERR;
}

int32 usb_device_request_irq(struct usb_device *p_usb_d, usbdev_irq_hdl handle, uint32 data)
{
    if (p_usb_d && ((const struct usb_hal_ops *)p_usb_d->dev.ops)->request_irq) {
        return ((const struct usb_hal_ops *)p_usb_d->dev.ops)->request_irq(p_usb_d, handle, data);
    }
    return RET_ERR;
}

