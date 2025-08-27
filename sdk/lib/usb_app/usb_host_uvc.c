#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/irq.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/task.h"
#include "osal/work.h"
#include "osal/event.h"

#include "hal/usb_device.h"
#include "hal/gpio.h"

#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/uvc_host.h"

#define HOST_EP0_BUF  p_dev->usb_ep0_rxbuf

uint8_t uvc_dat[UVC_HOST_NUM][2048+12] __attribute__((aligned(4)));
uint8_t usbmic_dat[1024] __attribute__((aligned(4)));
uint8 uvc_set_addr = 2;
uint8 hub_set_addr = 11;
uint8 hub_insert = 0;
uint8 uvc_head_res = 0;


void delay_us(uint32 n);
void hgusb20_host_reset_phy(struct hgusb20_dev * p_dev);


void delay_ms(uint32 n)
{
    //uint32 sys_clk = sys_get_apbclk();//ll_cc_sys_clk_get();

    if (n) {
        delay_us(n*1000);
    }
}


//获取描述符
bool usb_host_get_descriptor(struct hgusb20_dev *p_dev, uint8 desc_type, uint8 desc_index, uint8 *p_buf, uint16 len)
{
    p_dev->usb_ctrl.cmd.rtype = 0x80;
    p_dev->usb_ctrl.cmd.request = 0x06;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = ((uint16)desc_type << 8) | desc_index;
    p_dev->usb_ctrl.cmd.index = 0;
    p_dev->usb_ctrl.cmd.length = len;
    
    return usb_host_ep0_request(p_dev, p_buf);
}

//接收完剩余数据，发送空包
bool usb_host_get_descriptor_end_do(struct hgusb20_dev *p_dev)
{
    while (p_dev->ep0_ptr.rx_len >= p_dev->usb_host.ep0_pkt) {              //收完剩余数据
         hgusb20_host_ep0_rx(p_dev, p_dev->usb_ep0_rxbuf, 0);
     }
     if (!p_dev->usb_host.stall) {
         hgusb20_host_ep0_tx(p_dev, NULL, 0);                                           //数据包接收完毕，发送一个空包
     }
     if (p_dev->usb_ctrl.error) {
         return FALSE;
     }
     return TRUE;
}


//第一次尝试获取设备描述符
bool usb_host_get_device_descriptor_1_do(struct hgusb20_dev *p_dev)
{
    p_dev->usb_ctrl.error = 0;
    if (usb_host_get_descriptor(p_dev, DEVICE_DESCRIPTOR, 0, p_dev->usb_ep0_rxbuf, 64)) {
        p_dev->usb_host.ep0_pkt = p_dev->usb_ep0_rxbuf[7];                             //Max Pkt
        if (p_dev->usb_host.ep0_pkt != 8 && p_dev->usb_host.ep0_pkt != 16 &&
            p_dev->usb_host.ep0_pkt != 32 && p_dev->usb_host.ep0_pkt != 64) {
            p_dev->usb_ctrl.error = 1;
            return FALSE;
        }    
    } else {
        return FALSE;
    }

    return TRUE;
}

//跳过N个ep0的值
void usb_host_ep0buf_skip_do(struct hgusb20_dev *p_dev, uint8 offset)
{
    p_dev->ep0_ptr.offset += offset;
    while (p_dev->ep0_ptr.offset >= p_dev->ep0_ptr.rx_len && p_dev->ep0_ptr.rx_len >= p_dev->usb_host.ep0_pkt) {
        p_dev->ep0_ptr.offset -= p_dev->ep0_ptr.rx_len;
        hgusb20_host_ep0_rx(p_dev, p_dev->usb_ep0_rxbuf, 0);
        if (p_dev->ep0_ptr.rx_len == 0) {
            _os_printf("ep0 buf skip err\n");
            //usb_sw->usb_ctrl.error = 1;
            return;
        }
    }

}

//从EP0_BUF中获取USB的相关信息
void usb_host_get_info(struct hgusb20_dev *p_dev)
{
    uint16 vid = get_unaligned_le16(&HOST_EP0_BUF[p_dev->ep0_ptr.offset]);                //获取描述符中的相关信息
    uint16 pid = get_unaligned_le16(&HOST_EP0_BUF[p_dev->ep0_ptr.offset + 2]);
    uint16 ver = get_unaligned_le16(&HOST_EP0_BUF[p_dev->ep0_ptr.offset + 4]);
	
	//警告消除
	(void)vid;
	(void)pid;
	(void)ver;
}


//第二次获取设备描述符中的VID PID等产品信息
bool usb_host_get_device_descriptor_2_do(struct hgusb20_dev *p_dev)
{
    if (usb_host_get_descriptor(p_dev, DEVICE_DESCRIPTOR, 0, p_dev->usb_ep0_rxbuf, 0x12)) {
        //获取USB的相关信息
        //usb_host_ep0buf_skip_do(p_dev, 8);
        p_dev->ep0_ptr.offset += 8;
        usb_host_get_info(p_dev);
        return TRUE;
    }
    return FALSE;
}

//第一次尝试获取配置描述符
bool usb_host_get_configuration_descriptor_1(struct hgusb20_dev *p_dev, uint8 cfg_index)
{
    return usb_host_get_descriptor(p_dev, CONFIGURATION_DESCRIPTOR, cfg_index, p_dev->usb_ep0_rxbuf, 9);       //第一次，尝试获取配置描述符
}

//第二次获取配置描述符的Bulk端点信息等
bool usb_host_get_configuration_descriptor_3(struct hgusb20_dev *p_dev, uint8 cfg_index, uint16 desc_len, uint8* buf)
{
    if (desc_len < 18) {
        _os_printf("desc_len err\n");
        return FALSE;
    }
    return usb_host_get_descriptor(p_dev, CONFIGURATION_DESCRIPTOR, cfg_index, buf, desc_len);
}


//设置配置
bool usb_host_set_configuration(struct hgusb20_dev *p_dev, uint8 cfg_value)
{
    p_dev->usb_ctrl.cmd.rtype = 0x00;
    p_dev->usb_ctrl.cmd.request = 0x09;                 //Set Configuration
    p_dev->usb_ctrl.cmd.value = cfg_value;
    p_dev->usb_ctrl.cmd.index = 0;
    p_dev->usb_ctrl.cmd.length = 0;
    return usb_host_ep0_request(p_dev, NULL);
}


//设置接口
bool usb_host_set_interface(struct hgusb20_dev *p_dev, uint8 index, uint8 value)
{
    p_dev->usb_ctrl.cmd.rtype = 0x01;
    p_dev->usb_ctrl.cmd.request = 0x0b;                    //Set Interface
    p_dev->usb_ctrl.cmd.value = value;
    p_dev->usb_ctrl.cmd.index = index;
    p_dev->usb_ctrl.cmd.length = 0;
    return usb_host_ep0_request(p_dev, NULL);
}

//获取描述符
bool usb_host_uvc_get_info(struct hgusb20_dev *p_dev, uint16 uid, uint16 cs, uint32 len, uint8 *pinf)
{
    p_dev->usb_ctrl.cmd.rtype = 0xa1;
    p_dev->usb_ctrl.cmd.request = UVC_GET_INFO;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = cs;
    p_dev->usb_ctrl.cmd.index = uid;
    p_dev->usb_ctrl.cmd.length = len;


    return usb_host_ep0_request(p_dev, pinf);
}

bool usb_host_get_cur(struct hgusb20_dev *p_dev, uint16 uid, uint16 cs, uint32 len, uint8 *pinf)
{
    p_dev->usb_ctrl.cmd.rtype = 0xa1;
    p_dev->usb_ctrl.cmd.request = UVC_GET_CUR;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = cs;
    p_dev->usb_ctrl.cmd.index = uid;
    p_dev->usb_ctrl.cmd.length = len;


    return usb_host_ep0_request(p_dev, pinf);
}

bool usb_host_get_min(struct hgusb20_dev *p_dev, uint16 uid, uint16 cs, uint32 len, uint8 *pinf)
{
    p_dev->usb_ctrl.cmd.rtype = 0xa1;
    p_dev->usb_ctrl.cmd.request = UVC_GET_MIN;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = cs;
    p_dev->usb_ctrl.cmd.index = uid;
    p_dev->usb_ctrl.cmd.length = len;

    return usb_host_ep0_request(p_dev, pinf);	
}

bool usb_host_get_max(struct hgusb20_dev *p_dev, uint16 uid, uint16 cs, uint32 len, uint8 *pinf)
{
    p_dev->usb_ctrl.cmd.rtype = 0xa1;
    p_dev->usb_ctrl.cmd.request = UVC_GET_MAX;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = cs;
    p_dev->usb_ctrl.cmd.index = uid;
    p_dev->usb_ctrl.cmd.length = len;

    return usb_host_ep0_request(p_dev, pinf);	
}

bool usb_host_get_res(struct hgusb20_dev *p_dev, uint16 uid, uint16 cs, uint32 len, uint8 *pinf)
{
    p_dev->usb_ctrl.cmd.rtype = 0xa1;
    p_dev->usb_ctrl.cmd.request = UVC_GET_RES;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = cs;
    p_dev->usb_ctrl.cmd.index = uid;
    p_dev->usb_ctrl.cmd.length = len;

    return usb_host_ep0_request(p_dev, pinf);	
}

bool usb_host_get_def(struct hgusb20_dev *p_dev, uint16 uid, uint16 cs, uint32 len, uint8 *pinf)
{
    p_dev->usb_ctrl.cmd.rtype = 0xa1;
    p_dev->usb_ctrl.cmd.request = UVC_GET_DEF;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = cs;
    p_dev->usb_ctrl.cmd.index = uid;
    p_dev->usb_ctrl.cmd.length = len;

    return usb_host_ep0_request(p_dev, pinf);	
}

//获取描述符
bool usb_host_set_cur(struct hgusb20_dev *p_dev, uint16 uid, uint16 cs, uint32 len, uint8 *pinf)
{
    p_dev->usb_ctrl.cmd.rtype = 0x21;
    p_dev->usb_ctrl.cmd.request = UVC_SET_CUR;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = cs;
    p_dev->usb_ctrl.cmd.index = uid;
    p_dev->usb_ctrl.cmd.length = len;


    return usb_host_ep0_request(p_dev, pinf);
}

