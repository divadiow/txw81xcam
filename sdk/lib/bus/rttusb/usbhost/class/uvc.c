#include "rtthread.h"
#include "include/usb_host.h"

#ifdef RT_USBH_UVC
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/uvc_host.h"
#include "uvc.h"
#include "cdc.h"

static struct uclass_driver uvc_driver;

#if 0
int32 demo_atcmd_select_resolution(const char *cmd, char *argv[], uint32 argc)
{
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev_get(HG_USBDEV_DEVID);

    if(*argv[0] == '1') {
        usbspk_open = 0;
        os_printf("uvc.strm_intfs:%d spk->strm_intfs:%d mic->strm_intfs:%d\n",uvc.strm_intfs,usb_spk.strm_intfs,usb_mic.strm_intfs);
        select_cur(p_dev,11); 
    } 
	printf("OK/n");
    return 0;
}
#endif

extern volatile uint8_t uvc_format;
static rt_err_t rt_usbh_uvc_enable(void *arg){
    struct uhintf **intf = arg;
    uint8 ret;
    uhcd_t hcd = NULL;
    int dpi = UVC_720P;
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev_get(HG_USBDEV_DEVID);


    if (intf[0] == NULL) {
        return -EIO;
    }

    hcd = intf[0]->device->hcd;
    os_printf("subclass %d, protocal %d\r\n",
        intf[0]->intf_desc->bInterfaceSubClass,
        intf[0]->intf_desc->bInterfaceProtocol);
    os_printf("device:%x cfg_desc:%x len:%d\n",(uinst_t)intf[0]->device,intf[0]->device->cfg_desc,intf[0]->device->cfg_desc->wTotalLength);

    analysis_uvc_desc((uint8 *)(intf[0]->device->cfg_desc),intf[0]->device->cfg_desc->wTotalLength);
    printf_uvc_vsdesc(&uvc_des);
    select_uvc_intfs(&uvc_des,(UVCDEV *)&uvc);
	split_uac_intfs(&uac_des,(UACDEV *)&uac);

    usb_host_set_configuration(p_dev, 1);       //设置配置

    os_sleep_ms(5);
    if (uvc.ctyp == UVC_CLASS) {
        _os_printf("strm_intfs:%x, epstrm:%x, strm_pload: %x, strm_altset:%x \r\n",uvc.strm_intfs ,uvc.epstrm,uvc.strm_pload, uvc.strm_altset);
        get_uvc_process_support(&uvc_des, &uvc_proc);
        set_uvc_process_ctlval(&uvc_proc_info);
        get_uvc_process_info(p_dev, &uvc_proc, &uvc_proc_info);
        dpi = uvc_default_dpi();
        ret = enum_set_resolution(p_dev,dpi);
        if(ret != 0){
            
        } 
		usb_host_enum_finish_init(uvc_format);

        uvc_room_init(p_dev);
        _os_printf(" uvc.epstrm %x,uvc.strm_pload %x,uvc.strm_interval %x\r\n",uvc.epstrm&0x7f,uvc.strm_pload,uvc.strm_interval);
        uvc_ep_init(p_dev);
        drv_get_specific_pipe(UVC_EP, USB_DIR_IN);

    } 
    if((uac.spk_type == Speaker) && (uac.mic_type == Microphone))
    {
		select_mic_intfs((UACDEV*)&uac, (MICDEV*)&usb_mic);
        select_spk_intfs((UACDEV*)&uac, (SPKDEV*)&usb_spk);
		usbmic_stream_enable(p_dev, (MICDEV*)&usb_mic, 0);
		usbspk_stream_enable(p_dev, (SPKDEV*)&usb_spk, 0);
        get_uac_vol(p_dev, (UACDEV*)&uac, 0);
        get_uac_vol(p_dev, (UACDEV*)&uac, 1);
        set_uac_mute(p_dev, (UACDEV*)&uac, 0, 0);
        set_uac_volume(p_dev, (UACDEV*)&uac, 0, 0);
		usbmic_stream_enable(p_dev, (MICDEV*)&usb_mic, 1);
		usbspk_stream_enable(p_dev, (SPKDEV*)&usb_spk, 1);
		extern void usbmic_enum_finish(void);
		usbmic_enum_finish();
		usbmic_ep_init(p_dev, (MICDEV*)&usb_mic);
        usbspk_ep_init(p_dev, (SPKDEV*)&usb_spk);

        drv_get_specific_pipe(UAC_EP, USB_DIR_IN);
    }

	if(uac.spk_type == Speaker) {
		usbspk_open = 1;
		extern void usbspk_enum_finish(void);
		usbspk_enum_finish();
		usbtx_sema_init();
		usbspk_tx_thread(p_dev);
	}

    return RET_OK;
}

static rt_err_t rt_usbh_uvc_disable(void *arg){
   
    //struct uhintf **intf = arg;
   
    LOG_D("rt_usbh_storage_stop");

	usbspk_open = 0;
    drv_free_specific_pipe(UVC_EP, USB_DIR_IN);
    drv_free_specific_pipe(UAC_EP, USB_DIR_IN);

    return RET_OK;
}

ucd_t rt_usbh_class_driver_uvc(void)
{
    uvc_driver.class_code = USB_CLASS_VIDEO;

    uvc_driver.enable = rt_usbh_uvc_enable;
    uvc_driver.disable = rt_usbh_uvc_disable;

    return &uvc_driver;

}
#endif

