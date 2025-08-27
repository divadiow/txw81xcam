/*************************************************************************************** 
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "osal/string.h"
#include "stream_frame.h"

stream *yuv_to_jpg_stream(const char *name);
extern lv_indev_t * indev_keypad;

static void print_system_msg(lv_event_t * e)
{
    //uint32_t buf[1024/4];
    //print_custom_psram_heap_status(buf,1024);
}

lv_obj_t *system_msg_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "systemMsg");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, print_system_msg, LV_EVENT_PRESSED, NULL);
    lv_group_add_obj(group, btn);
    return btn;
}