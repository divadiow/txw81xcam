/*****************************************************************************
* Module    : usb
* File      : usb_dev_mass.c
* Author    :
* Function  : USB mass  huge-ic defined, refer to ch9.h
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
#include "osal/irq.h"
#include "osal/sleep.h"
#include "hal/usb_device.h"
#include "dev/usb/hgusb20_v1_base.h"
#include "dev/usb/hgusb20_v1_dev_ep0.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
//#include "usb/device/usb20_v1/hgusb20_v1_hw.h"
#include "lib/usb/usb_device_mass.h"
#include "lib/sdhost/sdhost.h"
#include "lib/video/uvc/hg_usb_device.h"
#include "dev/usb/uvc_host.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/lcd/lcd.h"
#include "video_app.h"
#include "hal/scale.h"
#include "hal/vpp.h"
#include "flashdisk/flashdisk.h"

#ifndef RT_USING_USB_DEVICE
#ifdef  USB_DEVICE_MASS_OR_UVC 

#define UVC_DATA_EP                     2//BULK_EP
#define UVC_CTL_EP                      1
#define BULK_EP                         1
#define USB_MASS_TX_EP                  1
#define USB_MASS_RX_EP                  1
#define USB_MASS_EP_MAX_PKT_SIZE_HS        512
#define USB_MASS_EP_MAX_PKT_SIZE_FS        64

#define USB_MASS_IF_NUMS                1

#define USB_VID                         0x05E3

#ifndef USB_PID
#define USB_PID                         0x0751
#endif

#define		NO_SENSE					0
#define		INVALID_FIELD_IN_COMMAND	1
#define		NEW_MEDIUM_ARRIVEL			2
#define		WRITE_PROTECTED				3
//#define		PASSWORD_ERROR				4
//#define		USERNAME_ERROR				5
//#define		USER_PROHIBITTED			6
#define		MEDIUM_NOT_PRESENT			4  //7
#define		DATA_PHASE_ERROR			5  //8

extern struct hg_usb_device_test usb_test;
//设备描述符
const uint8_t tbl_usb_mass_device_descriptor[18] = {
    18,                 // Num bytes of the descriptor
    1,                  // Device Descriptor type
    0x00, 0x02,         // Revision of USB Spec. (in BCD)
    0x00,               // Class : user-defined
    0x00,               // Sub Class : user-defined
    0x00,               // Class specific protocol : user-defined
    0x40,               // Max packet size of Endpoint 0
    (USB_VID >> 0) & 0xff,
    (USB_VID >> 8) & 0xff,         // Vendor ID
    (USB_PID >> 0) & 0xff,
    (USB_PID >> 8) & 0xff,         // Product ID
    0x04, 0x14,         // Device Revision (in BCD)
    1,                  // Index of Manufacture string descriptor
    2,                  // Index of Product string descriptor
    3,                  // Index of Serial No. string descriptor
    1                   // Num Configurations, Must = 1
};

//配置描述符 通用配置
const uint8_t tbl_usb_mass_config_all_descriptor_gen[9] = {
    9,                  // Num bytes of this descriptor
    2,                  // Configuration descriptor type
    (sizeof(tbl_usb_mass_config_all_descriptor_gen) & 0xFF), 
    ((sizeof(tbl_usb_mass_config_all_descriptor_gen)>>8) & 0xFF),             // Total size of configuration
    USB_MASS_IF_NUMS,                  // Num Interface, 会根据最后实际接口的数量来配置
    1,                  // Configuration number
    0,                  // Index of Configuration string descriptor
    0x80,               // Configuration characteristics: BusPowerd
    0x31//200, //0x32,           // Max current, unit is 2mA
};

//配置描述符 U盘
const uint8_t tbl_usb_mass_config_all_descriptor_fs[23+9] = {
    9,                  // Num bytes of this descriptor
    2,                  // Configuration descriptor type
    (sizeof(tbl_usb_mass_config_all_descriptor_fs) & 0xFF), 
    ((sizeof(tbl_usb_mass_config_all_descriptor_fs)>>8) & 0xFF),             // Total size of configuration
    USB_MASS_IF_NUMS,                  // Num Interface, 会根据最后实际接口的数量来配置
    1,                  // Configuration number
    0,                  // Index of Configuration string descriptor
    0x80,               // Configuration characteristics: BusPowerd
    0x31,//200, //0x32,           // Max current, unit is 2mA

//Bulk MassStorage Interface 9 + 7 + 7= 23
    9,                  // Num bytes of this descriptor
    4,                  // Interface descriptor type
    0,                  // Interface Number，暂时填0，会根据最后实际配置来依次递加
    0,                  // Alternate interface number
    2,                  // Num endpoints of this interface
    0x08,               // Interface Class: Mass Storage
    0x06,               // Interface Sub Class: RCB
    0x50,               // Class specific protocol:
    0,                  // Index of Interface string descriptor

    7,                  // Num bytes of this descriptor
    5,                  // Endpoint descriptor type
    USB_MASS_TX_EP | 0x80, //BULKOUT_EP, // Endpoint number, bit7=0 shows OUT
    2,                  // Bulk endpoint
    (USB_MASS_EP_MAX_PKT_SIZE_FS >> 0) & 0xff, // Maximum packet size
    (USB_MASS_EP_MAX_PKT_SIZE_FS >> 8) & 0xff,
    0,                  // no use for bulk endpoint

    7,                  // Num bytes of this descriptor
    5,                  // Endpoint descriptor type
    USB_MASS_RX_EP,   // Endpoint number, bit7=1 shows IN
    2,                  // Bulk endpoint
    (USB_MASS_EP_MAX_PKT_SIZE_FS >> 0) & 0xff, // Maximum packet size
    (USB_MASS_EP_MAX_PKT_SIZE_FS >> 8) & 0xff,
    0,                  // No use for bulk endpoint
};
    

//配置描述符 U盘
const uint8_t tbl_usb_mass_config_all_descriptor_hs[23+9] = {
    9,                  // Num bytes of this descriptor
    2,                  // Configuration descriptor type
    (sizeof(tbl_usb_mass_config_all_descriptor_hs) & 0xFF), 
    ((sizeof(tbl_usb_mass_config_all_descriptor_hs)>>8) & 0xFF),             // Total size of configuration
    USB_MASS_IF_NUMS,                  // Num Interface, 会根据最后实际接口的数量来配置
    1,                  // Configuration number
    0,                  // Index of Configuration string descriptor
    0x80,               // Configuration characteristics: BusPowerd
    0x31,//200, //0x32,           // Max current, unit is 2mA

//Bulk MassStorage Interface 9 + 7 + 7= 23
    9,                  // Num bytes of this descriptor
    4,                  // Interface descriptor type
    0,                  // Interface Number，暂时填0，会根据最后实际配置来依次递加
    0,                  // Alternate interface number
    2,                  // Num endpoints of this interface
    0x08,               // Interface Class: Mass Storage
    0x06,               // Interface Sub Class: RCB
    0x50,               // Class specific protocol:
    0,                  // Index of Interface string descriptor

    7,                  // Num bytes of this descriptor
    5,                  // Endpoint descriptor type
    USB_MASS_TX_EP | 0x80, //BULKOUT_EP, // Endpoint number, bit7=0 shows OUT
    2,                  // Bulk endpoint
    (USB_MASS_EP_MAX_PKT_SIZE_HS >> 0) & 0xff, // Maximum packet size
    (USB_MASS_EP_MAX_PKT_SIZE_HS >> 8) & 0xff,
    0,                  // no use for bulk endpoint

    7,                  // Num bytes of this descriptor
    5,                  // Endpoint descriptor type
    USB_MASS_RX_EP,   // Endpoint number, bit7=1 shows IN
    2,                  // Bulk endpoint
    (USB_MASS_EP_MAX_PKT_SIZE_HS >> 0) & 0xff, // Maximum packet size
    (USB_MASS_EP_MAX_PKT_SIZE_HS >> 8) & 0xff,
    0,                  // No use for bulk endpoint
};

//语言
const uint8_t tbl_usb_mass_language_id[4] = {
    4,              // Num bytes of this descriptor
    3,              // String descriptor
    0x09, 0x04,     // Language ID
};

//厂商信息
const uint8_t tbl_usb_mass_str_manufacturer[24] = {
    24,             // Num bytes of this descriptor
    3,              // String descriptor
    'U',    0,
    'S',    0,
    'B',    0,
    ' ',    0,
    'S',    0,
    't',    0,
    'o',    0,
    'r',    0,
    'a',    0,
    'g',    0,
    'e',    0
};

const uint8_t tbl_usb_uvc_str_manufacturer[] = {
#if 1
	0x1E,
	0x03,
	0x6C,
	0x00,
	0x69,
	0x00,
	0x68,
	0x00,
	0x61,
	0x00,
	0x70,
	0x00,
	0x70,
	0x00,
	0x65,
	0x00,
	0x38,
	0x00,
	0x20,
	0x00,
	0x43,
	0x00,
	0x6F,
	0x00,
	0x72,
	0x00,
	0x70,
	0x00,
	0x2E,
	0x00,
#else
0x20,
0x03,
0x48,
0x00,
0x45,
0x00,
0x53,
0x00,
0x2D,
0x00,
0x39,
0x00,
0x37,
0x00,
0x33,
0x00,
0x34,
0x00,
0x2D,
0x00,
0x31,
0x00,
0x39,
0x00,
0x31,
0x00,
0x32,
0x00,
0x31,
0x00,
0x32,
0x00,

#endif
};



//产品信息
const uint8_t tbl_usb_mass_str_product[24] = {
    24,             // Num bytes of this descriptor
    3,              // String descriptor
    'U',    0,
    'S',    0,
    'B',    0,
    ' ',    0,
    'S',    0,
    't',    0,
    'o',    0,
    'r',    0,
    'a',    0,
    'g',    0,
    'e',    0
};

//产品信息
const uint8_t tbl_usb_uvc_str_product[] = {
#if 1
	0x2A,
	0x03,
	0x55,
	0x00,
	0x4E,
	0x00,
	0x49,
	0x00,
	0x51,
	0x00,
	0x55,
	0x00,
	0x45,
	0x00,
	0x53,
	0x00,
	0x4B,
	0x00,
	0x59,
	0x00,
	0x5F,
	0x00,
	0x43,
	0x00,
	0x41,
	0x00,
	0x52,
	0x00,
	0x5F,
	0x00,
	0x43,
	0x00,
	0x41,
	0x00,
	0x4D,
	0x00,
	0x45,
	0x00,
	0x52,
	0x00,
	0x41,
	0x00,
#else
0x24,
0x03,
0x49,
0x00,
0x6E,
0x00,
0x74,
0x00,
0x65,
0x00,
0x67,
0x00,
0x72,
0x00,
0x61,
0x00,
0x74,
0x00,
0x65,
0x00,
0x64,
0x00,
0x20,
0x00,
0x57,
0x00,
0x65,
0x00,
0x62,
0x00,
0x63,
0x00,
0x61,
0x00,
0x6D,
0x00,
#endif

};

const uint8_t tbl_usb_uvc_str2[] = {
	0x2A,
	0x03,
	0x49,
	0x00,
	0x6E,
	0x00,
	0x74,
	0x00,
	0x65,
	0x00,
	0x67,
	0x00,
	0x72,
	0x00,
	0x61,
	0x00,
	0x74,
	0x00,
	0x65,
	0x00,
	0x64,
	0x00,
	0x5F,
	0x00,
	0x57,
	0x00,
	0x65,
	0x00,
	0x62,
	0x00,
	0x63,
	0x00,
	0x61,
	0x00,
	0x6D,
	0x00,
	0x5F,
	0x00,
	0x48,
	0x00,
	0x44,
	0x00,

};

//序列号
const uint8_t tbl_usb_mass_str_serial_number[30] = {
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

const uint8_t tbl_usb_uvc_bos_descriptor[12] = {
	0x05,
	0x0F,
	0x0C,
	0x00,
	0x01,
	0x07,
	0x10,
	0x02,
	0x02,
	0x00,
	0x00,
	0x00,
};

const uint8_t tbl_usb_uvc_device_descriptor[18] = {
    18,                 // Num bytes of the descriptor
    1,                  // Device Descriptor type

	0x00, 0x02,
	0xEF,                       /*bDeviceClass*/ //239 Miscellaneous Device
	0x02,                       /*bDeviceSubClass*/
	0x01,                       /*bDeviceProtocol*/
    
    0x40,               // Max packet size of Endpoint 0
    (USB_VID >> 0) & 0xff,
    (USB_VID >> 8) & 0xff,         // Vendor ID
    (USB_PID >> 0) & 0xff,
    (USB_PID >> 8) & 0xff,         // Product ID
    0x12, 0x12,         // Device Revision (in BCD)
    1,                  // Index of Manufacture string descriptor
    2,                  // Index of Product string descriptor
    0,                  // Index of Serial No. string descriptor
    1                   // Num Configurations, Must = 1
};

