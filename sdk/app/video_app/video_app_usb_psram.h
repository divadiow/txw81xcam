#ifndef __VIDEO_APP_USB_PSRAM_H
#define __VIDEO_APP_USB_PSRAM_H
#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "stream_frame.h"
#include "dev/usb/uvc_host.h"

    stream *usb_jpeg_stream_init();
    struct list_head* get_uvc_frame();
    void set_uvc_frame_using(UVC_MANAGE* uvc);
    int get_usb_node_count(struct list_head *head);
    bool free_usb_node(struct list_head *del);
    void del_usb_frame(void *get_f);
    void *get_usb_jpeg_node_buf(void *d);
    void del_uvc_frame(UVC_MANAGE* uvc);
    int get_node_uvc_len();
    extern int usb_dma_irq_times;
	
	void usb_jpeg_psram_stream_deinit();
#endif