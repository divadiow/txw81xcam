/*************************************************************************************** 
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "osal/string.h"
#include "stream_frame.h"
#include "keyWork.h"
#include "keyScan.h"
#include "fatfs/osal_file.h"


#include "custom_mem/custom_mem.h"


//data申请空间函数
#define STREAM_MALLOC     custom_malloc_psram
#define STREAM_FREE       custom_free_psram
#define STREAM_ZALLOC     custom_zalloc_psram

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     custom_malloc
#define STREAM_LIBC_FREE       custom_free
#define STREAM_LIBC_ZALLOC     custom_zalloc
#define LV_OBJ_TAKEPHOTO_FLAG LV_OBJ_FLAG_USER_1


extern lv_indev_t * indev_keypad;
extern lv_style_t g_style;
struct preview_encode_takephoto_960P_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;
    lv_group_t    *now_group;
    lv_obj_t    *now_ui;
    lv_timer_t *timer;

    stream *s; 
    stream *scale1_stream;
    stream *jpg0_stream;
    stream *takephoto_s;

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
					key_ret = 'q';
				break;
				case AD_DOWN:
					key_ret = 's';
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


				default:
				break;

			}
		}
		else if((val&0xff) ==   KEY_EVENT_LUP)
		{
			

				switch(val>>8)
				{
					case AD_PRESS:
						key_ret = 'e';
					break;

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
    struct preview_encode_takephoto_960P_ui_s *ui_s = (struct preview_encode_takephoto_960P_ui_s *)t->user_data;
    struct data_structure *data_s;
    //看看对应的流有没有图片,如果有,则保存
    data_s = recv_real_data(ui_s->takephoto_s);
    if(data_s)
    {
        enable_stream(ui_s->takephoto_s,0);
        char filename[64];
        sprintf(filename,"0:%08d.jpg",(uint32_t)os_jiffies());
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
    struct preview_encode_takephoto_960P_ui_s *ui_s = (struct preview_encode_takephoto_960P_ui_s *)lv_event_get_user_data(e);
    stream *s = ui_s->takephoto_s;
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





static void exit_preview_encode_960P_ui(lv_event_t * e)
{
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        struct preview_encode_takephoto_960P_ui_s *ui_s = (struct preview_encode_takephoto_960P_ui_s *)lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        close_stream(ui_s->s);
        close_stream(ui_s->scale1_stream);
        close_stream(ui_s->jpg0_stream);
        close_stream(ui_s->takephoto_s);
        lv_obj_del(ui_s->now_ui);
        set_lvgl_get_key_func(NULL);
    }

}
//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_preview_encode_960P_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct preview_encode_takephoto_960P_ui_s *ui_s = (struct preview_encode_takephoto_960P_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    ui_s->s = scale3_stream_not_bind(S_PREVIEW_ENCODE_QVGA,640,480,320,240,YUV_P0);
    //绑定流到Video P0显示
    streamSrc_bind_streamDest(ui_s->s,R_VIDEO_P0);
    enable_stream(ui_s->s,1);

    //打开scale1
    ui_s->scale1_stream = scale1_stream_not_bind(S_SCALE1_STREAM,640,480,1280,960);
    os_sleep_ms(10);

    ui_s->jpg0_stream = new_video_app_stream_with_mode(R_JPEG_DEMO,2,1280,960);
    streamSrc_bind_streamDest(ui_s->jpg0_stream,R_LVGL_TAKEPHOTO);
    streamSrc_bind_streamDest(ui_s->jpg0_stream,R_RTP_JPEG);
    enable_stream(ui_s->jpg0_stream,1);

    ui_s->takephoto_s = takephoto_stream(R_LVGL_TAKEPHOTO);

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);


    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_preview_encode_960P_ui, LV_EVENT_KEY, ui_s);

    //拍照的按键事件
    lv_obj_add_event_cb(ui, encode_takephoto_ui, LV_EVENT_PRESSED, ui_s);
}

lv_obj_t *preview_encode_960P_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct preview_encode_takephoto_960P_ui_s *ui_s = (struct preview_encode_takephoto_960P_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct preview_encode_takephoto_960P_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "preview_encode_960P");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_preview_encode_960P_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}