/*****************************************************************************
* Module    : usb
* File      : usb_dev_wifi.c
* Author    :
* Function  : USB wifi  huge-ic defined, refer to ch9.h
*****************************************************************************/
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "hal/usb_device.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/hgusb_dev_tbl.h"
#include "lib/usb/usb_device_wifi.h"

#ifndef RT_USING_USB_DEVICE

/* TX & RX EP is for device */
#define USB_WIFI_TX_EP                  1
#define USB_WIFI_RX_EP                  1
#define USB_WIFI_EP_MAX_PKT_SIZE_HS        512
#define USB_WIFI_EP_MAX_PKT_SIZE_FS        64

#define USB_WIFI_CFG_DESC_LEN           (sizeof(tbl_usb_wifi_config_all_descriptor_wifi_hs))
#define USB_WIFI_IF_NUMS                1

#ifndef USB_VID
#define USB_VID                         0xA012
#endif
#ifndef USB_PID
#define USB_PID                         0x0000
#endif


//设备描述符
const uint8_t tbl_usb_wifi_device_descriptor[18] = {
    18,                 // Num bytes of the descriptor
    1,                  // Device Descriptor type
    0x00, 0x02,         // Revision of USB Spec. (in BCD)
    0xFF,               // Class : user-defined
    0xFF,               // Sub Class : user-defined
    0xFF,               // Class specific protocol : user-defined
    0x40,               // Max packet size of Endpoint 0
    (USB_VID >> 0) & 0xff,
    (USB_VID >> 8) & 0xff,         // Vendor ID
    (USB_PID >> 0) & 0xff,
    (USB_PID >> 8) & 0xff,         // Product ID
    0x00, 0x02,         // Device Revision (in BCD)
    1,                  // Index of Manufacture string descriptor
    2,                  // Index of Product string descriptor
    0,                  // Index of Serial No. string descriptor
    1                   // Num Configurations, Must = 1
};

//配置描述符 自定义WIFI
const uint8_t tbl_usb_wifi_config_all_descriptor_wifi_fs[23 + 9] = {
    9,                  // Num bytes of this descriptor
    2,                  // Configuration descriptor type
    (sizeof(tbl_usb_wifi_config_all_descriptor_wifi_fs) & 0xFF), 
    ((sizeof(tbl_usb_wifi_config_all_descriptor_wifi_fs)>>8) & 0xFF),             // Total size of configuration
    USB_WIFI_IF_NUMS,   // Num Interface, 会根据最后实际接口的数量来配置
    1,                  // Configuration number
    0,                  // Index of Configuration string descriptor
    0x80,               // Configuration characteristics: BusPowerd
    200, //0x32,           // Max current, unit is 2mA

//Bulk Interface 9 + 7 + 7= 23
    9,                  // Num bytes of this descriptor
    4,                  // Interface descriptor type
    0,                  // Interface Number，暂时填0，会根据最后实际配置来依次递加
    0,                  // Alternate interface number
    2,                  // Num endpoints of this interface
    0xff,               // Interface Class: unknown
    0xff,               // Interface Sub Class: unknown
    0xff,               // Class specific protocol: unknown
    0,                  // Index of Interface string descriptor

    7,                  // Num bytes of this descriptor
    5,                  // Endpoint descriptor type
    USB_WIFI_RX_EP,            //BULKOUT_EP, // Endpoint number, bit7=0 shows OUT
    2,                  // Bulk endpoint
    (USB_WIFI_EP_MAX_PKT_SIZE_FS >> 0) & 0xff, // Maximum packet size
    (USB_WIFI_EP_MAX_PKT_SIZE_FS >> 8) & 0xff,
    0,                  // no use for bulk endpoint

    7,                  // Num bytes of this descriptor
    5,                  // Endpoint descriptor type
    USB_WIFI_TX_EP | 0x80,   // Endpoint number, bit7=1 shows IN
    2,                  // Bulk endpoint
    (USB_WIFI_EP_MAX_PKT_SIZE_FS >> 0) & 0xff, // Maximum packet size
    (USB_WIFI_EP_MAX_PKT_SIZE_FS >> 8) & 0xff,
    0,                  // No use for bulk endpoint
};