void usb_host_set_address(struct hgusb20_dev *p_dev, uint8 addr)
{
    os_sleep_ms(10);
    p_dev->usb_ctrl.cmd.rtype = 0x00;
    p_dev->usb_ctrl.cmd.request = 0x05;                 //Set Address
    p_dev->usb_ctrl.cmd.value = addr;
    p_dev->usb_ctrl.cmd.index = 0;
    p_dev->usb_ctrl.cmd.length = 0;
    usb_host_ep0_request(p_dev, NULL);
}

bool uvc_clear_ep_feature(struct hgusb20_dev *p_dev, uint16 ep_addr)
{
    p_dev->usb_ctrl.cmd.rtype = 0x02;
    p_dev->usb_ctrl.cmd.request = 0x01;                 //Set Configuration
    p_dev->usb_ctrl.cmd.value = 0;
    p_dev->usb_ctrl.cmd.index = ep_addr;
    p_dev->usb_ctrl.cmd.length = 0;	
    return usb_host_ep0_request(p_dev, NULL);
}



struct VIDEO_COMMIT commit;
UVC_SLEST uvc_select;
UVC_PROCESS uvc_proc;
UVC_PROCESS_INFO uvc_proc_info;
UVC_DES uvc_des;
UAC_DES uac_des;
EP_DES  videostream;
volatile UVCDEV uvc;
UACDEV uac;
MICDEV usb_mic;
SPKDEV usb_spk;
volatile uint8 uvc_cur_indx = 0;
uint8 *uvc_cfg_des;//[2014] = {0};


void analysis_uvc_desc(uint8 *desc,uint32 desclen)
{
#define EP_CNT      16
	EP_DES  eptmp[EP_CNT];  //缓存6点节点信息
	uint8   epnum = 0;
	uint8_t uvc_dat_ep = 0;
	uint32_t itk=0;
    uint8 *temp_desc;
    uint8 is_cv = 0;
	uint8 is_vs = 0;
    static uint8 cun = 0;
    cun ++;
    _os_printf("_____analysis_uvc_desc cun _____%d\r\n",cun);
    uint32 desc_total_len = desclen;
    uint32 des_len = 0;
    uint32 des_type = 0;
    uint8 uvc_or_uac = 0;
	uint8 au_cs = 0;
    temp_desc = desc;
    memset((uint8 *)&uvc_des,0,sizeof(UVC_DES));
    memset((uint8 *)&videostream,0,sizeof(EP_DES));	
	memset((uint8 *)&eptmp,0,EP_CNT*sizeof(EP_DES));
	memset((uint8 *)&uac_des,0,sizeof(UAC_DES));
    uvc_des.control_endpoint = 0xff;
    while(desc_total_len)
    {
        des_len = temp_desc[0];
        des_type = temp_desc[1];

        if(des_type == STRAME_O_CONTORL) {
            if(temp_desc[5] == 0x0e)
                uvc_or_uac = 1;
            else if(temp_desc[5] == 0x01)
                uvc_or_uac = 2;
        }

        if(uvc_or_uac == 1) {
            uvc_des.DesLen = temp_desc[0];
            uvc_des.DescriptorType = temp_desc[1];  
            uvc_des.vs_head.bDescriptorType = temp_desc[5];
            uvc_des.vs_head.bDescriptorSubtype = temp_desc[6];

            if(uvc_des.DescriptorType == ENDPOINT_DESC){
                eptmp[epnum].ep     = temp_desc[2];
                eptmp[epnum].eptpye = temp_desc[3];
                eptmp[epnum].len =    temp_desc[4]|(temp_desc[5]<<8);
                epnum++;
            }
            
            if(uvc_des.DescriptorType == STRAME_O_CONTORL){
                is_cv=temp_desc[6];
                if(is_cv == 1)
                    uvc_des.control_intfs = temp_desc[2];
                if((uvc_des.vs_head.bDescriptorType == 0x0e) && (uvc_des.vs_head.bDescriptorSubtype == 0x02)){
                    is_vs = 1;
                }else{
                    is_vs = 0;
                }
            }
            
            if(is_cv == 1){//cv
                    if(uvc_des.DescriptorType == INTERFACE_Type){
                        uvc_des.Subtype     = temp_desc[2];
                        if(uvc_des.Subtype == UVC_PROCESSING_UNIT)
                        {                            
                            memcpy(&uvc_des.vc_process, temp_desc, 8);
                            uvc_des.vc_process.wMaxMultiplier = temp_desc[6]<<8|temp_desc[5];
                            for(uint8 i=0; i<uvc_des.vc_process.bControlSize; i++)
                                uvc_des.vc_process.bmControls = (uvc_des.vc_process.bmControls|(temp_desc[8+i]<<(i*8)));
                        }
                        //_os_printf("CV_subtype :%d\n",uvc_des.Subtype);
                    }
                    if(uvc_des.DescriptorType == ENDPOINT_DESC && temp_desc[3]==0x03){
                        uvc_des.control_endpoint= temp_desc[2]&0x7f;
                        _os_printf("control_endpoint :%d \n",uvc_des.control_endpoint);
                    }
            }
            if(is_cv == 2){//SV
                if(uvc_des.DescriptorType == INTERFACE_Type){
                        uvc_des.Subtype     = temp_desc[2];
                        if(uvc_des.Subtype ==VS_INPUT_HEADER ){
                            memcpy(&uvc_des.vs_head, temp_desc, sizeof(VS_HEAD));
                            if(is_vs){
                                uvc_dat_ep = temp_desc[6];
                            }
                        }
                    else if((uvc_des.Subtype ==VS_FORMAT_UNCOMPRESSED) || (uvc_des.Subtype ==VS_FORMAT_MJPEG)||(uvc_des.Subtype ==VS_FORMAT_FRAME_BASED)){
                        memcpy(&uvc_des.vs_format[uvc_des.vs_format_num++], temp_desc, sizeof(VS_FORMAT));
                    }/*
                        else if(uvc_des.Subtype ==VS_FRAME_UNCOMPRESSED || uvc_des.Subtype ==VS_FRAME_MJPEG ){
                            my_memcpy(&uvc_des.vs_frame[uvc_des.vs_frame_num], temp_desc, sizeof(VS_FRAME));
                            uvc_des.vs_frame[uvc_des.vs_frame_num].wWidth = temp_desc[6]<<8|temp_desc[5];
                            uvc_des.vs_frame[uvc_des.vs_frame_num].wHeight= temp_desc[8]<<8|temp_desc[7];
                            uvc_des.vs_frame_num++;
                        }*/
                    else if( (uvc_des.Subtype ==VS_FRAME_MJPEG)||(uvc_des.Subtype ==VS_FRAME_FRAME_BASED) ){
                        memcpy(&uvc_des.vs_frame[uvc_des.vs_frame_num], temp_desc, sizeof(VS_FRAME));
                        uvc_des.vs_frame[uvc_des.vs_frame_num].wWidth = temp_desc[6]<<8|temp_desc[5];
                        uvc_des.vs_frame[uvc_des.vs_frame_num].wHeight= temp_desc[8]<<8|temp_desc[7];
                        uvc_des.vs_frame_num++;
                        _os_printf("^^^^^^^^");
                    }
                    }
                if(uvc_des.DescriptorType == STRAME_O_CONTORL && temp_desc[0]==sizeof(SINTF_DESC)){
                    memcpy(&uvc_des.intfs[uvc_des.intfs_count++],temp_desc,temp_desc[0]);
                    if(uvc_des.intfs_count>= __INTFS_STACK__)uvc_des.intfs_count =0;
                }
                if(uvc_des.DescriptorType == ENDPOINT_DESC && temp_desc[0]==(sizeof(SEP_DESC)-1)){
                    memcpy(&uvc_des.edpt[uvc_des.edpt_count++],temp_desc,temp_desc[0]);
                    if(uvc_des.edpt_count>= __INTFS_STACK__)uvc_des.edpt_count =0;
                }
            }
        }
        if(uvc_or_uac == 2) {
            uac_des.DesLen = temp_desc[0];
            uac_des.DescriptorType = temp_desc[1];  

            if(uac_des.DescriptorType == STRAME_O_CONTORL){
                au_cs=temp_desc[6];
                if(au_cs == 1)
                    uac_des.control_interface = temp_desc[2];
            }  

            if(au_cs == 1) {
                if(uac_des.DescriptorType == INTERFACE_Type) {
                    uac_des.Subtype = temp_desc[2];
                    if(uac_des.Subtype == AC_INPUT_DES) {
                        memcpy(&uac_des.ac_input[uac_des.ac_input_num], temp_desc, uac_des.DesLen);
                        uac_des.ac_input_num++;
                    }
                if(uac_des.Subtype == AC_FEATURE_DES) {
                        if(uac_des.DesLen >= sizeof(AC_FEATURE))
                            memcpy(&uac_des.ac_feature[uac_des.ac_feature_num], temp_desc, sizeof(AC_FEATURE));
                        else
                            memcpy(&uac_des.ac_feature[uac_des.ac_feature_num], temp_desc, uac_des.DesLen);
                        uac_des.ac_feature_num++;
                    }

                    if(uac_des.Subtype == AC_OUTPUT_DES) {
                        memcpy(&uac_des.ac_output[uac_des.ac_output_num], temp_desc, uac_des.DesLen);
                        uac_des.ac_output_num++;
                    }
                }
            }   
            else if(au_cs == 2) {
                if(uac_des.DescriptorType == INTERFACE_Type) {
                    uac_des.Subtype = temp_desc[2];
                    if(uac_des.Subtype == AS_General_Des) {
                        memcpy(&uac_des.as_general[uac_des.as_general_num], temp_desc, 5);
						uac_des.as_general[uac_des.as_general_num].wFormatTag = (temp_desc[6]<<8)|(temp_desc[5]);
                        uac_des.as_general_num++;
                    }
                    if(uac_des.Subtype == AS_Format_Des) {
                        memcpy(&uac_des.as_format[uac_des.as_format_num], temp_desc, 8);
                        if(temp_desc[7] <= 4) {
                            for(uint8 i=0; i<temp_desc[7]; i++)
                                uac_des.as_format[uac_des.as_format_num].tSamFreq[i] = (temp_desc[10]<<16)|(temp_desc[9]<<8)|(temp_desc[8]);
                        }
                        else {
                            uac_des.as_format[uac_des.as_format_num].bSamFreqType = 4;
                            for(uint8 i=0; i<4; i++)
                                uac_des.as_format[uac_des.as_format_num].tSamFreq[i] = (temp_desc[10]<<16)|(temp_desc[9]<<8)|(temp_desc[8]);                          
                        }
                        uac_des.as_format_num++;
                    }
                }
                else if(uac_des.DescriptorType == STRAME_O_CONTORL){
                    memcpy(&uac_des.as_interface[uac_des.as_interface_num],temp_desc,uac_des.DesLen);
                    uac_des.as_interface_num++;
                }
                else if(uac_des.DescriptorType == ENDPOINT_DESC){
                    memcpy(&uac_des.as_endpoint[uac_des.as_endpoint_num],temp_desc,uac_des.DesLen);
                    uac_des.as_endpoint_num++;
                }
            }  

        }
        temp_desc += des_len;
        desc_total_len -= des_len;
    }

	if(uvc_dat_ep){
		for(itk = 0;itk < EP_CNT;itk++){
			if(uvc_dat_ep == eptmp[itk].ep){
				_os_printf("select ep:%x\r\n",eptmp[itk].ep);
				_os_printf("ep tpye:%d\r\n",eptmp[itk].eptpye);
				_os_printf("ep len:%d\r\n",eptmp[itk].len);
				memcpy((uint8 *)&videostream,(uint8 *)&eptmp[itk],sizeof(EP_DES));				
				break;
			}
		}
	}else{
		os_printf("video stream ep error...\r\n");
	}
}

