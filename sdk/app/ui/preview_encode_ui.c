/*************************************************************************************** 
S_PREVIEW_SCALE3    ---->   R_VIDEO_P0(320x240)
(scale3)            |         (lcd_video_p0)
                    |
                    ---->   R_YUV_TO_JPG(启动jpg0,video_app)
                            (编码成jpg:320x240)
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "osal/string.h"
#include "stream_frame.h"
#include "custom_mem/custom_mem.h"


//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc

extern lv_indev_t * indev_keypad;
extern lv_style_t g_style;
struct preview_encode_QVGA_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_ui;

    stream *s;
    stream *yuv_jpg_s;
    stream *jpg_s;

};

static void exit_preview_encode_ui(lv_event_t * e)
{
    struct preview_encode_QVGA_ui_s *ui_s = (struct preview_encode_QVGA_ui_s *)lv_event_get_user_data(e);
    lv_indev_set_group(indev_keypad, ui_s->last_group);
    lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
    close_stream(ui_s->s);
    close_stream(ui_s->jpg_s);
    close_stream(ui_s->yuv_jpg_s);
    lv_obj_del(ui_s->now_ui);
}
//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_preview_encode_ui(lv_event_t * e)
{
    struct preview_encode_QVGA_ui_s *ui_s = (struct preview_encode_QVGA_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    ui_s->s = scale3_stream_not_bind(S_PREVIEW_ENCODE_QVGA,640,480,320,240,YUV_P0);
    //绑定流到Video P0显示
    streamSrc_bind_streamDest(ui_s->s,R_VIDEO_P0);
    //绑定流到YUV_TO_JPG,意思就是将320x240数据给到yuv_to_jpg,然后yuv_to_jpg启动jpg0编码,得到320x240的jpg数据
    streamSrc_bind_streamDest(ui_s->s,R_YUV_TO_JPG);
    enable_stream(ui_s->s,1);

    //先创建jpg_s,然后绑定流(jpg给到谁用),最后使能
    ui_s->jpg_s = new_video_app_stream(S_JPEG);
    //给到图传测试使用
    streamSrc_bind_streamDest(ui_s->jpg_s,R_RTP_JPEG);
    enable_stream(ui_s->jpg_s,1);

    ui_s->yuv_jpg_s = prc_stream(R_YUV_TO_JPG,ui_s->jpg_s);
    enable_stream(ui_s->yuv_jpg_s,1);



    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);


    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_preview_encode_ui, LV_EVENT_PRESSED, ui_s);
}

lv_obj_t *preview_encode_QVGA_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct preview_encode_QVGA_ui_s *ui_s = (struct preview_encode_QVGA_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct preview_encode_QVGA_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "preview_encode_QVGA");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_preview_encode_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}