//配置描述符 自定义WIFI
const uint8_t tbl_usb_wifi_config_all_descriptor_wifi_hs[23 + 9] = {
    9,                  // Num bytes of this descriptor
    2,                  // Configuration descriptor type
    (USB_WIFI_CFG_DESC_LEN & 0xFF), 
    ((USB_WIFI_CFG_DESC_LEN>>8) & 0xFF),             // Total size of configuration
    USB_WIFI_IF_NUMS,   // Num Interface, 会根据最后实际接口的数量来配置
    1,                  // Configuration number
    0,                  // Index of Configuration string descriptor
    0x80,               // Configuration characteristics: BusPowerd
    200, //0x32,           // Max current, unit is 2mA

//Bulk Interface 9 + 7 + 7= 23
    9,                  // Num bytes of this descriptor
    4,                  // Interface descriptor type
    0,                  // Interface Number，暂时填0，会根据最后实际配置来依次递加
    0,                  // Alternate interface number
    2,                  // Num endpoints of this interface
    0xff,               // Interface Class: unknown
    0xff,               // Interface Sub Class: unknown
    0xff,               // Class specific protocol: unknown
    0,                  // Index of Interface string descriptor

    7,                  // Num bytes of this descriptor
    5,                  // Endpoint descriptor type
    USB_WIFI_RX_EP,            //BULKOUT_EP, // Endpoint number, bit7=0 shows OUT
    2,                  // Bulk endpoint
    (USB_WIFI_EP_MAX_PKT_SIZE_HS >> 0) & 0xff, // Maximum packet size
    (USB_WIFI_EP_MAX_PKT_SIZE_HS >> 8) & 0xff,
    0,                  // no use for bulk endpoint

    7,                  // Num bytes of this descriptor
    5,                  // Endpoint descriptor type
    USB_WIFI_TX_EP | 0x80,   // Endpoint number, bit7=1 shows IN
    2,                  // Bulk endpoint
    (USB_WIFI_EP_MAX_PKT_SIZE_HS >> 0) & 0xff, // Maximum packet size
    (USB_WIFI_EP_MAX_PKT_SIZE_HS >> 8) & 0xff,
    0,                  // No use for bulk endpoint
};

//配置描述符 通用配置
const uint8_t tbl_usb_wifi_config_all_descriptor_gen[9] = {
    9,                  // Num bytes of this descriptor
    2,                  // Configuration descriptor type
    (USB_WIFI_CFG_DESC_LEN & 0xFF), 
    ((USB_WIFI_CFG_DESC_LEN>>8) & 0xFF),             // Total size of configuration
    USB_WIFI_IF_NUMS,   // Num Interface, 会根据最后实际接口的数量来配置
    1,                  // Configuration number
    0,                  // Index of Configuration string descriptor
    0x80,               // Configuration characteristics: BusPowerd
    200, //0x32,           // Max current, unit is 2mA
};


//语言
const uint8_t tbl_usb_wifi_language_id[4] = {
    4,              // Num bytes of this descriptor
    3,              // String descriptor
    0x09, 0x04,     // Language ID
};

//厂商信息
const uint8_t tbl_usb_wifi_str_manufacturer[16] = {
    16,             // Num bytes of this descriptor
    3,              // String descriptor
    'G',    0,
    'e',    0,
    'n',    0,
    'e',    0,
    'r',    0,
    'i',    0,
    'c',    0
};

