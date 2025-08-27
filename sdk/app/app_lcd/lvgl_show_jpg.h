#ifndef __LVGL_SHOW_JPG_H
#define __LVGL_SHOW_JPG_H
int lvgl_jpg_send(uint32_t *jpg_data,uint32_t jpg_size,uint32_t out_w,uint32_t out_h,uint32_t jpg_w,uint32_t jpg_h,uint32_t step_w,uint32_t step_h);
void *lvgl_jpg_stream(const char *name);
void lvgl_broadcast_exit();
int lvgl_jpg_from_fs_send(uint8_t *filename,uint32_t out_w,uint32_t out_h,uint32_t step_w,uint32_t step_h);
#endif