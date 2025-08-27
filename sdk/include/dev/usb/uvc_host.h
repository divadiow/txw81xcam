#ifndef _UVC_HOST_H
#define _UVC_HOST_H

#include "typesdef.h"
#include "list.h"

typedef  enum  
{
	UVC_DESCRIPTOR_UNDEFINED=0,
	UVC_HEADER,
	UVC_INPUT_TERMINAL,
	UVC_OUTPUT_TERMINAL,
	UVC_SELECTOR_UNIT,
	UVC_PROCESSING_UNIT,
	UVC_EXTENSION_UNIT,
	UVC_ENCODING_UNIT
}VC_Subtype;

typedef  enum  
{
	VS_UNDEFINED=0,
	VS_INPUT_HEADER,
	VS_OUTPUT_HEADER,
	VS_STILL_IMAGE_FRAME,
	VS_FORMAT_UNCOMPRESSED,
	VS_FRAME_UNCOMPRESSED,
	VS_FORMAT_MJPEG,
	VS_FRAME_MJPEG,
	Reserved,
	Reserved1,
	VS_FORMAT_MPEG2TS,
	Reserved2,
	VS_FORMAT_DV,
	VS_COLORFORMAT,
	Reserved3,
	Reserved4,
	VS_FORMAT_FRAME_BASED,
	VS_FRAME_FRAME_BASED,
	VS_FORMAT_STREAM_BASED,
	VS_FORMAT_H264,
	VS_FRAME_H264,
	VS_FORMAT_H264_SIMULCAST
}VS_Subtype;

// Standard Configuration Descriptor
typedef struct
{
    uint8  length;                // Size of descriptor in uint8
    uint8  type;                  // Configuration
    uint16 t_length;              // Total length
    uint8  num_intf;              // Number of interface
    uint8  cv;                    // bConfigurationValue
    uint8  index;                 // iConfiguration
    uint8  attr;                  // Configuration Characteristic
    uint8  max_power;             // Power config
} SCFG_DESC, *P_SCFG_DESC;



// Standard Interface Descriptor
typedef struct
{
    uint8 length;
    uint8 type;
    uint8 num;
    uint8 alt_tring;
    uint8 end_points;
    uint8 iclass;
    uint8 sub;
    uint8 proto;
    uint8 index;
} SINTF_DESC, *P_INTF_DESC;

// Standard Device Descriptor
typedef struct
{
    uint8  length;
    uint8  descriptor_type;
    uint16 bcd_usb;
    uint8  device_class;
    uint8  device_subclass;
    uint8  device_protocol;
    uint8  maxpacket_size0;
    uint16 id_vendor;
    uint16 id_product;
    uint16 bcd_device;
    uint8  manufacturer;
    uint8  product;
    uint8  serial_number;
    uint8  num_configurations;
} SDEV_DESC, *P_DEV_DESC;

// Standard EndPoint Descriptor
typedef struct
{
    uint8  length;
    uint8  type;
    uint8  ep_addr;
    uint8  attr;
    uint16 pay_load;               // low-speed this must be 0x08
    uint8  interval;
} SEP_DESC, *P_EP_DESC;

struct VC_PROCESS{
	uint8  bLength;
	uint8  bDescriptorType;
	uint8  bDescriptorSubtype;
	uint8  bUnitID;
	uint8  bSourceID;
	uint16 wMaxMultiplier;
	uint8 bControlSize;
	uint32 bmControls;
	uint8 iProcessing;
}__attribute__((packed));

typedef struct{
	uint8	bLength;                           
	uint8	bDescriptorType;                   
	uint8	bDescriptorSubtype;                
	uint8	bFrameIndex;                   
	uint8	bmCapabilities;                   
	uint16	wWidth;                           
	uint16	wHeight;      

}VS_FRAME;

typedef struct{
	uint8	bLength;                           
	uint8	bDescriptorType;                   
	uint8	bDescriptorSubtype;                
	uint8	bFrameIndex;   
	uint8 	bNumFrameDescriptors;
}VS_FORMAT;