//产品信息
const uint8_t tbl_usb_wifi_str_product[28] = {
    28,             // Num bytes of this descriptor
    3,              // String descriptor
    'U',    0,
    'S',    0,
    'B',    0,
    '2',    0,
    '.',    0,
    '0',    0,
    ' ',    0,
    'D',    0,
    'e',    0,
    'v',    0,
    'i',    0,
    'c',    0,
    'e',    0
};

//序列号
const uint8_t tbl_usb_wifi_str_serial_number[30] = {
    30,         // Num bytes of this descriptor
    3,          // String descriptor
    '2',    0,
    '0',    0,
    '1',    0,
    '7',    0,
    '0',    0,
    '8',    0,
    '2',    0,
    '9',    0,
    '0',    0,
    '0',    0,
    '0',    0,
    '0',    0,
    '0',    0,
    '1',    0
};

static const struct usb_device_cfg usb_dev_wifi_cfg = {
    .vid        = USB_VID,
    .pid        = USB_PID,
    .speed      = USB_SPEED_HIGH,
    .p_device_descriptor = (uint8 *)tbl_usb_wifi_device_descriptor,

    .p_config_desc_hs = (uint8 *)tbl_usb_wifi_config_all_descriptor_wifi_hs,
    .p_config_desc_fs = (uint8 *)tbl_usb_wifi_config_all_descriptor_wifi_fs,
    .config_desc_len = sizeof(tbl_usb_wifi_config_all_descriptor_wifi_hs),
    .interface_num  =   1,

    .ep_nums              =  2,
    .ep_cfg[0].ep_id      =  USB_WIFI_RX_EP,
    .ep_cfg[0].ep_type    =  USB_ENDPOINT_XFER_BULK,
    .ep_cfg[0].ep_dir_tx  =  0,
    .ep_cfg[0].max_packet_size_hs      =  USB_WIFI_EP_MAX_PKT_SIZE_HS,
    .ep_cfg[0].max_packet_size_fs      =  USB_WIFI_EP_MAX_PKT_SIZE_FS,
    .ep_cfg[1].ep_id      =  USB_WIFI_TX_EP,
    .ep_cfg[1].ep_type    =  USB_ENDPOINT_XFER_BULK,
    .ep_cfg[1].ep_dir_tx  =  1,
    .ep_cfg[1].max_packet_size_hs      =  USB_WIFI_EP_MAX_PKT_SIZE_HS,
    .ep_cfg[1].max_packet_size_fs      =  USB_WIFI_EP_MAX_PKT_SIZE_FS,
};

//OS2.0 descriptor set index
bool usb_device_ep0_get_wcid_descriptor(struct hgusb20_dev *p_dev, uint8_t index, uint8_t len)
{
    uint8_t descriptor_len = 0;
    uint8_t *p_dat = 0;
    
    //hgusb20_dev_ep0_rx(p_dev, (void*)p_dev->usb_ep0_rxbuf, 63);

    if (!len) {
        return hgusb20_dev_ep0_clrrx_pkt0(p_dev);
    }

    switch (index) {
        case 4:
            p_dat = (uint8_t *)tbl_winusb_compatible_id_feature_descriptor;
            descriptor_len = sizeof(tbl_winusb_compatible_id_feature_descriptor);
            break;        
        case 5:
            p_dat = (uint8_t *)tbl_winusb_device_interface_guids;
            descriptor_len = sizeof(tbl_winusb_device_interface_guids);
            break;
//        case 7:
//            p_dat = (uint8_t *)tbl_winusb20_wcid_descriptor_set;
//            descriptor_len = sizeof(tbl_winusb20_wcid_descriptor_set);
//            break;
        default:
            return FALSE;
            break;
    }
    
    if (len > descriptor_len) {
        len = descriptor_len;
    }

    hgusb20_dev_ep0_descriptor(p_dev, p_dat, descriptor_len);

    return TRUE;
}

