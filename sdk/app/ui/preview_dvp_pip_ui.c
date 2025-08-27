/***************************************************************************************************************************************************************** 
这个主要是实现通过一个dvp,通过编码再解码达到画中画效果


S_PREVIEW_SCALE3    ---->   R_VIDEO_P0(320x240)
    (scale3)        |       (lcd_video_p0)
                    |
                    ---->   R_YUV_TO_JPG(启动jpg0,video_app,然后默认绑定了SR_OTHER_JPG) ---->SR_OTHER_JPG          ---->S_JPG_DECODE    ---->R_VIDEO_P1(160x120)
                            (借用prc寄存器,启动jpg0,打开video_app的流)                       (接收jpg,配置解码参数)       (jpg1解码)           (lcd_video_p1)
                            (使用yuv:320x240,编码成320x240)                                 (jpg0:320x240)              (yuv:160x120)
******************************************************************************************************************************************************************* */
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "osal/string.h"
#include "stream_frame.h"
#include "other_jpg_show/other_jpg_show_stream.h"
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
struct preview_pip_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_ui;

//这个是需要的流,但命名不一定对应,有可能通过中间流进行中转实现
    stream *scale3_s;
    stream *yuv_jpg_s;
    stream *other_jpg_s;
    stream *decode_s;
    stream *jpg_s;

};

static void exit_preview_dvp_pip_ui(lv_event_t * e)
{
    struct preview_pip_ui_s *ui_s = (struct preview_pip_ui_s *)lv_event_get_user_data(e);
    lv_indev_set_group(indev_keypad, ui_s->last_group);
    lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
    close_stream(ui_s->scale3_s);
    close_stream(ui_s->yuv_jpg_s);
    close_stream(ui_s->jpg_s);
    close_stream(ui_s->other_jpg_s);
    close_stream(ui_s->decode_s);
    lv_obj_del(ui_s->now_ui);
}
//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_preview_dvp_pip_ui(lv_event_t * e)
{
    struct preview_pip_ui_s *ui_s = (struct preview_pip_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    //scale3 从640x480拉伸到320x240
    ui_s->scale3_s = scale3_stream_not_bind(S_PREVIEW_SCALE3,640,480,320,240,YUV_P0);

    //绑定流到Video P0显示
    streamSrc_bind_streamDest(ui_s->scale3_s,R_VIDEO_P0);
    //给到yuv_to_jpg编码
    streamSrc_bind_streamDest(ui_s->scale3_s,R_YUV_TO_JPG);
    enable_stream(ui_s->scale3_s,1);



    ui_s->decode_s = jpg_decode_stream_not_bind(S_JPG_DECODE);
    //将解码的数据推送到Video P1
    streamSrc_bind_streamDest(ui_s->decode_s,R_VIDEO_P1);
    enable_stream(ui_s->decode_s,1);


    //先创建jpg_s,然后绑定流(jpg给到谁用),最后使能
    ui_s->jpg_s = new_video_app_stream(S_JPEG);
    //给到图传测试使用
    streamSrc_bind_streamDest(ui_s->jpg_s,SR_OTHER_JPG);
    enable_stream(ui_s->jpg_s,1);


    //scale3的数据发送到R_YUV_TO_JPG,R_YUV_TO_JPG接收到yuv后,启动jpg0编码
    ui_s->yuv_jpg_s = prc_stream(R_YUV_TO_JPG,ui_s->jpg_s);
    enable_stream(ui_s->yuv_jpg_s,1);


    //从video_app那边接收到jpg数据,然后配置解码参数(160x120),最后给到解码模块
    ui_s->other_jpg_s = other_jpg_stream_not_bind(SR_OTHER_JPG,160,120,160,120,YUV_P1);
    //将other_jpg的数据给到S_JPG_DECODE进行编码
    streamSrc_bind_streamDest(ui_s->other_jpg_s,S_JPG_DECODE);
    enable_stream(ui_s->other_jpg_s,1);

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);


    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_preview_dvp_pip_ui, LV_EVENT_PRESSED, ui_s);
}

lv_obj_t *preview_dvp_pip_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct preview_pip_ui_s *ui_s = (struct preview_pip_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct preview_pip_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "preview_dvp_pip");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_preview_dvp_pip_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}