typedef struct{
	uint8	bLength;                           
	uint8	bDescriptorType;                   
	uint8	bDescriptorSubtype;                
	uint8	bNumFormats;                     
}VS_HEAD;

#define INTERFACE_Type                   	0x24
#define STRAME_O_CONTORL             		0x04
#define ENDPOINT_DESC             		    0x05

#define VS_FAME_TYPE_MAXNUM    			    20
#define VS_FORMAT_TYPE_MAXNUM 			    2

#define __INTFS_STACK__		32
#define IS_UVC_JPEG   						1   

#define USB_DEV_UVC_CLASS 	0xef 
#define USB_DEV_HUB_CLASS	0X09


#define UVC_SET_REQ				0x21
#define UVC_GET_REQ				0xA1
#define UVC_SET_CUR				0x01
#define UVC_GET_CUR				0x81
#define UVC_GET_MIN				0x82
#define UVC_GET_MAX				0x83
#define UVC_GET_RES				0x84
#define UVC_GET_LEN				0x85
#define UVC_GET_INFO			    0x86
#define UVC_GET_DEF				0x87
#define UVC_CLASS    0x0e
#define UAC_CLASS    0x01
#define UVC_VGA 	 1
#define UVC_720P     2


typedef struct{
	uint8 DesLen;
	uint8 DescriptorType;
	uint8 Subtype;
	uint8 control_endpoint;
	VS_HEAD	    vs_head;
	VS_FRAME   vs_frame[VS_FAME_TYPE_MAXNUM];
	VS_FORMAT vs_format[VS_FORMAT_TYPE_MAXNUM];
	uint8 vs_frame_num;
	uint8 vs_format_num;
	uint32 intfs_count;
	uint32 edpt_count;
	SINTF_DESC intfs[__INTFS_STACK__];
	SEP_DESC edpt[__INTFS_STACK__]; 
	
	struct VC_PROCESS  vc_process;
	uint8 control_intfs;
}UVC_DES;

typedef struct{
	uint8  ep;
	uint8  eptpye;
	uint16 len; 
}EP_DES;

     
typedef struct
{
	uint8 ctyp;
	
	uint8 cv;
	uint8 epctl;
	uint8 ctl_intfs;
	uint8 ctl_altset;
	uint8 ctl_pload;
	uint8 ctl_ttyp;
	uint8 ctl_interval;
	
	uint8 epstrm;
	uint8 strm_intfs;
	uint8 strm_altset;
	uint16 strm_pload;
	uint8 strm_ttyp;
	uint8 strm_interval;
}UVCDEV;

typedef struct{
	uint8 ctl_intfs;
	uint32 process_ctl;
	uint8 process_unitID;
}UVC_PROCESS;

typedef struct {
	uint8 ctl_val;
	int16 info;
	int16 max;
	int16 min;
	uint16 res;
	int16 cur;
}UVC_PROC_REQ;

typedef struct{
	UVC_PROC_REQ brightness;
	UVC_PROC_REQ contrast;
	UVC_PROC_REQ hue;
	UVC_PROC_REQ saturation;
	UVC_PROC_REQ sharpness;
	UVC_PROC_REQ gamma;
	UVC_PROC_REQ white_bal_temp;
	UVC_PROC_REQ white_bal_comp;
	UVC_PROC_REQ backlight_comp;
	UVC_PROC_REQ gain;
	UVC_PROC_REQ power_line_fre;
	UVC_PROC_REQ hue_auto;
	UVC_PROC_REQ white_bal_temp_auto;
	UVC_PROC_REQ white_bal_comp_auto;
	UVC_PROC_REQ digital_mult;
	UVC_PROC_REQ digital_mult_limit;
	UVC_PROC_REQ ana_video_stan;
	UVC_PROC_REQ ana_lock_sta;
	UVC_PROC_REQ contrast_auto;
}UVC_PROCESS_INFO;