////类命令
//bool usb_device_ep0_class(struct hgusb20_dev *p_dev)
//{
//    return hgusb20_dev_ep0_class_communications(p_dev);
////    switch (p_dev->cfg->class_code) {
////        case 0x02: // Communications and CDC Control
////        default:
////            break;
////    }
////    return FALSE;
//}

//类命令
bool usb_device_vendor_cmd(struct hgusb20_dev *p_dev)
{
    if (WCID_VENDOR_CODE == p_dev->usb_ctrl.cmd.request) {
        /* OS2.0 descriptor set index */
         return usb_device_ep0_get_wcid_descriptor(p_dev, BYTE0(p_dev->usb_ctrl.cmd.index), p_dev->usb_ctrl.cmd.length);
    } 
    return FALSE;
}

bool usb_device_ep0_get_descriptor(struct hgusb20_dev *p_dev)
{
//    _os_printf("usb_ep0_get_descriptor\n");
    switch (USB_REQUEST_DESCRIPTOR_TYPE) {       //Request Descriptor Type
        case DEVICE_DESCRIPTOR:
            //设备描述符
            hgusb20_dev_cfg_desc_prepare(p_dev);
            return hgusb20_dev_ep0_descriptor(p_dev, (char *)p_dev->p_com_desc, sizeof(tbl_usb_wifi_device_descriptor));
            //return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_wifi_device_descriptor, sizeof(tbl_usb_wifi_device_descriptor));
        case CONFIGURATION_DESCRIPTOR:
            //配置描述符
            return hgusb20_dev_ep0_descriptor(p_dev, p_dev->p_config_desc, get_unaligned_le16((const void *)&p_dev->p_config_desc[2]));
        case STRING_DESCRIPTOR:
            //字符串
            switch (USB_REQUEST_DESCRIPTOR_INDEX) {
                case 0:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_wifi_language_id, sizeof(tbl_usb_wifi_language_id));
                case 1:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_wifi_str_manufacturer, sizeof(tbl_usb_wifi_str_manufacturer));
                case 2:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_wifi_str_product, sizeof(tbl_usb_wifi_str_product));
                case 3:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_wifi_str_serial_number, sizeof(tbl_usb_wifi_str_serial_number));
                case WCID_DEVICE_DESCRIPTOR:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_winusb_device_descriptor, sizeof(tbl_winusb_device_descriptor));
                default:
                    return FALSE;
            }
//    case INTERFACE_DESCRIPTOR:
//    case ENDPOINT_DESCRIPTOR:
        case DEVICE_QUALIFIER_DESCRIPTOR:
            //usb_device_qualifier 限定描述符
            memcpy(p_dev->usb_ep0_rxbuf, (char *)p_dev->cfg->p_device_descriptor, 8);
            p_dev->usb_ep0_rxbuf[0] = 10;
            p_dev->usb_ep0_rxbuf[1] = DEVICE_QUALIFIER_DESCRIPTOR;
            p_dev->usb_ep0_rxbuf[8] = p_dev->cfg->p_device_descriptor[17];
            p_dev->usb_ep0_rxbuf[9] = 0;
            return hgusb20_dev_ep0_descriptor(p_dev, p_dev->usb_ep0_rxbuf, 10);
        default:
            return FALSE;
    }
}

/**
 * setup packet is in “(uint8_t *)&p_dev->usb_ctrl.cmd” or p_dev->usb_ep0_rxbuf 
 */
bool usb_device_ep0_request(struct hgusb20_dev *p_dev)
{
    uint8_t rtype = USB_REQUEST_TYPE;
    if (0 == rtype) {
        //标准命令处理
        if (0x06 == p_dev->usb_ctrl.cmd.request) {
            if (!usb_device_ep0_get_descriptor(p_dev)) {
                return FALSE;
            }
        } else {
            if (!hgusb20_dev_ep0_standard(p_dev)) {
                return FALSE;
            }
        }

        return TRUE;
    } else if (0x20 == rtype) {
        //类命令
        //return usb_device_ep0_class(p_dev);                     //类命令处理
    } else if (0x40 == rtype) {
        //verdor requeset
        return usb_device_vendor_cmd(p_dev);
    }
    return FALSE;
}

