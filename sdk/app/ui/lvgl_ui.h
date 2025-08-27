#ifndef __LVGL_UI_H
#define __LVGL_UI_H
#include "lvgl/lvgl.h"
lv_obj_t *system_msg_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_encode_selct_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_encode_selct_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *player2_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_usb_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_usb_select_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *photo_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_encode_QVGA_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_encode_QQVGA_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_dvp_pip_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *player_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *player2_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_encode_takephoto_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_encode_960P_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_encode_selct_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *zbar_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *zbar_dvp_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *zbar_usb_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *photo_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_usb_encode_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_dvp_usb_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_net_jpg_ui(lv_group_t *group,lv_obj_t *base_ui);


#include "stream_frame.h"
stream *scale3_stream_not_bind(const char *name,uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh,uint16_t type);
stream *new_video_app_stream(const char *name);
stream *prc_stream(const char *name,stream *d_s);
stream *jpg_decode_stream_not_bind(const char *name);
stream *other_jpg_stream_not_bind(const char *name,uint16_t out_w,uint16_t out_h,uint16_t step_w,uint16_t step_h,uint32_t type);
stream *scale1_stream_not_bind(const char *name,uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh);
stream *new_video_app_stream_with_mode(const char *name,uint8_t from_vpp,uint16_t w,uint16_t h);
stream *zbar_stream(const char *name,uint32_t filter);
stream *independ_video_usb();
stream *newavi_player_init(const char *stream_name,const char *filename);
stream *prc_stream_filter(const char *name,stream *d_s,uint16_t type);

void set_lvgl_get_key_func(void *func);
#endif