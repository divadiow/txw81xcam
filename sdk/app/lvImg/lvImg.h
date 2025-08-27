#ifndef __LVIMG_H
#define __LVIMG_H
#include "../lvgl.h"
struct lvImg_res_gc_s
{
    struct lvImg_res_gc_s *next;
    lv_img_dsc_t *img;
};


struct lvImg_res_gc_s *lvImg_gc_obj_add(struct lvImg_res_gc_s *gc_img,lv_img_dsc_t *img);   
void lvImg_gc_loop(struct lvImg_res_gc_s *gc_img);
void *get_fs_lv_img(const char *filename);
void fs_lv_img_free(lv_img_dsc_t * img);
void *get_fs_lv_img_lzo(const char *filename);

#endif