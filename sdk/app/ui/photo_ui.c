/*************************************************************************************** 
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "osal/string.h"
#include "stream_frame.h"
#include "osal/work.h"

#include "keyWork.h"
#include "keyScan.h"
#include "new_avi/newavi_player.h"
#include "custom_mem/custom_mem.h"
#include "fatfs/osal_file.h"


#define CHECK_DIR   "photo"
#define EXT_NAME    "*jpg"



//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc

extern void set_lvgl_get_key_func(void *func);
extern lv_style_t g_style;
extern lv_indev_t * indev_keypad;






struct photo_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;
    lv_group_t    *now_group;
    lv_obj_t    *now_ui;


    stream *photo_s;
    stream *other_s;
    stream *decode_s;
    lv_obj_t * label_path;

};

struct photo_list_param
{
    lv_group_t *group;
    lv_obj_t *ui;
    struct photo_ui_s *ui_s;
};
typedef int (*list_ui)(const char *filename,void *data);




static void exit_show_filelist(lv_event_t * e)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *list_item = lv_event_get_current_target(e);
    if(ui_s->now_group)
    {
        lv_indev_set_group(indev_keypad, ui_s->now_group);
    }
    if(list_item->user_data)
    {
        //由于是子控件回调函数需要删除父控件,所以需要用到异步删除,否则删除内部链表会有异常
        lv_obj_del_async(list_item->user_data);
    }
}



//读取图片,发送出去显示
static void play_photo(lv_event_t * e)
{   
    void *fp = NULL;
    void *photo_buf = NULL;
	struct data_structure *data_s;
    uint32_t len;
    struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *list_item = lv_event_get_current_target(e);
    lv_obj_t *list = list_item->user_data;
    char path[64];
    const char *filename = lv_list_get_btn_text(list,list_item);
    os_sprintf(path,"0:%s/%s",CHECK_DIR,filename);
	if(!ui_s->photo_s)
	{
		goto play_photo_fail;
	}

    //读取图片文件,通过流去发送
    data_s = get_src_data_f(ui_s->photo_s);
    if(data_s)
    {
        lv_label_set_text(ui_s->label_path, path);
        //开始显示图片
        fp = osal_fopen(path,"rb");
        if(fp)
        {
            uint32_t filesize = osal_fsize(fp);
            photo_buf = (void*)STREAM_MALLOC(filesize);
            if(!photo_buf)
            {
                force_del_data(data_s);
                goto play_photo_fail;
            }
            else
            {
                len = osal_fread(photo_buf,1,filesize,fp);
                if(len != filesize)
                {
                    goto play_photo_fail;
                }
                data_s->data = (void*)photo_buf;
                data_s->type = SET_DATA_TYPE(JPEG,JPEG_FULL);
                set_stream_real_data_len(data_s,filesize);
                
                send_data_to_stream(data_s);
                photo_buf = NULL;
                data_s = NULL;
                goto play_photo_end;
            }
            
        }

        goto play_photo_fail;

    }
play_photo_fail:
    //显示失败
    os_sprintf(path,"0:%s/%s fail",CHECK_DIR,filename);
    lv_label_set_text(ui_s->label_path, path);
play_photo_end:
    if(fp)
    {
        osal_fclose(fp);
    }

    if(photo_buf)
    {
        STREAM_FREE(photo_buf);
    }


    exit_show_filelist(e);
}



static int photo_list_show(const char *filename,void *data)
{
    struct photo_list_param *param = (struct photo_list_param*)data;
	lv_obj_t * btn = lv_list_add_btn(param->ui, NULL, (const char *)filename);
    btn->user_data = (void*)param->ui;
    if(param->group)
    {
        lv_group_add_obj(param->group, btn);
    }
	
    //回调函数,进入回放功能
	lv_obj_add_event_cb(btn, play_photo, LV_EVENT_CLICKED, param->ui_s);
	return 0;
}

//遍历文件夹
//遍历文件夹
static int each_read_file(list_ui fn,void *param)
{
    DIR  dir;
    FILINFO finfo;
    FRESULT fr;
    if(!fn)
    {
        goto  each_read_file_end;
    }

    fr = f_findfirst(&dir, &finfo, CHECK_DIR, EXT_NAME);


	while(fr == FR_OK && finfo.fname[0] != 0)
	{
        fn(finfo.fname,param);
        fr = f_findnext(&dir, &finfo);
	}

    f_closedir(&dir);
each_read_file_end:
    os_printf("%s:%d\tret:%d\n",__FUNCTION__,__LINE__,fr);
    return fr;
}

static void clear_list_group_ui(lv_event_t * e)
{
    lv_group_t *group = (lv_group_t *)lv_event_get_user_data(e);
    os_printf("group:%X\n",group);
    if(group)
    {
        lv_group_del(group);
    }
}



static void show_filelist(lv_event_t * e)
{
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'e')
    {
        struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
        struct photo_list_param param;
        lv_obj_t *list = lv_list_create(ui_s->now_ui);
        lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
        param.ui_s = ui_s;
        param.ui = list;
        param.group = lv_group_create();
        lv_indev_set_group(indev_keypad, param.group);
        //创建exit的控件
        lv_obj_t *list_item = lv_list_add_btn(list, NULL, "exit");
        list_item->user_data = (void*)list;
        lv_group_add_obj(param.group, list_item);
        lv_obj_add_event_cb(list_item, exit_show_filelist, LV_EVENT_SHORT_CLICKED, ui_s);

        each_read_file(photo_list_show,(void*)&param);

        lv_obj_add_event_cb(list, clear_list_group_ui, LV_EVENT_DELETE, param.group);
    }

}









static uint32_t self_key(uint32_t val)
{
    uint32_t key_ret = 0;
	if(val > 0)
	{
		if((val&0xff) ==   KEY_EVENT_SUP)
		{
			switch(val>>8)
			{
				case AD_UP:
					key_ret = LV_KEY_PREV;
				break;
				case AD_DOWN:
					key_ret = LV_KEY_NEXT;
				break;
				case AD_LEFT:
					key_ret = 'a';
				break;
				case AD_RIGHT:
					key_ret = 'd';
				break;
                case AD_A:
                    key_ret = 'q';
                break;
                case AD_B:
                    key_ret = 'e';
                break;
				case AD_PRESS:
					key_ret = LV_KEY_ENTER;
				break;


				break;
				default:
				break;

			}
		}
	}
    return key_ret;
}



static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
        case STREAM_DATA_DIS:
        {
        }
        break;
		case STREAM_RECV_DATA_FINISH:
        {
        }
		break;
		//在发送到这个流的时候,进行数据包过滤
		case STREAM_FILTER_DATA:
        {
        }
		break;
		//流接收后,数据包也要检查是否需要过滤或者是不是因为逻辑条件符合需要过滤
		case STREAM_RECV_FILTER_DATA:
        {
        }
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                STREAM_FREE(data->data);
                data->data = NULL;
            }
		}

		break;

        case STREAM_CLOSE_EXIT:
        {

        }
        break;

        case STREAM_MARK_GC_END:
        {

            
        }
            
        break;


		default:
		break;
	}
	return res;
}



//创建播放器的流,这个流主要实现的是控制速度转发作用,暂时控制的是视频
//如果要控制音频,需要计算时间来控制速度
static stream *photo_stream(const char *name)
{
    stream *s = open_stream_available(name,4,0,opcode_func,NULL);
    return s;
}

static void exit_player_ui(lv_event_t * e)
{
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        
        

        close_stream(ui_s->decode_s);
        close_stream(ui_s->other_s);
        close_stream(ui_s->photo_s);

        ui_s->decode_s = NULL;
        ui_s->other_s = NULL;
        ui_s->photo_s = NULL;

        
        lv_obj_del(ui_s->now_ui);

        set_lvgl_get_key_func(NULL);
    }

}




static void enter_player_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *base_ui = ui_s->base_ui;
    lv_obj_add_flag(base_ui, LV_OBJ_FLAG_HIDDEN);

    
    lv_obj_t *ui = lv_obj_create(lv_scr_act()); 
    ui_s->now_ui = ui; 
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));


    ui_s->label_path = lv_label_create(ui);
    lv_label_set_text(ui_s->label_path, "");
    lv_obj_align(ui_s->label_path,LV_ALIGN_TOP_MID,0,0);


    ui_s->photo_s = photo_stream(S_LVGL_PHOTO);
    streamSrc_bind_streamDest(ui_s->photo_s,SR_OTHER_JPG);
    enable_stream(ui_s->photo_s,1);


    //SR_OTHER_JPG接收数据,然后发送到decode
    ui_s->other_s = other_jpg_stream_not_bind(SR_OTHER_JPG,320,240,320,240,YUV_P0);
    streamSrc_bind_streamDest(ui_s->other_s,S_JPG_DECODE);
    enable_stream(ui_s->other_s,1);

    //decode发送到S_NEWPLAYER控制速度后,转发到P0
    ui_s->decode_s = jpg_decode_stream_not_bind(S_JPG_DECODE);
    streamSrc_bind_streamDest(ui_s->decode_s,R_VIDEO_P0);
    enable_stream(ui_s->decode_s,1);


    

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_group_add_obj(group, ui);
    ui_s->now_group = group;


    //弹出菜单栏,用于切换分辨率
    lv_obj_add_event_cb(ui, show_filelist, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(ui, exit_player_ui, LV_EVENT_KEY, ui_s);
}

lv_obj_t *photo_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct photo_ui_s *ui_s = (struct photo_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct photo_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "photo");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_player_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}