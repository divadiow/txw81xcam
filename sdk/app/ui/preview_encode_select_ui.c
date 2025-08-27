/*************************************************************************************** 
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "osal/string.h"
#include "stream_frame.h"
#include "keyWork.h"
#include "keyScan.h"


#include "custom_mem/custom_mem.h"
#include "osal_file.h"

#define PHOTO_DIR   "photo"

extern stream *avi_save_stream(const char *manage_stream_name,const char *video_name,const char *audio_name,uint16_t video_cache_num,uint16_t audio_cache_num,uint16_t fps,uint16_t frq);


//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc
#define LV_OBJ_TAKEPHOTO_FLAG   LV_OBJ_FLAG_USER_1
#define LV_OBJ_RECORD_FLAG      LV_OBJ_FLAG_USER_2

extern void set_lvgl_get_key_func(void *func);
extern lv_indev_t * indev_keypad;
extern lv_style_t g_style;
struct preview_encode_takephoto_selct_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;
    lv_group_t    *now_group;
    lv_obj_t    *now_ui;
    lv_obj_t    *status_obj;
    lv_timer_t *timer;

    stream *s; 
    stream *scale1_stream;
    stream *jpg0_stream;
    stream *takephoto_s;
    stream *avi_save_s;
    stream *audio_s;

    uint8_t is_takephoto;

};




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
				case AD_PRESS:
					key_ret = LV_KEY_ENTER;
				break;
                case AD_A:
                    key_ret = 'q';
                break;


				default:
				break;

			}
		}
		else if((val&0xff) ==   KEY_EVENT_LUP)
		{
			

				switch(val>>8)
				{


					default:
					break;

				}
		}
	}
    return key_ret;
}






//生成一个流,专门接收图片,用于拍照的,拍照之前可以将缓冲区的删除,这样可以保证得到最新的图片
//要考虑一些问题,如果拍照放在workqueue,要考虑写卡时间是否影响workqueue
//这里尝试使用lvgl的timer事件去保存(影响lvgl,如果需要更加好,可能要创建任务去保存)



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


static stream *takephoto_stream(const char *name)
{
    //设置接收1,是为了不要接收太多的图片,因为是拍照,需要保持实时性
    //如果接收过多,如果不处理,会导致源头没有空间产生新的图片
    //如果要结合录像,则需要在应用层去处理(实际录像时可以创建另一个独立的流)
    stream *s = open_stream_available(name,0,1,opcode_func,NULL);
    return s;
}

extern int no_frame_record_video_psram(void *fp,void *d,int flen);

static void takephoto_timer(lv_timer_t *t)
{
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)t->user_data;
    struct data_structure *data_s;
    //看看对应的流有没有图片,如果有,则保存
    data_s = recv_real_data(ui_s->takephoto_s);
    if(data_s)
    {
        enable_stream(ui_s->takephoto_s,0);
        char filename[64];
        sprintf(filename,"0:%s/%08d.jpg",PHOTO_DIR,(uint32_t)os_jiffies());
        void *fp = osal_fopen(filename,"wb");
        if(fp)
        {
            no_frame_record_video_psram(fp,data_s,get_stream_real_data_len(data_s));
            osal_fclose(fp);
        }
        else
        {
            os_printf("takephoto fail,filename:%s\n",filename);
        }
        //保存图片
        free_data(data_s);
        while(data_s)
        {
            data_s = recv_real_data(ui_s->takephoto_s);
            if(data_s)
            {
                free_data(data_s);
            }
        }
        os_printf("takephoto success\n");
        lv_obj_clear_flag(ui_s->now_ui, LV_OBJ_TAKEPHOTO_FLAG); 
        ui_s->timer = NULL;
        lv_timer_del(t);
    }
}

//使能拍照
static void encode_takephoto_ui(lv_event_t * e)
{
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
    stream *s = ui_s->takephoto_s;
    if(ui_s->is_takephoto)
    {
        if(!lv_obj_has_flag(ui_s->now_ui,LV_OBJ_TAKEPHOTO_FLAG))
        {
            lv_obj_add_flag(ui_s->now_ui, LV_OBJ_TAKEPHOTO_FLAG); 
            enable_stream(s,1);
            //启动timer,10ms检查一次
            ui_s->timer = lv_timer_create(takephoto_timer, 10,  (void*)ui_s);
        }
        else
        {
            //已经在拍照了
            os_printf("%s already running\n",__FUNCTION__);
        }
    }
    else
    {
        if(!lv_obj_has_flag(ui_s->now_ui,LV_OBJ_RECORD_FLAG))
        {
            if(ui_s->audio_s)
            {
                ui_s->avi_save_s = avi_save_stream(MANAGE_STREAM_NAME,R_AVI_SAVE_VIDEO,R_AVI_SAVE_AUDIO,2,8,30,8000);
            }
            else
            {
                ui_s->avi_save_s = avi_save_stream(MANAGE_STREAM_NAME,R_AVI_SAVE_VIDEO,NULL,2,0,30,0);
            }
            
            if(ui_s->avi_save_s)
            {
                lv_obj_add_flag(ui_s->now_ui, LV_OBJ_RECORD_FLAG); 
            }
        }
        else
        {
            close_stream(ui_s->avi_save_s);
            ui_s->avi_save_s = NULL;
        }
    }


}





static void exit_preview_encode_select_ui(lv_event_t * e)
{
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        close_stream(ui_s->avi_save_s);
        close_stream(ui_s->s);
        close_stream(ui_s->scale1_stream);
        close_stream(ui_s->jpg0_stream);
        close_stream(ui_s->takephoto_s);
        close_stream(ui_s->audio_s);        //因为外部打开的,所以实际这里关闭不会真的关闭,需要所有地方都关闭才会真的关闭的
        lv_obj_del(ui_s->now_ui);
        set_lvgl_get_key_func(NULL);
    }

}




static void exit_menu_select_ui(lv_event_t * e)
{
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *list_item = lv_event_get_current_target(e);
    os_printf("list_item:%X\n",list_item);
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

static void clear_menu_select_ui(lv_event_t * e)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    lv_group_t *group = (lv_group_t *)lv_event_get_user_data(e);
    os_printf("group:%X\n",group);
    if(group)
    {
        lv_group_del(group);
    }
}

static void DPI_640x480_select_ui(lv_event_t * e)
{
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
    stream *jpeg_s = ui_s->jpg0_stream;
    stream *scale1_stream = ui_s->scale1_stream;

    //scale1先停止
    stream_self_cmd_func(scale1_stream,STREAM_STOP_CMD,0);
    //先停止jpg?
    stream_self_cmd_func(jpeg_s,STREAM_STOP_CMD,0);

    //重新配置参数
    stream_self_cmd_func(jpeg_s,RESET_MJPEG_FROM,1);

    //启动jpeg
    stream_self_cmd_func(jpeg_s,RESET_MJPEG_DPI,(640<<16 | 480));
    //不需要重启scale1

    exit_menu_select_ui(e);
}



static void DPI_1280x960_select_ui(lv_event_t * e)
{
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
    stream *jpeg_s = ui_s->jpg0_stream;
    stream *scale1_stream = ui_s->scale1_stream;

    //scale1先停止
    stream_self_cmd_func(scale1_stream,STREAM_STOP_CMD,0);
    //先停止jpg?
    stream_self_cmd_func(jpeg_s,STREAM_STOP_CMD,0);

    //重新配置参数
    stream_self_cmd_func(jpeg_s,RESET_MJPEG_FROM,2);

    //启动jpeg
    stream_self_cmd_func(jpeg_s,RESET_MJPEG_DPI,(1280<<16 | 960));
    //启动scale1
    stream_self_cmd_func(scale1_stream,SCALE1_RESET_DPI,(1280<<16 | 960));
    exit_menu_select_ui(e);
}


static void DPI_1920x1440_select_ui(lv_event_t * e)
{
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
    stream *jpeg_s = ui_s->jpg0_stream;
    stream *scale1_stream = ui_s->scale1_stream;

    //scale1先停止
    stream_self_cmd_func(scale1_stream,STREAM_STOP_CMD,0);
    //先停止jpg?
    stream_self_cmd_func(jpeg_s,STREAM_STOP_CMD,0);

    //重新配置参数
    stream_self_cmd_func(jpeg_s,RESET_MJPEG_FROM,2);

    //启动jpeg
    stream_self_cmd_func(jpeg_s,RESET_MJPEG_DPI,(1920<<16 | 1440));
    //启动scale1
    stream_self_cmd_func(scale1_stream,SCALE1_RESET_DPI,(1920<<16 | 1440));

    exit_menu_select_ui(e);
}


//切换拍照还是录像
static void change_mode_ui(lv_event_t * e)
{
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'd')
    {
        if(ui_s->is_takephoto)
        {
            ui_s->is_takephoto = 0;
        }
        else
        {
            ui_s->is_takephoto = 1;
        }

        //将ui修改
        if(ui_s->is_takephoto)
        {
            //修改成了拍照模式,那么看看是不是在录像,在录像就要停止录像
            if(lv_obj_has_flag(ui_s->now_ui,LV_OBJ_RECORD_FLAG))
            {
                lv_obj_clear_flag(ui_s->now_ui, LV_OBJ_RECORD_FLAG); 
                //关闭录像的流
                close_stream(ui_s->avi_save_s);
                ui_s->avi_save_s = NULL;
            }
            lv_label_set_text(ui_s->status_obj,"takephoto");
        }
        else
        {
            lv_label_set_text(ui_s->status_obj,"record");
        }
    }
}

//菜单栏
static void enter_menu_select_ui(lv_event_t * e)
{
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'a')
    {
        //创建ui
        lv_obj_t *list;
        lv_obj_t *list_item;
        lv_group_t *group;
        group = lv_group_create();
        lv_indev_set_group(indev_keypad, group);
        
        struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
        list = lv_list_create(ui_s->now_ui);
        //删除group
        lv_obj_add_event_cb(list, clear_menu_select_ui, LV_EVENT_DELETE, group);
        

        //分辨率切换完毕后,就退出
        list_item = lv_list_add_btn(list, NULL, "640x480");
        list_item->user_data = (void*)list;
        lv_group_add_obj(group, list_item);
        os_printf("list_item:%X\n",list_item);
        lv_obj_add_event_cb(list_item, DPI_640x480_select_ui, LV_EVENT_SHORT_CLICKED, ui_s);

        //分辨率切换完毕后,就退出
        list_item = lv_list_add_btn(list, NULL, "1280x960");
        list_item->user_data = (void*)list;
        lv_group_add_obj(group, list_item);
        os_printf("list_item:%X\n",list_item);
        lv_obj_add_event_cb(list_item, DPI_1280x960_select_ui, LV_EVENT_SHORT_CLICKED, ui_s);


        list_item = lv_list_add_btn(list, NULL, "1920x1440");
        list_item->user_data = (void*)list;
        lv_group_add_obj(group, list_item);
        os_printf("list_item:%X\n",list_item);
        lv_obj_add_event_cb(list_item, DPI_1920x1440_select_ui, LV_EVENT_SHORT_CLICKED, ui_s);

#if 1
        //退出,还原group
        list_item = lv_list_add_btn(list, NULL, "exit");
        list_item->user_data = (void*)list;
        lv_group_add_obj(group, list_item);
        os_printf("list_item:%X\n",list_item);
        lv_obj_add_event_cb(list_item, exit_menu_select_ui, LV_EVENT_SHORT_CLICKED, ui_s);
#endif
    }
}
//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_preview_encode_select_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;

    //创建takephoto或者record的显示ui
    ui_s->status_obj = lv_label_create(ui);
    ui_s->is_takephoto = 1;
    lv_label_set_text(ui_s->status_obj,"takephoto");
    lv_obj_set_style_text_color(ui_s->status_obj, lv_palette_main(LV_PALETTE_RED), 0);

    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    ui_s->s = scale3_stream_not_bind(S_PREVIEW_ENCODE_QVGA,640,480,320,240,YUV_P0);
    //绑定流到Video P0显示
    streamSrc_bind_streamDest(ui_s->s,R_VIDEO_P0);
    enable_stream(ui_s->s,1);


    //默认拍照分辨率
    //打开scale1
    ui_s->scale1_stream = scale1_stream_not_bind(S_SCALE1_STREAM,640,480,1280,960);
    os_sleep_ms(10);

    ui_s->jpg0_stream = new_video_app_stream_with_mode(R_JPEG_DEMO,2,1280,960);
    streamSrc_bind_streamDest(ui_s->jpg0_stream,R_LVGL_TAKEPHOTO);
    //发送到图传
    streamSrc_bind_streamDest(ui_s->jpg0_stream,R_RTP_JPEG);
    //发送到录像
    streamSrc_bind_streamDest(ui_s->jpg0_stream,R_AVI_SAVE_VIDEO);
    enable_stream(ui_s->jpg0_stream,1);

    ui_s->takephoto_s = takephoto_stream(R_LVGL_TAKEPHOTO);

    //获取adc,由外部已经打开了,如果没有打开,这里要处理一下,这里名称固定了(因为与硬件绑定,就不作为灵活变动),就不做成灵活变动了
    ui_s->audio_s = get_stream_available(S_ADC_AUDIO);
    //发送测试一下
    streamSrc_bind_streamDest(ui_s->audio_s,R_AVI_SAVE_AUDIO);

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);


    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_preview_encode_select_ui, LV_EVENT_KEY, ui_s);

    //拍照的按键事件
    lv_obj_add_event_cb(ui, encode_takephoto_ui, LV_EVENT_PRESSED, ui_s);
    

    //弹出菜单栏,用于切换分辨率
    lv_obj_add_event_cb(ui, enter_menu_select_ui, LV_EVENT_KEY, ui_s);

    //切换录像还是拍照功能
    lv_obj_add_event_cb(ui, change_mode_ui, LV_EVENT_KEY, ui_s);
}

lv_obj_t *preview_encode_selct_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct preview_encode_takephoto_selct_ui_s *ui_s = (struct preview_encode_takephoto_selct_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct preview_encode_takephoto_selct_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "preview_encode_slect");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_preview_encode_select_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}