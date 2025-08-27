/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>

#ifdef RT_USB_DEVICE_VIDEO

#include "include/usb_device.h"
#include "usbd_video.h"
#include "include/usb_video.h"
#include "jpgdef.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/lcd/lcd.h"
#include "hal/vpp.h"
#include "hal/scale.h"
#include "custom_mem/custom_mem.h"
#include "video_app.h"

#define VS_HEADER_SIZ (unsigned int)(VIDEO_SIZEOF_VS_INPUT_HEADER_DESC(1,1) + VIDEO_SIZEOF_VS_FORMAT_MJPEG_DESC + VIDEO_SIZEOF_VS_FRAME_MJPEG_DESC(1))
#define CONFIG_USB_HS

#ifdef CONFIG_USB_HS
#define MAX_PAYLOAD_SIZE  1024 // for high speed with one transcations every one micro frame
#define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 1)) | (0x00 << 11))

// #define MAX_PAYLOAD_SIZE  2048 // for high speed with two transcations every one micro frame
// #define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 2)) | (0x01 << 11))

// #define MAX_PAYLOAD_SIZE  3072 // for high speed with three transcations every one micro frame
// #define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 3)) | (0x02 << 11))

#else
#define MAX_PAYLOAD_SIZE  1020
#define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 1)) | (0x00 << 11))
#endif

#define VIDEO_IN_EP  0x81
#define VIDEO_INT_EP 0x83

#define WIDTH  (unsigned int)(640)
#define HEIGHT (unsigned int)(480)

#define CAM_FPS        (30)
#define INTERVAL       (unsigned long)(10000000 / CAM_FPS)
#define MIN_BIT_RATE(width, height)   (unsigned long)(width * height * 16 * CAM_FPS) //16 bit
#define MAX_BIT_RATE(width, height)   (unsigned long)(width * height * 16 * CAM_FPS)
#define MAX_FRAME_SIZE(width, height) (unsigned long)(width * height * 2)

#define UVC_MAX_PACKET_SIZE 64
#define VIDEO_BUFFER_SZ     1024
#define UVC_INTF_STR_INDEX  0x02

#define EVENT_VIDEO_START   (1 << 0)
#define EVENT_VIDEO_STOP    (1 << 1)
#define EVENT_VIDEO_DATA    (1 << 2)

enum usbd_video_mode
{
    USBD_VIDEO_CLOSE = 0,
    USBD_VIDEO_OPEN,
};

struct video_entity_info {
    rt_uint8_t bDescriptorSubtype;
    rt_uint8_t bEntityId;
    rt_uint16_t wTerminalType;
};

#define CONFIG_USBDEV_MAX_BUS 1

typedef void (*uvc_set_resolution)(uint32_t in_weight, uint32_t in_height, uint32_t out_weight, uint32_t out_height, uint32_t en);

struct usbd_video_ops
{
    uvc_set_resolution  set_resolution;
};

struct usbd_video_priv {
    struct video_probe_and_commit_controls probe;
    struct video_probe_and_commit_controls commit;
    rt_uint8_t power_mode;
    rt_uint8_t error_code;
    struct video_entity_info info[3];
    struct usbd_video_ops ops;
} g_usbd_video[CONFIG_USBDEV_MAX_BUS];

struct uvc_vc_noep_descriptor
{
#ifdef RT_USB_DEVICE_COMPOSITE
    struct uiad_descriptor iad_desc;
#endif
    struct uinterface_descriptor intf_desc;
    struct video_cs_if_vc_header_descriptor hdr_desc;
    struct video_cs_if_vc_input_terminal_descriptor it_desc;
    struct video_cs_if_vc_processing_unit_descriptor pu_desc;
    struct video_cs_if_vc_output_terminal_descriptor ot_desc;
};

struct uvc_vs_noep_descriptor
{
    struct uinterface_descriptor intf_desc;
    struct video_cs_if_vs_input_header_descriptor it_desc;
    struct video_cs_if_vs_format_uncompressed_descriptor fmt_desc;
    struct video_cs_if_vs_frame_uncompressed_descriptor frame_desc;
    struct video_cs_if_vs_frame_uncompressed_descriptor frame2_desc;
};

struct uvc_vs_ep_descriptor
{
    struct uinterface_descriptor intf_desc;
    struct uendpoint_descriptor ep_desc;
};

/*
 * uvc class device type
 */

struct uvc_class_device
{
    rt_device_t  dev;
    rt_event_t   event;
    rt_uint8_t   open_count;

    rt_uint8_t  *buffer;
    rt_uint32_t  buffer_index;

    uep_t        ep;
    rt_thread_t  thread;
};
struct uvc_class_device usbd_uvc;

rt_align(4)
static struct udevice_descriptor dev_desc =
{
    USB_DESC_LENGTH_DEVICE,     //bLength;
    USB_DESC_TYPE_DEVICE,       //type;
    USB_BCD_VERSION,            //bcdUSB;
    USB_CLASS_MISC,           //bDeviceClass;
    0x02,                       //bDeviceSubClass;
    0x01,                       //bDeviceProtocol;
    UVC_MAX_PACKET_SIZE,        //bMaxPacketSize0;
    _VENDOR_ID,                 //idVendor;
    _PRODUCT_ID,                //idProduct;
    USB_BCD_DEVICE,             //bcdDevice;
    USB_STRING_MANU_INDEX,      //iManufacturer;
    USB_STRING_PRODUCT_INDEX,   //iProduct;
    USB_STRING_SERIAL_INDEX,    //iSerialNumber;Unused.
    0x00,                //bNumConfigurations;    
};

//FS and HS needed
rt_align(4)
static struct usb_qualifier_descriptor dev_qualifier =
{
    sizeof(dev_qualifier),          //bLength
    USB_DESC_TYPE_DEVICEQUALIFIER,  //bDescriptorType
    0x0200,                         //bcdUSB
    USB_CLASS_VIDEO,                //bDeviceClass
    0x00,                           //bDeviceSubClass
    0x00,                           //bDeviceProtocol
    64,                             //bMaxPacketSize0
    0x01,                           //bNumConfigurations
    0,
};

rt_align(4)
const static char *_ustring[] =
{
    "Language",
    "USBZH",            //"Cherry USB",
    "USB2.0 Camera",    //"USB Composite Device",
    "20240411",
};

rt_align(4)
static struct uvc_vc_noep_descriptor vc_desc =
{
#ifdef RT_USB_DEVICE_COMPOSITE
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x02,
        USB_CLASS_VIDEO,
        VIDEO_SC_VIDEO_INTERFACE_COLLECTION,
        0x00,
        0x00,        
    },
#endif
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x00,
        USB_CLASS_VIDEO,
        VIDEO_SC_VIDEOCONTROL,
        VIDEO_PC_PROTOCOL_UNDEFINED,
#ifdef RT_USB_DEVICE_COMPOSITE
        UVC_INTF_STR_INDEX,
