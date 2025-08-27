/*****************************************************************************
* Module    : usb
* File      : usb_dev_cdc_ncm.h
* Author    : 
* Function  : USB CDC NCM驱动的一些定义
*****************************************************************************/
#ifndef USB_DEV_CDC_NCM_H
#define USB_DEV_CDC_NCM_H

#ifdef __cplusplus
 extern "C" {
#endif

/*
 * CDC NCM transfer headers, CDC NCM subclass 3.2
 */

#define USB_CDC_NCM_NTH16_SIGN      0x484D434E /* NCMH */
#define USB_CDC_NCM_NTH32_SIGN      0x686D636E /* ncmh */

struct usb_cdc_ncm_nth16 {
    uint32 sign;
    uint16 hdr_len;
    uint16 seq;
    uint16 total_len;
    uint16 ndp_index;
} __packed;

struct usb_cdc_ncm_nth32 {
    uint32 sign;
    uint16 hdr_len;
    uint16 seq;
    uint32 total_len;
    uint32 ndp_index;
} __packed;

/*
 * CDC NCM datagram pointers, CDC NCM subclass 3.3
 */

#define USB_CDC_NCM_NDP16_CRC_SIGN      0x314D434E /* NCM1 */
#define USB_CDC_NCM_NDP16_NOCRC_SIGN    0x304D434E /* NCM0 */
#define USB_CDC_NCM_NDP32_CRC_SIGN      0x316D636E /* ncm1 */
#define USB_CDC_NCM_NDP32_NOCRC_SIGN    0x306D636E /* ncm0 */

/* 16-bit NCM Datagram Pointer Entry */
struct usb_cdc_ncm_dpe16 {
    uint16 datagram_index;
    uint16 datagram_len;
} __packed;

/* 16-bit NCM Datagram Pointer Table */
struct usb_cdc_ncm_ndp16 {
    uint32 sign;
    uint16 hdr_len;
    uint16 next_ndp_index;
    struct usb_cdc_ncm_dpe16 dpe16[0];
} __packed;

/* 32-bit NCM Datagram Pointer Entry */
struct usb_cdc_ncm_dpe32 {
    uint32 datagram_index;
    uint32 datagram_len;
} __packed;

/* 32-bit NCM Datagram Pointer Table */
struct usb_cdc_ncm_ndp32 {
    uint32 sign;
    uint16 hdr_len;
    uint16 resv_0;
    uint32 next_ndp_index;
    uint32 resv_1;
    struct usb_cdc_ncm_dpe32 dpe32[0];
} __packed;

/* CDC NCM subclass 3.2.1 and 3.2.2 */
#define USB_CDC_NCM_NDP16_INDEX_MIN         0x000C
#define USB_CDC_NCM_NDP32_INDEX_MIN         0x0010

/* CDC NCM subclass 3.3.3 Datagram Formatting */
#define USB_CDC_NCM_DATAGRAM_FORMAT_CRC         0x30
#define USB_CDC_NCM_DATAGRAM_FORMAT_NOCRC       0X31

struct usb_cdc_ncm_ntb_params {
    uint16 Length;
    uint16 NtbFormatsSupported;
    uint32 NtbInMaxSize;
    uint16 NdpInDivisor;
    uint16 NdpInPayloadRemainder;
    uint16 NdpInAlignment;
    uint16 Reserved;
    uint32 NtbOutMaxSize;
    uint16 NdpOutDivisor;
    uint16 NdpOutPayloadRemainder;
    uint16 NdpOutAlignment;
    uint16 NtbOutMaxDatagrams;
} __packed;

extern int32 usb_device_cdc_ncm_open(struct usb_device *p_usb_d);
extern int32 usb_device_cdc_ncm_close(struct usb_device *p_usb_d);
extern int32 usb_device_cdc_ncm_auto_tx_null_pkt_disable(struct usb_device *p_usb_d);
extern int32 usb_device_cdc_ncm_auto_tx_null_pkt_enable(struct usb_device *p_usb_d);
extern int32 usb_device_cdc_ncm_write(struct usb_device *p_usb_d, uint8 *buff, uint32 len);
extern int32 usb_device_cdc_ncm_write_scatter(struct usb_device *p_usb_d, scatter_data *data, int count);
extern int32 usb_device_cdc_ncm_read(struct usb_device *p_usb_d, uint8 *buff, uint32 len);
extern int32 usb_device_cdc_ncm_notify_network_connection(struct usb_device *p_usb_d, uint8 link_up);
extern int32 usb_device_cdc_ncm_notify_connection_speed_change(struct usb_device *p_usb_d, uint32 down_speed, uint32 up_speed);


#ifdef __cplusplus
}
#endif


#endif