void printf_uvc_vsdesc(UVC_DES *ud)
{
    uint32 i;
    _os_printf("=======VS_HEADER============\r\n");
    _os_printf("bLength:                           %d\r\n",ud->vs_head.bLength );
    _os_printf("bDescriptorType:                   %d\r\n",ud->vs_head.bDescriptorType );
    _os_printf("bDescriptorSubtype:                %d\r\n",ud->vs_head.bDescriptorSubtype );
    _os_printf(" bNumFormats:                      %d\r\n",ud->vs_head.bNumFormats );
    _os_printf(" \r\n__________________________\r\n");
    _os_printf("=======VS_FORMAT============\r\n");
    for(i = 0;i< ud->vs_format_num;i++){
            _os_printf(" bLength:                          %d\r\n",ud->vs_format[i].bLength );
            _os_printf(" bDescriptorType:                  %d\r\n",ud->vs_format[i].bDescriptorType );
            _os_printf(" bDescriptorSubtype:               %d\r\n",ud->vs_format[i].bDescriptorSubtype );
            _os_printf(" bFrameIndex:                      %d\r\n",ud->vs_format[i].bFrameIndex );
            _os_printf(" bNumFrameDescriptors:             %d\r\n",ud->vs_format[i].bNumFrameDescriptors );
            _os_printf(" \r\n__________________________\r\n");
    }
    _os_printf("=======VS_FRAME============\r\n");
    for(i = 0;i< ud->vs_frame_num;i++){
            _os_printf("    bLength:                         %d\r\n",ud->vs_frame[i].bLength );
            _os_printf("    bDescriptorType:                 %d\r\n",ud->vs_frame[i].bDescriptorType );
            _os_printf("    bDescriptorSubtype:              %d\r\n",ud->vs_frame[i].bDescriptorSubtype );
            _os_printf("    bFrameIndex:                     %d\r\n",ud->vs_frame[i].bFrameIndex );
            _os_printf("    bmCapabilities:                  %d\r\n",ud->vs_frame[i].bmCapabilities );
            _os_printf("    wWidth:                          %d\r\n",ud->vs_frame[i].wWidth );
            _os_printf("    wHeight:                         %d\r\n",ud->vs_frame[i].wHeight );
            _os_printf(" \r\n__________________________\r\n");
    } 
}

void get_uvc_process_support(UVC_DES *ud, UVC_PROCESS *uvc_p)
{
    uvc_p->ctl_intfs = ud->control_intfs;
	uvc_p->process_unitID = ud->vc_process.bUnitID;
    uvc_p->process_ctl = ud->vc_process.bmControls;
}
void set_uvc_process_ctlval(UVC_PROCESS_INFO *uvc_p_info)
{
    uvc_p_info->brightness.ctl_val = 0x02;
    uvc_p_info->contrast.ctl_val = 0x03;
    uvc_p_info->hue.ctl_val = 0x06;
    uvc_p_info->saturation.ctl_val = 0x07;
    uvc_p_info->sharpness.ctl_val = 0x08;
    uvc_p_info->gamma.ctl_val = 0x09;
    uvc_p_info->white_bal_temp.ctl_val = 0x0A;
    uvc_p_info->white_bal_comp.ctl_val = 0x0C;
    uvc_p_info->backlight_comp.ctl_val = 0x02;
    uvc_p_info->gain.ctl_val = 0x04;
    uvc_p_info->power_line_fre.ctl_val = 0x05;
    uvc_p_info->hue_auto.ctl_val = 0x10;
    uvc_p_info->white_bal_temp_auto.ctl_val = 0x0B;
    uvc_p_info->white_bal_comp_auto.ctl_val = 0x0D;
    uvc_p_info->digital_mult.ctl_val = 0x0E;
    uvc_p_info->digital_mult_limit.ctl_val = 0x0F;
    uvc_p_info->ana_video_stan.ctl_val = 0x11;
    uvc_p_info->ana_lock_sta.ctl_val = 0x12;
    uvc_p_info->contrast_auto.ctl_val = 0x13;
}
void get_uvc_process_info(struct hgusb20_dev *p_dev, UVC_PROCESS *uvc_p, UVC_PROCESS_INFO *uvc_p_info)
{
    uint16 uid = (uvc_p->process_unitID<<8)|uvc_p->ctl_intfs;
    uint16 cs = 0;
    UVC_PROC_REQ *ptr = NULL;
	ptr = &(uvc_p_info->brightness);
    for(uint8 i=0; i<16; i++)
    {
        if(uvc_p->process_ctl & BIT(i)){
            cs = (ptr->ctl_val<<8)|0x00;
            usb_host_uvc_get_info(p_dev, uid, cs, 1, (uint8*)&(ptr->info));
            os_sleep_ms(2);
            usb_host_get_min(p_dev, uid, cs, 2, (uint8*)&(ptr->min));
            os_sleep_ms(2);
            usb_host_get_max(p_dev, uid, cs, 2, (uint8*)&(ptr->max));
            os_sleep_ms(2);
            usb_host_get_res(p_dev, uid, cs, 2, (uint8*)&(ptr->res));
            os_sleep_ms(2);
            usb_host_get_def(p_dev, uid, cs, 2, (uint8*)&(ptr->cur));
            os_printf("p_dev->usb_ctrl.error:%d\n",p_dev->usb_ctrl.error);
        }
        ptr++;
    }
}
int set_uvc_process_cur(struct hgusb20_dev *p_dev, UVC_PROCESS *uvc_p, UVC_PROCESS_INFO *uvc_p_info, UVC_PROCESS_ENUM proc, int16 cur)
{
    if(!(uvc_p->process_ctl&BIT(proc))) {
        _os_printf("request %d is not support!\n", proc);
        return 0;
    }
    UVC_PROC_REQ *ptr = NULL;
	ptr = (&(uvc_p_info->brightness)+proc);
    if(cur >= ptr->max) {
        cur = ptr->max;
        _os_printf("uvc process %d max val:%d\n", proc, ptr->max);
    }
    if(cur <= ptr->min) {
        cur = ptr->min;
        _os_printf("uvc process %d min val:%d\n", proc, ptr->min);
    }
    uint16 uid = (uvc_p->process_unitID<<8)|uvc_p->ctl_intfs;
    uint16 cs = (ptr->ctl_val<<8)|0x00;
	irq_disable(USB20DMA_IRQn);
    if(!usb_host_set_cur(p_dev, uid, cs, 2, (uint8*)&cur)) {
		irq_enable(USB20DMA_IRQn);
		_os_printf("uvc process %d err\n", proc);
		return 0;
	}
	irq_enable(USB20DMA_IRQn);
	return 1;
}

void select_uvc_intfs(UVC_DES *ud,UVCDEV *uvc)
{   uint32 i;
    uint32 temp = 0;
    uint32 intfs_indx=0;
    uint32 edpt_indx=0;
    for(i =0;i<ud->intfs_count;i++)
    {
        if(ud->intfs[i].alt_tring > temp && ud->intfs[i].iclass == UVC_CLASS)
            intfs_indx = i;
        temp = ud->intfs[i].alt_tring;
    }
    temp =0;
    for(i =0;i<ud->edpt_count;i++)
    {
        //_os_printf("temp%d plaload%d\n",temp,ud->edpt[i].pay_load);
        if(ud->edpt[i].pay_load > temp)
            edpt_indx = i;
        temp = ud->edpt[i].pay_load;
    }
//  ud->intfs_count
//  ud->edpt_count
    _os_printf("intfs indx :%d edpt_indx %d \r\n",intfs_indx,edpt_indx);
    uvc->epstrm = ud->edpt[edpt_indx].ep_addr;
    uvc->strm_pload= ud->edpt[edpt_indx].pay_load;
    uvc->strm_interval = ud->edpt[edpt_indx].interval;

    uvc->ctyp = ud->intfs[intfs_indx].iclass;
    uvc->strm_intfs=ud->intfs[intfs_indx].num;
    uvc->strm_altset=ud->intfs[intfs_indx].alt_tring;
}


int uvc_resolution2indx(uint32 width,uint32 height)
{
   uint32 i = 0;
   for(i = 0;i < uvc_des.vs_frame_num;i++)
   {
       if(uvc_des.vs_frame[i].wWidth == width && uvc_des.vs_frame[i].wHeight == height)
           return uvc_des.vs_frame[i].bFrameIndex;
    }
    
    return -1;
}

