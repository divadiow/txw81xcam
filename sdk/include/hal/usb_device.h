#ifndef _HAL_USB_DEVICE_H_
#define _HAL_USB_DEVICE_H_

#include "usb_ch9.h"

#ifdef __cplusplus
extern "C" {
#endif

enum usb_dev_irqs {
    USB_DEV_RESET_IRQ,
    USB_DEV_SUSPEND_IRQ,
    USB_DEV_RESUME_IRQ,
    USB_DEV_SOF_IRQ,
    USB_DEV_CTL_IRQ,
    USB_EP_RX_IRQ,
    USB_EP_TX_IRQ,
    USB_CONNECT,
    USB_DISCONNECT,
    USB_BABBLE,
    USB_XACT_ERR,
};

enum usb_dev_io_cmd {
    USB_DEV_IO_CMD_AUTO_TX_NULL_PKT_ENABLE,
    USB_DEV_IO_CMD_AUTO_TX_NULL_PKT_DISABLE,
    USB_DEV_IO_CMD_REMOTE_WAKEUP,

    /* this function need call after attatch & before open
     * msg[1:0] : vid 
     * msg[3:2] : pid 
     */
    USB_DEV_IO_CMD_SET_ID,
    USB_HOST_HY_TUNE_EN,
    USB_HOST_SET_HY_TUNE,
    USB_HOST_SET_TX_VERF,
    USB_HOST_SET_TX_SLIVER,
    USB_HOST_GET_TX_SLIVER,
    USB_HOST_SET_SLIVER_TOG_EN,
    USB_HOST_SET_RX_SLIVER,
    USB_HOST_GET_RX_SLIVER,
};


struct usb_device_ep_cfg {
    uint8                   ep_id;
    uint8                   ep_type;
    uint8                   ep_dir_tx;
    uint16                  max_packet_size_hs;
    uint16                  max_packet_size_fs;
};

struct usb_device_cfg {
    uint16                      vid;                        /* usb device VID */
    uint16                      pid;                        /* usb device PID */
    uint8                       speed;                      /* not used */
    uint8                      *p_device_descriptor;        /* usb device descriptor */
    uint8                      *p_config_desc_hs;           /* usb config descriptor */
    uint8                      *p_config_desc_fs;           /* usb config descriptor */
    uint16                      config_desc_len;            /* usb config descriptor total len */
    uint8                       interface_num;

    uint8                       ep_nums;
    struct usb_device_ep_cfg    ep_cfg[8];      /* must same with config_desc */
};

typedef uint32 (*usbdev_irq_hdl)(uint32 irq, uint32 param1, uint32 param2, uint32 param3);

struct usb_device {
    struct dev_obj dev;
};

struct usb_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct usb_device *p_usb_d, struct usb_device_cfg *p_usbdev_cfg);
    int32(*close)(struct usb_device *p_usb_d);
    int32(*read)(struct usb_device *p_usb_d, uint8 ep, uint8 *buff, uint32 len, uint8 sync);
    int32(*write)(struct usb_device *p_usb_d, uint8 ep, uint8 *buff, uint32 len, uint8 sync);
    int32(*write_scatter)(struct usb_device *p_usb_d, uint8 ep, scatter_data *data, uint32 count, uint8 sync);
    int32(*ioctl)(struct usb_device *p_usb_d, uint32 cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct usb_device *p_usb_d, usbdev_irq_hdl irqhdl, uint32 data);
};

int32 usb_device_open(struct usb_device *p_usb_d, struct usb_device_cfg * p_usbdev_cfg);

int32 usb_device_close(struct usb_device *p_usb_d);

int32 usb_device_write(struct usb_device *p_usb_d, uint8 ep, uint8 *buff, uint32 len, uint8 sync);

int32 usb_device_write_scatter(struct usb_device *p_usb_d, uint8 ep, scatter_data *data, uint32 count, uint8 sync);

int32 usb_device_read(struct usb_device *p_usb_d, uint8 ep, uint8 *buff, uint32 len, uint8 sync);

int32 usb_device_ioctl(struct usb_device *p_usb_d, uint32 cmd, uint32 param1, uint32 param2);

int32 usb_device_request_irq(struct usb_device *p_usb_d, usbdev_irq_hdl handle, uint32 data);


#ifdef __cplusplus
}
#endif
#endif