#else
        0x00,
#endif
    },
    /* Header Descriptor */
    {
        sizeof(struct video_cs_if_vc_header_descriptor),    //0x0d,
        VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE,
        VIDEO_VC_HEADER_DESCRIPTOR_SUBTYPE,
        0x0100,
        VIDEO_VC_TERMINAL_LEN,
        48000000,
        0x01,
        {0x01},
    },
    /* Input Terminal Descriptor */
    {
        sizeof(struct video_cs_if_vc_input_terminal_descriptor),   //0x12,
        VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE,
        VIDEO_VC_INPUT_TERMINAL_DESCRIPTOR_SUBTYPE,
        0x01,
        VIDEO_ITT_CAMERA,
        0x00,
        0x00,
        0x0000,
        0x0000,
        0x0000,
        0x03,
        {0x00, 0x00, 0x00},
    },
    /* Processing Unit Descriptor */
    {
        sizeof(struct video_cs_if_vc_processing_unit_descriptor),   //0x0c,
        VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE,
        VIDEO_VC_PROCESSING_UNIT_DESCRIPTOR_SUBTYPE,
        0x02,
        0x01,
        0x0000,
        0x02,
        {0x00,0x00},
        0x00,
        0x00,
    },
    /* Output Terminal Descriptor */
    {
        sizeof(struct video_cs_if_vc_output_terminal_descriptor),   //0x09,
        VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE,
        VIDEO_VC_OUTPUT_TERMINAL_DESCRIPTOR_SUBTYPE,
        0x03,
        VIDEO_TT_STREAMING,
        0x00,
        0x02,
        0x00,
    },
};

rt_align(4)
static struct uvc_vs_noep_descriptor vs_noep_desc = 
{
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        0x01,
        0x00,
        0x00,
        USB_CLASS_VIDEO,
        VIDEO_SC_VIDEOSTREAMING,
        0x00,
        0x00,
    },
    /* Header Descriptor */
    {
        sizeof(struct video_cs_if_vs_input_header_descriptor),      //0x0d,
        VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE,
        VIDEO_VS_INPUT_HEADER_DESCRIPTOR_SUBTYPE,
        0x01,
        VS_HEADER_SIZ,
        VIDEO_IN_EP,                                                
        0x00,
        0x03,
        0x00,
        0x00,
        0x00,
        0x02,
        {0x00,0x00},/* bmaControls : No VideoStreaming specific controls are supported.*/
    },
    /* VS Format Descriptor */
    {
        sizeof(struct video_cs_if_vs_format_uncompressed_descriptor),    //0x0b,
        VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE,
        VIDEO_VS_FORMAT_MJPEG_DESCRIPTOR_SUBTYPE,
        0x01,
        0x02,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /* VS Frame 640x480 Descriptor */
    {
        sizeof(struct video_cs_if_vs_frame_uncompressed_descriptor),    //0x1a,
        VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE,
        VIDEO_VS_FRAME_MJPEG_DESCRIPTOR_SUBTYPE,
        0x01,
        0x00,
        WIDTH,
        HEIGHT,
        MIN_BIT_RATE(WIDTH,HEIGHT),
        MAX_BIT_RATE(WIDTH,HEIGHT),
        MAX_FRAME_SIZE(WIDTH,HEIGHT),
        INTERVAL,
        0x01,
        {INTERVAL},
    },
    /* VS Frame 1280x720 Descriptor */
    {
        sizeof(struct video_cs_if_vs_frame_uncompressed_descriptor),    //0x1a,
        VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE,
        VIDEO_VS_FRAME_MJPEG_DESCRIPTOR_SUBTYPE,
        0x02,
        0x00,
        1280,
        720,
        MIN_BIT_RATE(1280,720),
        MAX_BIT_RATE(1280,720),
        MAX_FRAME_SIZE(1280,720),
        INTERVAL,
        0x01,
        {INTERVAL},
    },    
};

rt_align(4)
static struct uvc_vs_ep_descriptor vs_ep_desc = 
{
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        0x01,
        0x01,
        0x01,
        USB_CLASS_VIDEO,
        VIDEO_SC_VIDEOSTREAMING,
        0x00,
        0x00,
    },
    /* Endpoint Descriptor */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        VIDEO_IN_EP | USB_DIR_OUT,
        0x05,
        VIDEO_PACKET_SIZE,
        0x01,        
    },    
};


static rt_bool_t usbd_video_control_request_handler(rt_uint8_t busid, ufunction_t func, ureq_t setup)
{
    rt_uint8_t control_selector = (rt_uint8_t)(setup->wValue >> 8);
    rt_uint8_t buf[26]; 
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    switch (control_selector) {
        case VIDEO_VC_VIDEO_POWER_MODE_CONTROL:
            switch (setup->bRequest) {
                case VIDEO_REQUEST_SET_CUR:
                    break;
                case VIDEO_REQUEST_GET_CUR:
                    break;
                case VIDEO_REQUEST_GET_INFO:
                    break;
                default:
                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                    return -1;
            }

            break;
        case VIDEO_VC_REQUEST_ERROR_CODE_CONTROL:
            switch (setup->bRequest) {
                case VIDEO_REQUEST_GET_CUR:
                    buf[0] = 0x06;
                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)buf, 1);
                    break;
                case VIDEO_REQUEST_GET_INFO:
                    break;
                default:
                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                    return -1;
            }

            break;
        default:
            break;
    }

    return 0;
}