volatile uint8_t uvc_format = 1;    //1:jpg    2:264
static uint8 select_uvc_format(UVC_DES *ud,UVC_SLEST *us,uint8 index)
{
	uint8 j;
    uint32 i;
    uint8 format_type;
    //slect vs format
    memset((uint8 *)us,0,sizeof(UVC_SLEST));
	for(j = 0;j < 2;j++){
		if(j == 0){
			if(IS_UVC_JPEG){
				format_type = VS_FORMAT_MJPEG;	
			}
			else{
				format_type = VS_FORMAT_UNCOMPRESSED;
			}
		}else{
			format_type = VS_FORMAT_FRAME_BASED;	
		}

        for(i=0;i<ud->vs_format_num;i++){
            if(ud->vs_format[i].bDescriptorSubtype ==  format_type){
                us->vs_format =     ud->vs_format[i];
				if(format_type == VS_FORMAT_MJPEG){
					uvc_format = 1;
				}else if(format_type == VS_FORMAT_FRAME_BASED){
					uvc_format = 2;
				}
				
                break;
            }
        }
	}
		
    if(us->vs_format.bFrameIndex == 0){
        _os_printf("can't suppost UVCFORMAT\r\n");
        return FALSE;
    }

     for(i=0;i<ud->vs_frame_num;i++){
        if(ud->vs_frame[i].bFrameIndex == index)
            us->vs_frame =  ud->vs_frame[i];
     }
    _os_printf("FORMAT INDX %d\r\n",us->vs_format.bFrameIndex);
    _os_printf("FRAME  INDX %d\r\n",us->vs_frame.bFrameIndex);
    return 1;
}

void select_resolution(uint8 *p,UVC_SLEST *su)
{
    p[2] = su->vs_format.bFrameIndex;
    p[3] = su->vs_frame.bFrameIndex;
    commit.bmhint = (p[1] << 8) + p[0];
    commit.bFormatIndex = p[2];
    commit.bFrameIndex = p[3];
    commit.dwFrameInterval = (p[7] << 24) +(p[6] << 16) +(p[5] << 8) +(p[4] << 0) ;
    commit.wKeyFrameRate = (p[9] << 8) + p[8];
    commit.wPFrameRate = (p[11] << 8) + p[10];
    commit.wCompQuality = (p[13] << 8) + p[12];
    commit.wCompWindowSize = (p[15] << 8) + p[14];
    commit.wDelay = (p[17] << 8) + p[16];
    commit.dwMaxVideoFrameSize=(p[21] << 24) +(p[20] << 16) +(p[19] << 8) +(p[18] << 0);
    commit.dwMaxPayloadTSize = (p[25] << 24) +(p[24] << 16) +(p[23] << 8) +(p[22] << 0);
    
    // _os_printf(".bmhint:%x\r\n",        commit.bmhint);
    // _os_printf(".bFormatIndex:%x\r\n",  commit.bFormatIndex);
    // _os_printf(".bFrameIndex:%x\r\n",   commit.bFrameIndex);
    // _os_printf(".dwFrameInterval:%x\r\n",commit.dwFrameInterval);
    // _os_printf(".wKeyFrameRate:%x\r\n", commit.wKeyFrameRate);
    // _os_printf(".wPFrameRate:%x\r\n",   commit.wPFrameRate);
    // _os_printf(".wCompQuality:%x\r\n",  commit.wCompQuality);
    // _os_printf(".wCompWindowSize:%x\r\n",commit.wCompWindowSize);
    // _os_printf(".wDelay:%x\r\n",        commit.wDelay);
    // _os_printf(".dwMaxVideoFrameSize:%x\r\n",commit.dwMaxVideoFrameSize);
    // _os_printf(".dwMaxPayloadTSize:%x\r\n",commit.dwMaxPayloadTSize);   
}

void cnvr(uint8 *p)
{
    commit.bmhint = (p[1] << 8) + p[0];
    commit.bFormatIndex = p[2];
    commit.bFrameIndex = p[3];
    commit.dwFrameInterval = (p[7] << 24) +(p[6] << 16) +(p[5] << 8) +(p[4] << 0) ;
    commit.wKeyFrameRate = (p[9] << 8) + p[8];
    commit.wPFrameRate = (p[11] << 8) + p[10];
    commit.wCompQuality = (p[13] << 8) + p[12];
    commit.wCompWindowSize = (p[15] << 8) + p[14];
    commit.wDelay = (p[17] << 8) + p[16];
    commit.dwMaxVideoFrameSize=(p[21] << 24) +(p[20] << 16) +(p[19] << 8) +(p[18] << 0) ;
    commit.dwMaxPayloadTSize = (p[25] << 24) +(p[24] << 16) +(p[23] << 8) +(p[22] << 0) ;
}




int8 select_cur(struct hgusb20_dev *p_dev,uint8 index)
{
	uint8 pcommit[26];
    os_sleep_ms(5); //50
    if(!select_uvc_format(&uvc_des,&uvc_select,index))
    {
        return 0xFF;//error
    }
    
    uvc_cur_indx = index;  
    if(uvc.strm_altset > 0){
        if(!usb_host_set_interface(p_dev,uvc.strm_intfs ,0)){
            _os_printf("select interface error\r\n");
            return 0xFF;
        }
    }


    if (!usb_host_get_cur(p_dev,1,0x0100,0x1a,pcommit)) {   //uvc_probe_control
    	_os_printf("select cur error\r\n");
        return 0xFF;
    }
    select_resolution(pcommit,&uvc_select);
    if (!usb_host_set_cur(p_dev,1,0x0100,0x1a,pcommit)) {   //uvc_probe_control
		return 0xFF;
    }
    if (!usb_host_get_cur(p_dev,1,0x0100,0x1a,pcommit)) {   //uvc_probe_control
		return 0xFF;
    }
    cnvr(pcommit);
    _os_printf("select cur finish\r\n");
    // for(itk = 0;itk < 26;itk++){
    //     _os_printf("%x ",pcommit[itk]);
    // }
    // _os_printf("\r\n");

    if (!usb_host_set_cur(p_dev,1,0x0100,0x1a,pcommit)) {   //uvc_probe_control
		return 0xFF;
    }
    if (!usb_host_get_cur(p_dev,1,0x0100,0x1a,pcommit)) {   //uvc_probe_control
		return 0xFF;
    }
    cnvr(pcommit);
    _os_printf("select cur finish2\r\n");
    // for(itk = 0;itk < 26;itk++){
    //     _os_printf("%x ",pcommit[itk]);
    // }
    // _os_printf("\r\n");
	if(commit.wCompWindowSize == 0x1e){
		pcommit[4] = 0x15;
		pcommit[5] = 0x16;
		pcommit[6] = 0x05;
		pcommit[7] = 0;
	}

    if (!usb_host_set_cur(p_dev,1,0x0200,0x1a,pcommit)) {
		return 0xFF;
    }

    //uvc on
    _os_printf("strm_altset:%d\r\n",uvc.strm_altset);
    os_sleep_ms(5);//250
	if((videostream.eptpye&0X03) == USB_ENDPOINT_XFER_ISOC){
        if(uvc.strm_altset > 0){
		    //usb_host_set_interface(p_dev,uvc.strm_intfs ,uvc.strm_altset);
        }
	}
	//_os_printf("---p_dev->usb_ctrl.error:%d\r\n",p_dev->usb_ctrl.error);
    return 0;
}



uint8 uvc_dpi = 0;
bool enum_set_resolution(struct hgusb20_dev *p_dev,uint8 resolution)
{
    int indx = 0; 
	_os_printf("resolution:%d\r\n",resolution);
    if(resolution == 1)//VGA
    {
        indx=uvc_resolution2indx(640,480);
        uvc_dpi = 1;
    }
    if(resolution == 2){//720P
        indx=uvc_resolution2indx(1280,720);
		_os_printf("uvc can set resolution :1280*720\r\n");		
        uvc_dpi = 2;
        if(indx == -1){
            indx=uvc_resolution2indx(640,480);
            uvc_dpi = 1;
            _os_printf("uvc can not set resolution :720p\r\n");
        }
    }
    return select_cur(p_dev,indx);
}
extern int enum_deal;
extern int uvc_default_dpi();
extern void usb_host_enum_finish_init(uint32_t uvc_format);
void uvc_stream_open(struct hgusb20_dev *p_dev,uint8 enable){
    uint8 ret;
    int dpi = UVC_720P;
	uac_stop();
    if(enable){

        dpi = uvc_default_dpi();
        ret = enum_set_resolution(p_dev,dpi);
        if(ret != 0){
            os_printf("enum set resolution failed\n");

        } 
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        if(uvc.strm_altset > 0){
            uvc_room_init_mjpeg(p_dev);
            usb_host_set_interface(p_dev,uvc.strm_intfs ,uvc.strm_altset);
            usb_host_enum_finish_init(uvc_format);
            hgusb20_ep_rx_kick(p_dev, UVC_EP, (uint32)(uvc_dat[0]+uvc_head_res), 0x400);
        }else{
                uvc_room_init_mjpeg(p_dev);
            	if((videostream.eptpye&0X03) == USB_ENDPOINT_XFER_BULK){
                    hgusb20_host_rx_ep_init(p_dev, UVC_EP, uvc.epstrm, USB_ENDPOINT_XFER_BULK, 1024);
                    uvc_head_res = BULK_HEAD;
                }else{
                    hgusb20_host_rx_ep_init(p_dev, UVC_EP, uvc.epstrm, USB_ENDPOINT_XFER_ISOC, 1024);
                    uvc_head_res = ISO_HEAD;
                }
                os_printf("%s %d\n",__FUNCTION__,__LINE__);
                usb_host_enum_finish_init(uvc_format);
                os_printf("%s %d\n",__FUNCTION__,__LINE__);
                hgusb20_ep_rx_kick(p_dev, UVC_EP, (uint32)(uvc_dat[0]+uvc_head_res), 0x400);
                enum_deal = 1;
                os_printf("%s %d\n",__FUNCTION__,__LINE__);
        }
    }else{
        if(uvc.strm_altset > 0){
            hgusb20_ep_rx_abort(p_dev, UVC_EP);
            usb_host_set_interface(p_dev,uvc.strm_intfs ,0);
        }else{
            hgusb20_ep_rx_abort(p_dev, UVC_EP);
            uvc_clear_ep_feature(p_dev, uvc.epstrm);
            memset(uvc_dat[0], 0, sizeof(uvc_dat[0]));
        }
    }
	uac_start(p_dev, usbmic_dat);
}