/** 
  * @brief  usb device retrive rx packet to usb_ctrl.cmd
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note
  */
static uint32_t usb_device_ep0_get_request(struct hgusb20_dev *p_dev, uint8 len)
{
    uint32_t usb_rx_counter = len;
//    struct hgusb20_dev_hw *hw = (struct hgusb20_dev_hw *)p_dev->usb_hw;

    uint8_t *pBuf = (uint8_t *)&p_dev->usb_ctrl.cmd;

    os_memcpy(pBuf, (uint32)p_dev->usb_ep0_rxbuf, usb_rx_counter);

#if 0
        //_os_printf("rx %x %02x\r\n", usb_rx_counter, read_usb_reg(M_INTRUSBE));
        _os_printf(">>");
        pBuf = (uint8_t *)&p_dev->usb_ctrl.cmd;
        for (int i=0; i<usb_rx_counter; i++) {
            _os_printf(" %02x", *pBuf++);
        } 
        _os_printf("\r\n");
#endif
    return usb_rx_counter;
}

uint32 usb_device_ep0_setup_irq(struct usb_device *p_usb_d, uint32 len)
{
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)&p_usb_d->dev;
    
//    _os_printf("setup: %d\r\n", len);
    if (len == 8) {
        //获取命令
        usb_device_ep0_get_request(p_dev, 8);
        //处理命令
        if (!usb_device_ep0_request(p_dev)) {           
            hgusb20_dev_stall_ep(p_dev, 0x00);
        }
    } else {
       // _os_printf("rx_cb: %d\r\n", len);
    } 
    
    return 0;
}

uint32 usb_device_ep0_rx_irq(struct usb_device *p_usb_d, uint32 len)
{
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)&p_usb_d->dev;
    
//    _os_printf("rx_cb: %d\r\n", len);

    hgusb20_dev_ep0_clrrx_pkt0(p_dev);
    return 0;
}

uint32 usb_device_ep0_tx_irq(struct usb_device *p_usb_d, uint32 len)
{    
//    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)&p_usb_d->dev;
//    _os_printf("tx_cb: %d\r\n", len);
    return 0;
}

__init int32 usb_device_wifi_open(struct usb_device *p_usb_d)
{
    return usb_device_open(p_usb_d, (struct usb_device_cfg *)&usb_dev_wifi_cfg);
}

int32 usb_device_wifi_close(struct usb_device *p_usb_d)
{
    return usb_device_close(p_usb_d);
}

int32 usb_device_wifi_auto_tx_null_pkt_disable(struct usb_device *p_usb_d)
{
    return usb_device_ioctl(p_usb_d, USB_DEV_IO_CMD_AUTO_TX_NULL_PKT_DISABLE, USB_WIFI_TX_EP, 0);
}

int32 usb_device_wifi_auto_tx_null_pkt_enable(struct usb_device *p_usb_d)
{
    return usb_device_ioctl(p_usb_d, USB_DEV_IO_CMD_AUTO_TX_NULL_PKT_ENABLE, USB_WIFI_TX_EP, 0);
}

int32 usb_device_wifi_write(struct usb_device *p_usb_d, uint8 *buff, uint32 len)
{
    return usb_device_write(p_usb_d, USB_WIFI_TX_EP, buff, len, 1);
}

int32 usb_device_wifi_write_scatter(struct usb_device *p_usb_d, scatter_data *data, int count)
{
    return usb_device_write_scatter(p_usb_d, USB_WIFI_TX_EP, data, count, 1);
}

int32 usb_device_wifi_read(struct usb_device *p_usb_d, uint8 *buff, uint32 len)
{
    return usb_device_read(p_usb_d, USB_WIFI_RX_EP, buff, len, 0);
}


#endif