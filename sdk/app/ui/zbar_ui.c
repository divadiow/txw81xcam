#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "osal/string.h"
#include "stream_frame.h"
#include "other_jpg_show/other_jpg_show_stream.h"

#include "custom_mem/custom_mem.h"
#include "fatfs/osal_file.h"

#include "keyWork.h"
#include "keyScan.h"


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
struct lvgl_zbar_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t  *now_group;
    lv_obj_t    *now_ui;

//这个是需要的流,但命名不一定对应,有可能通过中间流进行中转实现
    stream *zbar_jpg_s;
    stream *decode_s;
    stream *lvgl_zbar_sd_s;
    stream *zbar_stream;
};

//这创建一个由lvgl读取sd,然后发送的的数据流
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


static uint32_t lvgl_zbar_stream_cmd_func(stream *s,int opcode,uint32_t arg)
{
    uint32_t res = 0;
//    uint32_t flags;
    switch(opcode)
    {

        case STREAM_ZBAR_FROM_SD:
        {
            //sd卡读取,然后发送,arg是图片地址
            //获取一个data_s
            struct data_structure *data_s = get_src_data_f(s);
            uint8_t flag = 0;
            if(data_s)
            {
                os_printf("%s:%d\tfilename:%s\n",__FUNCTION__,__LINE__,(char*)arg);
                void *fp = osal_fopen((const char*)arg,"rb");
                if(fp)
                {
                    uint32_t filesize = osal_fsize(fp);
                    uint8_t *buf = (uint8_t*)STREAM_MALLOC(filesize);
                    if(buf)
                    {
                        osal_fread(buf,1,filesize,fp);
                        //发送出去
                        data_s->data = buf;
                        data_s->type = SET_DATA_TYPE(JPEG,JPEG_FULL);
                        set_stream_real_data_len(data_s,filesize);
                        send_data_to_stream(data_s);
                        os_printf("zbar read file send end\n");
                        flag = 1;
                    }
                    
                    osal_fclose(fp);
                }

                if(!flag)
                {
                    force_del_data(data_s);
                }

            }
            
        }
        break;
        default:
        break;
    }
    return res;
}


static int opcode_func_not_bind(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            register_stream_self_cmd_func(s,lvgl_zbar_stream_cmd_func);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{ 
		}
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                //os_printf("%s:%d\tdata:%X\tdata->data:%X\n",__FUNCTION__,__LINE__,data,data->data);
                STREAM_FREE(data->data);
                data->data = NULL;
            }
		}

		break;

        case STREAM_FILTER_DATA:
        {
        }
        break;

        case STREAM_DATA_FREE_END:
        break;


		//数据发送完成,可以选择唤醒对应的任务
		case STREAM_SEND_DATA_FINISH:
		break;


		case STREAM_SEND_DATA_START:
		{
		}
		break;

		case STREAM_DATA_DESTORY:
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
			//默认都返回成功
		break;
	}
	return res;
}


stream *lvgl_zbar_from_sd_stream(const char *name)
{

	stream *s = open_stream_available(name,8,0,opcode_func_not_bind,NULL);
	return s;
}


static void send_zbar_ui(lv_event_t * e)
{
    struct lvgl_zbar_s *ui_s = (struct lvgl_zbar_s *)lv_event_get_user_data(e);
    //发送命令,去读取sd卡数据发到对应的流
    stream_self_cmd_func(ui_s->lvgl_zbar_sd_s,STREAM_ZBAR_FROM_SD,(uint32_t)"0:1.jpg");
}


static void exit_zbar_ui(lv_event_t * e)
{
    struct lvgl_zbar_s *ui_s = (struct lvgl_zbar_s *)lv_event_get_user_data(e);
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        close_stream(ui_s->zbar_jpg_s);
        close_stream(ui_s->decode_s);
        close_stream(ui_s->lvgl_zbar_sd_s);
        close_stream(ui_s->zbar_stream);
        lv_obj_del(ui_s->now_ui);
        set_lvgl_get_key_func(NULL);
    }

}


//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_zbar_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct lvgl_zbar_s *ui_s = (struct lvgl_zbar_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    ui_s->decode_s = jpg_decode_stream_not_bind(S_JPG_DECODE);
    //将解码的数据推送到Video P0和Video P1显示
    streamSrc_bind_streamDest(ui_s->decode_s,R_VIDEO_P0);
    streamSrc_bind_streamDest(ui_s->decode_s,R_ZBAR);
    enable_stream(ui_s->decode_s,1);


    //显示一下要识别的二维码,还需要创建一个从sd读取jpg图到SR_ZBAR_JPG
    ui_s->zbar_jpg_s = other_jpg_stream_not_bind(SR_ZBAR_JPG,200,120,200,120,YUV_P0 | YUV_OTHER);
    //将other_jpg的数据给到S_JPG_DECODE进行编码
    streamSrc_bind_streamDest(ui_s->zbar_jpg_s,S_JPG_DECODE);
    enable_stream(ui_s->zbar_jpg_s,1);

    //启动二维码的流
    ui_s->zbar_stream = zbar_stream(R_ZBAR,YUV_OTHER);


    ui_s->lvgl_zbar_sd_s = lvgl_zbar_from_sd_stream(S_ZBAR_FROM_SD);
    streamSrc_bind_streamDest(ui_s->lvgl_zbar_sd_s,SR_ZBAR_JPG);
    enable_stream(ui_s->zbar_jpg_s,1);


    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);


    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_zbar_ui, LV_EVENT_KEY, ui_s);

    //发送一张图片去识别
    lv_obj_add_event_cb(ui, send_zbar_ui, LV_EVENT_PRESSED, ui_s);
}



lv_obj_t *zbar_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct lvgl_zbar_s *ui_s = (struct lvgl_zbar_s*)STREAM_LIBC_ZALLOC(sizeof(struct lvgl_zbar_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "zbar_from_sd_test");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_zbar_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}