void uvc_ep_init(struct hgusb20_dev *p_dev)
{
	if((videostream.eptpye&0X03) == USB_ENDPOINT_XFER_BULK){
		hgusb20_host_rx_ep_init(p_dev, UVC_EP, uvc.epstrm, USB_ENDPOINT_XFER_BULK, 1024);
		uvc_head_res = BULK_HEAD;
	}else{
		hgusb20_host_rx_ep_init(p_dev, UVC_EP, uvc.epstrm, USB_ENDPOINT_XFER_ISOC, 1024);
		uvc_head_res = ISO_HEAD;
	}

}

int uvc_ep_bulk_or_isoc()
{
    if((videostream.eptpye&0X03) == USB_ENDPOINT_XFER_BULK){
        return USB_ENDPOINT_XFER_BULK;
    }
    else{
        return USB_ENDPOINT_XFER_ISOC;
    }
    return RET_ERR;
}

//usb host枚举成功后会执行的一个函数,默认为空,使用弱引用,返回是一个分辨率的值
__attribute__((weak)) void usb_host_enum_finish_init(uint32_t uvc_format)
{
}

__attribute__((weak)) int uvc_default_dpi()
{
    return UVC_720P;
}

void split_uac_intfs(UAC_DES *ud,UACDEV *uac)
{
    int8 temp = -1;
    uint8 intfs_indx = 0;
    uint8 edps_indx = 0;
    uint8 altset_indx = 0;
	os_memset((uint8 *)uac, 0, sizeof(UACDEV));
	
    for(uint8 i=0; i<ud->ac_input_num; i++)
    {
        temp = -1;
		  
        if((ud->ac_input[i].wTerminalType==USB_streaming) && ((ud->ac_output[i].wTerminalType&0x03ff)>0x0300))
        {
            uac->spk_type = ud->ac_output[i].wTerminalType;
            altset_indx = 0;
            uac->spk_feaunitID = ud->ac_feature[i].bUnitID;
            os_memcpy(uac->spk_feaunit_ctl, ud->ac_feature[i].bmaControls, 2);
            for(uint8 j=intfs_indx; j<ud->as_interface_num; j++)
            {
                if((ud->as_interface[j].bAlternateSetting <= temp) || (ud->as_interface[j].bInterfaceClass != UAC_CLASS))
                    break;
                uac->spk_strm_intfs = ud->as_interface[j].bInterfaceNumber;
                temp = ud->as_interface[j].bAlternateSetting;
                uac->spk_strm_altset[altset_indx] = ud->as_interface[j].bAlternateSetting;
                if(ud->as_interface[j].bNumEndpoints > 0)
                {
                    uac->spk_fmttag[altset_indx] = ud->as_general[edps_indx].wFormatTag;
                    uac->spk_fmttype[altset_indx] = ud->as_format[edps_indx].bFormatType;
                    uac->spk_nrchl[altset_indx] = ud->as_format[edps_indx].bNrChannels;
                    uac->spk_fmsize[altset_indx] = ud->as_format[edps_indx].bSubframeSize;
                    uac->spk_bitres[altset_indx] = ud->as_format[edps_indx].bBitResolution;
                    uac->spk_samfretype[altset_indx] = ud->as_format[edps_indx].bSamFreqType;
                    for(uint8 k=0; k<uac->spk_samfretype[altset_indx]; k++)
                    {
                        uac->spk_samfre[altset_indx][k] = ud->as_format[edps_indx].tSamFreq[k];
                    }
                    uac->spk_epstrm[altset_indx] = ud->as_endpoint[edps_indx].bEndpointAddress;
                    uac->spk_strm_pload[altset_indx] = ud->as_endpoint[edps_indx].wMaxPacketSize;
                    uac->spk_eptype[altset_indx] = ud->as_endpoint[edps_indx].bmAttributes;
                    edps_indx++;
                }
                altset_indx++;
                intfs_indx++;
            }
        }  
        else if(((ud->ac_input[i].wTerminalType&0x02ff)>0x0200) && (ud->ac_output[i].wTerminalType==USB_streaming))
        {
            uac->mic_type = ud->ac_input[i].wTerminalType;
            altset_indx = 0;
            uac->mic_feaunitID = ud->ac_feature[i].bUnitID;
            os_memcpy(uac->mic_feaunit_ctl, ud->ac_feature[i].bmaControls, 2);
            for(uint8 j=intfs_indx; j<ud->as_interface_num; j++)
            {
                if((ud->as_interface[j].bAlternateSetting <= temp) || (ud->as_interface[j].bInterfaceClass != UAC_CLASS))
                    break;
                uac->mic_strm_intfs = ud->as_interface[j].bInterfaceNumber;
                temp = ud->as_interface[j].bAlternateSetting;
                uac->mic_strm_altset[altset_indx] = ud->as_interface[j].bAlternateSetting;
                if(ud->as_interface[j].bNumEndpoints > 0)
                {
                    uac->mic_fmttag[altset_indx] = ud->as_general[edps_indx].wFormatTag;
                    uac->mic_fmttype[altset_indx] = ud->as_format[edps_indx].bFormatType;
                    uac->mic_nrchl[altset_indx] = ud->as_format[edps_indx].bNrChannels;
                    uac->mic_fmsize[altset_indx] = ud->as_format[edps_indx].bSubframeSize;
                    uac->mic_bitres[altset_indx] = ud->as_format[edps_indx].bBitResolution;
                    uac->mic_samfretype[altset_indx] = ud->as_format[edps_indx].bSamFreqType;
                    for(uint8 k=0; k<uac->mic_samfretype[altset_indx]; k++)
                    {
                        uac->mic_samfre[altset_indx][k] = ud->as_format[edps_indx].tSamFreq[k];
                    }
                    uac->mic_epstrm[altset_indx] = ud->as_endpoint[edps_indx].bEndpointAddress;
                    uac->mic_strm_pload[altset_indx] = ud->as_endpoint[edps_indx].wMaxPacketSize;
                    uac->mic_eptype[altset_indx] = ud->as_endpoint[edps_indx].bmAttributes;
                    edps_indx++;
                }
                altset_indx++;
                intfs_indx++;
            }
        }  
    }
	uac->ctl_intfs = ud->control_interface;
}

void select_mic_intfs(UACDEV *uac, MICDEV *mic)
{
    memset((uint8 *)mic, 0, sizeof(MICDEV));
    for(uint8 i=0; i<4; i++)
    {
        /*PCM 单通道 16bit*/
        if((uac->mic_fmttag[i]==0x0001) && (uac->mic_fmttype[i]==1) && 
            (uac->mic_nrchl[i]==1) && (uac->mic_bitres[i]==16))
        {
            mic->strm_intfs = uac->mic_strm_intfs;
            mic->strm_altset = i;
            mic->epstrm = uac->mic_epstrm[i];
            mic->eptype = uac->mic_eptype[i];
            mic->strm_pload = uac->mic_strm_pload[i];
        }
    }
}
void set_usbmic_intfs(struct hgusb20_dev *p_dev, MICDEV *mic, uint8 altset)
{
    usb_host_set_interface(p_dev, mic->strm_intfs, altset);
}
void usbmic_ep_init(struct hgusb20_dev *p_dev, MICDEV *mic)
{
	hgusb20_host_rx_ep_init(p_dev, UAC_EP, mic->epstrm, 1, 1024);
	if(p_dev->usb_ctrl.bus_high_speed)
		hgusb20_host_set_interval(p_dev, UAC_EP, 0, 4);
	else
		hgusb20_host_set_interval(p_dev, UAC_EP, 0, 1);
    hgusb20_ep_rx_kick(p_dev, UAC_EP, (uint32)usbmic_dat, 1024);
}
void usbmic_stream_enable(struct hgusb20_dev *p_dev, MICDEV *mic, uint8 enable)
{
	int ret = 0;
    if(enable) {
        ret = usb_host_set_interface(p_dev, mic->strm_intfs, mic->strm_altset);
    } else {
        ret = usb_host_set_interface(p_dev, mic->strm_intfs, 0);
	}
	if(!ret) {
		os_printf("usbmic SetInterface err\n");
	}
}