const uint8_t tbl_usb_uvc_config_all_descriptor_gen[] = {
    9,                  // Num bytes of this descriptor
    2,                  // Configuration descriptor type
    0xa6, 0x01,             // Total size of configuration
    0x02,                  // Num Interface, 会根据最后实际接口的数量来配置
    0x01,                  // Configuration number
    4,                  // Index of Configuration string descriptor
    0x80,               // Configuration characteristics: BusPowerd, Remote Wakeup
    0XFA, //0x32,           // Max current, unit is 2mA

	0x08,       // bLength                  8
	11, // bDescriptorType         11
	0x00,                                      // bFirstInterface          0
	0x02,                                      // bInterfaceCount          2
	14,                                  // bFunctionClass          14 Video
	3,             // bFunctionSubClass        3 Video Interface Collection
	0,                     // bInterfaceProtocol       0 (protocol undefined)
	0x05,                                      // iFunction                2

	/* Standard VC Interface Descriptor  = interface 0 */
	9,                   // bLength                  9
	4,             // bDescriptorType          4
	0x00,                          // bInterfaceNumber         0 index of this interface (VC)
	0x00,                                      // bAlternateSetting        0 index of this setting
	0x01,                                      // bNumEndpoints            0 no endpoints
	0x0E,                                  // bInterfaceClass         14 Video
	0x01,                           // bInterfaceSubClass       1 Video Control
	0x00,                     // bInterfaceProtocol       0 (protocol undefined)
	0x05,                                      // iFunction                2	

	/* Class-specific VC Interface Descriptor */
	UVC_VC_INTERFACE_HEADER_DESC_SIZE(1),	   // bLength				  13    12 + 1 (header + 1*interface
	CS_INTERFACE,							   // bDescriptorType		  36 (INTERFACE)
	VC_HEADER,								   // bDescriptorSubtype	  1 (HEADER)
	WBVAL(UVC_VERSION), 					   // bcdUVC				  1.10 or 1.00
	WBVAL(VC_TERMINAL_SIZ),				               // wTotalLength			 header+units+terminals
	DBVAL(0x00e4e1c0),						   // dwClockFrequency	15.000000 MHz
	0x01,									   // bInCollection 		   1 one streaming interface
	0x01,									   // baInterfaceNr( 0) 	   1 VS interface 1 belongs to this VC interface

	/* Input Terminal Descriptor (Camera) */
	UVC_CAMERA_TERMINAL_DESC_SIZE(3),  
	CS_INTERFACE,                              // bDescriptorType         36 (INTERFACE)
	VC_INPUT_TERMINAL,                         // bDescriptorSubtype       2 (INPUT_TERMINAL)
	CAM_IT_ID,                                 // bTerminalID              1 ID of this Terminal
	WBVAL(ITT_CAMERA),                         // wTerminalType            0x0201 Camera Sensor
	0x00,                                      // bAssocTerminal           0 no Terminal associated
	0x00,                                      // iTerminal                0 no description available
	WBVAL(0x0000),                             // wObjectiveFocalLengthMin 0
	WBVAL(0x0000),                             // wObjectiveFocalLengthMax 0
	WBVAL(0x0000),                             // wOcularFocalLength       0
	0x03,                                      // bControlSize             3
	WBVAL(0x0a0e),							   // bmControls          0x0a0e no controls supported
//	0x0e, 0x0a,                                 	
	0x00,

	// Processing Unit Descriptor 
	UVC_VC_PROCESSING_UNIT_SIZE, //0x0C, // bLength
	CS_INTERFACE, // bDescriptorType : 0x24
	VC_PROCESSING_UNIT, // bDescriptorSubtype : 0x05 VC_PROCESSING_UNIT descriptor subtype
	CAM_PU_ID, // bUnitID : ID of this unit
	CAM_IT_ID, // bSourceID : This input pin of this unit is connected to the output pin of unit with ID 0x01
	WBVAL(0x0000), // wMaxMultiplier : Unused
	0x02, // bControlSize : Size of the bmControls field
	WBVAL(0x157F), // bmControls
	// [12] White Balance Temperature, Auto
	// [11] Hue, Auto
	// [10] Power Line Frequency
	// [8]  Backlight Compensation
	// [6]  White Balance Temperature
	// [4:0]Sharpness, Saturation, Hue, Contrast, Brightness
	0x00, // iProcessing : Unused
	//0x00, // bmVideoStandards : Unused UVC1.0	

	/* Output Terminal Descriptor */
	UVC_OUTPUT_TERMINAL_DESC_SIZE(0),		   // bLength				   9
	CS_INTERFACE,							   // bDescriptorType		  36 (INTERFACE)
	VC_OUTPUT_TERMINAL, 					   // bDescriptorSubtype	   3 (OUTPUT_TERMINAL)
	CAM_OT_ID,										// bTerminalID				3 ID of this Terminal
	WBVAL(TT_STREAMING),					   // wTerminalType 	  0x0101 USB streaming terminal
	0x00,									   // bAssocTerminal		   0 no Terminal assiciated
	0x04,	//CAM_IT_ID,									// bSourceID				1 input pin connected to output pin unit 1
	0x00,									   // iTerminal 			   0 no description available

	/* VC EXTENSION UNIT Descriptor	*/
	VC_EXTENSION_UNIT_LEN,	              /* bLength 27*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VC_EXTENSION_UNIT,					  /* bDescriptorSubType : VC_EXTENSION_UNIT subtype */
	CAM_EU_ID,								  /* bUnitID : First (and only) format descriptor */
	0x8c,								  /* guidExtensionCode : Vendor-specific code identifying the Extension Unit . 16byte*/
	0xa7,								  
	0x29,								 
	0x12,								  
	0xb4,								  
	0x47,								  
	0x94,								  
	0x40,
	0xb0,
	0xce,
	0xdb,
	0x07,
	0x38,
	0x6f,
	0xb9,
	0x38,	

	0x02,                                 /* bNumControls :number of controls in this extern unit   2*/                     
	0x01,								  /* bNrInPins */
	0x02,								  /* baSourceID :ID of this unit*/
	0x02,								  /* bcontrolsize size of the bmcontrols field*/
	WBVAL(0x0600),						  /* bmcontrols:size 2byte ,control channel has 16,and now set 06 00,so channel 10 and 11 is select*/                                
	0x00,

	/* Standard VS Isochronous Video data Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE, 				  // bLength				  7
	USB_ENDPOINT_DESCRIPTOR_TYPE,			  // bDescriptorType		  5 (ENDPOINT)
	USB_ENDPOINT_IN(UVC_CTL_EP),						// bEndpointAddress 	 0x82 EP 2 IN
	//USB_ENDPOINT_TYPE_ISOCHRONOUS,			// bmAttributes 			1 isochronous transfer type
	USB_ENDPOINT_TYPE_ISOCHRONOUS|USB_ENDPOINT_TYPE_BULK, /* bmAttributes : USB_ENDPOINT_TYPE_ISOCHRONOUS | USB_ENDPOINT_SYNC_ASYNCHRONOUS*/
	WBVAL(0X0010), 				              // wMaxPacketSize   16byte
	0x06,	

	//CS_ENDPOINT 
	0x05,// bLength
	0x25,// bDescriptorType
	0x03,// bDescriptorSubType
	0x10, 0x00,// wMaxTransferSize


	/* Standard VC Interface Descriptor  = interface 1 */
	9,                   // bLength                  9
	4,             // bDescriptorType          4
	0x01,                          // bInterfaceNumber         0 index of this interface (VC)
	0x00,                                      // bAlternateSetting        0 index of this setting
	0x00,                                      // bNumEndpoints            0 no endpoints
	0x0E,                                  // bInterfaceClass         14 Video
	0x02,                           // bInterfaceSubClass       1 Video Control
	0x00,                     // bInterfaceProtocol       0 (protocol undefined)
	0x00,                                      // iFunction                0	

	/* Class-specific VS Header Descriptor (Input) */
	UVC_VS_INTERFACE_INPUT_HEADER_DESC_SIZE(2,1),// bLength 			  15 13 + (2*1) (no specific controls used)
	CS_INTERFACE,							   // bDescriptorType		  36 (INTERFACE)
	VS_INPUT_HEADER,						   // bDescriptorSubtype	   1 (INPUT_HEADER)
	0x02,									   // bNumFormats			   2 two format descriptor follows
	WBVAL(0x0119),                              //size of VS HEAD           
	USB_ENDPOINT_IN(UVC_DATA_EP),						 // bEndPointAddress	  0x81 EP 1 IN
	0x00,									   // bmInfo				   0 no dynamic format change supported
	CAM_OT_ID,										// bTerminalLink			2 supplies terminal ID 2 (Output terminal)
	0x02,									   // bStillCaptureMethod	   0 NO supports still image capture
	0x01,									   // bTriggerSupport		   0 HW trigger supported for still image capture
	0x00,									   // bTriggerUsage 		   0 HW trigger initiate a still image capture
	0x01,									   // bControlSize			   1 one byte bmaControls field size
	0x00,									   // bmaControls(0)		   0 no VS specific controls
	0x00,

	/* Class-specific VS Format Descriptor	*/
	0x0B,	  /* bLength 27*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_FORMAT_MJPEG,					  /* bDescriptorSubType : VS_FORMAT_MJPEG subtype */
	0x01,								  /* bFormatIndex : First (and only) format descriptor */
	0x03,								  /* bNumFrameDescriptors : One frame descriptor for this format follows. */
	0x01,								  /* bmFlags : Uses fixed size samples.. */
	0x01,								  /* bDefaultFrameIndex : Default frame index is 1. */
	0x00,								  /* bAspectRatioX : Non-interlaced stream not required. */
	0x00,								  /* bAspectRatioY : Non-interlaced stream not required. */
	0x00,								  /* bmInterlaceFlags : Non-interlaced stream */
	0x00,								  /* bCopyProtect : No restrictions imposed on the duplication of this video stream. */

	/* Class-specific VS Frame Descriptor */
	VS_FRAME_UNCOMPRESSED_DESC_SIZE,	  /* bLength 30*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_FRAME_MJPEG, 			   /* bDescriptorSubType : VS_FRAME_UNCOMPRESSED */
	0x01,								  /* bFrameIndex : First (and only) frame descriptor */
	0x00,								  /* bmCapabilities : Still images using capture method 0 are supported at this frame setting.D1: Fixed frame-rate. */
	WBVAL(UVC_720P_W),						  /* wWidth (2bytes): Width of frame is 128 pixels. */
	WBVAL(UVC_720P_H),						  /* wHeight (2bytes): Height of frame is 64 pixels. */
	DBVAL(UVC_720P_W*UVC_720P_H*16*MJPEG_FRAME_RATE),				      /* dwMinBitRate (4bytes): Min bit rate in bits/s	*/ // 1280*720*16(bit)*30(frame) = 0x1a5e0000  //30fps
	DBVAL(UVC_720P_W*UVC_720P_H*16*MJPEG_FRAME_RATE),				      /* dwMaxBitRate (4bytes): Max bit rate in bits/s	*/ // 1280*720*16(bit)*30(frame) = 0x1a5e0000
	DBVAL(UVC_720P_W*UVC_720P_H*2),				      /* dwMaxVideoFrameBufSize (4bytes): Maximum video or still frame size, in bytes. */ // 1280*720*2 = 0x001c2000
	DBVAL(10000000/MJPEG_FRAME_RATE),					  /* dwDefaultFrameInterval : 1,000,000 * 100ns -> 10 FPS */ // 5 FPS -> 200ms -> 200,000 us -> 2,000,000 X 100ns = 0x001e8480
	0x01,								  /* bFrameIntervalType : Continuous frame interval */
	DBVAL(10000000/MJPEG_FRAME_RATE),					  /* dwMinFrameInterval : 1,000,000 ns	*100ns -> 10 FPS */

	/* Class-specific VS Frame Descriptor */
	VS_FRAME_UNCOMPRESSED_DESC_SIZE,	  /* bLength 30*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_FRAME_MJPEG, 			   /* bDescriptorSubType : VS_FRAME_UNCOMPRESSED */
	0x02,								  /* bFrameIndex : First (and only) frame descriptor */
	0x00,								  /* bmCapabilities : Still images using capture method 0 are supported at this frame setting.D1: Fixed frame-rate. */
	WBVAL(UVC_960H_W),						  /* wWidth (2bytes): Width of frame is 128 pixels. */
	WBVAL(UVC_960H_H),						  /* wHeight (2bytes): Height of frame is 64 pixels. */
	DBVAL(UVC_960H_W*UVC_960H_H*16*MJPEG_FRAME_RATE),				      /* dwMinBitRate (4bytes): Min bit rate in bits/s	*/ // 1280*720*16(bit)*30(frame) = 0x1a5e0000  //30fps
	DBVAL(UVC_960H_W*UVC_960H_H*16*MJPEG_FRAME_RATE),				      /* dwMaxBitRate (4bytes): Max bit rate in bits/s	*/ // 1280*720*16(bit)*30(frame) = 0x1a5e0000
	DBVAL(UVC_960H_W*UVC_960H_H*2),				      /* dwMaxVideoFrameBufSize (4bytes): Maximum video or still frame size, in bytes. */ // 1280*720*2 = 0x001c2000
	DBVAL(10000000/MJPEG_FRAME_RATE),					  /* dwDefaultFrameInterval : 1,000,000 * 100ns -> 10 FPS */ // 5 FPS -> 200ms -> 200,000 us -> 2,000,000 X 100ns = 0x001e8480
	0x01,								  /* bFrameIntervalType : Continuous frame interval */
	DBVAL(10000000/MJPEG_FRAME_RATE),					  /* dwMinFrameInterval : 1,000,000 ns	*100ns -> 10 FPS */
	
	/* Class-specific VS Frame Descriptor */
	VS_FRAME_UNCOMPRESSED_DESC_SIZE,	  /* bLength 30*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_FRAME_MJPEG, 			   /* bDescriptorSubType : VS_FRAME_UNCOMPRESSED */
	0x03,								  /* bFrameIndex : First (and only) frame descriptor */
	0x00,								  /* bmCapabilities : Still images using capture method 0 are supported at this frame setting.D1: Fixed frame-rate. */
	WBVAL(UVC_VGA_W),						  /* wWidth (2bytes): Width of frame is 128 pixels. */
	WBVAL(UVC_VGA_H),						  /* wHeight (2bytes): Height of frame is 64 pixels. */
	DBVAL(UVC_VGA_W*UVC_VGA_H*16*MJPEG_FRAME_RATE),				      /* dwMinBitRate (4bytes): Min bit rate in bits/s	*/ // 1280*720*16(bit)*30(frame) = 0x1a5e0000  //30fps
	DBVAL(UVC_VGA_W*UVC_VGA_H*16*MJPEG_FRAME_RATE),				      /* dwMaxBitRate (4bytes): Max bit rate in bits/s	*/ // 1280*720*16(bit)*30(frame) = 0x1a5e0000
	DBVAL(UVC_VGA_W*UVC_VGA_H*2),				      /* dwMaxVideoFrameBufSize (4bytes): Maximum video or still frame size, in bytes. */ // 1280*720*2 = 0x001c2000
	DBVAL(10000000/MJPEG_FRAME_RATE),					  /* dwDefaultFrameInterval : 1,000,000 * 100ns -> 10 FPS */ // 5 FPS -> 200ms -> 200,000 us -> 2,000,000 X 100ns = 0x001e8480
	0x01,								  /* bFrameIntervalType : Continuous frame interval */
	DBVAL(10000000/MJPEG_FRAME_RATE),					  /* dwMinFrameInterval : 1,000,000 ns	*100ns -> 10 FPS */
	
	/*Still Image Frame Descriptor*/
	0X12,                                  //LEN 
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_STILL_IMAGE_FRAME,
	0X00,                                 /*bEndpointAddress  ,if method 2 of still image capture is used ,this field shall be set zero*/
	3,									  /*bNumImageSizePatterns  ,Number of Image Size patterns of this format:*/
	WBVAL(UVC_720P_W),
	WBVAL(UVC_720P_H),
	WBVAL(UVC_960H_W),
	WBVAL(UVC_960H_H),	
	WBVAL(UVC_360P_W),
	WBVAL(UVC_360P_H),	
	0x00,								 /*num of compress pattern of this format :0*/


	/* Color Matching Descriptor */
	VS_COLOR_MATCHING_DESC_SIZE,		  /* bLength */
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_COLORFORMAT,								  /* bDescriptorSubType : VS_COLORFORMAT */
	0x01,								  /* bColorPrimarie : 1: BT.709, sRGB (default) */
	0x01,								  /* bTransferCharacteristics : 1: BT.709 (default) */
	0x04,								  /* bMatrixCoefficients : 1: BT. 709. */


	/* Class-specific VS Format Descriptor	*/
	VS_FORMAT_UNCOMPRESSED_DESC_SIZE,	  /* bLength 27*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_FORMAT_UNCOMPRESSED,			      /* bDescriptorSubType : VS_FORMAT_UNCOMPRESSED subtype */
	0x02,								  /* bFormatIndex : First (and only) format descriptor */
	0x03,								  /* bNumFrameDescriptors : One frame descriptor for this format follows. */
	0x59,								  /* bmFlags : Uses fixed size samples.. */
	0x55,								  /* bDefaultFrameIndex : Default frame index is 1. */
	0x59,								  /* bAspectRatioX : Non-interlaced stream not required. */
	0x32,								  /* bAspectRatioY : Non-interlaced stream not required. */
	0x00,								  /* bmInterlaceFlags : Non-interlaced stream */
	0x00,								  /* bCopyProtect : No restrictions imposed on the duplication of this video stream. */
	0x10,
	0x00,
	0x80,
	0x00,
	0x00,
	0xaa,
	0x00,
	0x38,
	0x9b,	
	0x71,
	0x10,	
	0x01,
	0x00,	
	0x00,
	0x00,
	0x00,


	/* Class-specific VS Frame Descriptor */
	VS_FRAME_UNCOMPRESSED_DESC_SIZE,	  /* bLength 30*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_FRAME_UNCOMPRESSED, 			   /* bDescriptorSubType : VS_FRAME_UNCOMPRESSED */
	0x01,								  /* bFrameIndex : First (and only) frame descriptor */
	0x00,								  /* bmCapabilities : Still images using capture method 0 are supported at this frame setting.D1: Fixed frame-rate. */
	WBVAL(UVC_720P_W),						  /* wWidth (2bytes): Width of frame is 128 pixels. */
	WBVAL(UVC_720P_H),						  /* wHeight (2bytes): Height of frame is 64 pixels. */
	DBVAL(UVC_720P_W*UVC_720P_H*16*YUV_FRAME_RATE),				      /* dwMinBitRate (4bytes): Min bit rate in bits/s	*/ // 1280*720*16(bit)*30(frame) = 0x1a5e0000  //30fps
	DBVAL(UVC_720P_W*UVC_720P_H*16*YUV_FRAME_RATE),				      /* dwMaxBitRate (4bytes): Max bit rate in bits/s	*/ // 1280*720*16(bit)*30(frame) = 0x1a5e0000
	DBVAL(UVC_720P_W*UVC_720P_H*2),				      /* dwMaxVideoFrameBufSize (4bytes): Maximum video or still frame size, in bytes. */ // 1280*720*2 = 0x001c2000
	DBVAL(10000000/YUV_FRAME_RATE),					  /* dwDefaultFrameInterval : 1,000,000 * 100ns -> 10 FPS */ // 5 FPS -> 200ms -> 200,000 us -> 2,000,000 X 100ns = 0x001e8480
	0x01,								  /* bFrameIntervalType : Continuous frame interval */
	DBVAL(10000000/YUV_FRAME_RATE),					  /* dwMinFrameInterval : 1,000,000 ns	*100ns -> 10 FPS */

	/* Class-specific VS Frame Descriptor */
	VS_FRAME_UNCOMPRESSED_DESC_SIZE,	  /* bLength 30*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_FRAME_UNCOMPRESSED, 			   /* bDescriptorSubType : VS_FRAME_UNCOMPRESSED */
	0x02,								  /* bFrameIndex : First (and only) frame descriptor */
	0x00,								  /* bmCapabilities : Still images using capture method 0 are supported at this frame setting.D1: Fixed frame-rate. */
	WBVAL(UVC_960H_W),						  /* wWidth (2bytes): Width of frame is 128 pixels. */
	WBVAL(UVC_960H_H),						  /* wHeight (2bytes): Height of frame is 64 pixels. */
	DBVAL(UVC_960H_W*UVC_960H_H*16*(YUV_FRAME_RATE+5)),				      /* dwMinBitRate (4bytes): Min bit rate in bits/s	*/ // 1280*720*16(bit)*10(frame) = 0x1a5e0000  //10fps
	DBVAL(UVC_960H_W*UVC_960H_H*16*(YUV_FRAME_RATE+5)),				      /* dwMaxBitRate (4bytes): Max bit rate in bits/s	*/ // 1280*720*16(bit)*10(frame) = 0x1a5e0000
	DBVAL(UVC_960H_W*UVC_960H_H*2),				      /* dwMaxVideoFrameBufSize (4bytes): Maximum video or still frame size, in bytes. */ // 1280*720*2 = 0x001c2000
	DBVAL(10000000/(YUV_FRAME_RATE+5)),					  /* dwDefaultFrameInterval : 1,000,000 * 100ns -> 10 FPS */ // 5 FPS -> 200ms -> 200,000 us -> 2,000,000 X 100ns = 0x001e8480
	0x01,								  /* bFrameIntervalType : Continuous frame interval */
	DBVAL(10000000/(YUV_FRAME_RATE+5)),					  /* dwMinFrameInterval : 1,000,000 ns	*100ns -> 10 FPS */

	/* Class-specific VS Frame Descriptor */
	VS_FRAME_UNCOMPRESSED_DESC_SIZE,	  /* bLength 30*/
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_FRAME_UNCOMPRESSED, 			   /* bDescriptorSubType : VS_FRAME_UNCOMPRESSED */
	0x03,								  /* bFrameIndex : First (and only) frame descriptor */
	0x00,								  /* bmCapabilities : Still images using capture method 0 are supported at this frame setting.D1: Fixed frame-rate. */
	WBVAL(UVC_VGA_W),						  /* wWidth (2bytes): Width of frame is 128 pixels. */
	WBVAL(UVC_VGA_H),						  /* wHeight (2bytes): Height of frame is 64 pixels. */
	DBVAL(UVC_VGA_W*UVC_VGA_H*16*(YUV_FRAME_RATE+20)),				      /* dwMinBitRate (4bytes): Min bit rate in bits/s	*/ // 960*720*16(bit)*15(frame)
	DBVAL(UVC_VGA_W*UVC_VGA_H*16*(YUV_FRAME_RATE+20)),				      /* dwMaxBitRate (4bytes): Max bit rate in bits/s	*/ // 960*720*16(bit)*15(frame) 
	DBVAL(UVC_VGA_W*UVC_VGA_H*2),				      /* dwMaxVideoFrameBufSize (4bytes): Maximum video or still frame size, in bytes. */ // 960*720*2
	DBVAL(10000000/(YUV_FRAME_RATE+20)),					  /* dwDefaultFrameInterval : 1,000,000 * 100ns -> 10 FPS */ // 5 FPS -> 200ms -> 200,000 us -> 2,000,000 X 100ns = 0x001e8480
	0x01,								  /* bFrameIntervalType : Continuous frame interval */
	DBVAL(10000000/(YUV_FRAME_RATE+20)),					  /* dwMinFrameInterval : 1,000,000 ns	*100ns -> 10 FPS */

	/*Still Image Frame Descriptor*/
	0X12,                                  //LEN 
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_STILL_IMAGE_FRAME,
	0X00,                                 /*bEndpointAddress  ,if method 2 of still image capture is used ,this field shall be set zero*/
	3,									  /*bNumImageSizePatterns  ,Number of Image Size patterns of this format:*/
	WBVAL(UVC_720P_W),
	WBVAL(UVC_720P_H),
	WBVAL(UVC_960H_W),
	WBVAL(UVC_960H_H),	
	WBVAL(UVC_VGA_W),
	WBVAL(UVC_VGA_H),	
	0x00,								 /*num of compress pattern of this format :0*/
	
	/* Color Matching Descriptor */
	VS_COLOR_MATCHING_DESC_SIZE,		  /* bLength */
	CS_INTERFACE,						  /* bDescriptorType : CS_INTERFACE */
	VS_COLORFORMAT,								  /* bDescriptorSubType : VS_COLORFORMAT */
	0x01,								  /* bColorPrimarie : 1: BT.709, sRGB (default) */
	0x01,								  /* bTransferCharacteristics : 1: BT.709 (default) */
	0x04,								  /* bMatrixCoefficients : 1: BT. 709. */

	/*interface descriptor*/
	0x09,								  /* bLength */
	0x04,								  /* bDescriptoType */
	0x01,                                 /* bInterfaceNum   1 */
	0x01,                                 /* bAlternateSetting   1 */ 
	0x01,                                 /* bNumEndpoints       1 */
	CC_VIDEO,								  /* bInterfaceclass*/
	SC_VIDEOSTREAMING,								  /* bInterfaceSubClass*/
	0x00,								  /* bInterfaceProtocol*/
	0x00,								  /* bConfiguration*/

	/* Standard VS Isochronous Video data Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE, 				  // bLength				  7
	USB_ENDPOINT_DESCRIPTOR_TYPE,			  // bDescriptorType		  5 (ENDPOINT)
	USB_ENDPOINT_IN(UVC_DATA_EP),						// bEndpointAddress 	 0x81 EP 1 IN
	//USB_ENDPOINT_TYPE_BULK,			// bmAttributes 			1 isochronous transfer type
	USB_ENDPOINT_TYPE_ISOCHRONOUS|USB_ENDPOINT_SYNC_ASYNCHRONOUS, /* bmAttributes : USB_ENDPOINT_TYPE_ISOCHRONOUS | USB_ENDPOINT_SYNC_ASYNCHRONOUS*/
	WBVAL(0x13fc), 				              // wMaxPacketSize :max packet is 0x400[0,10], ISO tran 1 time[11,12] 
	0x01,									  // bInterval				  1 one frame interval
};


