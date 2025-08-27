#ifndef __CLASS_UVC_H__
#define __CLASS_UVC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

extern struct VIDEO_COMMIT commit;
extern UVC_SLEST uvc_select;
extern UVC_PROCESS uvc_proc;
extern UVC_PROCESS_INFO uvc_proc_info;
extern UVC_DES uvc_des;
extern UAC_DES uac_des;
extern volatile UVCDEV uvc;
extern UACDEV uac;
extern MICDEV usb_mic;
extern SPKDEV usb_spk;
extern EP_DES  videostream;
extern int usbspk_open;

extern void select_uvc_intfs(UVC_DES *ud,UVCDEV *uvc);
extern void split_uac_intfs(UAC_DES *ud,UACDEV *uac);
extern void printf_uvc_vsdesc(UVC_DES *ud);
extern void uvc_ep_init(struct hgusb20_dev *p_dev);
extern void get_uvc_process_support(UVC_DES *ud, UVC_PROCESS *uvc_p);
extern void set_uvc_process_ctlval(UVC_PROCESS_INFO *uvc_p_info);
extern void get_uvc_process_info(struct hgusb20_dev *p_dev, UVC_PROCESS *uvc_p, UVC_PROCESS_INFO *uvc_p_info);
extern void analysis_uvc_desc(uint8 *desc,uint32 desclen);
extern bool usb_host_set_configuration(struct hgusb20_dev *p_dev, uint8 cfg_value);
extern int uvc_default_dpi();
extern void usb_host_enum_finish_init(uint32_t uvc_format);
extern bool enum_set_resolution(struct hgusb20_dev *p_dev,uint8 resolution);
extern rt_uint8_t drv_get_specific_pipe(rt_uint8_t index, rt_uint8_t in_or_out);
extern void drv_free_specific_pipe(rt_uint8_t index, rt_uint8_t in_or_out);

void select_mic_intfs(UACDEV *uac, MICDEV *mic);
void select_spk_intfs(UACDEV *uac, SPKDEV *spk);
void usbmic_stream_enable(struct hgusb20_dev *p_dev, MICDEV *mic, uint8 enable);
void usbspk_stream_enable(struct hgusb20_dev *p_dev, SPKDEV *spk, uint8 enable);
void get_uac_vol(struct hgusb20_dev *p_dev, UACDEV *uac, uint8 audio_dev);
int set_uac_mute(struct hgusb20_dev *p_dev, UACDEV *uac, uint8 audio_dev, uint8 enable);  
int set_uac_volume(struct hgusb20_dev *p_dev, UACDEV *uac, uint8 audio_dev, int16 volume);
void usbmic_ep_init(struct hgusb20_dev *p_dev, MICDEV *mic);
void usbspk_ep_init(struct hgusb20_dev *p_dev, SPKDEV *spk);
void usbmic_audio_init(void);
void usbspk_audio_init(void);
void usbtx_sema_init();
void usbspk_tx_thread(void *dev);


struct usb_uvc {
    struct dev_obj dev;
    void *device;
    struct rt_thread recv_task;
    rt_uint8_t *at_cmd_buff;
    rt_uint32_t state;
    upipe_t pipe_uvc_in;
    upipe_t pipe_uac_in;
    upipe_t pipe_cdc_in;
    upipe_t pipe_cdc_out;
    rt_uint8_t retry;
};

#ifdef __cplusplus
}
#endif

#endif