void select_spk_intfs(UACDEV *uac, SPKDEV *spk)
{
    memset((uint8 *)spk, 0, sizeof(SPKDEV));
    for(uint8 i=0; i<4; i++)
    {
        /*PCM 单通道 16bit*/
        if((uac->spk_fmttag[i]==0x0001) && (uac->spk_fmttype[i]==1) && 
            (uac->spk_nrchl[i]==1) && (uac->spk_bitres[i]==16))
        {
            spk->strm_intfs = uac->spk_strm_intfs;
            spk->strm_altset = i;
            spk->epstrm = uac->spk_epstrm[i];
            spk->eptype = uac->spk_eptype[i];
            spk->strm_pload = uac->spk_strm_pload[i];
        }
    }
}
void set_usbspk_intfs(struct hgusb20_dev *p_dev, SPKDEV *spk, uint8 altset)
{
    usb_host_set_interface(p_dev, spk->strm_intfs, altset);
}
void usbspk_ep_init(struct hgusb20_dev *p_dev, SPKDEV *spk)
{
	hgusb20_host_tx_ep_init(p_dev, UAC_EP, spk->epstrm, 1, 1024);
	if(p_dev->usb_ctrl.bus_high_speed)
		hgusb20_host_set_interval(p_dev, UAC_EP, 1, 4);
	else
		hgusb20_host_set_interval(p_dev, UAC_EP, 1, 1);
	extern void usbspk_tx(struct hgusb20_dev *p_dev, uint8_t ep, uint32_t len);
	usbspk_tx(p_dev,UAC_EP,16);
}
void usbspk_stream_enable(struct hgusb20_dev *p_dev, SPKDEV *spk, uint8 enable)
{
	int ret = 0;
    if(enable) { 
        ret = usb_host_set_interface(p_dev, spk->strm_intfs, spk->strm_altset);
    } else {
        ret = usb_host_set_interface(p_dev, spk->strm_intfs, 0);
	}
	if(!ret) {
		os_printf("usbspk SetInterface err\n");
	}
} 
void get_uac_vol(struct hgusb20_dev *p_dev, UACDEV *uac, uint8 audio_dev)
{
    int8 channel = -1;
    uint8 *feaunit_ctl = NULL;
    uint8 feaunitID = 0;   
    int16 *min_vol = NULL, *max_vol = NULL;

	if(audio_dev == 0)
	{
		feaunit_ctl = uac->spk_feaunit_ctl;
        feaunitID = uac->spk_feaunitID;
        min_vol = &uac->spk_min_vol;
        max_vol = &uac->spk_max_vol;
	}
    else if(audio_dev == 1)
    {
		feaunit_ctl = uac->mic_feaunit_ctl;
        feaunitID = uac->mic_feaunitID;
        min_vol = &uac->mic_min_vol;
        max_vol = &uac->mic_max_vol;
    }

	for(uint8 i=0; i<2; i++) {
        if(feaunit_ctl[i] & 0x02) {
            channel = i;
            break;
        }
    }
    if(channel == -1) {
        _os_printf("UAC DEV:%d no channel to get the volume\n", audio_dev);
        return;
    }  
    uint16 uid = (feaunitID<<8)|uac->ctl_intfs;
    uint16 cs = (0x02<<8)|channel;
    uac_stop();
	if(!usb_host_get_min(p_dev, uid, cs, 2, (uint8*)min_vol)) {
		_os_printf("get uac dev:%d min vol err\n", audio_dev);
	}
	else {
		_os_printf("uac dev:%d min vol:%d\n", audio_dev, *min_vol);
	}	
	if(!usb_host_get_max(p_dev, uid, cs, 2, (uint8*)max_vol)) {
		_os_printf("get uac dev:%d max vol err\n", audio_dev);
	}
	else {
		_os_printf("uac dev:%d max vol:%d\n", audio_dev, *max_vol);
	}	
    uac_start(p_dev, usbmic_dat);   
}
int set_uac_mute(struct hgusb20_dev *p_dev, UACDEV *uac, uint8 audio_dev, uint8 enable)  
{
    int8 channel = -1;
    uint8 *feaunit_ctl = NULL;
    uint8 feaunitID = 0;
	if(audio_dev == 0)
	{
		feaunit_ctl = uac->spk_feaunit_ctl;
        feaunitID = uac->spk_feaunitID;
	}
    else if(audio_dev == 1)
    {
		feaunit_ctl = uac->mic_feaunit_ctl;
        feaunitID = uac->mic_feaunitID;
    }
	for(uint8 i=0; i<2; i++) {
        if(feaunit_ctl[i] & 0x01) {
            channel = i;
            break;
        }
    }
    if(channel == -1) {
        _os_printf("UAC DEV:%d no channel to set mute\n", audio_dev);
        return 0;
    }
    uint16 uid = (feaunitID<<8)|uac->ctl_intfs;
    uint16 cs = (0x01<<8)|channel;
    uac_stop();
	if(!usb_host_set_cur(p_dev, uid, cs, 1, &enable)) {
        _os_printf("set uac dev:%d mute err\n", audio_dev);
    }
    uac_start(p_dev, usbmic_dat);
    return 1;
}

int set_uac_volume(struct hgusb20_dev *p_dev, UACDEV *uac, uint8 audio_dev, int16 volume)
{
    int8 channel = -1;
    uint8 *feaunit_ctl = NULL;
    uint8 feaunitID = 0;
    int16 min_vol = 0x00, max_vol = 0x00;

	if(audio_dev == 0)
	{
		feaunit_ctl = uac->spk_feaunit_ctl;
        feaunitID = uac->spk_feaunitID;
        min_vol = uac->spk_min_vol;
        max_vol = uac->spk_max_vol;
	}
    else if(audio_dev == 1)
    {
		feaunit_ctl = uac->mic_feaunit_ctl;
        feaunitID = uac->mic_feaunitID;
        min_vol = uac->mic_min_vol;
        max_vol = uac->mic_max_vol;
    }

	for(uint8 i=0; i<2; i++) {
        if(feaunit_ctl[i] & 0x02) {
            channel = i;
            break;
        }
    }
    if(channel == -1) {
        _os_printf("UAC DEV:%d no channel to set the volume\n", audio_dev);
        return 0;
    }

    if(volume <= min_vol){
        volume = min_vol;
        _os_printf("UAC DEV:%d MIN VOLUME:%x\n",audio_dev, min_vol);			
    }
    if(volume>=max_vol) {
        volume = max_vol;
        _os_printf("UAC DEV:%d MAX VOLUME:%x\n",audio_dev, max_vol);			
    }

    uint16 uid = (feaunitID<<8)|uac->ctl_intfs;
    uint16 cs = (0x02<<8)|channel;
    uac_stop();
	if(!usb_host_set_cur(p_dev, uid, cs, 2, (uint8*)&volume)) {
		_os_printf("set uac dev:%d vol err\n", audio_dev);
	}
    uac_start(p_dev, usbmic_dat);
	return 1;

}

void uac_stream_open(struct hgusb20_dev *p_dev, uint8_t enable)
{
    uac_stop();
    if((uac.spk_type == Speaker) && (uac.mic_type == Microphone)) {
		usbspk_stream_enable(p_dev, (SPKDEV*)&usb_spk, enable);
        os_sleep_ms(10);
		usbmic_stream_enable(p_dev, (MICDEV*)&usb_mic, enable);
	}
    if(enable)
        uac_start(p_dev, usbmic_dat);
}
extern uint8 uac_open;
extern uint8_t uvc_open;
extern uint8_t uac_run;
extern void usbmic_enum_finish(void);
extern void usbspk_enum_finish(void);
uint8 usb_host_ep0_enum_uvc(struct hgusb20_dev *p_dev)
{
    //uint8 ret;
    uint8 addr = 0;
    uint16 desc_len;
    //int dpi = UVC_720P;
    addr = uvc_set_addr;
    uac_open = 0;
    uac_run = 0;
    irq_disable(USB20MC_IRQn);
    usb_host_set_address(p_dev, addr);                              //设置设备地址
    hgusb20_set_address(p_dev, addr);
    os_sleep_ms(5);
    
    if (!usb_host_get_device_descriptor_2_do(p_dev)) {          //第二次获取失败，一般是设地址错误
        _os_printf("SetAddr Err\r\n");
        goto __enum_err;
    }

    if (!usb_host_get_configuration_descriptor_1(p_dev, 0)) {
        goto __enum_err;
    }
    desc_len = (HOST_EP0_BUF[3] << 8)+ HOST_EP0_BUF[2];
    if(desc_len == 0){
        _os_printf("< configuration dec len er>\n");
        goto __enum_err;
    }   
	uvc_cfg_des = malloc(2014);
    if (!usb_host_get_configuration_descriptor_3(p_dev, 0, desc_len, uvc_cfg_des)) {
        _os_printf("GetCfg Err\r\n");
        goto __enum_err;
    }
    _os_printf("desc_len:%d\r\n",desc_len);
    analysis_uvc_desc(uvc_cfg_des,desc_len);
	free(uvc_cfg_des);
    //printf_uvc_vsdesc(&uvc_des);
    
    select_uvc_intfs(&uvc_des,(UVCDEV *)&uvc);
	split_uac_intfs(&uac_des,(UACDEV *)&uac);
    usb_host_set_configuration(p_dev, 1);       //设置配置
    
    os_sleep_ms(5);
    if (uvc.ctyp == UVC_CLASS) {
        _os_printf("strm_intfs:%x, epstrm:%x, strm_pload: %x, strm_altset:%x \r\n",uvc.strm_intfs ,uvc.epstrm,uvc.strm_pload, uvc.strm_altset);
        get_uvc_process_support(&uvc_des, &uvc_proc);
        set_uvc_process_ctlval(&uvc_proc_info);
        get_uvc_process_info(p_dev, &uvc_proc, &uvc_proc_info);
        // dpi = uvc_default_dpi();
        // ret = enum_set_resolution(p_dev,dpi);
        // if(ret != 0){
        //     goto __enum_err;
        // } 
		usb_host_enum_finish_init(uvc_format);
        uvc_room_init_mjpeg();

        _os_printf(" uvc.epstrm %x,uvc.strm_pload %x,uvc.strm_interval %x\r\n",uvc.epstrm&0x7f,uvc.strm_pload,uvc.strm_interval);
        uvc_ep_init(p_dev);

        if(uvc.strm_altset > 0)
        {
            usb_host_set_interface(p_dev,uvc.strm_intfs ,0);
        }

		os_sleep_ms(10);
        //if(uvc_host_init(uvc_addr_get(), uvc.epstrm,-1,uvc.strm_pload,uvc.strm_interval))
    //#if EN_UVC_TAKE_PHOTO
    //      uvc_actech_as_control();
    //#endif
    //      uvc_actech_as_usensor();
    } 
    if((uac.spk_type == Speaker) && (uac.mic_type == Microphone))
    {
		select_mic_intfs((UACDEV*)&uac, (MICDEV*)&usb_mic);
        select_spk_intfs((UACDEV*)&uac, (SPKDEV*)&usb_spk);
        usbspk_stream_enable(p_dev, (SPKDEV*)&usb_spk, 0);
		os_sleep_ms(5);
        usbmic_stream_enable(p_dev, (MICDEV*)&usb_mic, 0);
		os_sleep_ms(5);
        get_uac_vol(p_dev, (UACDEV*)&uac, 0);
        get_uac_vol(p_dev, (UACDEV*)&uac, 1);
        set_uac_mute(p_dev, (UACDEV*)&uac, 0, 0);
        set_uac_volume(p_dev, (UACDEV*)&uac, 0, 0);
        usbspk_stream_enable(p_dev, (SPKDEV*)&usb_spk, 1);
		usbmic_stream_enable(p_dev, (MICDEV*)&usb_mic, 1);
    }
    if (p_dev->usb_ctrl.error) {
        _os_printf("Enum ERR\r\n");
        goto __enum_err;
    }
    irq_enable(USB20MC_IRQn);
	if((uac.spk_type == Speaker) && (uac.mic_type == Microphone)) {
	 	uac_open = 1;
	 	uac_run = 1;
	 	usbmic_room_init();
		usbspk_room_init(16);
		usbmic_enum_finish();	
	 	usbspk_enum_finish();
	 	usbmic_ep_init(p_dev, (MICDEV*)&usb_mic);
		usbspk_ep_init(p_dev, (SPKDEV*)&usb_spk);
	}
	os_sleep_ms(20);
	if(uvc_open){
        uvc_stream_open(p_dev,1);
    }
    _os_printf("Enum OK\r\n");
    return TRUE;

__enum_err:
    irq_enable(USB20MC_IRQn);
    return FALSE;
}