typedef struct 
{
	uint8 info[1];				//func信息位
	uint8 res[8];				//分辨率值（调的等级间隔）
	uint8 min[8];				//最小值
	uint8 max[8];				//最大值
	uint8 def[8];				//默认值
	uint8 cur[8];              //当前值
	uint8 len[2];              //数据长度
}uvc_func_msg;

uint8_t uvc_cur_buf[64];
uint8_t video_set_cur[48];

uvc_func_msg uvc_exposure_time={
	.info = {0x0f},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x72,0x02,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x9C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0xa6,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_ae_priority={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	
};

uvc_func_msg uvc_zoom_abs={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},		
};

uvc_func_msg uvc_pantilt={
	.info = {0x03},
	.res  = {0x10,0x0e,0x00,0x00,0x10,0x0e,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xf0,0xf1,0x0f,0x0e,0xf0,0xf1,0x0f,0x0e},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_brightness={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0xc0,0xff,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_contrast={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x5F,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_hue={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x30,0xf8,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xD0,0x07,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_saturation={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x64,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x2D,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_sharpness={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_gamma={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x2c,0x01,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_whitebalance={
	.info = {0x0f},
	.res  = {0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0xf0,0x0a,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x64,0x19,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0xf8,0x11,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0xf8,0x11,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_backlight={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	
};

uvc_func_msg uvc_gain={
	.info = {0x0F},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	
};

uvc_func_msg uvc_powerline_frequency={
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};
#if 0
uvc_func_msg uvc_extension_unit4_c10={
	.len  = {0x08,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c11={
	.len  = {0x08,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};
#endif
uvc_func_msg uvc_extension_unit3_c1={
	.len  = {0x04,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0xfc,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x02,0x00,0xff,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0xfe,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0xfe,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit3_c2={
	.len  = {0x08,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};


uvc_func_msg uvc_extension_unit3_c3={
	.len  = {0x08,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit3_c4={
	.len  = {0x04,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c1={
	.len  = {0x01,0x00},
	.info = {0x01},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};


uvc_func_msg uvc_extension_unit4_c2={
	.len  = {0x02,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c3={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c4={
	.len  = {0x02,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c5={
	.len  = {0x02,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c6={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c7={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x0d,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c8={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c9={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c10={
	.len  = {0x08,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c11={
	.len  = {0x01,0x00},
	.info = {0x01},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c12={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c13={
	.len  = {0x04,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c14={
	.len  = {0x04,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c15={
	.len  = {0x00,0x04},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c16={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c17={
	.len  = {0x02,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c21={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c22={
	.len  = {0x01,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

uvc_func_msg uvc_extension_unit4_c23={
	.len  = {0x03,0x00},
	.info = {0x03},
	.res  = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.min  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.max  = {0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00},
	.def  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.cur  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};


struct VIDEO_COMMIT uvc_probe_control={
	.bmhint              = 0,
	.bFormatIndex        = 1,
	.bFrameIndex         = 1,
	.dwFrameInterval     = 10000000/MJPEG_FRAME_RATE,
	.wKeyFrameRate       = 0,
	.wPFrameRate         = 0,
	.wCompQuality        = 0,
	.wCompWindowSize     = 0,
	.wDelay              = 0x20,
	.dwMaxVideoFrameSize = 0x1c2000,
	.dwMaxPayloadTSize   = 0xbf4,
};

struct VIDEO_COMMIT uvc_probe_control_def={
	.bmhint              = 0,
	.bFormatIndex        = 1,
	.bFrameIndex         = 1,
	.dwFrameInterval     = 10000000/MJPEG_FRAME_RATE,
	.wKeyFrameRate       = 0,
	.wPFrameRate         = 0,
	.wCompQuality        = 0,
	.wCompWindowSize     = 0,
	.wDelay              = 0x20,
	.dwMaxVideoFrameSize = 0x1c2000,
	.dwMaxPayloadTSize   = 0xbf4,
};




static const struct usb_device_cfg usb_dev_mass_cfg = {
    .vid        = USB_VID,
    .pid        = USB_PID,
    .speed      = USB_SPEED_HIGH,
    .p_device_descriptor = (uint8 *)tbl_usb_mass_device_descriptor,
//    .p_config_descriptor_head = (uint8 *)tbl_usb_mass_config_all_descriptor_gen,

    .p_config_desc_hs = (uint8 *)tbl_usb_mass_config_all_descriptor_hs,
    .p_config_desc_fs = (uint8 *)tbl_usb_mass_config_all_descriptor_fs,
    .config_desc_len = sizeof(tbl_usb_mass_config_all_descriptor_hs),
    .interface_num  =   1,

//    .p_language_id = (uint8 *)tbl_usb_mass_language_id,
//    .language_id_len = sizeof(tbl_usb_mass_language_id),
//    .p_str_manufacturer = (uint8 *)tbl_usb_mass_str_manufacturer,
//    .str_manufacturer_len = sizeof(tbl_usb_mass_str_manufacturer),
//    .p_str_product = (uint8 *)tbl_usb_mass_str_product,
//    .str_product_len = sizeof(tbl_usb_mass_str_product),
//    .p_str_serial_number = (uint8 *)tbl_usb_mass_str_serial_number,
//    .str_serial_number_len = sizeof(tbl_usb_mass_str_serial_number),

    .ep_nums              =  2,
    .ep_cfg[0].ep_id      =  USB_MASS_RX_EP,
    .ep_cfg[0].ep_type    =  USB_ENDPOINT_XFER_BULK,
    .ep_cfg[0].ep_dir_tx  =  0,
    .ep_cfg[0].max_packet_size_hs      =  USB_MASS_EP_MAX_PKT_SIZE_HS,
    .ep_cfg[0].max_packet_size_fs      =  USB_MASS_EP_MAX_PKT_SIZE_FS,
    .ep_cfg[1].ep_id      =  USB_MASS_TX_EP,
    .ep_cfg[1].ep_type    =  USB_ENDPOINT_XFER_BULK,
    .ep_cfg[1].ep_dir_tx  =  1,
    .ep_cfg[1].max_packet_size_hs      =  USB_MASS_EP_MAX_PKT_SIZE_HS,
    .ep_cfg[1].max_packet_size_fs      =  USB_MASS_EP_MAX_PKT_SIZE_FS,
};


static const struct usb_device_cfg usb_dev_uvc_cfg = {
    .vid        = 0XA012,
    .pid        = 0x6001,
    .speed      = USB_SPEED_HIGH,
    .p_device_descriptor = (uint8 *)tbl_usb_uvc_device_descriptor,
//    .p_config_descriptor_head = (uint8 *)tbl_usb_mass_config_all_descriptor_gen,

    .p_config_desc_hs = (uint8 *)tbl_usb_uvc_config_all_descriptor_gen,
    .config_desc_len = sizeof(tbl_usb_uvc_config_all_descriptor_gen),
    .interface_num  =   1,

//    .p_language_id = (uint8 *)tbl_usb_mass_language_id,
//    .language_id_len = sizeof(tbl_usb_mass_language_id),
//    .p_str_manufacturer = (uint8 *)tbl_usb_mass_str_manufacturer,
//    .str_manufacturer_len = sizeof(tbl_usb_mass_str_manufacturer),
//    .p_str_product = (uint8 *)tbl_usb_mass_str_product,
//    .str_product_len = sizeof(tbl_usb_mass_str_product),
//    .p_str_serial_number = (uint8 *)tbl_usb_mass_str_serial_number,
//    .str_serial_number_len = sizeof(tbl_usb_mass_str_serial_number),

    .ep_nums              =  2,
    .ep_cfg[0].ep_id      =  UVC_CTL_EP,
    .ep_cfg[0].ep_type    =  USB_ENDPOINT_XFER_BULK,
    .ep_cfg[0].ep_dir_tx  =  1,
    .ep_cfg[0].max_packet_size_hs      =  USB_MASS_EP_MAX_PKT_SIZE_HS,
    .ep_cfg[0].max_packet_size_fs      =  USB_MASS_EP_MAX_PKT_SIZE_FS,
    .ep_cfg[1].ep_id      =  UVC_DATA_EP,
    .ep_cfg[1].ep_type    =  USB_ENDPOINT_XFER_ISOC,
    .ep_cfg[1].ep_dir_tx  =  1,
    .ep_cfg[1].max_packet_size_hs      =  1024,
    .ep_cfg[1].max_packet_size_fs      =  USB_MASS_EP_MAX_PKT_SIZE_FS,
};

/** 
  * @brief  usb device retrive rx packet to usb_ctrl.cmd
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note
  */
static uint32_t usb_device_mass_uvc_ep0_get_request(struct hgusb20_dev *p_dev, uint8 len)
{
    uint32_t usb_rx_counter = len;
//    struct hgusb20_dev_hw *hw = (struct hgusb20_dev_hw *)p_dev->usb_hw;

    uint8_t *pBuf = (uint8_t *)&p_dev->usb_ctrl.cmd;

    os_memcpy(pBuf, (uint32)p_dev->usb_ep0_rxbuf, usb_rx_counter);


    return usb_rx_counter;
}

bool usb_device_mass_ep0_get_descriptor(struct hgusb20_dev *p_dev)
{
    switch (USB_REQUEST_DESCRIPTOR_TYPE) {       //Request Descriptor Type
        case DEVICE_DESCRIPTOR:
            //设备描述符
            hgusb20_dev_cfg_desc_prepare(p_dev);
            return hgusb20_dev_ep0_descriptor(p_dev, (char *)p_dev->p_com_desc, sizeof(tbl_usb_mass_device_descriptor));
            //return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_wifi_device_descriptor, sizeof(tbl_usb_wifi_device_descriptor));
        case CONFIGURATION_DESCRIPTOR:
            //配置描述符
            return hgusb20_dev_ep0_descriptor(p_dev, p_dev->p_config_desc, get_unaligned_le16((const void *)&p_dev->p_config_desc[2]));
        case STRING_DESCRIPTOR:
            //字符串
            switch (USB_REQUEST_DESCRIPTOR_INDEX) {
                case 0:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_mass_language_id, sizeof(tbl_usb_mass_language_id));
                case 1:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_mass_str_manufacturer, sizeof(tbl_usb_mass_str_manufacturer));
                case 2:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_mass_str_product, sizeof(tbl_usb_mass_str_product));
                case 3:
                    return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_mass_str_serial_number, sizeof(tbl_usb_mass_str_serial_number));
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

bool usb_device_uvc_ep0_get_descriptor(struct hgusb20_dev *p_dev)
{
	switch (USB_REQUEST_DESCRIPTOR_TYPE) {		 //Request Descriptor Type
		case DEVICE_DESCRIPTOR:
			//设备描述符
			hgusb20_dev_cfg_desc_prepare(p_dev);
			return hgusb20_dev_ep0_descriptor(p_dev, (char *)p_dev->p_com_desc, sizeof(tbl_usb_uvc_device_descriptor));
			//return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_wifi_device_descriptor, sizeof(tbl_usb_wifi_device_descriptor));
		case CONFIGURATION_DESCRIPTOR:
			//配置描述符
			return hgusb20_dev_ep0_descriptor(p_dev, p_dev->p_config_desc, get_unaligned_le16((const void *)&p_dev->p_config_desc[2]));
		case STRING_DESCRIPTOR:
			//字符串
			switch (USB_REQUEST_DESCRIPTOR_INDEX) {
				case 0:
					return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_mass_language_id, sizeof(tbl_usb_mass_language_id));
				case 1:
					return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_uvc_str_manufacturer, sizeof(tbl_usb_uvc_str_manufacturer));
				case 2:
					return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_uvc_str2, sizeof(tbl_usb_uvc_str2));
				case 3:
					return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_mass_str_serial_number, sizeof(tbl_usb_mass_str_serial_number));
				case 4:
					return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_uvc_str_product, sizeof(tbl_usb_uvc_str_product));
				case 5:
					return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_mass_str_manufacturer, sizeof(tbl_usb_mass_str_manufacturer));
				default:
					return FALSE;
			}
//	  case INTERFACE_DESCRIPTOR:
//	  case ENDPOINT_DESCRIPTOR:
		case DEVICE_QUALIFIER_DESCRIPTOR:
			//usb_device_qualifier 限定描述符
			memcpy(p_dev->usb_ep0_rxbuf, (char *)p_dev->cfg->p_device_descriptor, 8);
			p_dev->usb_ep0_rxbuf[0] = 10;
			p_dev->usb_ep0_rxbuf[1] = DEVICE_QUALIFIER_DESCRIPTOR;
			p_dev->usb_ep0_rxbuf[8] = p_dev->cfg->p_device_descriptor[17];
			p_dev->usb_ep0_rxbuf[9] = 0;
			return hgusb20_dev_ep0_descriptor(p_dev, p_dev->usb_ep0_rxbuf, 10);

		case BOS_DEVICE_DESCRIPTOR:
			return hgusb20_dev_ep0_descriptor(p_dev, (char *)tbl_usb_uvc_bos_descriptor, sizeof(tbl_usb_uvc_bos_descriptor));
		default:
			return FALSE;
	}
}

void printf_video_cmd(struct hgusb20_dev *p_dev){
	uint8 *buf;
	uint8 len;
	buf = (uint8 *)&p_dev->usb_ctrl.cmd;
	printf("\r\n");
	for(len = 0;len < 8;len++){
		printf("0x%02x ",buf[len]);
	}
	printf("\r\n");
}

void set_video_data(struct hgusb20_dev *p_dev,uint8* buf,uint8 len){
	uint8 itk;
	for(itk = 0;itk < len;itk++){
		uvc_cur_buf[itk] = buf[itk];
	}
}

uint8_t usb_vedio_get_def(struct hgusb20_dev *p_dev){
		switch(p_dev->usb_ctrl.cmd.index){
			case (CAM_IT_ID<<8):
				switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
					//case AUTO_EXPOSURE_MODE:
					//	printf_video_cmd(usb_sw);
					//break;
					case AUTO_EXPOSURE_PRIORITY:
						set_video_data(p_dev,uvc_ae_priority.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXPOSURE_TIME_ABS:
						set_video_data(p_dev,uvc_exposure_time.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case ZOOM_ABS:
						set_video_data(p_dev,uvc_zoom_abs.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case PAN_ABS:
						set_video_data(p_dev,uvc_pantilt.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;					
					default:
						printf_video_cmd(p_dev);
						p_dev->usb_ctrl.cmd.length = 0;
					break;	
				}
				break;
			case (CAM_PU_ID<<8):
				switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
					case BACKLIGHT_CONTROL:
						set_video_data(p_dev,uvc_backlight.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;							
					case BRIGHTNESS_CONTROL:
						set_video_data(p_dev,uvc_brightness.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;							
					case CONTRAST_CONTROL:
						set_video_data(p_dev,uvc_contrast.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;		
					case GAIN_CONTROL:
						set_video_data(p_dev,uvc_gain.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case POWER_LINE_FREQUENCY_CONTROL:
						set_video_data(p_dev,uvc_powerline_frequency.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;							
					case HUE_CONTROL:
						set_video_data(p_dev,uvc_hue.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;							
					case SATURATION_CONTROL:
						set_video_data(p_dev,uvc_saturation.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;							
					case SHARPNESS_CONTROL:
						set_video_data(p_dev,uvc_sharpness.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;							
					case GAMMA_CONTROL:
						set_video_data(p_dev,uvc_gamma.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;							
					case WHITE_BALANCE_CONTROL:	
						set_video_data(p_dev,uvc_whitebalance.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;							
					default:
						printf_video_cmd(p_dev);						
					return 0;							
				}
				break;
			case (CAM_OT_ID<<8):
				switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
					case EXTENSION_UINT3_C1:
						set_video_data(p_dev,uvc_extension_unit3_c1.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT3_C2:
						set_video_data(p_dev,uvc_extension_unit3_c2.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;					
					case EXTENSION_UINT3_C3:
						set_video_data(p_dev,uvc_extension_unit3_c3.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT3_C4:
						set_video_data(p_dev,uvc_extension_unit3_c4.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					default:
						printf_video_cmd(p_dev);
					return 0; 
				}
				break;			
			case (CAM_EU_ID<<8):
				switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
					case EXTENSION_UINT4_C1:
						set_video_data(p_dev,uvc_extension_unit4_c1.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C2:
						set_video_data(p_dev,uvc_extension_unit4_c2.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C3:
						set_video_data(p_dev,uvc_extension_unit4_c3.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C4:
						set_video_data(p_dev,uvc_extension_unit4_c4.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;					
					case EXTENSION_UINT4_C5:
						set_video_data(p_dev,uvc_extension_unit4_c5.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C6:
						set_video_data(p_dev,uvc_extension_unit4_c6.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C7:
						set_video_data(p_dev,uvc_extension_unit4_c7.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C8:
						set_video_data(p_dev,uvc_extension_unit4_c8.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C9:
						set_video_data(p_dev,uvc_extension_unit4_c9.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C10:
						set_video_data(p_dev,uvc_extension_unit4_c10.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C11:
						set_video_data(p_dev,uvc_extension_unit4_c11.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C12:
						set_video_data(p_dev,uvc_extension_unit4_c12.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;					
					case EXTENSION_UINT4_C13:
						set_video_data(p_dev,uvc_extension_unit4_c13.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C14:
						set_video_data(p_dev,uvc_extension_unit4_c14.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C15:
						set_video_data(p_dev,uvc_extension_unit4_c15.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C16:
						set_video_data(p_dev,uvc_extension_unit4_c16.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C17:
						set_video_data(p_dev,uvc_extension_unit4_c17.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C21:
						set_video_data(p_dev,uvc_extension_unit4_c21.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C22:
						set_video_data(p_dev,uvc_extension_unit4_c22.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C23:
						set_video_data(p_dev,uvc_extension_unit4_c23.def,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					default:
						printf_video_cmd(p_dev);
					return 0;
				}
				break;					
			default:
				printf_video_cmd(p_dev);
			return 0;
		}
		return 1;
}

uint8_t usb_vedio_get_cur(struct hgusb20_dev *p_dev){

	switch(p_dev->usb_ctrl.cmd.index){
		case (CAM_IT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				//case AUTO_EXPOSURE_MODE:
				//	printf_video_cmd(usb_sw);
				//break;
				case AUTO_EXPOSURE_PRIORITY:
					set_video_data(p_dev,uvc_ae_priority.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXPOSURE_TIME_ABS:
					set_video_data(p_dev,uvc_exposure_time.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case ZOOM_ABS:
					set_video_data(p_dev,uvc_zoom_abs.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case PAN_ABS:
					set_video_data(p_dev,uvc_pantilt.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				default:
					uvc_cur_buf[0] = 0x06;
					printf_video_cmd(p_dev);						
				break;	
			}
			break;
		case (CAM_PU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case BACKLIGHT_CONTROL:
					set_video_data(p_dev,uvc_backlight.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case BRIGHTNESS_CONTROL:
					set_video_data(p_dev,uvc_brightness.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case CONTRAST_CONTROL:
					set_video_data(p_dev,uvc_contrast.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case GAIN_CONTROL:
					set_video_data(p_dev,uvc_gain.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case POWER_LINE_FREQUENCY_CONTROL:
					set_video_data(p_dev,uvc_powerline_frequency.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case HUE_CONTROL:
					set_video_data(p_dev,uvc_hue.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SATURATION_CONTROL:
					set_video_data(p_dev,uvc_saturation.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SHARPNESS_CONTROL:
					set_video_data(p_dev,uvc_sharpness.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case GAMMA_CONTROL:
					set_video_data(p_dev,uvc_gamma.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case WHITE_BALANCE_CONTROL: 
					set_video_data(p_dev,uvc_whitebalance.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				default:
					printf_video_cmd(p_dev);						
				return 0; 						
			}
			break;
		case (CAM_OT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT3_C1:
					set_video_data(p_dev,uvc_extension_unit3_c1.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C2:
					set_video_data(p_dev,uvc_extension_unit3_c2.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT3_C3:
					set_video_data(p_dev,uvc_extension_unit3_c3.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;				
				case EXTENSION_UINT3_C4:
					set_video_data(p_dev,uvc_extension_unit3_c4.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 
			}
			break; 		
		case (CAM_EU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT4_C1:
					set_video_data(p_dev,uvc_extension_unit4_c1.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C2:
					set_video_data(p_dev,uvc_extension_unit4_c2.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C3:
					set_video_data(p_dev,uvc_extension_unit4_c3.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C4:
					set_video_data(p_dev,uvc_extension_unit4_c4.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C5:
					set_video_data(p_dev,uvc_extension_unit4_c5.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C6:
					set_video_data(p_dev,uvc_extension_unit4_c6.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C7:
					set_video_data(p_dev,uvc_extension_unit4_c7.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C8:
					set_video_data(p_dev,uvc_extension_unit4_c8.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C9:
					set_video_data(p_dev,uvc_extension_unit4_c9.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C10:
					set_video_data(p_dev,uvc_extension_unit4_c10.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C11:
					set_video_data(p_dev,uvc_extension_unit4_c11.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C12:
					set_video_data(p_dev,uvc_extension_unit4_c12.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C13:
					set_video_data(p_dev,uvc_extension_unit4_c13.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C14:
					set_video_data(p_dev,uvc_extension_unit4_c14.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C15:
					set_video_data(p_dev,uvc_extension_unit4_c15.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C16:
					set_video_data(p_dev,uvc_extension_unit4_c16.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C17:
					set_video_data(p_dev,uvc_extension_unit4_c17.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C21:
					set_video_data(p_dev,uvc_extension_unit4_c21.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C22:
					set_video_data(p_dev,uvc_extension_unit4_c22.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C23:
					set_video_data(p_dev,uvc_extension_unit4_c23.cur,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 						
			}
			break;	
		case CAM_VS_ID:
			set_video_data(p_dev, (uint8 *)&uvc_probe_control, p_dev->usb_ctrl.cmd.length&0xff); 
			break;
		default:
			printf_video_cmd(p_dev);
			uvc_cur_buf[0] = 0x06;
		break;
	}
	return 1;

}

uint8_t usb_vedio_get_info(struct hgusb20_dev *p_dev){
	switch(p_dev->usb_ctrl.cmd.index){
		case (CAM_IT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case AUTO_EXPOSURE_MODE:
					printf_video_cmd(p_dev);
				return 0;
				case AUTO_EXPOSURE_PRIORITY:
					set_video_data(p_dev,uvc_ae_priority.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXPOSURE_TIME_ABS:
					set_video_data(p_dev,uvc_exposure_time.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case ZOOM_ABS:
					set_video_data(p_dev,uvc_zoom_abs.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case PAN_ABS:
					set_video_data(p_dev,uvc_pantilt.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
	
				default:
					printf_video_cmd(p_dev);	
					hgusb20_dev_stall_ep(p_dev, 0x00);
				return 0; 
			}
			break;
		case (CAM_PU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case BACKLIGHT_CONTROL:
					set_video_data(p_dev,uvc_backlight.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case BRIGHTNESS_CONTROL:
					set_video_data(p_dev,uvc_brightness.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case CONTRAST_CONTROL:
					set_video_data(p_dev,uvc_contrast.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case GAIN_CONTROL:
					set_video_data(p_dev,uvc_gain.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;				
				case POWER_LINE_FREQUENCY_CONTROL:
					set_video_data(p_dev,uvc_powerline_frequency.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case HUE_CONTROL:
					set_video_data(p_dev,uvc_hue.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SATURATION_CONTROL:
					set_video_data(p_dev,uvc_saturation.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SHARPNESS_CONTROL:
					set_video_data(p_dev,uvc_sharpness.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case GAMMA_CONTROL:
					set_video_data(p_dev,uvc_gamma.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case WHITE_BALANCE_CONTROL: 
					set_video_data(p_dev,uvc_whitebalance.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				default:
					printf_video_cmd(p_dev);
				return 0; 						
			}
			break;
		case (CAM_OT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT3_C1:
					set_video_data(p_dev,uvc_extension_unit3_c1.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C2:
					set_video_data(p_dev,uvc_extension_unit3_c2.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C3:
					set_video_data(p_dev,uvc_extension_unit3_c3.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C4:
					set_video_data(p_dev,uvc_extension_unit3_c4.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 
			}
			break;
		case (CAM_EU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT4_C1:
					set_video_data(p_dev,uvc_extension_unit4_c1.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C2:
					set_video_data(p_dev,uvc_extension_unit4_c2.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C3:
					set_video_data(p_dev,uvc_extension_unit4_c3.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C4:
					set_video_data(p_dev,uvc_extension_unit4_c4.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C5:
					set_video_data(p_dev,uvc_extension_unit4_c5.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C6:
					set_video_data(p_dev,uvc_extension_unit4_c6.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C7:
					set_video_data(p_dev,uvc_extension_unit4_c7.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C8:
					set_video_data(p_dev,uvc_extension_unit4_c8.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C9:
					set_video_data(p_dev,uvc_extension_unit4_c9.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C10:
					set_video_data(p_dev,uvc_extension_unit4_c10.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C11:
					set_video_data(p_dev,uvc_extension_unit4_c11.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C12:
					set_video_data(p_dev,uvc_extension_unit4_c12.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C13:
					set_video_data(p_dev,uvc_extension_unit4_c13.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C14:
					set_video_data(p_dev,uvc_extension_unit4_c14.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C15:
					set_video_data(p_dev,uvc_extension_unit4_c15.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C16:
					set_video_data(p_dev,uvc_extension_unit4_c16.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C17:
					set_video_data(p_dev,uvc_extension_unit4_c17.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C21:
					set_video_data(p_dev,uvc_extension_unit4_c21.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C22:
					set_video_data(p_dev,uvc_extension_unit4_c22.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C23:
					set_video_data(p_dev,uvc_extension_unit4_c23.info,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 						
			}
			break;					
		default:
			printf_video_cmd(p_dev);
		return 0;
	}
	return 1;
}

uint8_t usb_vedio_get_min(struct hgusb20_dev *p_dev){
	switch(p_dev->usb_ctrl.cmd.index){
		case (CAM_IT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case AUTO_EXPOSURE_MODE:		
					printf_video_cmd(p_dev);
				return 0;
				case AUTO_EXPOSURE_PRIORITY:
					set_video_data(p_dev,uvc_ae_priority.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXPOSURE_TIME_ABS:
					set_video_data(p_dev,uvc_exposure_time.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case ZOOM_ABS:
					set_video_data(p_dev,uvc_zoom_abs.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case PAN_ABS:
					set_video_data(p_dev,uvc_pantilt.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
	
				default:
					printf_video_cmd(p_dev);						
					p_dev->usb_ctrl.cmd.length = 0;
				break;	
			}
			break;
		case (CAM_PU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case BACKLIGHT_CONTROL:
					set_video_data(p_dev,uvc_backlight.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case BRIGHTNESS_CONTROL:
					set_video_data(p_dev,uvc_brightness.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case CONTRAST_CONTROL:
					set_video_data(p_dev,uvc_contrast.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case GAIN_CONTROL:
					set_video_data(p_dev,uvc_gain.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case POWER_LINE_FREQUENCY_CONTROL:
					set_video_data(p_dev,uvc_powerline_frequency.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case HUE_CONTROL:
					set_video_data(p_dev,uvc_hue.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SATURATION_CONTROL:
					set_video_data(p_dev,uvc_saturation.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SHARPNESS_CONTROL:
					set_video_data(p_dev,uvc_sharpness.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case GAMMA_CONTROL:
					set_video_data(p_dev,uvc_gamma.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case WHITE_BALANCE_CONTROL: 
					set_video_data(p_dev,uvc_whitebalance.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				default:
					printf_video_cmd(p_dev);						
				return 0; 						
			}
			break;					
		case (CAM_OT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT3_C1:
					set_video_data(p_dev,uvc_extension_unit3_c1.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C2:
					set_video_data(p_dev,uvc_extension_unit3_c2.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C3:
					set_video_data(p_dev,uvc_extension_unit3_c3.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C4:
					set_video_data(p_dev,uvc_extension_unit3_c4.min,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 
			}
			break;
		case (CAM_EU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
					case EXTENSION_UINT4_C1:
						set_video_data(p_dev,uvc_extension_unit4_c1.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C2:
						set_video_data(p_dev,uvc_extension_unit4_c2.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C3:
						set_video_data(p_dev,uvc_extension_unit4_c3.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C4:
						set_video_data(p_dev,uvc_extension_unit4_c4.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;					
					case EXTENSION_UINT4_C5:
						set_video_data(p_dev,uvc_extension_unit4_c5.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C6:
						set_video_data(p_dev,uvc_extension_unit4_c6.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C7:
						set_video_data(p_dev,uvc_extension_unit4_c7.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C8:
						set_video_data(p_dev,uvc_extension_unit4_c8.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C9:
						set_video_data(p_dev,uvc_extension_unit4_c9.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C10:
						set_video_data(p_dev,uvc_extension_unit4_c10.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C11:
						set_video_data(p_dev,uvc_extension_unit4_c11.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C12:
						set_video_data(p_dev,uvc_extension_unit4_c12.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;					
					case EXTENSION_UINT4_C13:
						set_video_data(p_dev,uvc_extension_unit4_c13.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C14:
						set_video_data(p_dev,uvc_extension_unit4_c14.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C15:
						set_video_data(p_dev,uvc_extension_unit4_c15.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C16:
						set_video_data(p_dev,uvc_extension_unit4_c16.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C17:
						set_video_data(p_dev,uvc_extension_unit4_c17.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C21:
						set_video_data(p_dev,uvc_extension_unit4_c21.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;	
					case EXTENSION_UINT4_C22:
						set_video_data(p_dev,uvc_extension_unit4_c22.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;
					case EXTENSION_UINT4_C23:
						set_video_data(p_dev,uvc_extension_unit4_c23.min,p_dev->usb_ctrl.cmd.length&0xff);
					break;			
				default:
					printf_video_cmd(p_dev);
				return 0; 						
			}
			break;
		case CAM_VS_ID:
			set_video_data(p_dev, (uint8 *)&uvc_probe_control,p_dev->usb_ctrl.cmd.length&0xff); 
			break;	
		default:
			printf_video_cmd(p_dev);
		return 0;
	}
	return 1;
}

uint8_t usb_vedio_get_max(struct hgusb20_dev *p_dev){
	switch(p_dev->usb_ctrl.cmd.index){
		case (CAM_IT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case AUTO_EXPOSURE_MODE:
					printf_video_cmd(p_dev);
				return 0;
				case AUTO_EXPOSURE_PRIORITY:
					set_video_data(p_dev,uvc_ae_priority.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXPOSURE_TIME_ABS:
					set_video_data(p_dev,uvc_exposure_time.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case ZOOM_ABS:
					set_video_data(p_dev,uvc_zoom_abs.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case PAN_ABS:
					set_video_data(p_dev,uvc_pantilt.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
	
				default:
					printf_video_cmd(p_dev);						
					p_dev->usb_ctrl.cmd.length = 0;
				break;
			}
			break;
		case (CAM_PU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case BACKLIGHT_CONTROL:
					set_video_data(p_dev,uvc_backlight.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case BRIGHTNESS_CONTROL:
					set_video_data(p_dev,uvc_brightness.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case CONTRAST_CONTROL:
					set_video_data(p_dev,uvc_contrast.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case GAIN_CONTROL:
					set_video_data(p_dev,uvc_gain.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;				
				case POWER_LINE_FREQUENCY_CONTROL:
					set_video_data(p_dev,uvc_powerline_frequency.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case HUE_CONTROL:
					set_video_data(p_dev,uvc_hue.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SATURATION_CONTROL:
					set_video_data(p_dev,uvc_saturation.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SHARPNESS_CONTROL:
					set_video_data(p_dev,uvc_sharpness.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case GAMMA_CONTROL:
					set_video_data(p_dev,uvc_gamma.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case WHITE_BALANCE_CONTROL: 
					set_video_data(p_dev,uvc_whitebalance.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				default:
					printf_video_cmd(p_dev);						
				return 0; 						
			}
			break;					
		case (CAM_OT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT3_C1:
					set_video_data(p_dev,uvc_extension_unit3_c1.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C2:
					set_video_data(p_dev,uvc_extension_unit3_c2.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C3:
					set_video_data(p_dev,uvc_extension_unit3_c3.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C4:
					set_video_data(p_dev,uvc_extension_unit3_c4.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 
			}
			break;
		case (CAM_EU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT4_C1:
					set_video_data(p_dev,uvc_extension_unit4_c1.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C2:
					set_video_data(p_dev,uvc_extension_unit4_c2.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C3:
					set_video_data(p_dev,uvc_extension_unit4_c3.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C4:
					set_video_data(p_dev,uvc_extension_unit4_c4.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C5:
					set_video_data(p_dev,uvc_extension_unit4_c5.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C6:
					set_video_data(p_dev,uvc_extension_unit4_c6.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C7:
					set_video_data(p_dev,uvc_extension_unit4_c7.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C8:
					set_video_data(p_dev,uvc_extension_unit4_c8.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C9:
					set_video_data(p_dev,uvc_extension_unit4_c9.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C10:
					set_video_data(p_dev,uvc_extension_unit4_c10.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C11:
					set_video_data(p_dev,uvc_extension_unit4_c11.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C12:
					set_video_data(p_dev,uvc_extension_unit4_c12.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C13:
					set_video_data(p_dev,uvc_extension_unit4_c13.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C14:
					set_video_data(p_dev,uvc_extension_unit4_c14.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C15:
					set_video_data(p_dev,uvc_extension_unit4_c15.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C16:
					set_video_data(p_dev,uvc_extension_unit4_c16.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C17:
					set_video_data(p_dev,uvc_extension_unit4_c17.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C21:
					set_video_data(p_dev,uvc_extension_unit4_c21.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C22:
					set_video_data(p_dev,uvc_extension_unit4_c22.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C23:
					set_video_data(p_dev,uvc_extension_unit4_c23.max,p_dev->usb_ctrl.cmd.length&0xff);
				break;

				default:
					printf_video_cmd(p_dev);
				return 0; 						
			}
			break;
		case CAM_VS_ID:
			set_video_data(p_dev, (uint8 *)&uvc_probe_control,p_dev->usb_ctrl.cmd.length&0xff); 
			break;					
		default:
			printf_video_cmd(p_dev);
		return 0;
	}
	return 1;
}

uint8_t usb_vedio_get_res(struct hgusb20_dev *p_dev){
	switch(p_dev->usb_ctrl.cmd.index){
		case (CAM_IT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case AUTO_EXPOSURE_MODE:
					printf_video_cmd(p_dev);
				return 0;
				case AUTO_EXPOSURE_PRIORITY:
					set_video_data(p_dev,uvc_ae_priority.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXPOSURE_TIME_ABS:
					set_video_data(p_dev,uvc_exposure_time.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case ZOOM_ABS:
					set_video_data(p_dev,uvc_zoom_abs.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case PAN_ABS:
					set_video_data(p_dev,uvc_pantilt.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
	
				default:
					printf_video_cmd(p_dev);						
					p_dev->usb_ctrl.cmd.length = 0;
				break;	
			}
			break;
		case (CAM_PU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case BACKLIGHT_CONTROL:
					set_video_data(p_dev,uvc_backlight.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case BRIGHTNESS_CONTROL:
					set_video_data(p_dev,uvc_brightness.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case CONTRAST_CONTROL:
					set_video_data(p_dev,uvc_contrast.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case GAIN_CONTROL:
					set_video_data(p_dev,uvc_gain.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;				
				case POWER_LINE_FREQUENCY_CONTROL:
					set_video_data(p_dev,uvc_powerline_frequency.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case HUE_CONTROL:
					set_video_data(p_dev,uvc_hue.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SATURATION_CONTROL:
					set_video_data(p_dev,uvc_saturation.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case SHARPNESS_CONTROL:
					set_video_data(p_dev,uvc_sharpness.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case GAMMA_CONTROL:
					set_video_data(p_dev,uvc_gamma.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				case WHITE_BALANCE_CONTROL: 
					set_video_data(p_dev,uvc_whitebalance.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;							
				default:
					printf_video_cmd(p_dev);						
				return 0;						
			}
			break;						
		case (CAM_OT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT3_C1:
					set_video_data(p_dev,uvc_extension_unit3_c1.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C2:
					set_video_data(p_dev,uvc_extension_unit3_c2.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C3:
					set_video_data(p_dev,uvc_extension_unit3_c3.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C4:
					set_video_data(p_dev,uvc_extension_unit3_c4.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 
			}
			break;

		case (CAM_EU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT4_C1:
					set_video_data(p_dev,uvc_extension_unit4_c1.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C2:
					set_video_data(p_dev,uvc_extension_unit4_c2.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C3:
					set_video_data(p_dev,uvc_extension_unit4_c3.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C4:
					set_video_data(p_dev,uvc_extension_unit4_c4.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C5:
					set_video_data(p_dev,uvc_extension_unit4_c5.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C6:
					set_video_data(p_dev,uvc_extension_unit4_c6.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C7:
					set_video_data(p_dev,uvc_extension_unit4_c7.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C8:
					set_video_data(p_dev,uvc_extension_unit4_c8.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C9:
					set_video_data(p_dev,uvc_extension_unit4_c9.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C10:
					set_video_data(p_dev,uvc_extension_unit4_c10.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C11:
					set_video_data(p_dev,uvc_extension_unit4_c11.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C12:
					set_video_data(p_dev,uvc_extension_unit4_c12.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C13:
					set_video_data(p_dev,uvc_extension_unit4_c13.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C14:
					set_video_data(p_dev,uvc_extension_unit4_c14.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C15:
					set_video_data(p_dev,uvc_extension_unit4_c15.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C16:
					set_video_data(p_dev,uvc_extension_unit4_c16.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C17:
					set_video_data(p_dev,uvc_extension_unit4_c17.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C21:
					set_video_data(p_dev,uvc_extension_unit4_c21.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C22:
					set_video_data(p_dev,uvc_extension_unit4_c22.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C23:
					set_video_data(p_dev,uvc_extension_unit4_c23.res,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 						
			}
			break;
		default:
			printf_video_cmd(p_dev);
		return 0;
	}
	return 1;

}

uint8_t usb_vedio_get_len(struct hgusb20_dev *p_dev){
	switch(p_dev->usb_ctrl.cmd.index){
		case (CAM_EU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT4_C1:
					set_video_data(p_dev,uvc_extension_unit4_c1.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C2:
					set_video_data(p_dev,uvc_extension_unit4_c2.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C3:
					set_video_data(p_dev,uvc_extension_unit4_c3.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C4:
					set_video_data(p_dev,uvc_extension_unit4_c4.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C5:
					set_video_data(p_dev,uvc_extension_unit4_c5.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C6:
					set_video_data(p_dev,uvc_extension_unit4_c6.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C7:
					set_video_data(p_dev,uvc_extension_unit4_c7.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C8:
					set_video_data(p_dev,uvc_extension_unit4_c8.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C9:
					set_video_data(p_dev,uvc_extension_unit4_c9.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C10:
					set_video_data(p_dev,uvc_extension_unit4_c10.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C11:
					set_video_data(p_dev,uvc_extension_unit4_c11.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C12:
					set_video_data(p_dev,uvc_extension_unit4_c12.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;					
				case EXTENSION_UINT4_C13:
					set_video_data(p_dev,uvc_extension_unit4_c13.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C14:
					set_video_data(p_dev,uvc_extension_unit4_c14.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C15:
					set_video_data(p_dev,uvc_extension_unit4_c15.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C16:
					set_video_data(p_dev,uvc_extension_unit4_c16.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C17:
					set_video_data(p_dev,uvc_extension_unit4_c17.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C21:
					set_video_data(p_dev,uvc_extension_unit4_c21.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				case EXTENSION_UINT4_C22:
					set_video_data(p_dev,uvc_extension_unit4_c22.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT4_C23:
					set_video_data(p_dev,uvc_extension_unit4_c23.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;	
				default:
					printf_video_cmd(p_dev);
					p_dev->usb_ctrl.cmd.length = 0;
				break;							
			}				
		break;
		case (CAM_OT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_UINT3_C1:
					set_video_data(p_dev,uvc_extension_unit3_c1.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C2:
					set_video_data(p_dev,uvc_extension_unit3_c2.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				case EXTENSION_UINT3_C3:
					set_video_data(p_dev,uvc_extension_unit3_c3.len,p_dev->usb_ctrl.cmd.length&0xff);
				break;
				default:
					printf_video_cmd(p_dev);
				return 0; 
			}
		break;
			
		default:
			printf_video_cmd(p_dev);
		break;				
	}
	return 1;

}


bool usb_ep0_class_vedio_req(struct hgusb20_dev *p_dev)
{
	switch(p_dev->usb_ctrl.cmd.request){
		case GET_DEF:
			if(usb_vedio_get_def(p_dev) == 0)
				return true;
			
		    return hgusb20_dev_ep0_tx(p_dev,uvc_cur_buf, p_dev->usb_ctrl.cmd.length&0xff);		
		case GET_CUR:
			if(usb_vedio_get_cur(p_dev) == 0)
				return true;
		    return hgusb20_dev_ep0_tx(p_dev,uvc_cur_buf, p_dev->usb_ctrl.cmd.length&0xff);		
			
		case GET_INFO:
			if(usb_vedio_get_info(p_dev) == 0)
				return true;

		    return hgusb20_dev_ep0_tx(p_dev,uvc_cur_buf, p_dev->usb_ctrl.cmd.length&0xff);		

		case GET_MIN:
			if(usb_vedio_get_min(p_dev) == 0)
				return true;				
		    return hgusb20_dev_ep0_tx(p_dev,uvc_cur_buf, p_dev->usb_ctrl.cmd.length&0xff);		

		case GET_MAX:
			if(usb_vedio_get_max(p_dev) == 0)
				return true;				
		    return hgusb20_dev_ep0_tx(p_dev,uvc_cur_buf, p_dev->usb_ctrl.cmd.length&0xff);		

		case GET_RES:
			if(usb_vedio_get_res(p_dev) == 0)
				return true;					
		    return hgusb20_dev_ep0_tx(p_dev,uvc_cur_buf, p_dev->usb_ctrl.cmd.length&0xff);		

		case GET_LEN:
			if(usb_vedio_get_len(p_dev) == 0){
				hgusb20_dev_ep0_tx(p_dev, NULL, 0);
				return true;
			}
		    return hgusb20_dev_ep0_tx(p_dev,uvc_cur_buf, p_dev->usb_ctrl.cmd.length&0xff);		
		default:	
			printf_video_cmd(p_dev);
	}
    return false;
}

void video_set_cur_class(struct hgusb20_dev *p_dev,uint8_t *buf,uint32_t len){
	switch(p_dev->usb_ctrl.cmd.index){
		case (CAM_IT_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case AUTO_EXPOSURE_MODE:
					printf_video_cmd(p_dev);
				return;
				case AUTO_EXPOSURE_PRIORITY:
					memcpy(uvc_ae_priority.cur,buf,len);
				break;
				case EXPOSURE_TIME_ABS:
					memcpy(uvc_exposure_time.cur,buf,len);
				break;
				case ZOOM_ABS:
					memcpy(uvc_zoom_abs.cur,buf,len);
				break;
				case PAN_ABS:
					memcpy(uvc_pantilt.cur,buf,len);					
				break;

				default:
					printf_video_cmd(p_dev);						
				break;	
			}
			break;			
		case (CAM_PU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case BACKLIGHT_CONTROL:
					memcpy(uvc_backlight.cur,buf,len);					
				break;							
				case BRIGHTNESS_CONTROL:
					memcpy(uvc_brightness.cur,buf,len);					
				break;							
				case CONTRAST_CONTROL:
					memcpy(uvc_contrast.cur,buf,len);					
				break;							
				case POWER_LINE_FREQUENCY_CONTROL:
					memcpy(uvc_powerline_frequency.cur,buf,len);					
				break;							
				case HUE_CONTROL:
					memcpy(uvc_hue.cur,buf,len);					
				break;							
				case SATURATION_CONTROL:
					memcpy(uvc_saturation.cur,buf,len);					
				break;							
				case SHARPNESS_CONTROL:
					memcpy(uvc_sharpness.cur,buf,len);					
				break;							
				case GAMMA_CONTROL:
					memcpy(uvc_gamma.cur,buf,len);					
				break;							
				case WHITE_BALANCE_CONTROL:	
					memcpy(uvc_whitebalance.cur,buf,len);					
				break;							
				default:
					printf_video_cmd(p_dev);						
				return;							
			}
			break;			
		case (CAM_OT_ID<<8):
			printf_video_cmd(p_dev);
			return;			
		case (CAM_EU_ID<<8):
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case EXTENSION_C10:
					memcpy(uvc_extension_unit4_c10.cur,buf,len);					
				break;
				case EXTENSION_C11:
					memcpy(uvc_extension_unit4_c11.cur,buf,len);					
				break;					
				default:
					printf_video_cmd(p_dev);
				return; 						
			}
			break;
		case CAM_VS_ID:
			switch((p_dev->usb_ctrl.cmd.value&0xff00) >> 8){
				case VS_PROBE_CONTROL:
					if(len == 26)		//do not change the dwMaxPayloadTSize
						len = 22;		
					memcpy(&uvc_probe_control,buf,len);
				break;
				case VS_COMMIT_CONTROL:
//					printf("VS_COMMIT_CONTROL deal\r\n");
					memcpy(&uvc_probe_control_def,buf,len);
					if((uvc_probe_control_def.bFormatIndex == 1) &&(uvc_probe_control_def.bFrameIndex == 3)){
						os_printf("VS_COMMIT_CONTROL  SET VGA\r\n");
						//set_config_unlock = 1;
					}else{
						os_printf("VS_COMMIT_CONTROL  SET 720P\r\n");
						//set_config_unlock = 0;
					}
				break;
			}
		break;

		
		default:
			printf_video_cmd(p_dev);
		break;
	}

}

void hgusb20_dev_ep0_receive_event(struct hgusb20_dev *p_dev){
	uint8_t usb_itk;
    for (usb_itk = 0; usb_itk < hgusb20_ep_get_sie_rx_len(p_dev, 0); usb_itk++) {
         video_set_cur[usb_itk] = p_dev->usb_ep0_rxbuf[usb_itk];
    }
	
	video_set_cur_class(p_dev,video_set_cur,hgusb20_ep_get_sie_rx_len(p_dev, 0));
	hgusb20_dev_ep0_rx(p_dev, (void*)p_dev->usb_ep0_rxbuf, 63);
}


bool usb_device_uvc_ep0_class(struct hgusb20_dev *p_dev){
	uint8_t *pcmd;
	pcmd = (uint8_t *)&p_dev->usb_ctrl.cmd;
	if((p_dev->usb_ctrl.cmd.rtype == 0x21)&&(0x0a == p_dev->usb_ctrl.cmd.request)){
		return hgusb20_dev_ep0_clrrx_pkt0(p_dev);
	}else if((0x21 == p_dev->usb_ctrl.cmd.rtype) && (0x01 == p_dev->usb_ctrl.cmd.request)){
		//_os_printf("class  %02x %02x %02x %02x %02x %02x %02x %02x\r\n",pcmd[0],pcmd[1],pcmd[2],pcmd[3],pcmd[4],pcmd[5],pcmd[6],pcmd[7]);
		return hgusb20_dev_ep0_rx(p_dev,(void*)p_dev->usb_ep0_rxbuf, 48);
	}else if(0xa1 == p_dev->usb_ctrl.cmd.rtype && (0x80 == (p_dev->usb_ctrl.cmd.request & 0x80))){
		//_os_printf("%02x %02x %02x %02x %02x %02x %02x %02x\r\n",pcmd[0],pcmd[1],pcmd[2],pcmd[3],pcmd[4],pcmd[5],pcmd[6],pcmd[7]);
		return usb_ep0_class_vedio_req(p_dev); 
	}
	return 0;
}


//类命令
bool usb_device_mass_vendor_cmd(struct hgusb20_dev *p_dev)
{
    return FALSE;
}

volatile uint8_t uvc_device_on = 0;
extern uint8 *yuvbuf;
extern Vpp_stream photo_msg;

bool hgusb20_dev_uvc_ep0_set_interface(struct hgusb20_dev *p_dev)
{
	struct scale_device *scale_dev;
	struct scale_device *scale3_dev;
	//struct hgusb20_dev_hw *hw = (struct hgusb20_dev_hw *)p_dev->usb_hw;
	struct vpp_device *vpp_dev;
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	//usb空包发送
    hgusb20_dev_ep0_clrrx_pkt0(p_dev);
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	scale3_dev= (struct scale_device *)dev_get(HG_SCALE3_DEVID); 
	if (p_dev->usb_ctrl.cmd.value == 1) {
		os_printf("isoc_tx_enable_int 1\r\n");
		uvc_device_on = 1;	

		if((uvc_probe_control_def.bFormatIndex == 1) &&(uvc_probe_control_def.bFrameIndex == 3)){
			os_printf("set interface VGA\r\n");
			photo_msg.out0_h = 480;
			photo_msg.out0_w = 640;
			jpg_cfg(HG_JPG0_DEVID,VPP_DATA0);
		}else{
			os_printf("set interface 720P\r\n");
			scale_from_vpp_to_jpg(scale_dev,(uint32)yuvbuf,640,480,1280,720);
			photo_msg.out0_h = 720;
			photo_msg.out0_w = 1280;
			jpg_cfg(HG_JPG0_DEVID,SCALER_DATA);
		}
		start_jpeg();
		scale_close(scale3_dev);
		vpp_open(vpp_dev);
	}else{
		os_printf("isoc_tx_enable_int 0\r\n");
		uvc_device_on = 0;
		vpp_close(vpp_dev);
		scale_close(scale_dev);
		scale_open(scale3_dev);
		stop_jpeg();
	}

    return TRUE;
}

/**
 * setup packet is in “(uint8_t *)&p_dev->usb_ctrl.cmd” or p_dev->usb_ep0_rxbuf 
 */
bool usb_device_mass_uvc_ep0_request(struct hgusb20_dev *p_dev)
{
    uint8_t rtype = USB_REQUEST_TYPE;
    if (0 == rtype) {
        //标准命令处理
        if (0x06 == p_dev->usb_ctrl.cmd.request) {
			if(p_dev->usb_os_msg->dev_type == UDISK_DEV){
				if (!usb_device_mass_ep0_get_descriptor(p_dev)) {
					return FALSE;
				}
			}else{
				if(!usb_device_uvc_ep0_get_descriptor(p_dev)) {
                	return FALSE;
				}
			}

			
        }else if(0x09 == p_dev->usb_ctrl.cmd.request){
			hgusb20_dev_ep0_set_configuration(p_dev);
		}else if(0x0b == p_dev->usb_ctrl.cmd.request){
			if(p_dev->usb_os_msg->dev_type == UDISK_DEV){
				hgusb20_dev_ep0_set_interface(p_dev);
			}else{
				hgusb20_dev_uvc_ep0_set_interface(p_dev);
			}
		} else {
            if (!hgusb20_dev_ep0_standard(p_dev)) {
                return FALSE;
            }
        }

        return TRUE;
    } else if (0x20 == rtype) {
        //类命令
        return usb_device_uvc_ep0_class(p_dev);                     //类命令处理
    } else if (0x40 == rtype) {
        //verdor requeset
        return usb_device_mass_vendor_cmd(p_dev);
    }
    return FALSE;
}


uint32 usb_device_mass_uvc_ep0_setup_irq(struct usb_device *p_usb_d, uint32 len)
{
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)&p_usb_d->dev;
    if (len == 8) {
        //获取命令
        usb_device_mass_uvc_ep0_get_request(p_dev, 8);
        //处理命令
        if (!usb_device_mass_uvc_ep0_request(p_dev)) {           
            hgusb20_dev_stall_ep(p_dev, 0x00);
        }
    } else {
       // _os_printf("rx_cb: %d\r\n", len);
    } 
    
    return 0;
}



__init int32 usb_device_mass_uvc_open(struct usb_device *p_usb_d)
{
	struct hgusb20_dev *p_dev = (struct hgusb20_dev *)&p_usb_d->dev;
	if(p_dev->usb_os_msg->dev_type == UDISK_DEV)
    	return usb_device_open(p_usb_d, (struct usb_device_cfg *)&usb_dev_mass_cfg);
	else
		return usb_device_open(p_usb_d, (struct usb_device_cfg *)&usb_dev_uvc_cfg);
}

int32 usb_device_mass_uvc_close(struct usb_device *p_usb_d)
{
    return usb_device_close(p_usb_d);
}

int32 usb_device_mass_auto_tx_null_pkt_disable(struct usb_device *p_usb_d)
{
    return usb_device_ioctl(p_usb_d, USB_DEV_IO_CMD_AUTO_TX_NULL_PKT_DISABLE, USB_MASS_TX_EP, 0);
}

int32 usb_device_mass_auto_tx_null_pkt_enable(struct usb_device *p_usb_d)
{
    return usb_device_ioctl(p_usb_d, USB_DEV_IO_CMD_AUTO_TX_NULL_PKT_ENABLE, USB_MASS_TX_EP, 0);
}

int32 usb_device_mass_write(struct usb_device *p_usb_d, int8 *buff, uint32 len)
{
    return usb_device_write(p_usb_d, USB_MASS_TX_EP, (uint8 *)buff, len, 1);
}

int32 usb_device_mass_read(struct usb_device *p_usb_d, int8 *buff, uint32 len)
{
    return usb_device_read(p_usb_d, USB_MASS_RX_EP, (uint8 *)buff, len, 0);
}

int32_t usb_bulk_tx(uint8_t epx, uint32_t adr, uint32_t len)//
{
	struct usb_device *dev;
	dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
    return usb_device_write(dev, epx,  (uint8 *)adr, len, 1);
	//return _usb_bulk_tx(epx,adr,len,__MSC_SIZE__);
}
int32_t usb_bulk_rx(uint8_t epx, uint32_t adr, uint32_t len)
{
	struct usb_device *dev;
	dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
	return usb_device_read(dev, epx,  (uint8 *)adr, len, 1);
	//return _usb_bulk_rx(epx,adr,len,__MSC_SIZE__);
}

int32_t usb_uvc_tx(uint8_t epx, uint32_t adr, uint32_t len)//
{
	struct usb_device *dev;
	dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
    return usb_device_write(dev, epx,  (uint8 *)adr, len, 1);
	//return _usb_bulk_tx(epx,adr,len,__MSC_SIZE__);
}


#if 1


#define true    1
#define false   0

const uint8_t  MscSenseCode[][2] = {
	/*NO_SENSE			*/ 			{0x00, 0x00},
	/*INVALID_FIELD_IN_COMMAND	*/ 	{0x05, 0x24},
	/*NEW_MEDIUM_ARRIVEL		*/ 	{0x06, 0x28},
	/*WRITE_PROTECTED		*/ 		{0x07, 0x27},
	/*MEDIUM_NOT_PRESENT		*/ 	{0x02, 0x3A},
	/*DATA_PHASE_ERROR  */			{0x04, 0x4b},
};
typedef struct {
//	unsigned long		Signature;
	uint32_t		CbwTag;
	uint32_t		CbwTrxLength;
	uint32_t		CbwFlag;
	uint32_t		CbwLun;
//	unsigned char		CbwCbLen;

	uint32_t		OpCode;
	uint32_t		SubOpCode;
	uint32_t		Address;
	uint32_t		SubEx;
	uint32_t		Length;
	uint32_t		Residue;
    uint32_t		SubEx1;
    uint32_t		SubEx2;
    uint32_t		SubEx3;
    uint32_t     Func1;
    uint32_t     DataAddr;
    uint32_t     Func2;
    uint32_t     Param;
	//unsigned long		TrxLen;
//	unsigned char		Page;
} MSC_CMD;

typedef struct
{
	uint8_t pc_move;
	uint8_t epxin;
	uint8_t epxout;
	uint8_t uspeed;
	
	uint8_t bstall;
	uint8_t sense;
	uint8_t cswsta;
	uint8_t *pcsw;
	uint8_t *pcbw;
	uint8_t *ptxbuf;
	uint8_t *prxbuf;
}SCSI;

MSC_CMD MscCmd;
SCSI scsi; 

static usb_disk_buf usb_disk;

//获取结构体大小
uint32_t get_usb_disk_space_size()
{
	return sizeof(usb_disk_buf)*USB_DISK_BUF_COUNT;
}

//初始化
void init_usb_disk_buf(uint32_t bufSize)
{
	#if USBDISK == 1
	usb_disk.bufSize = bufSize;
	usb_disk.count = 0;
	usb_disk.lba = 0;
	usb_disk.usbbuf = (uint8_t 	*)os_malloc(bufSize*MULTI_SECTOR_COUNT);
	memset(usb_disk.usbbuf,0,bufSize*MULTI_SECTOR_COUNT);
	#if PINGPANG_BUF_EN
	usb_disk.usb_pingpang_buf = (uint8_t *)os_malloc(bufSize*MULTI_SECTOR_COUNT);
	memset(usb_disk.usb_pingpang_buf, 0, bufSize*MULTI_SECTOR_COUNT);
	usb_disk.udisk_speed_info.buf_1 = usb_disk.usbbuf;
	usb_disk.udisk_speed_info.buf_2 = usb_disk.usb_pingpang_buf;
	usbd_mass_speed_optimize_thread_init(&usb_disk.udisk_speed_info);
	#endif
	#elif USBDISK == 2
	usb_disk.bufSize = bufSize;
	usb_disk.count = 0;
	usb_disk.lba = 0;
	usb_disk.usbbuf = (uint8_t 	*)os_malloc(bufSize);
	memset(usb_disk.usbbuf,0,bufSize);
	#endif
}

void deinit_usb_disk_buf()
{
	#if USBDISK == 1
	if(usb_disk.usbbuf)
	{
		free(usb_disk.usbbuf);
	}
	#if PINGPANG_BUF_EN
	if(usb_disk.usb_pingpang_buf)
	{
		free(usb_disk.usb_pingpang_buf);
	}
	usbd_mass_speed_optimize_thread_deinit();
	#endif

	#elif USBDISK == 2
	if(usb_disk.usbbuf)
	{
		free(usb_disk.usbbuf);
	}
	#endif

}

//获取空闲的空间
//后续可以换成系统的队列去获取,这样保证获取安全获取,又能可以在缓冲区不够的时候不需要单独去防止阻塞导致其他任务不能执行
usb_disk_buf * get_usb_disk_free()
{
//	if(!&usb_disk)
//	{
//		_os_printf("%s err\n",__FUNCTION__);
//		return NULL;
//	}
	return &usb_disk;
}



void usb_bulk_init(void)
{	 
	
	scsi.pcsw 	= (uint8_t *)(usb_test.tx_buf);
	scsi.pcbw 	= (uint8_t *)(usb_test.rx_buf);
	scsi.ptxbuf = (uint8_t *)(usb_test.tx_buf);
	scsi.prxbuf = (uint8_t *)(usb_test.rx_buf);
	scsi.epxin 	= USB_MASS_RX_EP;
	scsi.epxout = USB_MASS_TX_EP;//改成宏定义
}

void scsi_bulk_clr_rx(){
    memset(usb_test.rx_buf,0,sizeof(usb_test.rx_buf));//根据设备直接清rx

}


int32_t ScsiStallIn(uint8_t stall)//响应host
{
	int i = 0;
	struct hgusb20_dev *p_dev = (struct hgusb20_dev *)usb_test.dev;
    if (stall) 
	{
		//_os_printf("- stall in:%d\n",usb_dev_class.ep1_tx_stall);
        //usb_stall_ep(__MSC_EPIN__);
		//tmout_reset();
		hgusb20_dev_stall_ep(p_dev, USB_MASS_TX_EP | 0x80);
		//os_sleep(2);
		//_os_printf("- stall in to\n");
        while (p_dev->usb_dev.ep_tx_stall & BIT(USB_MASS_TX_EP))
		{
			i++;
			if(i>=20000)
			{
				//_os_printf("- stall in to\n");
				break;
			}		
		}		
    }
	return true;

}

#define __LSB__(a,b,c,d) (((uint32_t)d << 24) | ((uint32_t)c << 16) | ((uint32_t)b << 8) | ((uint32_t)a << 0))
#define __MSB__(a,b,c,d) (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | ((uint32_t)d << 0))
int8_t get_cbw(void)
{
	uint8_t *prxbuf = scsi.pcbw;
	

	
	//debgbuf(scsi.prxbuf,31);
	//debgbuf(scsi.ptxbuf,31);
	
	//decode cbw
	if(!((prxbuf[0] == 0x55) && (prxbuf[1] == 0x53) && (prxbuf[2] == 0x42) && (prxbuf[3] == 0x43)))
		return false;	
		
	//MscCmd.CbwTag
	
	MscCmd.CbwTag = __LSB__(prxbuf[4],prxbuf[5],prxbuf[6],prxbuf[7]);
	
	//MscCmd.CbwTrxLength
	MscCmd.CbwTrxLength = __LSB__(prxbuf[8],prxbuf[9],prxbuf[10],prxbuf[11]);
	
	//MscCmd.CbwFlag
	MscCmd.CbwFlag = prxbuf[12];
	
	//Lun
	MscCmd.CbwLun = prxbuf[13];
	
	//MscCmd.OpCode
	MscCmd.OpCode = prxbuf[15];
	
	//MscCmd.SubOpCode
	MscCmd.SubOpCode = prxbuf[16];
	
	//MscCmd.Address
	MscCmd.Address = __MSB__(prxbuf[17],prxbuf[18],prxbuf[19],prxbuf[20]);
	
	//MscCmd.SubEx
	MscCmd.SubEx = prxbuf[21];
	
	//MscCmd.Length
	MscCmd.Length = __MSB__(0,0,prxbuf[22],prxbuf[23]);
	//MscCmd.SubEx1
	MscCmd.SubEx1 = prxbuf[24];

	//MscCmd.SubEx2
	MscCmd.SubEx2 = __MSB__(prxbuf[25],prxbuf[26],prxbuf[27],prxbuf[28]);

	//MscCmd.SubEx3
	MscCmd.SubEx3 = __MSB__(0,0,prxbuf[29],prxbuf[30]);

    MscCmd.Func1 = __LSB__(prxbuf[16],prxbuf[17],prxbuf[18],prxbuf[19]);
    MscCmd.DataAddr = __LSB__(prxbuf[20],prxbuf[21],prxbuf[22],prxbuf[23]);
    MscCmd.Func2 = __LSB__(prxbuf[24],prxbuf[25],prxbuf[26],prxbuf[27]);
    MscCmd.Param = __MSB__(0,prxbuf[30],prxbuf[29],prxbuf[28]);
	//debg(".OpCode:%x,\n",MscCmd.OpCode);//debgbuf(prxbuf, 31);
	
	
	return true;
}
#undef __LSB__
#undef __MSB__

/*-----------------------------------------------------------------------------------*/
/**
* @brief  		
* @param[in]	 
* @retval		
* @details
* @note
* @attention
*/
/*-----------------------------------------------------------------------------------*/
const uint8_t device_inquiry_data[36] =
{
	0x00,	// Peripheral Device Type: direct access devices
	0x80,	// Removable: UFD is removable
	0x06,	// ANSI version
	0x02, 	// Response Data Format: compliance with UFI  
	0x1f,	// Additional Length (Number of unsigned chars following this one): 31, totally 36 unsigned chars
	0x00, 0x00, 0x00,	// reserved
	'G', 'e', 'n', 'e', 'r', 'i', 'c', ' ', 'S', 'T', 'O', 'R', 'A', 'G', 'E',				//BuildwinMedia-Player  can't be change , updata tools need it
	' ', 'D', 'E', 'V', 'I', 'C', 'E', ' ', ' ', '1', '4', '0', '4'				//len full
};
	
#define SENSE_ERROR_CODE		0x70
#define	REQUESTDATA_SIZE		0x12
#define INQUIRY_SIZE  sizeof(device_inquiry_data)
#define FORMTCAPACITY_SIZE		0x0C
#define DISKCAPACITY_SIZE		0x08
#define UDISK_SECTOR_SIZE				0x200
#define MODE_SENSE10_LEN	0x08

void Host_In_Data(uint8_t *ptxbuf, uint32_t len);
extern uint8_t get_usb_is_mound_sd();
int32_t mscCmd_Inquiry(void)
{
	
	uint8_t *ptxbuf = scsi.ptxbuf;
	memcpy(ptxbuf, (uint8_t*)device_inquiry_data, INQUIRY_SIZE);
	Host_In_Data(ptxbuf,INQUIRY_SIZE);
	return true;
}

void mscSet_Status(uint8_t status);
uint8_t get_sd_status(void);
int sd_scsi_read2(uint32 lba,uint8* buf);
int32_t mscCmd_Read(void)
{ 
	int ret;
	usb_disk_buf *disk = NULL;
	uint32_t lba = MscCmd.Address;
	uint32_t sec = MscCmd.Length ;
	uint32_t multi_sector_en;
	MscCmd.Residue = MscCmd.CbwTrxLength;
	multi_sector_en = 0;
	if(get_sd_status() == SD_OFF){
		mscSet_Status(MEDIUM_NOT_PRESENT);
		scsi.bstall = 1;
		return false;
	}else{
		while(!(disk = get_usb_disk_free()));
		//不能不存在
		if(!disk)
		{
			_os_printf("!!!!!%s err\n",__FUNCTION__);
			scsi.bstall = 1;
		}
		else
		{
			//唤醒线程,去读取需要读取的扇区
			//type=2,代表读扇区
			disk->type = 2;
			disk->lba = lba;
			disk->count = sec;
			#if PINGPANG_BUF_EN
			usbd_mass_speed_optimize_send_mq(g_dev, lba, sec, READ_FLAG);
			
			if((sec >= MULTI_SECTOR_COUNT) && (sec % MULTI_SECTOR_COUNT == 0))
			{
				sec = sec / MULTI_SECTOR_COUNT;
				multi_sector_en = 1;
			}
			
			#endif

			while(sec--)
			{
				#if PINGPANG_BUF_EN
					os_sema_down(g_dev->sem, osWaitForever);

					if(g_dev->error_flag == 1)
					{
						os_sema_up(g_dev->usb_sem_write);
						mscSet_Status(DATA_PHASE_ERROR);
						return false;
					}

					if(pingpang_flag)
					{
						os_sema_up(g_dev->usb_sem_write);
						#if USB_IO_TEST_TIME
						gpio_set_val(PA_10, 1);
						#endif
						if(multi_sector_en)
						{
							usb_bulk_tx(scsi.epxout,(uint32_t)disk->usb_pingpang_buf,disk->bufSize*MULTI_SECTOR_COUNT);
							MscCmd.Residue -= (disk->bufSize*MULTI_SECTOR_COUNT);
						}
						else
						{
							usb_bulk_tx(scsi.epxout,(uint32_t)disk->usb_pingpang_buf,disk->bufSize);
							MscCmd.Residue -= disk->bufSize;
						}

						#if USB_IO_TEST_TIME
						gpio_set_val(PA_10, 0);
						#endif
					}
					else
					{
						os_sema_up(g_dev->usb_sem_write);
						#if USB_IO_TEST_TIME
						gpio_set_val(PA_10, 1);
						#endif
						if(multi_sector_en)
						{
							usb_bulk_tx(scsi.epxout,(uint32_t)disk->usbbuf,disk->bufSize*MULTI_SECTOR_COUNT);
							MscCmd.Residue -= (disk->bufSize*MULTI_SECTOR_COUNT);
						}
						else
						{
							usb_bulk_tx(scsi.epxout,(uint32_t)disk->usbbuf,disk->bufSize);
							MscCmd.Residue -= disk->bufSize;
						}
						#if USB_IO_TEST_TIME
						gpio_set_val(PA_10, 0);
						#endif
					}
					
					
				#else
				ret = sd_scsi_read2(lba,disk->usbbuf);
				if(ret){
					return false;
				}
				usb_bulk_tx(scsi.epxout,(uint32_t)disk->usbbuf,disk->bufSize);
				MscCmd.Residue -= disk->bufSize;
				lba++;
				#endif
			}
		}
	}
	return 0;
}


int32_t mscCmd_Read_flash(void)
{ 
	int ret;
	usb_disk_buf *disk = NULL;
	disk = get_usb_disk_free();
	uint32_t lba = MscCmd.Address;
	uint32_t sec = MscCmd.Length ;

	MscCmd.Residue = MscCmd.CbwTrxLength;
	while(sec--)
	{
		//内部认为usbbuf空间给足够
		ret = flashdisk_usb_read(lba,1,disk->usbbuf);
		if(ret)
		{
			return false;
		}
		usb_bulk_tx(scsi.epxout,(uint32_t)disk->usbbuf,disk->bufSize);
		MscCmd.Residue -= disk->bufSize;
		lba++;
	}
	return 0;
}


int sd_scsi_write2(uint32 lba,uint8* buf);
extern volatile uint8_t rx_done;
int32_t mscCmd_Write(void){
	uint32_t lba = MscCmd.Address;
	usb_disk_buf *disk = NULL;
	uint32_t sec = MscCmd.Length ;
	uint32_t multi_sector_en;
	multi_sector_en = 0;
	MscCmd.Residue = MscCmd.CbwTrxLength;
	if(get_sd_status() == SD_OFF){
		mscSet_Status(MEDIUM_NOT_PRESENT);
		scsi.bstall = 1;
		return false;
	}else{
		while(!(disk = get_usb_disk_free()));
		disk->lba = lba;
		disk->type = 1;
		disk->count = 1;
		//不应该出现这种情况,需要检查
		if(!disk)
		{
			_os_printf("%s err\n",__FUNCTION__);
		}
		#if PINGPANG_BUF_EN

		if((sec >= MULTI_SECTOR_COUNT) && (sec % MULTI_SECTOR_COUNT == 0))
		{
			sec = sec / MULTI_SECTOR_COUNT;
			multi_sector_en = 1;
		}

		if(pingpang_flag)
		{

			if(multi_sector_en)
			{
				usb_bulk_rx(scsi.epxin,(uint32_t)disk->usbbuf,disk->bufSize*MULTI_SECTOR_COUNT);
				MscCmd.Residue -= (disk->bufSize*MULTI_SECTOR_COUNT);
			}
			else
			{
				usb_bulk_rx(scsi.epxin,(uint32_t)disk->usbbuf,disk->bufSize);
				MscCmd.Residue -= disk->bufSize;
			}
			
		}
		else
		{

			if(multi_sector_en)
			{
				usb_bulk_rx(scsi.epxin,(uint32_t)disk->usb_pingpang_buf,disk->bufSize*MULTI_SECTOR_COUNT);
				MscCmd.Residue -= (disk->bufSize*MULTI_SECTOR_COUNT);
			}
			else
			{
				usb_bulk_rx(scsi.epxin,(uint32_t)disk->usb_pingpang_buf,disk->bufSize);
				MscCmd.Residue -= disk->bufSize;
			}
			
		}
		
		if(multi_sector_en)
		{
			usbd_mass_speed_optimize_send_mq(g_dev, lba, sec*MULTI_SECTOR_COUNT, WRITE_FLAG);
		}
		else
		{
			usbd_mass_speed_optimize_send_mq(g_dev, lba, sec, WRITE_FLAG);		
		}

		sec--;	
		
		if(sec == 0)
		{
			os_sema_up(g_dev->usb_sem_write);
			os_sema_down(g_dev->sem, osWaitForever);
			return 0;
		}
		
		#endif
		
		do{
			#if PINGPANG_BUF_EN
			os_sema_down(g_dev->sem, osWaitForever);
			if(pingpang_flag)
			{
				if(multi_sector_en)
				{
					usb_bulk_rx(scsi.epxin,(uint32_t)disk->usbbuf,disk->bufSize*MULTI_SECTOR_COUNT);
					MscCmd.Residue -= (disk->bufSize*MULTI_SECTOR_COUNT);
				}
				else
				{
					usb_bulk_rx(scsi.epxin,(uint32_t)disk->usbbuf,disk->bufSize);
					MscCmd.Residue -= disk->bufSize;
				}
				os_sema_up(g_dev->usb_sem_write);
			}
			else
			{
				if(multi_sector_en)
				{
					usb_bulk_rx(scsi.epxin,(uint32_t)disk->usb_pingpang_buf,disk->bufSize*MULTI_SECTOR_COUNT);
					MscCmd.Residue -= (disk->bufSize*MULTI_SECTOR_COUNT);
				}
				else
				{
					usb_bulk_rx(scsi.epxin,(uint32_t)disk->usb_pingpang_buf,disk->bufSize);
					MscCmd.Residue -= disk->bufSize;
				}
				os_sema_up(g_dev->usb_sem_write);
			}			
			#else
			usb_bulk_rx(scsi.epxin,(uint32_t)disk->usbbuf,disk->bufSize);
			sd_scsi_write2(lba,disk->usbbuf);
			MscCmd.Residue -= disk->bufSize;
			lba++;	
			#endif
			
		}while(--sec);	

		#if PINGPANG_BUF_EN
		os_sema_up(g_dev->usb_sem_write);
		os_sema_down(g_dev->sem, osWaitForever);

		if(g_dev->error_flag)
		{
			MscCmd.Residue = MscCmd.CbwTrxLength;
			os_printf("g_dev error flag\n");
			mscSet_Status(DATA_PHASE_ERROR);
			return false;
		}

		#endif
	}
	
	return 0;
}



int32_t mscCmd_Write_flash(void)
{
	uint32_t lba = MscCmd.Address;
	usb_disk_buf *disk = NULL;
	disk = get_usb_disk_free();
	uint32_t sec = MscCmd.Length ;
	MscCmd.Residue = MscCmd.CbwTrxLength;
	do
	{
		usb_bulk_rx(scsi.epxin,(uint32_t)disk->usbbuf,disk->bufSize);
		flashdisk_usb_write(lba,1,disk->usbbuf);
		MscCmd.Residue -= disk->bufSize;
		lba++;	
	}while(--sec);		
	return 0;
}

int32_t mscCmd_RequestSense(void)
{
	uint8_t *ptxbuf = scsi.ptxbuf;
	uint8_t * pSenseCode = (uint8_t*)&MscSenseCode[scsi.sense];
	//_os_printf("sense is %d\r\n",scsi.sense);
	memset(ptxbuf, 0, REQUESTDATA_SIZE);
	
	ptxbuf[0] = SENSE_ERROR_CODE;		// error code
	ptxbuf[2] = pSenseCode[0];			// sense key
	ptxbuf[7] = REQUESTDATA_SIZE - 8;	// Additional Sense data length
	ptxbuf[12] = pSenseCode[1];			// Additional Sense Code
	ptxbuf[13] = 0;  					//MscStatusCode.ASCQ;Additional Sense Code Qualifier
	
	Host_In_Data(ptxbuf,REQUESTDATA_SIZE);

	return true;
}


int32_t mscCmd_ModeSense6(void)
{
	uint8_t *ptxbuf = scsi.ptxbuf;
	
	uint32_t dwTxdata;
	if(get_sd_status() == SD_OFF)
	{
		//_os_printf("get_sd_status() != 0\r\n");
		mscSet_Status(MEDIUM_NOT_PRESENT);
		scsi.bstall = 1;	
	}
	
	dwTxdata = 0x03000000L;
	
	ptxbuf[0] = (uint8_t)(dwTxdata >> 24);
	ptxbuf[1] = (uint8_t)(dwTxdata >> 16);
	ptxbuf[2] = (uint8_t)(dwTxdata >> 8);
	ptxbuf[3] = (uint8_t)(dwTxdata >> 0);
	
	Host_In_Data(ptxbuf,4);
	return true;
}

int32_t mscCmd_ModeSense6_flash(void)
{
	uint8_t *ptxbuf = scsi.ptxbuf;
	
	uint32_t dwTxdata;
	dwTxdata = 0x03000000L;
	
	ptxbuf[0] = (uint8_t)(dwTxdata >> 24);
	ptxbuf[1] = (uint8_t)(dwTxdata >> 16);
	ptxbuf[2] = (uint8_t)(dwTxdata >> 8);
	ptxbuf[3] = (uint8_t)(dwTxdata >> 0);
	
	Host_In_Data(ptxbuf,4);
	return true;
}

uint32 get_sd_cap() ;
int32_t mscCmd_ReadFormatCapacity(void)
{
	uint32_t cap;
	uint8_t *ptxbuf = scsi.ptxbuf;

	if(get_sd_status() != SD_OFF){
		cap = get_sd_cap() - 1;
	}else{
		cap = 0;
		mscSet_Status(MEDIUM_NOT_PRESENT);
		scsi.bstall = 1;		
	}
	
	ptxbuf[0] = 0x00;
	ptxbuf[1] = 0x00;
	ptxbuf[2] = 0x00;
	ptxbuf[3] = 0x08;	

	ptxbuf[4] = (uint8_t)(cap >> 24);
	ptxbuf[5] = (uint8_t)(cap >> 16);
	ptxbuf[6] = (uint8_t)(cap >> 8);
	ptxbuf[7] = (uint8_t)(cap >> 0);
	
	ptxbuf[8] = 0x00;
	ptxbuf[9] = 0x00;
	ptxbuf[10] = 0x02;
	ptxbuf[11] = 0x00;
	
	//debg("-bstall:%x\n",scsi.bstall);
	Host_In_Data(ptxbuf,12);

	return true;
}


int32_t mscCmd_ReadFormatCapacity_flash(void)
{
	uint32_t cap;
	uint32_t sec_size;
	uint8_t *ptxbuf = scsi.ptxbuf;
	
	cap = flashdisk_get_sec_count() - 1;
	sec_size = flashdisk_get_sec_size();
	
	ptxbuf[0] = 0x00;
	ptxbuf[1] = 0x00;
	ptxbuf[2] = 0x00;
	ptxbuf[3] = 0x08;	

	ptxbuf[4] = (uint8_t)(cap >> 24);
	ptxbuf[5] = (uint8_t)(cap >> 16);
	ptxbuf[6] = (uint8_t)(cap >> 8);
	ptxbuf[7] = (uint8_t)(cap >> 0);
	
	#if 1
	ptxbuf[8] 	= 	(uint8_t)(sec_size >> 24);
	ptxbuf[9] 	=	(uint8_t)(sec_size >> 16);
	ptxbuf[10] 	=	(uint8_t)(sec_size >> 8);
	ptxbuf[11] 	=	(uint8_t)(sec_size >> 0);
	#endif
	//memcpy(ptxbuf+8,&sec_size,4);
	
	//debg("-bstall:%x\n",scsi.bstall);
	Host_In_Data(ptxbuf,12);

	return true;
}

int32_t mscCmd_ReadCapacity(void)
{
	uint32_t cap;
	uint8_t *ptxbuf = scsi.ptxbuf;
	
	if(get_sd_status() != SD_OFF){
		cap = get_sd_cap() - 1;
	}else{
		cap = 0;
		mscSet_Status(MEDIUM_NOT_PRESENT);
		scsi.bstall = 1;	
	}
	
	ptxbuf[0] = (uint8_t)(cap >> 24);
	ptxbuf[1] = (uint8_t)(cap >> 16);
	ptxbuf[2] = (uint8_t)(cap >> 8);
	ptxbuf[3] = (uint8_t)(cap >> 0);

	ptxbuf[4] = 0;
	ptxbuf[5] = 0;
	ptxbuf[6] = 0x02;
	ptxbuf[7] = 0x00;
	
	Host_In_Data(ptxbuf,8);
	
	return true;
}

int32_t mscCmd_ReadCapacity_flash(void)
{
	uint32_t cap;
	uint32_t sec_size;
	uint8_t *ptxbuf = scsi.ptxbuf;
	
	cap = flashdisk_get_sec_count() - 1;
	sec_size = flashdisk_get_sec_size();
	

	
	ptxbuf[0] = (uint8_t)(cap >> 24);
	ptxbuf[1] = (uint8_t)(cap >> 16);
	ptxbuf[2] = (uint8_t)(cap >> 8);
	ptxbuf[3] = (uint8_t)(cap >> 0);

	#if 1
	ptxbuf[4]	= 	(uint8_t)(sec_size >> 24);
	ptxbuf[5]	=	(uint8_t)(sec_size >> 16);
	ptxbuf[6] 	=	(uint8_t)(sec_size >> 8);
	ptxbuf[7] 	=	(uint8_t)(sec_size >> 0);
	#endif
	//memcpy(ptxbuf+4,&sec_size,4);

	
	Host_In_Data(ptxbuf,8);
	
	return true;
}

int32_t ScsiStallout(uint8_t stall)
{
	int i = 0;
	struct hgusb20_dev *p_dev = (struct hgusb20_dev *)usb_test.dev;
    if (stall) {
		_os_printf("- stall out\n");
		hgusb20_dev_stall_ep(p_dev, USB_MASS_RX_EP);
        //usb_stall_ep(__MSC_EPOUT__);
		//_os_printf("- stall OUT to\n");
		//tmout_reset();
		//os_sleep(2);
        while (p_dev->usb_dev.ep_rx_stall & BIT(USB_MASS_RX_EP))
		{
			i++;
			if(i>=20000)
			{
				//_os_printf("- stall in to\n");
				break;
			}		
		}	
    }
	return true;
}

int32_t change_sd = 0;
int32_t sent_csw(void);

int32_t scsi_cmd_analysis (void) {
	static uint8_t sd_state = 0;
	MscCmd.Residue = 0;
	scsi.bstall = 0;
	scsi.cswsta = 0;
	usb_test.ready = 0;//在接受USB数据阶段不重入
//	struct hgusb20_dev *dev = (struct hgusb20_dev *)usb_test.dev;
	if(MscCmd.CbwTrxLength == 0){
		scsi_bulk_clr_rx();
		if (MscCmd.OpCode == 0x00) {
			if(sd_state){
				if(get_sd_status() == SD_IDLE){    //掉线过后再检查到重新插入，则表明新状态调整
					change_sd = 1;
					sd_state = 0;
				}
			}

		
			if(change_sd)//需要sd_init和get_sd_status配合改一下
			{
			
			   //_os_printf("new device input..............\r\n");
			   mscSet_Status(NEW_MEDIUM_ARRIVEL);
			   change_sd = 0;				
			}
//判断是否没有sd卡或者判断sd卡是否需要挂载
			if(get_sd_status() == SD_OFF)
			{
				//_os_printf("no device input,loss..............\r\n");
				sd_state     = 1;
				mscSet_Status(MEDIUM_NOT_PRESENT);
				//scsi.bstall = 1;
				
				//init_sd_scsi();
				//if(get_sd_status() != SD_OFF)
				//	change_sd = 1;
			}

		}
		else if(MscCmd.OpCode == 0x1e)
		{
			;
		}
		else
		{
			mscSet_Status(INVALID_FIELD_IN_COMMAND);
			scsi.bstall = 1;
		}
		ScsiStallIn(scsi.bstall);

	}else if(0x80 == MscCmd.CbwFlag){
		//_os_printf("scsi_cmd_analysis 0x28 %dms\n",os_jiffies());
		scsi_bulk_clr_rx();
		if(MscCmd.OpCode == 0x28) {
			mscCmd_Read();
		} else if(MscCmd.OpCode == 0x03) {
			mscCmd_RequestSense();
		} else if (MscCmd.OpCode == 0x12) {
			mscCmd_Inquiry();
		} else if (MscCmd.OpCode == 0x1A) {
			mscCmd_ModeSense6();
		} else if (MscCmd.OpCode == 0x23) {
			mscCmd_ReadFormatCapacity();
		} else if (MscCmd.OpCode == 0x25) {
			mscCmd_ReadCapacity();
		}
		else{
			MscCmd.Residue = MscCmd.CbwTrxLength;
			mscSet_Status(INVALID_FIELD_IN_COMMAND);
			scsi.bstall = 1;
		}
		ScsiStallIn(scsi.bstall);
		//os_sleep_us(50);

	}
	else{
		if ((MscCmd.OpCode != 0x2A && MscCmd.OpCode != 0xCB)) {  
			scsi_bulk_clr_rx();
		}
				
		if(MscCmd.OpCode == 0x2a) {
			mscCmd_Write();
		}else{
			MscCmd.Residue = MscCmd.CbwTrxLength;
			mscSet_Status(INVALID_FIELD_IN_COMMAND);
			scsi.bstall = 1;
		}
		ScsiStallout(scsi.bstall);
	}
	//_os_printf("1");
	//os_sleep_us(50);
	//_os_printf("!");
	sent_csw();
	usb_test.ready = 1;
    //hgusb20_dev_irq_set(dev, USB_EP_TX_IRQ, 1, 1);
	//_os_printf("scsi_cmd_analysis end %dms\n",os_jiffies());
	//irq_enable(USB20MC_IRQn);
	//_os_printf("sent_csw %dms\n",os_jiffies());
	return true;

}


int32_t scsi_cmd_analysis_flash(void) {
	MscCmd.Residue = 0;
	scsi.bstall = 0;
	scsi.cswsta = 0;
	usb_test.ready = 0;//在接受USB数据阶段不重入
//	struct hgusb20_dev *dev = (struct hgusb20_dev *)usb_test.dev;
	if(MscCmd.CbwTrxLength == 0){
		scsi_bulk_clr_rx();
		if (MscCmd.OpCode == 0x00) 
		{
		}
		else if(MscCmd.OpCode == 0x1e)
		{
		}
		else
		{
			mscSet_Status(INVALID_FIELD_IN_COMMAND);
			scsi.bstall = 1;
		}
		ScsiStallIn(scsi.bstall);

	}else if(0x80 == MscCmd.CbwFlag){
		//_os_printf("scsi_cmd_analysis 0x28 %dms\n",os_jiffies());
		scsi_bulk_clr_rx();
		if(MscCmd.OpCode == 0x28) {
			mscCmd_Read_flash();
		} else if(MscCmd.OpCode == 0x03) {
			mscCmd_RequestSense();
		} else if (MscCmd.OpCode == 0x12) {
			mscCmd_Inquiry();
		} else if (MscCmd.OpCode == 0x1A) {
			mscCmd_ModeSense6_flash();
		} else if (MscCmd.OpCode == 0x23) {
			mscCmd_ReadFormatCapacity_flash();
		} else if (MscCmd.OpCode == 0x25) {
			mscCmd_ReadCapacity_flash();
		}
		else{
			MscCmd.Residue = MscCmd.CbwTrxLength;
			mscSet_Status(INVALID_FIELD_IN_COMMAND);
			scsi.bstall = 1;
		}
		ScsiStallIn(scsi.bstall);
		//os_sleep_us(50);

	}
	else{
		if ((MscCmd.OpCode != 0x2A && MscCmd.OpCode != 0xCB)) {  
			scsi_bulk_clr_rx();
		}
				
		if(MscCmd.OpCode == 0x2a) {
			mscCmd_Write_flash();
		}else{
			MscCmd.Residue = MscCmd.CbwTrxLength;
			mscSet_Status(INVALID_FIELD_IN_COMMAND);
			scsi.bstall = 1;
		}
		ScsiStallout(scsi.bstall);
	}

	sent_csw();
	usb_test.ready = 1;

	return true;

}



void mscSet_Status(uint8_t status)
{
	scsi.sense = status;
	if(status){
		scsi.cswsta = 1;	
	}
}


int32_t sent_csw(void)
{
	uint8_t eptx = scsi.epxout;
	uint8_t *ptxbuf = scsi.pcsw;
	//uint32_t dwTxdata;
	//_os_printf("%s\r\n", __FUNCTION__);
	//USB_BulkInWait_Rdy();
	
	ptxbuf[0] = 0x55;
	ptxbuf[1] = 0x53;
	ptxbuf[2] = 0x42;
	ptxbuf[3] = 0x53;
	
	ptxbuf[4] = (uint8_t)(MscCmd.CbwTag >> 0);
	ptxbuf[5] = (uint8_t)(MscCmd.CbwTag >> 8);
	ptxbuf[6] = (uint8_t)(MscCmd.CbwTag >> 16);
	ptxbuf[7] = (uint8_t)(MscCmd.CbwTag >> 24);

	ptxbuf[8] =  (uint8_t)(MscCmd.Residue >> 0);
	ptxbuf[9] =  (uint8_t)(MscCmd.Residue >> 8);
	ptxbuf[10] = (uint8_t)(MscCmd.Residue >> 16);
	ptxbuf[11] = (uint8_t)(MscCmd.Residue >> 24);

	ptxbuf[12] = scsi.cswsta ? 1 : 0;	
	usb_bulk_tx(eptx, (uint32_t)ptxbuf, 13);
	
	return true;
}

void Host_In_Data(uint8_t *ptxbuf, uint32_t len)
{
	uint8_t eptx = scsi.epxout;
	//_os_printf("stall:%d in:%d\n",scsi.bstall,scsi.epxout);
	if (!scsi.bstall) {
		if (MscCmd.CbwTrxLength > len) {
			MscCmd.Residue = MscCmd.CbwTrxLength - len;
			usb_bulk_tx(eptx,(uint32_t)ptxbuf,len);
			scsi.bstall = 1;
			//_os_printf("will stall\r\n");
		} else {
			usb_bulk_tx(eptx,(uint32_t)ptxbuf,MscCmd.CbwTrxLength);
		}
		mscSet_Status(NO_SENSE);
	}	
	else {
		MscCmd.Residue = MscCmd.CbwTrxLength;
	}
}
#endif


#endif
#endif