static rt_bool_t usbd_video_control_unit_terminal_request_handler(rt_uint8_t busid, ufunction_t func, ureq_t setup)
{
    rt_uint8_t entity_id = (rt_uint8_t)(setup->wIndex >> 8);
    rt_uint8_t control_selector = (rt_uint8_t)(setup->wValue >> 8);
    rt_uint8_t data[26];
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    for (rt_uint8_t i = 0; i < 3; i++) {
        struct video_entity_info *entity_info = &g_usbd_video[busid].info[i];
        if (entity_info->bEntityId == entity_id) {
            switch (entity_info->bDescriptorSubtype) {
                case VIDEO_VC_HEADER_DESCRIPTOR_SUBTYPE:
                    break;
                case VIDEO_VC_INPUT_TERMINAL_DESCRIPTOR_SUBTYPE:
                    if (entity_info->wTerminalType == VIDEO_ITT_CAMERA) {
                        switch (control_selector) {
                            case VIDEO_CT_AE_MODE_CONTROL:
                                switch (setup->bRequest) {
                                    case VIDEO_REQUEST_GET_CUR:
                                        data[0] = 0x08;
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                        break;
                                    default:
                                        LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                        return -1;
                                }
                                break;
                            case VIDEO_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                                switch (setup->bRequest) {
                                    case VIDEO_REQUEST_GET_CUR: {
                                        rt_uint32_t dwExposureTimeAbsolute = 2500;
                                        memcpy(data, (rt_uint8_t *)&dwExposureTimeAbsolute, 4);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 4);
                                    } break;
                                    case VIDEO_REQUEST_GET_MIN: {
                                        rt_uint32_t dwExposureTimeAbsolute = 5; //0.0005sec
                                        memcpy(data, (rt_uint8_t *)&dwExposureTimeAbsolute, 4);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 4);
                                    } break;
                                    case VIDEO_REQUEST_GET_MAX: {
                                        rt_uint32_t dwExposureTimeAbsolute = 2500; //0.2500sec
                                        memcpy(data, (rt_uint8_t *)&dwExposureTimeAbsolute, 4);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 4);
                                    } break;
                                    case VIDEO_REQUEST_GET_RES: {
                                        rt_uint32_t dwExposureTimeAbsolute = 5; //0.0005sec
                                        memcpy(data, (rt_uint8_t *)&dwExposureTimeAbsolute, 4);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 4);
                                    } break;
                                    case VIDEO_REQUEST_GET_INFO:
                                        data[0] = 0x03; //struct video_camera_capabilities
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                        break;
                                    case VIDEO_REQUEST_GET_DEF: {
                                        rt_uint32_t dwExposureTimeAbsolute = 2500; //0.2500sec
                                        memcpy(data, (rt_uint8_t *)&dwExposureTimeAbsolute, 4);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 4);
                                    } break;
                                    default:
                                        LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                        return -1;
                                }
                                break;
                            case VIDEO_CT_FOCUS_ABSOLUTE_CONTROL:
                                switch (setup->bRequest) {
                                    case VIDEO_REQUEST_GET_CUR: {
                                        rt_uint16_t wFocusAbsolute = 0x0080;
                                        memcpy(data, (rt_uint8_t *)&wFocusAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_MIN: {
                                        rt_uint16_t wFocusAbsolute = 0;
                                        memcpy(data, (rt_uint8_t *)&wFocusAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_MAX: {
                                        rt_uint16_t wFocusAbsolute = 0x00ff;
                                        memcpy(data, (rt_uint8_t *)&wFocusAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_RES: {
                                        rt_uint16_t wFocusAbsolute = 0x0001;
                                        memcpy(data, (rt_uint8_t *)&wFocusAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_INFO:
                                        data[0] = 0x03; //struct video_camera_capabilities
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                        break;
                                    case VIDEO_REQUEST_GET_DEF: {
                                        rt_uint16_t wFocusAbsolute = 0x0080;
                                        memcpy(data, (rt_uint8_t *)&wFocusAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    default:
                                        LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                        return -1;
                                }
                                break;
                            case VIDEO_CT_ZOOM_ABSOLUTE_CONTROL:
                                switch (setup->bRequest) {
                                    case VIDEO_REQUEST_GET_CUR: {
                                        rt_uint16_t wObjectiveFocalLength = 0x0064;
                                        memcpy(data, (rt_uint8_t *)&wObjectiveFocalLength, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_MIN: {
                                        rt_uint16_t wObjectiveFocalLength = 0x0064;
                                        memcpy(data, (rt_uint8_t *)&wObjectiveFocalLength, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_MAX: {
                                        rt_uint16_t wObjectiveFocalLength = 0x00c8;
                                        memcpy(data, (rt_uint8_t *)&wObjectiveFocalLength, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_RES: {
                                        rt_uint16_t wObjectiveFocalLength = 0x0001;
                                        memcpy(data, (rt_uint8_t *)&wObjectiveFocalLength, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_INFO:
                                        data[0] = 0x03; //struct video_camera_capabilities
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                        break;
                                    case VIDEO_REQUEST_GET_DEF: {
                                        rt_uint16_t wObjectiveFocalLength = 0x0064;
                                        memcpy(data, (rt_uint8_t *)&wObjectiveFocalLength, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    default:
                                        LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                        return -1;
                                }
                                break;
                            case VIDEO_CT_ROLL_ABSOLUTE_CONTROL:
                                switch (setup->bRequest) {
                                    case VIDEO_REQUEST_GET_CUR: {
                                        rt_uint16_t wRollAbsolute = 0x0000;
                                        memcpy(data, (rt_uint8_t *)&wRollAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_MIN: {
                                        rt_uint16_t wRollAbsolute = 0x0000;
                                        memcpy(data, (rt_uint8_t *)&wRollAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_MAX: {
                                        rt_uint16_t wRollAbsolute = 0x00ff;
                                        memcpy(data, (rt_uint8_t *)&wRollAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_RES: {
                                        rt_uint16_t wRollAbsolute = 0x0001;
                                        memcpy(data, (rt_uint8_t *)&wRollAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    case VIDEO_REQUEST_GET_INFO:
                                        data[0] = 0x03; //struct video_camera_capabilities
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                        break;
                                    case VIDEO_REQUEST_GET_DEF: {
                                        rt_uint16_t wRollAbsolute = 0x0000;
                                        memcpy(data, (rt_uint8_t *)&wRollAbsolute, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    default:
                                        LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                        return -1;
                                }
                                break;
                            case VIDEO_CT_FOCUS_AUTO_CONTROL:
                                switch (setup->bRequest) {
                                    case VIDEO_REQUEST_GET_CUR: {
                                        rt_uint16_t wFocusAuto = 0x0000;
                                        memcpy(data, (rt_uint8_t *)&wFocusAuto, 2);
                                        rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    } break;
                                    default:
                                        LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                        return -1;
                                }
                                break;
                            default:
                                LOG_D("Unhandled Video Class control selector 0x%02x\r\n", control_selector);
                                return -1;
                        }
                    } else {
                        LOG_D("Unhandled Video Class wTerminalType 0x%02x\r\n", entity_info->wTerminalType);
                        return -2;
                    }
                    break;
                case VIDEO_VC_OUTPUT_TERMINAL_DESCRIPTOR_SUBTYPE:
                    break;
                case VIDEO_VC_SELECTOR_UNIT_DESCRIPTOR_SUBTYPE:
                    break;
                case VIDEO_VC_PROCESSING_UNIT_DESCRIPTOR_SUBTYPE:
                    switch (control_selector) {
                        case VIDEO_PU_BACKLIGHT_COMPENSATION_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_GET_CUR: {
                                    rt_uint16_t wBacklightCompensation = 0x0004;
                                    memcpy(data, (rt_uint8_t *)&wBacklightCompensation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MIN: {
                                    rt_uint16_t wBacklightCompensation = 0;
                                    memcpy(data, (rt_uint8_t *)&wBacklightCompensation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MAX: {
                                    rt_uint16_t wBacklightCompensation = 8;
                                    memcpy(data, (rt_uint8_t *)&wBacklightCompensation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_RES: {
                                    rt_uint16_t wBacklightCompensation = 1;
                                    memcpy(data, (rt_uint8_t *)&wBacklightCompensation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_INFO:
                                    data[0] = 0x03; //struct video_camera_capabilities
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                    break;
                                case VIDEO_REQUEST_GET_DEF: {
                                    rt_uint16_t wBacklightCompensation = 4;
                                    memcpy(data, (rt_uint8_t *)&wBacklightCompensation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        case VIDEO_PU_BRIGHTNESS_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_SET_CUR: {
                                    //rt_uint16_t wBrightness = (rt_uint16_t)(*data)[1] << 8 | (rt_uint16_t)(*data)[0];
                                } break;
                                case VIDEO_REQUEST_GET_CUR: {
                                    rt_uint16_t wBrightness = 0x0080;
                                    memcpy(data, (rt_uint8_t *)&wBrightness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MIN: {
                                    rt_uint16_t wBrightness = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wBrightness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MAX: {
                                    rt_uint16_t wBrightness = 0x00ff;
                                    memcpy(data, (rt_uint8_t *)&wBrightness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_RES: {
                                    rt_uint16_t wBrightness = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wBrightness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_INFO:
                                    data[0] = 0x03; //struct video_camera_capabilities
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                    break;
                                case VIDEO_REQUEST_GET_DEF: {
                                    rt_uint16_t wBrightness = 0x0080;
                                    memcpy(data, (rt_uint8_t *)&wBrightness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        case VIDEO_PU_CONTRAST_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_GET_CUR: {
                                    rt_uint16_t wContrast = 0x0080;
                                    memcpy(data, (rt_uint8_t *)&wContrast, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MIN: {
                                    rt_uint16_t wContrast = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wContrast, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MAX: {
                                    rt_uint16_t wContrast = 0x00ff;
                                    memcpy(data, (rt_uint8_t *)&wContrast, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_RES: {
                                    rt_uint16_t wContrast = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wContrast, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_INFO:
                                    data[0] = 0x03; //struct video_camera_capabilities
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                    break;
                                case VIDEO_REQUEST_GET_DEF: {
                                    rt_uint16_t wContrast = 0x0080;
                                    memcpy(data, (rt_uint8_t *)&wContrast, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        case VIDEO_PU_HUE_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_GET_CUR: {
                                    rt_uint16_t wHue = 0x0080;
                                    memcpy(data, (rt_uint8_t *)&wHue, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MIN: {
                                    rt_uint16_t wHue = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wHue, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MAX: {
                                    rt_uint16_t wHue = 0x00ff;
                                    memcpy(data, (rt_uint8_t *)&wHue, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_RES: {
                                    rt_uint16_t wHue = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wHue, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_INFO:
                                    data[0] = 0x03; //struct video_camera_capabilities
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                    break;
                                case VIDEO_REQUEST_GET_DEF: {
                                    rt_uint16_t wHue = 0x0080;
                                    memcpy(data, (rt_uint8_t *)&wHue, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        case VIDEO_PU_SATURATION_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_GET_MIN: {
                                    rt_uint16_t wSaturation = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wSaturation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MAX: {
                                    rt_uint16_t wSaturation = 0x00ff;
                                    memcpy(data, (rt_uint8_t *)&wSaturation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_RES: {
                                    rt_uint16_t wSaturation = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wSaturation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_INFO:
                                    data[0] = 0x03; //struct video_camera_capabilities
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                    break;
                                case VIDEO_REQUEST_GET_DEF: {
                                    rt_uint16_t wSaturation = 0x0080;
                                    memcpy(data, (rt_uint8_t *)&wSaturation, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        case VIDEO_PU_SHARPNESS_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_GET_MIN: {
                                    rt_uint16_t wSharpness = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wSharpness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MAX: {
                                    rt_uint16_t wSharpness = 0x00ff;
                                    memcpy(data, (rt_uint8_t *)&wSharpness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_RES: {
                                    rt_uint16_t wSharpness = 0x0001;
                                    memcpy(data, (rt_uint8_t *)&wSharpness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_INFO:
                                    data[0] = 0x03; //struct video_camera_capabilities
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                    break;
                                case VIDEO_REQUEST_GET_DEF: {
                                    rt_uint16_t wSharpness = 0x0080;
                                    memcpy(data, (rt_uint8_t *)&wSharpness, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        case VIDEO_PU_GAIN_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_GET_MIN: {
                                    rt_uint16_t wGain = 0;
                                    memcpy(data, (rt_uint8_t *)&wGain, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MAX: {
                                    rt_uint16_t wGain = 255;
                                    memcpy(data, (rt_uint8_t *)&wGain, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_RES: {
                                    rt_uint16_t wGain = 1;
                                    memcpy(data, (rt_uint8_t *)&wGain, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_INFO:
                                    data[0] = 0x03; //struct video_camera_capabilities
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                    break;
                                case VIDEO_REQUEST_GET_DEF: {
                                    rt_uint16_t wGain = 255;
                                    memcpy(data, (rt_uint8_t *)&wGain, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        case VIDEO_PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_GET_CUR: {
                                    rt_uint16_t wWhiteBalance_Temprature = 417;
                                    memcpy(data, (rt_uint8_t *)&wWhiteBalance_Temprature, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MIN: {
                                    rt_uint16_t wWhiteBalance_Temprature = 300;
                                    memcpy(data, (rt_uint8_t *)&wWhiteBalance_Temprature, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_MAX: {
                                    rt_uint16_t wWhiteBalance_Temprature = 600;
                                    memcpy(data, (rt_uint8_t *)&wWhiteBalance_Temprature, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_RES: {
                                    rt_uint16_t wWhiteBalance_Temprature = 1;
                                    memcpy(data, (rt_uint8_t *)&wWhiteBalance_Temprature, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                case VIDEO_REQUEST_GET_INFO:
                                    data[0] = 0x03; //struct video_camera_capabilities
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                    break;
                                case VIDEO_REQUEST_GET_DEF: {
                                    rt_uint16_t wWhiteBalance_Temprature = 417;
                                    memcpy(data, (rt_uint8_t *)&wWhiteBalance_Temprature, 2);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 2);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        case VIDEO_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
                            switch (setup->bRequest) {
                                case VIDEO_REQUEST_GET_CUR: {
                                    rt_uint16_t wWhiteBalance_Temprature_Auto = 1;
                                    memcpy(data, (rt_uint8_t *)&wWhiteBalance_Temprature_Auto, 1);
                                    rt_usbd_ep0_write(func->device, (rt_uint8_t *)data, 1);
                                } break;
                                default:
                                    LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                                    return -1;
                            }
                            break;
                        default:
                            g_usbd_video[busid].error_code = 0x06;
                            LOG_D("Unhandled Video Class control selector 0x%02x\r\n", control_selector);
                            return -1;
                    }
                    break;
                case VIDEO_VC_EXTENSION_UNIT_DESCRIPTOR_SUBTYPE:
                    break;
                case VIDEO_VC_ENCODING_UNIT_DESCRIPTOR_SUBTYPE:
                    break;

                default:
                    break;
            }
        }
    }
    return 0;
}


rt_uint32_t usbd_video_mjpeg_payload_fill(rt_uint8_t busid, rt_uint8_t *input, rt_uint32_t input_len, rt_uint8_t *output, rt_uint32_t *out_len)
{
    rt_uint32_t packets;
    rt_uint32_t last_packet_size;
    rt_uint32_t picture_pos = 0;
    static rt_uint8_t uvc_header[2] = { 0x02, 0x80 };

    packets = (input_len + (g_usbd_video[busid].probe.dwMaxPayloadTransferSize - 2) ) / (g_usbd_video[busid].probe.dwMaxPayloadTransferSize - 2);

    //os_printf("packets:%d input len:%d dwMaxPayloadSize:%d \n",packets,input_len,g_usbd_video[busid].probe.dwMaxPayloadTransferSize);

    last_packet_size = input_len - ((packets - 1) * (g_usbd_video[busid].probe.dwMaxPayloadTransferSize - 2));

    for (rt_size_t i = 0; i < packets; i++) {
        output[g_usbd_video[busid].probe.dwMaxPayloadTransferSize * i] = uvc_header[0];
        output[g_usbd_video[busid].probe.dwMaxPayloadTransferSize * i + 1] = uvc_header[1];
        if (i == (packets - 1)) {
            hw_memcpy(&output[2 + g_usbd_video[busid].probe.dwMaxPayloadTransferSize * i], &input[picture_pos], last_packet_size);
            output[g_usbd_video[busid].probe.dwMaxPayloadTransferSize * i + 1] |= (1 << 1);
        } else {
            hw_memcpy(&output[2 + g_usbd_video[busid].probe.dwMaxPayloadTransferSize * i], &input[picture_pos], g_usbd_video[busid].probe.dwMaxPayloadTransferSize - 2);
            picture_pos += g_usbd_video[busid].probe.dwMaxPayloadTransferSize - 2;
        }
    }
    uvc_header[1] ^= 1;
    *out_len = (input_len + 2 * packets);
    return packets;
}

rt_uint32_t usbd_video_mjpeg_payload_header_fill(rt_uint8_t *input, rt_uint32_t input_len, rt_uint8_t *output, rt_uint8_t *uvc_header, rt_uint8_t uvc_header_len)
{

    //os_printf("uvc header len:%d\n",uvc_header_len);

    if(input_len < VIDEO_PACKET_SIZE - uvc_header_len)       //2为header的长度
    {
        

        output[0] = uvc_header[0];
        output[1] = uvc_header[1];
        output[1] |= (1 << 1); 

        os_memcpy(&output[2], input, input_len);

        uvc_header[1] ^= 1;
    }
    else
    {
        output[0] = uvc_header[0];
        output[1] = uvc_header[1];  

        os_memcpy(&output[2], input, input_len);      
    }

    return input_len + uvc_header_len;
}

static rt_err_t usbd_video_open(rt_uint8_t busid, ufunction_t func)
{
    os_printf("usbd video open\r\n"); 
    
    os_printf("g_usbd_video[busid].probe.bFormatIndex:%d\n",g_usbd_video[busid].probe.bFormatIndex);
    
    if((g_usbd_video[busid].probe.bFormatIndex == 1)) 
    {
        switch(g_usbd_video[busid].probe.bFrameIndex){
            case 1:
                os_printf("set interface VGA\r\n");
                g_usbd_video[busid].ops.set_resolution(640, 480, WIDTH, HEIGHT, USBD_VIDEO_OPEN);
            break;

            case 2:
                os_printf("set interface 720P\r\n");
                g_usbd_video[busid].ops.set_resolution(640, 480, 1280, 720, USBD_VIDEO_OPEN);
            break;

            default:
            break;
        }
    } 

    usbd_uvc.ep->request.buffer = usbd_uvc.buffer;
    usbd_uvc.ep->request.size = VIDEO_PACKET_SIZE;
    usbd_uvc.ep->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, usbd_uvc.ep, &usbd_uvc.ep->request);
    usbd_uvc.open_count = 0;
    usbd_uvc.open_count++;
    rt_event_send(usbd_uvc.event, EVENT_VIDEO_START);

    return 0;
}

static rt_err_t usbd_video_close(rt_uint8_t busid, ufunction_t func)
{
    os_printf("usbd video close\r\n");

	struct scale_device *scale_dev;
	struct scale_device *scale3_dev;
	struct vpp_device *vpp_dev;
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	scale3_dev= (struct scale_device *)dev_get(HG_SCALE3_DEVID); 

    g_usbd_video[busid].ops.set_resolution((uint32_t)RT_NULL, (uint32_t)RT_NULL, (uint32_t)RT_NULL, (uint32_t)RT_NULL, USBD_VIDEO_CLOSE);

    usbd_uvc.open_count--;
    rt_event_send(usbd_uvc.event, EVENT_VIDEO_STOP);
    
    return 0;
}

void usbd_video_probe_and_commit_controls_init(rt_uint8_t busid, rt_uint32_t dwFrameInterval, rt_uint32_t dwMaxVideoFrameSize, rt_uint32_t dwMaxPayloadTransferSize)
{
    g_usbd_video[busid].probe.hintUnion.bmHint = 0x01;
    g_usbd_video[busid].probe.hintUnion1.bmHint = 0;
    g_usbd_video[busid].probe.bFormatIndex = 1;
    g_usbd_video[busid].probe.bFrameIndex = 1;
    g_usbd_video[busid].probe.dwFrameInterval = dwFrameInterval;
    g_usbd_video[busid].probe.wKeyFrameRate = 0;
    g_usbd_video[busid].probe.wPFrameRate = 0;
    g_usbd_video[busid].probe.wCompQuality = 0;
    g_usbd_video[busid].probe.wCompWindowSize = 0;
    g_usbd_video[busid].probe.wDelay = 0;
    g_usbd_video[busid].probe.dwMaxVideoFrameSize = dwMaxVideoFrameSize;
    g_usbd_video[busid].probe.dwMaxPayloadTransferSize = dwMaxPayloadTransferSize;
    g_usbd_video[busid].probe.dwClockFrequency = 0;
    g_usbd_video[busid].probe.bmFramingInfo = 0;
    g_usbd_video[busid].probe.bPreferedVersion = 0;
    g_usbd_video[busid].probe.bMinVersion = 0;
    g_usbd_video[busid].probe.bMaxVersion = 0;

    g_usbd_video[busid].commit.hintUnion.bmHint = 0x01;
    g_usbd_video[busid].commit.hintUnion1.bmHint = 0;
    g_usbd_video[busid].commit.bFormatIndex = 1;
    g_usbd_video[busid].commit.bFrameIndex = 1;
    g_usbd_video[busid].commit.dwFrameInterval = dwFrameInterval;
    g_usbd_video[busid].commit.wKeyFrameRate = 0;
    g_usbd_video[busid].commit.wPFrameRate = 0;
    g_usbd_video[busid].commit.wCompQuality = 0;
    g_usbd_video[busid].commit.wCompWindowSize = 0;
    g_usbd_video[busid].commit.wDelay = 0;
    g_usbd_video[busid].commit.dwMaxVideoFrameSize = dwMaxVideoFrameSize;
    g_usbd_video[busid].commit.dwMaxPayloadTransferSize = dwMaxPayloadTransferSize;
    g_usbd_video[busid].commit.dwClockFrequency = 0;
    g_usbd_video[busid].commit.bmFramingInfo = 0;
    g_usbd_video[busid].commit.bPreferedVersion = 0;
    g_usbd_video[busid].commit.bMinVersion = 0;
    g_usbd_video[busid].commit.bMaxVersion = 0;
}

void usbd_video_init_intf(rt_uint8_t busid, rt_uint32_t dwFrameInterval, rt_uint32_t dwMaxVideoFrameSize, rt_uint32_t dwMaxPayloadTransferSize)
{

    g_usbd_video[busid].info[0].bDescriptorSubtype = VIDEO_VC_INPUT_TERMINAL_DESCRIPTOR_SUBTYPE;
    g_usbd_video[busid].info[0].bEntityId = 0x01;
    g_usbd_video[busid].info[0].wTerminalType = VIDEO_ITT_CAMERA;
    g_usbd_video[busid].info[1].bDescriptorSubtype = VIDEO_VC_OUTPUT_TERMINAL_DESCRIPTOR_SUBTYPE;
    g_usbd_video[busid].info[1].bEntityId = 0x03;
    g_usbd_video[busid].info[1].wTerminalType = 0x00;
    g_usbd_video[busid].info[2].bDescriptorSubtype = VIDEO_VC_PROCESSING_UNIT_DESCRIPTOR_SUBTYPE;
    g_usbd_video[busid].info[2].bEntityId = 0x02;
    g_usbd_video[busid].info[2].wTerminalType = 0x00;

    usbd_video_probe_and_commit_controls_init(busid, dwFrameInterval, dwMaxVideoFrameSize, dwMaxPayloadTransferSize);
}

static rt_err_t _ep0_cmd_handler(udevice_t device, rt_size_t size)
{
    LOG_D("_ep0_cmd_handler");

    dcd_ep0_send_status(device->dcd);

    return RT_EOK;
}

static rt_err_t _vc_interface_as_handler(ufunction_t func, ureq_t setup)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    os_printf("_vc_interface_as_handler\n");

    rt_uint8_t entity_id = (rt_uint8_t)(setup->wIndex >> 8);
    rt_uint8_t busid = 0;

    if (entity_id == 0) {
        return usbd_video_control_request_handler(busid, func, setup); /* Interface Control Requests */
    } else {
        return usbd_video_control_unit_terminal_request_handler(busid, func, setup); /* Unit and Terminal Requests */
    }

    return RT_EOK;
}



static rt_err_t _vs_interface_as_handler(ufunction_t func, ureq_t setup)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    rt_uint8_t control_selector = (rt_uint8_t)(setup->wValue >> 8);
    rt_uint8_t busid = 0;
    rt_uint8_t buf[26]; 

    memset(buf, 0, 26);

    os_printf("_vs_interface_as_handler\n");
    os_printf("request_type:%x bRequest:%d control_selector:%d\n",(setup->request_type & USB_REQ_TYPE_MASK), setup->bRequest,control_selector);

#if 1
    if ((setup->request_type & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD)
    {
        switch (setup->bRequest)
        {
        case USB_REQ_GET_INTERFACE:
            break;
        case USB_REQ_SET_INTERFACE:
            LOG_D("set interface handler");
            if (setup->wValue == 1)
            {
                usbd_video_open(busid, func);
            }
            else if (setup->wValue == 0)
            {
                usbd_video_close(busid, func);
            }
            break;
        default:
            LOG_D("unknown uac request 0x%x", setup->bRequest);
            return -RT_ERROR;
        }
    }else{
        switch (control_selector) {
                case VIDEO_VS_PROBE_CONTROL:
                    switch (setup->bRequest) {
                        case VIDEO_REQUEST_SET_CUR:
                            rt_usbd_ep0_read(func->device, (rt_uint8_t *)&buf, setup->wLength, _ep0_cmd_handler);
                            os_sleep_ms(5);         //等待DMA读取完毕的延时
                            memcpy(&g_usbd_video[busid].probe, (rt_uint8_t *)&buf, 22);
                            os_printf("SET CUR FormatIndex:%x FrameIndex:%x buf[2]:%x %x\n",g_usbd_video[busid].probe.bFormatIndex,g_usbd_video[busid].probe.bFrameIndex,buf[2],buf[3]);
                            break;
                        case VIDEO_REQUEST_GET_CUR:
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)&g_usbd_video[busid].probe, setup->wLength);
                            //os_printf("GET CUR payloadsize:%d\n",g_usbd_video[busid].probe.dwMaxPayloadTransferSize);
                            break;

                        case VIDEO_REQUEST_GET_MIN:
                        case VIDEO_REQUEST_GET_MAX:
                        case VIDEO_REQUEST_GET_RES:
                        case VIDEO_REQUEST_GET_DEF:
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)&g_usbd_video[busid].probe, setup->wLength);
                            //os_printf("GET = payloadsize:%d\n",g_usbd_video[busid].probe.dwMaxPayloadTransferSize);
                            break;
                        case VIDEO_REQUEST_GET_LEN:
                            buf[0] = sizeof(struct video_probe_and_commit_controls);
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)buf, 1);
                            break;

                        case VIDEO_REQUEST_GET_INFO:;
                            buf[0] = 0x03;
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)buf, 1);                            
                            break;

                        default:
                            LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                            return -1;
                    }
                    break;
                case VIDEO_VS_COMMIT_CONTROL:
                    switch (setup->bRequest) {
                        case VIDEO_REQUEST_SET_CUR:
                            rt_usbd_ep0_read(func->device, (rt_uint8_t *)&g_usbd_video[busid].commit, setup->wLength, _ep0_cmd_handler);
                            os_sleep_ms(5);         //等待DMA读取完毕的延时
                            break;
                        case VIDEO_REQUEST_GET_CUR:
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)&g_usbd_video[busid].commit, setup->wLength);
                            break;
                        case VIDEO_REQUEST_GET_MIN:
                        case VIDEO_REQUEST_GET_MAX:
                        case VIDEO_REQUEST_GET_RES:
                        case VIDEO_REQUEST_GET_DEF:
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)&g_usbd_video[busid].commit, setup->wLength);
                            break;

                        case VIDEO_REQUEST_GET_LEN:
                            buf[0] = sizeof(struct video_probe_and_commit_controls);
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)buf, 1);
                            break;

                        case VIDEO_REQUEST_GET_INFO:;
                            buf[0] = 0x03;
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)buf, 1);
                            break;

                        default:
                            LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                            return -1;
                    }
                    break;
                case VIDEO_VS_STREAM_ERROR_CODE_CONTROL:
                    switch (setup->bRequest) {
                        case VIDEO_REQUEST_GET_CUR:
                            buf[0] = g_usbd_video[busid].error_code;
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)buf, 1);                            
                            break;
                        case VIDEO_REQUEST_GET_INFO:
                            buf[0] = 0x01;
                            rt_usbd_ep0_write(func->device, (rt_uint8_t *)buf, 1);                             
                            break;
                        default:
                            LOG_D("Unhandled Video Class bRequest 0x%02x\r\n", setup->bRequest);
                            return -1;
                    }
                    break;
                default:
                    break;
            }
    }
#endif

    return RT_EOK;
}

static rt_err_t _ep_data_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);

    rt_event_send(usbd_uvc.event, EVENT_VIDEO_DATA);

    return RT_EOK;
}

static rt_err_t _function_enable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);

    os_printf("uvc function enable");

    return RT_EOK;
}

static rt_err_t _function_disable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);

    os_printf("uvc function disable");

    return RT_EOK;
}

static struct ufunction_ops ops =
{
    _function_enable,
    _function_disable,
    RT_NULL,
};

/**
 * This function will configure uvc descriptor.
 *
 * @param comm the communication interface number.
 * @param data the data interface number.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _uvc_descriptor_config(struct uvc_vc_noep_descriptor *vc,
                                       rt_uint8_t cintf_nr, struct uvc_vs_noep_descriptor *vs, rt_uint8_t sintf_nr)
{
    vc->hdr_desc.baInterfaceNr[0] = sintf_nr;
#ifdef RT_USB_DEVICE_COMPOSITE
    vc->iad_desc.bFirstInterface = cintf_nr;
#endif

    return RT_EOK;
}

static rt_bool_t opcode_func(stream *s,void *priv,rt_bool_t opcode)
{
	rt_bool_t res = 0;

	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		default:
			//默认都返回成功
		break;
	}
	return res;
}

void usbd_uvc_entry(void *parameter)
{
    rt_uint32_t e;
    stream *s = NULL;
    rt_uint32_t flen = 0;
    rt_uint32_t node_len = 0;
    rt_uint32_t out_len;
    rt_uint32_t packets;
    rt_uint32_t timeout = 0;
    rt_uint32_t index = 0;
    rt_uint8_t *jpeg_buf_addr = NULL;
    struct data_structure *get_f = NULL;
    struct ufunction *func = (struct ufunction *)parameter;
    static rt_uint8_t uvc_header[2] = { 0x02, 0x80 };

    usbd_uvc.buffer = os_malloc(VIDEO_BUFFER_SZ);

    if (usbd_uvc.buffer == RT_NULL)
    {
        LOG_E("malloc failed\r\n");
        goto __exit;
    }

    s = open_stream_available(R_RTP_JPEG, 0, 8, opcode_func, NULL);
    if(s == RT_NULL)
    {   
        LOG_D("open stream failed\n");
        goto __exit;
    }

    while(1)
    {
        if (rt_event_recv(usbd_uvc.event, EVENT_VIDEO_START | EVENT_VIDEO_STOP,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          1000, &e) != RT_EOK)
        {
            printf("usbd video wait evevnt\r\n");
            continue;
        }

        if (e & EVENT_VIDEO_START)
        {

        }else
        {
            continue;
        }

        LOG_D("video display start\r\n");

#if 1
        while(1){
            get_f = recv_real_data(s);
            flen = 0;
            if(get_f)
            {
                printf("U");
                flen = get_stream_real_data_len(get_f);
                node_len = GET_NODE_LEN(get_f);
                jpeg_buf_addr = (rt_uint8_t *)GET_JPG_BUF(get_f);   
                packets = 0;
                out_len = 0;
                memset(usbd_uvc.buffer,0,VIDEO_BUFFER_SZ);

                while(flen)
                {
                    if (rt_event_recv(usbd_uvc.event, EVENT_VIDEO_DATA | EVENT_VIDEO_STOP,
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                        1000, &e) != RT_EOK)
                    {
                        os_printf("uvc_video_thread: recv event timeout\n");
                    }

                    if (e & EVENT_VIDEO_DATA)
                    {
                        //printf("D");
                        if(flen > (VIDEO_PACKET_SIZE-sizeof(uvc_header)))
                        {
                            out_len = usbd_video_mjpeg_payload_header_fill((rt_uint8_t *)(jpeg_buf_addr + index), (VIDEO_PACKET_SIZE-sizeof(uvc_header)), usbd_uvc.buffer, (rt_uint8_t*)&uvc_header,sizeof(uvc_header));
                            //os_printf("1 out len:%d buffer:%x  [0]:%x [1]:%x [2]:%x [3]:%x\n",out_len,jpeg_buf_addr + index, usbd_uvc.buffer[0], usbd_uvc.buffer[1],usbd_uvc.buffer[2],usbd_uvc.buffer[3]);
                            usbd_uvc.ep->request.buffer = usbd_uvc.buffer;
                            usbd_uvc.ep->request.size = VIDEO_PACKET_SIZE;
                            usbd_uvc.ep->request.req_type = UIO_REQUEST_WRITE;

                            rt_usbd_io_request(func->device, usbd_uvc.ep, &usbd_uvc.ep->request);
                            flen -= (VIDEO_PACKET_SIZE-sizeof(uvc_header));
                            index += (VIDEO_PACKET_SIZE-sizeof(uvc_header));
                        }else{

                            out_len = usbd_video_mjpeg_payload_header_fill((rt_uint8_t *)(jpeg_buf_addr + index), flen, usbd_uvc.buffer, (rt_uint8_t*)&uvc_header,sizeof(uvc_header));
                            // os_printf("2 out len:%d buffer:%x  [0]:%x [1]:%x [2]:%x [3]:%x\n",out_len,jpeg_buf_addr + index, usbd_uvc.buffer[0], usbd_uvc.buffer[1],usbd_uvc.buffer[2],usbd_uvc.buffer[3]);
                            usbd_uvc.ep->request.buffer = usbd_uvc.buffer;
                            usbd_uvc.ep->request.size = out_len;
                            usbd_uvc.ep->request.req_type = UIO_REQUEST_WRITE;

                            rt_usbd_io_request(func->device, usbd_uvc.ep, &usbd_uvc.ep->request);
                            flen = 0;
                            index = 0; 
                            break;                               
                        }
                    }

                    if (e & EVENT_VIDEO_STOP)
                    {
                        printf("P");
                        flen = 0;
                        index = 0;
                        timeout = 0; 
                        break;
                    }

                }

                if(get_f)
                {
                    free_data(get_f);
                    get_f = NULL;
                }
                timeout = 0;
            }
            else
            {
                timeout++;
                printf("n");
                os_sleep_ms(5);
                if(timeout > 100)
                {
                    timeout = 0;
                    break;
                }
                    
            }
        }
            
#endif
    }

__exit:
    if (usbd_uvc.buffer)
        os_free(usbd_uvc.buffer);
    if (s)
        close_stream(s);

}


/**
 * This function will create a uvc function instance.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
ufunction_t rt_usbd_function_uvc_device_create(udevice_t device)
{
    ufunction_t func;
    uintf_t intf_vc, intf_vs;
    ualtsetting_t setting_vs_noep;
    ualtsetting_t setting_vc, setting_vs_ep;
    struct uvc_vs_ep_descriptor *vs_ep_desc_t;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
#ifdef RT_USB_DEVICE_COMPOSITE
    rt_usbd_device_set_interface_string(device, UVC_INTF_STR_INDEX, _ustring[2]);
#else
    /* set usb device string description */
    rt_usbd_device_set_string(device, _ustring);
#endif

    /* create a uac function */
    func = rt_usbd_function_new(device, &dev_desc, &ops);
    //not support HS
    //rt_usbd_device_set_qualifier(device, &dev_qualifier);

    usbd_uvc_init(func);

    /* create interface */
    intf_vc = rt_usbd_interface_new(device, _vc_interface_as_handler);                   //配置接口handler
    intf_vs = rt_usbd_interface_new(device, _vs_interface_as_handler);                   //配置接口handler

    os_printf("===sizeof:%d %d %d\n",sizeof(struct uvc_vc_noep_descriptor),sizeof(struct uvc_vs_noep_descriptor),sizeof(struct uvc_vs_ep_descriptor));

    /* create alternate setting */
    setting_vc = rt_usbd_altsetting_new(sizeof(struct uvc_vc_noep_descriptor));  
    setting_vs_noep = rt_usbd_altsetting_new(sizeof(struct uvc_vs_noep_descriptor));
    setting_vs_ep = rt_usbd_altsetting_new(sizeof(struct uvc_vs_ep_descriptor));
    /* config desc in alternate setting */
    rt_usbd_altsetting_config_descriptor(setting_vc, &vc_desc,
                                         (rt_off_t) & ((struct uvc_vc_noep_descriptor *)0)->intf_desc);
    os_printf("intf_desc[0]:%x\n",&(vc_desc.intf_desc));
    rt_usbd_altsetting_config_descriptor(setting_vs_noep, &vs_noep_desc, 
                                         (rt_off_t) & ((struct uvc_vs_noep_descriptor *)0)->intf_desc);
    rt_usbd_altsetting_config_descriptor(setting_vs_ep, &vs_ep_desc,
                                         (rt_off_t) & ((struct uvc_vs_ep_descriptor *)0)->intf_desc);
    /* configure the uac interface descriptor */
    _uvc_descriptor_config(setting_vc->desc, intf_vc->intf_num, setting_vs_noep->desc, intf_vs->intf_num);
    //_uac_samplerate_config(setting_vs_ep->desc, AUDIO_SAMPLERATE);

    /* create endpoint */
    vs_ep_desc_t = (struct uvc_vs_ep_descriptor *)setting_vs_ep->desc;
    usbd_uvc.ep = rt_usbd_endpoint_new(&vs_ep_desc_t->ep_desc, _ep_data_handler);    //配置端点handler

    /* add the endpoint to the alternate setting */
    rt_usbd_altsetting_add_endpoint(setting_vs_ep, usbd_uvc.ep);

    /* add the alternate setting to the interface, then set default setting of the interface */
    rt_usbd_interface_add_altsetting(intf_vc, setting_vc);
    rt_usbd_set_altsetting(intf_vc, 0);
    rt_usbd_interface_add_altsetting(intf_vs, setting_vs_noep);
    rt_usbd_interface_add_altsetting(intf_vs, setting_vs_ep);
    rt_usbd_set_altsetting(intf_vs, 0);

    /* add the interface to the uac function */
    rt_usbd_function_add_interface(func, intf_vc);
    rt_usbd_function_add_interface(func, intf_vs);

    return func;
}

static void usbd_video_set_resolution(uint32_t in_weight, uint32_t in_height, uint32_t out_weight, uint32_t out_height, uint32_t en)
{
    struct vpp_device *vpp_dev;
    struct scale_device *scale_dev;
    struct scale_device *scale3_dev;

	extern uint8 *yuvbuf;

    vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
    scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
    scale3_dev= (struct scale_device *)dev_get(HG_SCALE3_DEVID);  

    if(en)
    {
        photo_msg.out0_h = out_height;
        photo_msg.out0_w = out_weight;

        if((in_weight != out_weight) || (in_height != out_height))
        {
            os_printf("---------SCALE TO MJPEG---------\n");
            scale_from_vpp_to_jpg(scale_dev,(uint32)yuvbuf,in_weight,in_height,out_weight,out_height);
            jpg_cfg(HG_JPG0_DEVID,SCALER_DATA); 
        }
        else
        {
            os_printf("---------VPP TO MJPEG---------\n");
            jpg_cfg(HG_JPG0_DEVID,VPP_DATA0);   
        }
        start_jpeg();
        scale_close(scale3_dev);
        vpp_open(vpp_dev);
    }
    else
    {
        vpp_close(vpp_dev);
        scale_close(scale_dev);
        scale_open(scale3_dev);
        stop_jpeg();
    }

}


extern void usb_uvc_thread();
rt_bool_t usbd_uvc_init(struct ufunction *func)
{
    rt_uint8_t busid = 0;
    rt_thread_t usbd_uvc_thread;
    
    g_usbd_video[busid].ops.set_resolution = usbd_video_set_resolution;

    usbd_uvc.event = rt_event_create("usbd_uvc_event", RT_IPC_FLAG_FIFO);


    usbd_uvc_thread = rt_thread_create("usbd_uvc_thread",
                                        usbd_uvc_entry, func,
                                        1024,
                                        OS_TASK_PRIORITY_NORMAL, 10);

    if(usbd_uvc_thread != RT_NULL)
        rt_thread_startup(usbd_uvc_thread);
    
    usbd_uvc.thread = usbd_uvc_thread;

    return RT_EOK;
}


void rt_usbd_function_uvc_device_delete()
{

}


/*
 *  register uvc class
 */
static struct udclass uvc_device_class =
{
    .rt_usbd_function_create = rt_usbd_function_uvc_device_create,
    .rt_usbd_function_delete = rt_usbd_function_uvc_device_delete,
};

int rt_usbd_uvc_device_class_register(void)
{
    rt_usbd_class_register(&uvc_device_class);
    usbd_video_init_intf(0, INTERVAL, MAX_FRAME_SIZE(WIDTH,HEIGHT), MAX_PAYLOAD_SIZE);
    return 0;
}


#endif