typedef enum {
	BRIGHTNESS,
	CONTRAST,
	HUE,
	SATURATION,
	SHARPNESS,
	GAMMA,
	WHITE_BAL_TEMP,
	WHITE_BAL_COMP,
	BACKLIGHT_COMP,
	GAIN,
	POWER_LINE_FRE,
	HUE_AUTO,
	WHITE_BAL_TEMP_AUTO,
	WHITE_BAL_COMP_AUTO,
	DIGITAL_MULT,
	DIGITAL_MULT_LIMIT,
	ANA_VIDEO_STAN,
	ANA_LOCK_STA,
	AONTRAST_AUTO
}UVC_PROCESS_ENUM;

typedef struct{
	VS_FRAME   vs_frame;
	VS_FORMAT vs_format;
}UVC_SLEST;

struct VIDEO_COMMIT
{
	uint16 bmhint;
	uint8 bFormatIndex;
	uint8 bFrameIndex;
	uint32 dwFrameInterval;
	uint16 wKeyFrameRate;
	uint16 wPFrameRate;
	uint16 wCompQuality;
	uint16 wCompWindowSize;
	uint16 wDelay;
	uint32 dwMaxVideoFrameSize;
	uint32 dwMaxPayloadTSize;
}__attribute__((packed));


typedef struct {
	struct list_head list;
	uint8 *cpbuff;     //当前写入数据的地址
	uint8 uvc_head[16];//one pack head data
	uint8  frame_end;   //frame结束标志，表示USB获取数据结束   0：进行中  1：正常结束    2：异常结束
	uint32 tgl;        
	uint32 sta;
	uint32 state;		//frame状态，0：空闲       	1：正在填充 			 2:可用      3:应用正在使用   
	uint32 frame_len;  //帧长度
	uint32 data_len;//one packe data len 
	uint32 head_len;//one packe head len
	uint32 re_space;//当前所写的buff剩余空间
	uint32 frame_counter;
}UVC_MANAGE;


typedef struct {
	struct list_head list;			//BLANK块节点互连
	uint32 frame_counter;				//此数据块代表的frame号
	uint32 blank_len;					//blank可使用的数据长度，最后一个数据块非满外，其他都满
	uint32 re_space;					//当前blank剩下多少空间可填
	uint8  blank_loop;                 //blank计数，每一帧都从0开始增加，便于后面重组
	uint8  *buf_ptr;					//数据块指针
	uint8  busy;						//blank块状态，0：空闲    1：正在填充或者发送    		2:可用      
}UVC_BLANK;

typedef struct{
	uint8 addr;
	uint8 int_endpoint;
	uint8 port_num;
	uint8 connect_port;
}HUB_manage;


enum usb_host_io_cmd {
    USB_HOST_IO_CMD_RESET,	   //usb复位
    USB_HOST_IO_CMD_SET_CUR,
    USB_HOST_IO_CMD_GET_CUR,
    USB_HOST_IO_CMD_SET_IDX,   //1:VGA, 2:720
};

struct uvc_user_arg
{
	void *usb_dev;
	uint32_t state;	//0:可以创建任务   1:正在运行 2:等待任务删除
	uint32_t uvc_format;	
};


/****************uac****************/
typedef enum
{
	AC_UNDEFINED=0,
	AC_HEADER_DES,
	AC_INPUT_DES,
	AC_OUTPUT_DES,
	AC_MIXER_DES,
	AC_SELECTOR_DES,
	AC_FEATURE_DES
}AC_Subtype;

typedef enum
{
	AS_UNDEFINED=0,
	AS_General_Des,
	AS_Format_Des
}AS_Subtype;

typedef enum
{
	USB_Undefined = 0x0100,
	USB_streaming = 0x0101,
	USB_vendor_specific = 0x01FF,
	Input_Undefined = 0x0200,
	Microphone = 0x0201,
	Desktop_microphone = 0x0202,
	Output_Undefined = 0x0300,
	Speaker = 0x0301,
	Headphones = 0x0302
}AC_Terminal_Types;