uint8 *hub_des;//[2014] = {0};
HUB_manage hub_manage;


uint8 get_int_enpoint(uint8 *desc)
{
    uint8 *temp = desc;
    uint32 temp_len =desc[0];
    while(temp_len){
        temp_len =temp[0];
        if(temp[0] == 0x7 && temp[1] == 0x5&&temp[3] == 0x3)
            return (temp[2]&0x7f);
        else{ 
            temp+=temp_len;
        }
    }
    return FALSE;
}

uint8 get_hub_port_num(uint8 *desc)
{
    
    uint8 *temp=desc;
    //uint32 len = desc[0];
    if(temp[0] == 0x9 && temp[1] == 0x29)
        return temp[2];
    else
        return FALSE;   
}


bool usb_hub_get_status(struct hgusb20_dev *p_dev,uint8 *pinf)
{
	bool ret;
    p_dev->usb_ctrl.cmd.rtype = 0xa0;
    p_dev->usb_ctrl.cmd.request = 0;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = 0;
    p_dev->usb_ctrl.cmd.index = 0;
    p_dev->usb_ctrl.cmd.length = 4;
    ret = usb_host_ep0_request(p_dev, pinf);
    return ret;
}

bool usb_hub_set_port_feature(struct hgusb20_dev *p_dev,uint8 port,uint8 feature)
{
	bool ret;
    p_dev->usb_ctrl.cmd.rtype = 0x23;
    p_dev->usb_ctrl.cmd.request = 0x03;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = feature;
    p_dev->usb_ctrl.cmd.index = port;
    p_dev->usb_ctrl.cmd.length = 0;
    p_dev->usb_ctrl.error = 0;
    ret = usb_host_ep0_request(p_dev, NULL);
    return ret;
}

bool usb_host_hub_get_descriptor(struct hgusb20_dev *p_dev, uint16 len)
{
	bool ret;
    p_dev->usb_ctrl.cmd.rtype = 0xa0;
    p_dev->usb_ctrl.cmd.request = 0x06;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = 0;
    p_dev->usb_ctrl.cmd.index = 0;
    p_dev->usb_ctrl.cmd.length = len;


    ret = usb_host_ep0_request(p_dev, p_dev->usb_ep0_rxbuf);
	return ret;

}

bool usb_host_get_hub_descriptor(struct hgusb20_dev *p_dev,uint8* buf)
{
	bool ret;
    ret = usb_host_hub_get_descriptor(p_dev, 64);         //第一次，尝试获取设备描述符
    //p_dev->usb_host.ep0_pkt = p_dev->usb_ep0buf[7];                             //Max Pkt
    memcpy(buf,p_dev->usb_ep0_rxbuf,64);   
    return ret;
}


void usb_host_hub_init_do(struct hgusb20_dev *p_dev)
{    
    hgusb20_host_rx_ep_init(p_dev, HUB_EP, hub_manage.int_endpoint, USB_ENDPOINT_XFER_BULK, 4);
    hgusb20_set_address(p_dev, hub_set_addr);
    hgusb20_ep_rx_kick(p_dev, HUB_EP, (uint32)uvc_dat[0], 0x4);
}

bool usb_hub_get_port_status(struct hgusb20_dev *p_dev,uint8 *desc,uint8 port)
{
	bool ret;
    p_dev->usb_ctrl.cmd.rtype = 0xa3;
    p_dev->usb_ctrl.cmd.request = 0;                 //Get Descriptor
    p_dev->usb_ctrl.cmd.value = 0;
    p_dev->usb_ctrl.cmd.index = port;
    p_dev->usb_ctrl.cmd.length = 4;
    p_dev->usb_ctrl.error = 0;
	p_dev->usb_host.ep0_pkt = 255;
    ret = usb_host_ep0_request(p_dev, desc);
    return ret;
}


void hub_ep_init(struct hgusb20_dev *p_dev)
{
    usb_host_hub_init_do(p_dev);            //ep init
    //hub_isr_init(p_dev);
    irq_enable(USB20MC_IRQn);
    irq_enable(USB20DMA_IRQn);
}



uint8 usb_host_ep0_enum_hub(struct hgusb20_dev *p_dev)
{
    uint8 ret;
    int i;
    uint8 addr = 0;
    uint16 desc_len;

    uint8 rst = hgusb20_host_reset(p_dev);
    if (rst) {
        return rst;
    }
    rst = hgusb20_host_reset(p_dev);
    if (rst) { 
        return rst;
    }

    if (!usb_host_get_device_descriptor_2_do(p_dev)) {
        _os_printf("GetDesc Err\r\n");
        return FALSE;
    }

    if (hgusb20_host_reset(p_dev)) {
        return FALSE;
    }
    if (p_dev->usb_ctrl.bus_high_speed) {
        _os_printf("high_speed device plug in\r\n");
    } else {
        _os_printf("full_speed device plug in\r\n");
    }    
    addr = hub_set_addr;
    //hub_set_addr++;
    //uvc_set_addr = 4;
    //addr = uvc_set_addr;
    usb_host_set_address(p_dev, hub_set_addr);                              //设置设备地址
    hgusb20_set_address(p_dev, addr);
    os_sleep_ms(10);

    if (!usb_host_get_device_descriptor_2_do(p_dev)) {          //第二次获取失败，一般是设地址错误
        _os_printf("SetAddr Err\r\n");
        return FALSE;
    }
    //usb_host_get_device_end(usb_sw);                     //获取设备描述符后留接口
    if (!usb_host_get_configuration_descriptor_1(p_dev, 0)) {
        return FALSE;
    }
    desc_len = (HOST_EP0_BUF[3] << 8)+ HOST_EP0_BUF[2];
    if(desc_len == 0){
        _os_printf("< configuration dec len er>\n");
        return FALSE;   
    } 
	hub_des = malloc(2014);
    if (!usb_host_get_configuration_descriptor_3(p_dev, 0, desc_len, hub_des)) {
        _os_printf("GetCfg Err\r\n");
		free(hub_des);
        return FALSE;
    }
    _os_printf("desc_len:%d\r\n",desc_len);
    hub_manage.int_endpoint = get_int_enpoint(hub_des);
    usb_host_set_configuration(p_dev, 1);        //设置配置
    
    os_sleep_ms(10);
    ret = usb_host_get_hub_descriptor(p_dev,hub_des);
    if(ret != 1){
        _os_printf("hub_descriptor err\r\n");	
		free(hub_des);
        return ret;
    }   
    os_sleep_ms(100);
    hub_manage.port_num = get_hub_port_num(hub_des);
    p_dev->usb_ctrl.error = 0;
    ret = usb_hub_get_status(p_dev,hub_des);
    if(ret != 1){
        _os_printf("get status err\r\n");		
		free(hub_des);
        return ret;
    }
    _os_printf("hub_des:%d port_num:%d  int_endpoint:%d\r\n",hub_des[0],hub_manage.port_num,hub_manage.int_endpoint);
    if(hub_des[0]==0)
    {
        for(i=1;i<hub_manage.port_num+1;i++){
            ret=usb_hub_set_port_feature(p_dev,i,0x08);
            if(ret != 1){			
				free(hub_des);
				return ret;
			}
        //  _os_printf("set port feature :%d \n",i);
        }
    }   

    for(i=1;i<hub_manage.port_num+1;i++){
        ret = usb_hub_get_port_status(p_dev,hub_des,i);
		if(ret == FALSE){	
			free(hub_des);
			return FALSE;
		}
			
        //_os_printf("%x %x %x %x\n",desc[0],desc[1],desc[2],desc[3]);
        //_os_printf("\n");
    }   	
	free(hub_des);
    _os_printf("hub_enum_finish\r\n");
    
    _os_printf("uvc_dat[0]:%x\r\n",uvc_dat[0][0]);
    hub_ep_init(p_dev);
    return TRUE;
}



bool usb_host_enum1(struct hgusb20_dev *p_dev)
{
    uint8 try_cnt = 3;
    uint8 res;
    do {
		
        if (!hgusb20_is_device_online(p_dev)) {
            _os_printf("host not online Err\r\n");
            break;
        }

        res = hgusb20_host_reset(p_dev);
        if (res) {
            return res;
        }
        res = hgusb20_host_reset(p_dev);
        if (res) { 
            return res;
        }
        if (!usb_host_get_device_descriptor_1_do(p_dev)) {
            _os_printf("GetDesc Err\r\n");
            return FALSE;
        }
        
        if (hgusb20_host_reset(p_dev)) {
            return FALSE;
        }
        if (p_dev->usb_ctrl.bus_high_speed) {
            _os_printf("high_speed device plug in\r\n");
        } else {
            _os_printf("full_speed device plug in\r\n");
        }    

        //usb_host_init(usb_sw);
        res = usb_host_ep0_enum_uvc(p_dev);
        if (res == 0) {
            //hgusb20_reset_phy(p_dev);
            continue;
        } else if (res == 2) {
            break;
        }
        return TRUE;
    } while (--try_cnt);

    return FALSE;
}


uint8 hub_ep0_clear_port_feature(struct hgusb20_dev *p_dev, uint8 port,uint8 feature)
{
    p_dev->usb_ctrl.cmd.rtype = 0x23;
    p_dev->usb_ctrl.cmd.request = 0x01;                 //Set Configuration
    p_dev->usb_ctrl.cmd.value = feature;
    p_dev->usb_ctrl.cmd.index = port;
    p_dev->usb_ctrl.cmd.length = 0;	
	p_dev->usb_host.ep0_pkt = 255;
    usb_host_ep0_request(p_dev, NULL);
    os_sleep_ms(10);
    return FALSE;
}

