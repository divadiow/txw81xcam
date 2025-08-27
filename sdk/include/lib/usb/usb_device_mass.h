/*****************************************************************************
* Module    : usb
* File      : usb_dev_mass.h
* Author    : 
* Function  : USB mass������һЩ����
*****************************************************************************/
#ifndef USB_DEV_mass_H
#define USB_DEV_mass_H

#include "usbd_mass_speed_optimize.h"

#ifdef __cplusplus
 extern "C" {
#endif
#define USB_DISK_BUF_COUNT 1
#define USB_DISK_BUF_SIZE 512




#define AUTO_EXPOSURE_MODE        2
#define AUTO_EXPOSURE_PRIORITY    3
#define EXPOSURE_TIME_ABS         4
#define ZOOM_ABS                  11
#define PAN_ABS                   13


#define BACKLIGHT_CONTROL                1
#define BRIGHTNESS_CONTROL        		 2
#define CONTRAST_CONTROL          		 3
#define GAIN_CONTROL                     4
#define POWER_LINE_FREQUENCY_CONTROL     5
#define HUE_CONTROL               		 6
#define SATURATION_CONTROL        		 7
#define SHARPNESS_CONTROL         		 8
#define GAMMA_CONTROL             		 9
#define WHITE_BALANCE_CONTROL     		 10

#define EXTENSION_C10             10
#define EXTENSION_C11             11


#define EXTENSION_UINT3_C1        1
#define EXTENSION_UINT3_C2        2
#define EXTENSION_UINT3_C3        3
#define EXTENSION_UINT3_C4        4


#define EXTENSION_UINT4_C1        1
#define EXTENSION_UINT4_C2        2
#define EXTENSION_UINT4_C3        3
#define EXTENSION_UINT4_C4        4
#define EXTENSION_UINT4_C5        5
#define EXTENSION_UINT4_C6        6
#define EXTENSION_UINT4_C7        7
#define EXTENSION_UINT4_C8        8
#define EXTENSION_UINT4_C9        9
#define EXTENSION_UINT4_C10        10
#define EXTENSION_UINT4_C11        11
#define EXTENSION_UINT4_C12        12
#define EXTENSION_UINT4_C13        13
#define EXTENSION_UINT4_C14        14
#define EXTENSION_UINT4_C15        15
#define EXTENSION_UINT4_C16        16
#define EXTENSION_UINT4_C17        17
#define EXTENSION_UINT4_C21        21
#define EXTENSION_UINT4_C22        22
#define EXTENSION_UINT4_C23        23


#define VS_PROBE_CONTROL          1
#define VS_COMMIT_CONTROL         2

//USB_DEV Audio音量调节
#define USB_DEV_AUDIO_STOP          0x80
#define USB_DEV_AUDIO_PREFILE       0x20
#define USB_DEV_AUDIO_NEXTFILE      0x10
#define USB_DEV_AUDIO_PLAYPAUSE     0x08
#define USB_DEV_AUDIO_MUTE          0x04
#define USB_DEV_AUDIO_VOLDOWN       0x02
#define USB_DEV_AUDIO_VOLUP         0x01
#define USB_DEV_AUDIO_NONE          0x00
// Video Class-Specific Request Codes
// (USB_Video_Class_1.1.pdf, A.8 Video Class-Specific Request Codes)
#define RC_UNDEFINED                               0x00
#define SET_CUR                                    0x01
#define GET_CUR                                    0x81
#define GET_MIN                                    0x82
#define GET_MAX                                    0x83
#define GET_RES                                    0x84
#define GET_LEN                                    0x85
#define GET_INFO                                   0x86
#define GET_DEF                                    0x87


#define CAM_IT_ID                                  0x01 //Input Terminal
#define CAM_PU_ID           					   0x02 //Processing Unit
#define CAM_OT_ID           					   0x03 //Output Terminal
#define CAM_EU_ID                                  0x04 //extension unit
#define CAM_EU6_ID                                 0x06 //extension unit

#define CAM_VS_ID                                  0x01 //VS ID


// (USB_Video_Class_1.1.pdf, A.1 Video Interface Class Code)
#define CC_VIDEO                                   0x0E
// Video Interface Subclass Codes
// (USB_Video_Class_1.1.pdf, A.2 Video Interface Subclass Code)
#define SC_UNDEFINED                               0x00
#define SC_VIDEOCONTROL                            0x01
#define SC_VIDEOSTREAMING                          0x02
#define SC_VIDEO_INTERFACE_COLLECTION              0x03



#define UVC_VC_INTERFACE_HEADER_DESC_SIZE(n)  (char)(12+n)
#define UVC_CAMERA_TERMINAL_DESC_SIZE(n)      (char)(15+n)
#define UVC_OUTPUT_TERMINAL_DESC_SIZE(n)      (char)(9+n)
#define USB_ENDPOINT_DESC_SIZE                (char)7
#define VS_FORMAT_UNCOMPRESSED_DESC_SIZE      (char)(27)
#define VC_EXTENSION_UNIT_LEN                 (char)(27)

#define UVC_VS_INTERFACE_INPUT_HEADER_DESC_SIZE(a,b) (char) (13+a*b)
#define UVC_VC_PROCESSING_UNIT_SIZE                0x0b
#define VS_COLOR_MATCHING_DESC_SIZE           (char)(6)


#define CS_INTERFACE                               0x24
#define VC_HEADER                                  0x01
#define USB_ENDPOINT_DESCRIPTOR_TYPE               0x05
#define VC_INPUT_TERMINAL                          0x02
#define VC_OUTPUT_TERMINAL                         0x03
#define VC_PROCESSING_UNIT                         0x05
#define VC_EXTENSION_UNIT                          0x06


#define USB_ENDPOINT_IN(addr)                      ((addr) | 0x80)
#define ITT_CAMERA                                 0x0201
#define TT_STREAMING               	             0x0101

/* bmAttributes in Endpoint Descriptor */
#define USB_ENDPOINT_TYPE_MASK                 0x03
#define USB_ENDPOINT_TYPE_CONTROL              0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS          0x01
#define USB_ENDPOINT_TYPE_BULK                 0x02
#define USB_ENDPOINT_TYPE_INTERRUPT            0x03
#define USB_ENDPOINT_SYNC_MASK                 0x0C
#define USB_ENDPOINT_SYNC_NO_SYNCHRONIZATION   0x00
#define USB_ENDPOINT_SYNC_ASYNCHRONOUS         0x04
//#define USB_ENDPOINT_SYNC_ADAPTIVE             0x08
#define USB_ENDPOINT_SYNC_SYNCHRONOUS          0x0C
#define USB_ENDPOINT_USAGE_MASK                0x30
#define USB_ENDPOINT_USAGE_DATA                0x00
#define USB_ENDPOINT_USAGE_FEEDBACK            0x10
#define USB_ENDPOINT_USAGE_IMPLICIT_FEEDBACK   0x20
#define USB_ENDPOINT_USAGE_RESERVED            0x30

//#define UVC_WIDTH                                         (unsigned int)640
//#define UVC_HEIGHT                                        (unsigned int)480
//#define VIDEO_PACKET_SIZE                                 (unsigned int)(UVC_WIDTH*3/2+2)//FOR YUY2 1216//
#define VIDEO_PACKET_SIZE                      1024


#define UVC_720P_W									      (unsigned int)1280
#define UVC_720P_H	                                      (unsigned int)720
#define VS_FRAME_UNCOMPRESSED_DESC_SIZE                   (char)(30)


#define UVC_960H_W									      (unsigned int)960
#define UVC_960H_H	                                      (unsigned int)720

#define UVC_VGA_W									      (unsigned int)640
#define UVC_VGA_H	                                      (unsigned int)480

#define UVC_360P_W									      (unsigned int)640
#define UVC_360P_H	                                      (unsigned int)360



#define MJPEG_FRAME_RATE                                        (char)(30)
#define YUV_FRAME_RATE                                          (char)(10)

#define WBVAL(x) ((x) & 0xFF),(((x) >> 8) & 0xFF)
#define DBVAL(x) ((x) & 0xFF),(((x) >> 8) & 0xFF),(((x) >> 16) & 0xFF),(((x) >> 24) & 0xFF)

#define UVC_VERSION                             0x0100      // UVC 1.0
#define VC_TERMINAL_SIZ (unsigned int)(UVC_VC_INTERFACE_HEADER_DESC_SIZE(1) +\
										UVC_CAMERA_TERMINAL_DESC_SIZE(3) +\
										UVC_VC_PROCESSING_UNIT_SIZE +\
										UVC_OUTPUT_TERMINAL_DESC_SIZE(0)+\
										VC_EXTENSION_UNIT_LEN)