typedef struct{
    uint8  bLength;
    uint8  bDescriptorType;
    uint8  bDescriptorSubtype;
    uint8  bTerminalID;
    uint16 wTerminalType;
    uint8  bAssocTerminal;
    uint8  bNrChannels;
    uint16 wChannelConfig;
    uint8  iChannelNames;
    uint8  iTerminal;
}AC_INPUT;

typedef struct{
	uint8 bLength;
	uint8 bDescriptorType;
    uint8 bDescriptorSubtype;
	uint8 bUnitID;
	uint8 bSourceID;
	uint8 bControlSize;
	uint8 bmaControls[2];
}AC_FEATURE;

typedef struct{
    uint8  bLength;
    uint8  bDescriptorType;
    uint8  bDescriptorSubtype;
    uint8  bTerminalID;
    uint16 wTerminalType;
    uint8  bAssocTerminal;
    uint8  bSourceID;
    uint8  iTerminal;	
}AC_OUTPUT;

typedef struct{
    uint8  bLength;
    uint8  bDescriptorType;
    uint8  bInterfaceNumber;
    uint8  bAlternateSetting;
    uint8  bNumEndpoints;
    uint8  bInterfaceClass;
    uint8  bInterfaceSubClass;
    uint8  bInterfaceProtocol;
    uint8  iInterface;	
}AS_INTERFACE;

typedef struct{
    uint8  bLength;
    uint8  bDescriptorType;
    uint8  bDescriptorSubtype;
    uint8  bTerminalLink;
    uint8  bDelay;
    uint16 wFormatTag;	
}AS_GENERAL;

typedef struct{
    uint8  bLength;
    uint8  bDescriptorType;
    uint8  bDescriptorSubtype;
    uint8  bFormatType;
    uint8  bNrChannels;
    uint8  bSubframeSize;
    uint8  bBitResolution;
    uint8  bSamFreqType;
    uint32 tSamFreq[4];
}AS_FORMAT;

typedef struct{
    uint8  bLength;
    uint8  bDescriptorType;
    uint8  bEndpointAddress;
    uint8  bmAttributes;
    uint16 wMaxPacketSize;
    uint8  bInterval;
    uint8  bRefresh;
    uint8  bSynchAddress;	
}AS_ENDPOINT;

#define AS_GENERAL_TYPE_MAXNUM    			10
#define AS_FORMAT_TYPE_MAXNUM 			    10

typedef struct{
	uint8 DesLen;
	uint8 DescriptorType;
	uint8 Subtype;
	uint8 control_endpoint;
	uint8 control_interface;
	AC_INPUT ac_input[3];
	AC_OUTPUT ac_output[3];
	AC_FEATURE ac_feature[3];
	AS_INTERFACE as_interface[4];
	AS_GENERAL as_general[AS_GENERAL_TYPE_MAXNUM];
	AS_FORMAT as_format[AS_FORMAT_TYPE_MAXNUM];
	AS_ENDPOINT as_endpoint[10];
	uint8 ac_input_num;
	uint8 ac_feature_num;
	uint8 ac_output_num;
	uint8 as_interface_num;
	uint8 as_general_num;
	uint8 as_format_num;
	uint8 as_endpoint_num;
}UAC_DES;

typedef struct{	
	uint16 spk_type;
	uint8 spk_strm_intfs;
	uint8 spk_strm_altset[4];
	uint16 spk_fmttag[4];
	uint8 spk_fmttype[4];
	uint8 spk_nrchl[4];
	uint8 spk_fmsize[4];
	uint8 spk_bitres[4];
	uint8 spk_samfretype[4];
	uint8 spk_samfre[4][4];
	uint8 spk_epstrm[4];
	uint8 spk_eptype[4];
	uint16 spk_strm_pload[4];
	uint8 spk_feaunitID;
	uint8 spk_feaunit_ctl[2];
	int16 spk_max_vol;
	int16 spk_min_vol;
	
	uint16 mic_type;
	uint8 mic_strm_intfs;
	uint8 mic_strm_altset[4];
	uint16 mic_fmttag[4];
	uint16 mic_fmttype[4];
	uint8 mic_nrchl[4];
	uint8 mic_fmsize[4];
	uint8 mic_bitres[4];
	uint8 mic_samfretype[4];
	uint8 mic_samfre[4][4];
	uint8 mic_epstrm[4];
	uint8 mic_eptype[4];
	uint16 mic_strm_pload[4];
	uint8 mic_feaunitID;
	uint8 mic_feaunit_ctl[2];
	int16 mic_max_vol;
	int16 mic_min_vol;

	uint8 ctl_intfs;
}UACDEV;