uint8 hub_ep0_set_port_feature(struct hgusb20_dev *p_dev, uint8 port,uint8 feature)
{
    p_dev->usb_ctrl.cmd.rtype = 0x23;
    p_dev->usb_ctrl.cmd.request = 0x03;              //Set Configuration
    p_dev->usb_ctrl.cmd.value = feature;
    p_dev->usb_ctrl.cmd.index = port;
    p_dev->usb_ctrl.cmd.length = 0;
	p_dev->usb_host.ep0_pkt = 255;
    usb_host_ep0_request(p_dev, NULL);
    return FALSE;
}

uint8 hub_wait_port_connect(struct hgusb20_dev *p_dev,uint8 port)
{
    uint8 buff[10];
    uint8 ret;
    do{
        ret = usb_hub_get_port_status(p_dev,buff,port);
		if(ret == FALSE){
			return FALSE;
		}		
        os_sleep_ms(1);    
    }while(!(buff[0] & HUB_STATU_CONNECT) );
	return TRUE;
}

uint8 hub_wait_port_restend(struct hgusb20_dev *p_dev,uint8 port)
{
    uint8 buff[10];
	uint8 ret;
    ret = usb_hub_get_port_status(p_dev,buff,port);
	if(ret == FALSE){
		return FALSE;
	}	
    do{
        ret = usb_hub_get_port_status(p_dev,buff,port);
		if(ret == FALSE){
			return FALSE;
		}			
        os_sleep_ms(1);
    }while((buff[3]&HUB_STATU_RESTEND) != 0);
	return TRUE;
}

uint8 clean_hub_port(struct hgusb20_dev *p_dev,uint8 port)
{
    
    uint8 ret = 0;

//  u_addr_set(5);
//  if(ret != 0)return ret;
    ret =hub_ep0_clear_port_feature(p_dev,port,PORT_CONNECT);

    if(ret != 0)return ret;

    ret = hub_ep0_set_port_feature(p_dev,port,PORT_REST);
    if(ret != 0)return ret;
    
    ret = hub_wait_port_connect(p_dev,port);  
	if(ret == FALSE) return FALSE;

    ret = hub_ep0_clear_port_feature(p_dev,port,CLEAR_PORT_REST);
    if(ret != 0)return ret;

    ret = hub_wait_port_restend(p_dev,port);
    return ret;

}



int usb_hub_isr_handl(struct hgusb20_dev *p_dev)
{
    uint8 hub_port = 0 ;
    uint8 ret;
    
    uint32 rx_len;
    rx_len = hgusb20_ep_get_sie_rx_len(p_dev, HUB_EP);
    _os_printf("uvc_dat[0]:%x  %d\r\n",uvc_dat[0][0],rx_len);
    if( rx_len > 0 ){
        for(hub_port = 1;hub_port<8;hub_port++ )
        {   
            if(uvc_dat[0][0]&BIT(hub_port))
                break;
        }
        
        _os_printf("hub port %d connect  \n",hub_port);
        hub_manage.connect_port =hub_port;
        ret = clean_hub_port(p_dev,hub_manage.connect_port);
		_os_printf("hub ret %d \n",ret);
		if(ret == FALSE)
    		return 1;
		else 
			return 0;
    }
    else{
        return 1;
    }
    if(ret!=0){     
        return 1;
    }
    return 0;
}


extern volatile uint8 hub_interrupt;



uint8 hub_enum_error = 0;
void usb_hub_device_insert(struct hgusb20_dev *p_dev)
{
	uint8 ret;
	while(1){
		
		os_sema_down(&p_dev->usb_os_msg->hub_sema, osWaitForever);
		if (usb_hub_isr_handl(p_dev) == 0) { 
			hgusb20_set_address(p_dev, 0);
			if (!usb_host_get_device_descriptor_1_do(p_dev)) {
				_os_printf("GetDesc hub Err\r\n");
				hub_enum_error = 1;
				os_sema_up(&p_dev->usb_os_msg->hub_finish_sema);
				continue;
			}
			_os_printf("enum uvc\r\n");
			ret = usb_host_ep0_enum_uvc(p_dev);
		}else{
			hub_enum_error = 1;
			os_sema_up(&p_dev->usb_os_msg->hub_finish_sema);
			continue;			
		}
		//hgusb20_ep_rx_kick(p_dev, HUB_EP, (uint32)uvc_dat, 0x4);
		//_os_printf("up finish sema \n");
		hub_enum_error = 0;
		os_sema_up(&p_dev->usb_os_msg->hub_finish_sema);
	}
} 




uint8 usb_host_ep0_enum_hub_dev(struct hgusb20_dev *p_dev)
{
	uint8 ret = TRUE;
#if 0	
    while(hub_interrupt == 0)
        os_sleep_ms(1); 
    hub_interrupt = 0;
    if (usb_hub_isr_handl(p_dev) == 0) { 
        hgusb20_set_address(p_dev, 0);
        if (!usb_host_get_device_descriptor_1_do(p_dev)) {
            _os_printf("GetDesc hub Err\r\n");
            return FALSE;
        }       
        ret = usb_host_ep0_enum_uvc(p_dev);
    }
#endif	
	//_os_printf("wait hub device insert\r\n");
	os_sema_down(&p_dev->usb_os_msg->hub_finish_sema, osWaitForever);
	//_os_printf("finish down \n");
	if(hub_enum_error){
		ret = FALSE;
	}
    return ret;
}

bool usb_host_enum2(struct hgusb20_dev *p_dev)
{
    uint8 try_cnt = 3;
    uint8 res;

    do {
        if (!hgusb20_is_device_online(p_dev)) {
            break;
        }

        //usb_host_init(usb_sw);
//        res = usb_host_ep0_enum_uvc(usb_sw);
        res = usb_host_ep0_enum_hub(p_dev);
        if (res == 0) {
            continue;
        } else if (res == 2) {
            break;
        }else if(res == 1){
            res = usb_host_ep0_enum_hub_dev(p_dev);
        }
		
        if (res == 0) {
            continue;
        } else if (res == 2) {
            break;
        }

        return TRUE;
    } while (--try_cnt);

    //usb_host_set_fail();
    return FALSE;
}


bool usb_uvc_host_enum(struct hgusb20_dev *p_dev)
{
	SDEV_DESC dev_desc;
    bool res = 0;
    res = hgusb20_host_reset(p_dev);
    if (res) {
        return res;
    }	
	hgusb20_set_address(p_dev, 0);
    if (!usb_host_get_device_descriptor_2_do(p_dev)) {          
        _os_printf("SetAddr Err\r\n");
        return 0;
    }

	memcpy(&dev_desc,p_dev->usb_ep0_rxbuf,sizeof(SDEV_DESC));

	switch(dev_desc.device_class){
		case USB_DEV_HUB_CLASS:
			res = usb_host_enum2(p_dev);
		break;
		case USB_DEV_UVC_CLASS:
			res = usb_host_enum1(p_dev);
		break;
		default:
			_os_printf("--USB can't suppost this device-- \n");
		break;	
	}

	if(res == 0){
		hgusb20_host_reset_phy(p_dev);
	}
    
    return res;
}


int32 usb_host_uvc_ioctl(struct usb_device *p_usb_d, uint32 cmd, uint32 param1, uint32 param2)
{
    uint16 *msg_buf;
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)p_usb_d;
    enum usb_host_io_cmd io_cmd = (enum usb_dev_io_cmd)cmd;
    int32 ret = RET_OK;

    switch (io_cmd) {
        case USB_HOST_IO_CMD_RESET:
            hgusb20_host_init(p_dev);
            break;

        case USB_HOST_IO_CMD_SET_CUR:
            msg_buf = (uint16*)param1;
            ret = usb_host_set_cur(p_dev,msg_buf[1], msg_buf[0], msg_buf[2], (uint8 *)&msg_buf[3]);   
            break;

        case USB_HOST_IO_CMD_GET_CUR:
            msg_buf = (uint16*)param1;
            ret = usb_host_get_cur(p_dev,msg_buf[1], msg_buf[0], msg_buf[2], (uint8 *)&msg_buf[3]);
            break;
            
        case USB_HOST_IO_CMD_SET_IDX:
            ret = enum_set_resolution(p_dev, param1);
            break;
        
        default:
            ret = -ENOTSUPP;
            break;
    }
    return ret;
}

extern int usb_dma_irq_times;
uint32 usb_host_bus_irq(uint32 irq, uint32 param1, uint32 param2, uint32 param3)
{
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)param1;
    uint32 ret_val = 1;
    uint32 ep = param2 & 0xF;
    uint32 len = len;
    usb_dma_irq_times++;
    switch (irq) {
        case USB_CONNECT:
            _os_printf("usb connect: \r\n");
            os_sema_up(&p_dev->usb_os_msg->ep0_sema);
            break;
        case USB_DISCONNECT:
			uac_open = 0;
			hgusb20_host_reset_phy(p_dev);
            _os_printf("usb disconnect................\r\n");
            break;
        case USB_DEV_CTL_IRQ:
            ret_val = 0;
            break;
        case USB_BABBLE:
            _os_printf("usb babble................\r\n");
            break;
        case USB_DEV_SUSPEND_IRQ:
            break;
        case USB_DEV_RESUME_IRQ:
            break;
//        case USB_DEV_SOF_IRQ:
//            break;
        case USB_EP_RX_IRQ:


//            _os_printf("usb rx: %d\r\n", ep);
			if(UVC_EP == ep) {
				usb_dma_irq(p_dev, uvc_dat[0], (uint8_t)uvc_head_res, (uint8_t)videostream.eptpye);
			}
            if (UAC_EP == ep) {
                usbmic_dma_irq(p_dev, usbmic_dat);
            }
            if (HUB_EP == ep) {
				os_sema_up(&p_dev->usb_os_msg->hub_sema);
				// _os_printf("hub_ep...isr...HUB:%d ISO:%d  %x\r\n",HUB_EP,ISO_EP,param3);
            }            
            break;
        case USB_EP_TX_IRQ:
			if (UAC_EP == ep) {
				usbspk_dma_irq(p_dev);
			}
//            os_mutex_unlock(&p_dev->tx_lock);
            break;
        default:
            break;
    }
    return ret_val;
}



volatile uint8 hub_interrupt = 0;

