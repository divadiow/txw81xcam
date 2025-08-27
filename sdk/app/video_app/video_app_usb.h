#ifndef __VIDEO_APP_USB_H
#define __VIDEO_APP_USB_H
#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "stream_frame.h"
#include "dev/usb/uvc_host.h"
void set_uvc_frame_using(UVC_MANAGE* uvc);
struct list_head* get_uvc_frame();
int get_usb_node_count(struct list_head *head);
bool free_usb_node(struct list_head *del);
void del_usb_frame(void *get_f);
void *get_usb_jpeg_node_buf(void *d);
void del_uvc_frame(UVC_MANAGE* uvc);
int get_node_uvc_len();
stream *usb_jpeg_stream_init();
void usb_jpeg_stream_deinit();

#endif