typedef struct{
	uint8 strm_intfs;
	uint8 strm_altset;	
	uint8 epstrm;
	uint8 eptype;
	uint16 strm_pload;
}MICDEV;

typedef struct{
	uint8 strm_intfs;
	uint8 strm_altset;	
	uint8 epstrm;
	uint8 eptype;
	uint16 strm_pload;
}SPKDEV;

typedef struct{
	struct list_head list;
	uint8 *data_addr;     //当前写入数据的地址
	uint32 data_len;
	uint32 respace_len;
	uint32 offset;
	uint8 sta;
}UAC_MANAGE;

/**********uac************/

#define HUB_STATU_CONNECT (BIT(1))
#define HUB_STATU_RESTEND (BIT(1))
/**hub clear feature **/
#define PORT_CONNECT 0x10
#define PORT_ENABLE 0X01
#define CLEAR_PORT_REST 0x14
/**hub set feature **/
#define	PORT_POWER 0X08
#define PORT_REST 0x04

#define UVC_EP              2
#define HUB_EP              1
#define UAC_EP              3
#define UVC2_EP             1


#define ISO_HEAD            0
#define BULK_HEAD           12
#define UVC_HEAD            BULK_HEAD


#define UVC_HOST_NUM        2

void uvc_room_init(void *p_dev);
void usb_dma_irq(void * dev, uint8_t* uvc_rx_buff, uint8_t uvc_head_res, uint8_t ep_type);

//void usb_host_irq(typedef_usb_t *usb_sw);

void uvc_user();

void uvc_room_init_mjpeg();
void uvc_room_deinit_mjpeg();
void uvc_room_init_h264();
void uvc_room_deinit_h264(void);
void uvc_reset_dev(uint8_t en);
void uvc_reset_dev_mjpeg(uint8_t en);
void uvc_reset_dev_h264(uint8_t en);

void uvc_sema_init();
void uvc_sema_down(int32 tmo_ms);
void uvc_sema_up();


void uvc_app_user_init();
void force_reset_uvc_frame();

UVC_MANAGE* get_uvc_message_gloal();
uint32 get_uvc_frame_len();
uint8* get_uvc_buf();
uint8 free_uvc_blank(UVC_BLANK* uvc_b);
UVC_BLANK* get_uvc_blank();

extern void del_usbmic_frame(UAC_MANAGE *uac_manage);
extern UAC_MANAGE *get_usbmic_frame(void);
extern uint32 get_uac_frame_datalen(UAC_MANAGE *uac_manage);
extern uint8 *get_uac_frame_data(UAC_MANAGE *uac_manage);
extern void set_uac_frame_datalen(UAC_MANAGE *uac_manage, uint32 len);
extern UAC_MANAGE *get_usbspk_new_frame(uint8 grab);
extern void put_usbspk_frame_to_use(UAC_MANAGE *uac_manage);
extern void set_uac_frame_sta(UAC_MANAGE *uac_manage, uint8 sta);
extern void usbspk_room_init(uint32_t empty_buf_len);
extern void usbmic_room_init(void);
extern void usbmic_dma_irq(void *dev, uint8_t* mic_rx_buff);
extern void usbspk_dma_irq(void *dev);
extern void uac_stop(void);
extern void uac_start(void *dev, uint8_t* mic_rx_buff);
extern int uvc_ep_bulk_or_isoc();
extern void force_reset_usbmic_frame();
extern void force_reset_usbspk_frame();

#endif