typedef struct 
{

	uint32_t bufSize;		//不变,USB_DISK_BUF_SIZE,这个暂时规定为扇区大小吧
	uint32_t count;			//写入扇区的数量
	uint32_t lba;				//扇区偏移
	uint16_t isFree;		 //是否空闲
	uint16_t type;		 //1:是写   2:是读
	//uint8_t 	usbbuf[USB_DISK_BUF_SIZE];//改呀
	uint8_t 	*usbbuf;//改呀
	uint8_t 	*usb_pingpang_buf;
	struct os_semaphore sem;
	#if PINGPANG_BUF_EN
	struct usbd_mass_speed_info udisk_speed_info;
	#endif
}usb_disk_buf;

int32 usb_device_mass_uvc_open(struct usb_device *p_usb_d);
int32 usb_device_mass_uvc_close(struct usb_device *p_usb_d);
int32 usb_device_mass_auto_tx_null_pkt_disable(struct usb_device *p_usb_d);
int32 usb_device_mass_auto_tx_null_pkt_enable(struct usb_device *p_usb_d);
int32 usb_device_mass_write(struct usb_device *p_usb_d, int8 *buff, uint32 len);
int32 usb_device_mass_read(struct usb_device *p_usb_d, int8 *buff, uint32 len);
uint32 usb_device_mass_uvc_ep0_setup_irq(struct usb_device *p_usb_d, uint32 len);
void mass_ep_init(struct usb_device *p_usb_d);
int8_t get_cbw(void);
int32_t scsi_cmd_analysis(void);
void usb_bulk_init(void);
void clr_usb_disk();
void usb_disk_write_thread(void *d);
void init_usb_disk_buf();
int32_t scsi_cmd_analysis_flash(void);
void deinit_usb_disk_buf();
int32_t usb_uvc_tx(uint8_t epx, uint32_t adr, uint32_t len);
//int8_t init_usb_disk_buf(usb_disk_buf **disk);
//void clr_usb_disk(usb_disk_buf **disk);
#ifdef __cplusplus
}
#endif


#endif
