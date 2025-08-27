/*****************************************************************************
* Module    : usb
* File      : usb_dev_wifi.h
* Author    : 
* Function  : USB WIFI驱动的一些定义
*****************************************************************************/
#ifndef USB_DEV_AUDIO_H
#define USB_DEV_AUDIO_H

#ifdef __cplusplus
 extern "C" {
#endif

extern const struct usb_device_cfg usb_dev_audio_cfg;

extern int32 usb_device_audio_auto_tx_null_pkt_disable(struct usb_device *p_usb_d);
extern int32 usb_device_audio_auto_tx_null_pkt_enable(struct usb_device *p_usb_d);
extern int32 usb_device_audio_write(struct usb_device *p_usb_d, int8 *buff, uint32 len);
extern int32 usb_device_audio_read(struct usb_device *p_usb_d, int8 *buff, uint32 len);



#ifdef __cplusplus
}
#endif


#endif
