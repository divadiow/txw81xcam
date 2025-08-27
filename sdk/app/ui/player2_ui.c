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

#define PLAY_UI_DIR "DCIM"
#define PLAY_AVI_STREAM_NAME    (64)
enum
{
    PLAY_STATUS_STOP_PLAY = BIT(0),
    PLAY_STATUS_PLAY_1FPS = BIT(1),  //播放一帧,然后自动暂停
    PLAY_STATUS_PLAY_END =  BIT(2),  //播放完毕,需要重新播放   
};

//转发流的播放流结构体
struct player_forward_stream_s
{
	struct os_work work;
    stream *s;
    struct data_structure *data_s;
    uint32_t magic;
    uint32_t start_time;
    uint32_t last_fps_play_time;       //上一帧播放的时间(非系统时间,是视频帧的时间)
    uint8_t play_status;    //自己的播放状态,第一bit代表播放或者暂停
};

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
stream *newavi_player2_init(const char *filename,const char *stream_name);

struct player_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_ui;

    stream *other_s;
    stream *decode_s;
    stream *player_s;
    stream *avi_s;
    lv_timer_t *timer;
    lv_obj_t * label_time;
    uint8_t *play_name;

};



//文件列表读取
static uint32_t avi_open_dir(DIR *avi_dir,char *dir_name)
{
    char filepath[64];
    os_sprintf(filepath,"0:%s",dir_name);
	os_printf("filepath:%s\n",filepath);
    return f_opendir(avi_dir,filepath);
}

static uint32_t avi_close_dir(DIR *avi_dir)
{
    f_closedir(avi_dir);
	return 0;
}
static uint32_t next_file(DIR *avi_dir,FILINFO* finfo)
{
    uint32_t ret = f_readdir(avi_dir,finfo);
    if(ret != FR_OK || finfo->fname[0] == 0)
    {
        return ~0;
    }
    return ret;
}

struct avi_list_param
{
    lv_group_t *group;
    lv_obj_t *ui;
    struct player_ui_s *ui_s;
};
typedef int (*creat_avi_list_ui)(const char *filename,void *data);




static void exit_show_filelist(lv_event_t * e)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    struct player_ui_s *ui_s = (struct player_ui_s *)lv_event_get_user_data(e);
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



//进入回放
static void enter_playback(lv_event_t * e)
{
    struct player_ui_s *ui_s = (struct player_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *list_item = lv_event_get_current_target(e);
    lv_obj_t *list = list_item->user_data;
    //将当前的流关闭,内部判断是否=NULL,这里就不判断了
    close_stream(ui_s->avi_s);
    if(ui_s->play_name)
    {
        STREAM_FREE(ui_s->play_name);
        ui_s->play_name = NULL;
    }

    os_printf("ui_s->label_time:%X\n",ui_s->label_time);
    lv_label_set_text(ui_s->label_time, "00:00");

    //重新打开一个视频文件
    uint8_t path[64];
    const char *filename = lv_list_get_btn_text(list,list_item);
    os_printf("play filename:%s\n",filename);
    os_sprintf(path,"0:%s/%s",PLAY_UI_DIR,filename);

    ui_s->play_name = (uint8_t*)STREAM_MALLOC(PLAY_AVI_STREAM_NAME);
    os_sprintf(ui_s->play_name,"%s_%04d",filename,(uint32_t)os_jiffies());
    os_printf("stream play_name:%s\n",ui_s->play_name);
    ui_s->avi_s = newavi_player2_init((const char *)ui_s->play_name,(const char *)path);
    if(ui_s->avi_s)
    {
        //设置播放一帧,先去停止播放器
        struct player_forward_stream_s *player_msg = ui_s->player_s->priv;
        os_work_cancle2(&player_msg->work, 1);


        streamSrc_bind_streamDest(ui_s->avi_s,S_NEWPLAYER);
        //streamSrc_bind_streamDest(ui_s->avi_s,SR_OTHER_JPG);
        //streamSrc_bind_streamDest(ui_s->avi_s,R_SPEAKER);
        

        //重新开始播放,修改启动时间
        player_msg->last_fps_play_time = 0;
        player_msg->start_time = os_jiffies();
        player_msg->magic = os_jiffies();
        stream_self_cmd_func(ui_s->avi_s,NEWAVI_PLAYER_MAGIC,player_msg->magic);
        player_msg->play_status = (PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);
        OS_WORK_REINIT(&player_msg->work);
        os_run_work_delay(&player_msg->work, 1);

        //文件开始解析
        enable_stream(ui_s->avi_s,1);
    }   

    
    exit_show_filelist(e);
}



static int avi_list_show(const char *filename,void *data)
{
    struct avi_list_param *param = (struct avi_list_param*)data;
	lv_obj_t * btn = lv_list_add_btn(param->ui, NULL, (const char *)filename);
    btn->user_data = (void*)param->ui;
    if(param->group)
    {
        lv_group_add_obj(param->group, btn);
    }
	
    //回调函数,进入回放功能
	lv_obj_add_event_cb(btn, enter_playback, LV_EVENT_CLICKED, param->ui_s);
	return 0;
}

//遍历文件夹
static int each_read_file(char *dir_name,creat_avi_list_ui fn,void *param)
{
    DIR  dir;
    FILINFO finfo;
    int ret = 0;
    if(!fn)
    {
        goto  each_read_file_end;
    }
    ret = avi_open_dir(&dir,dir_name);
    if(ret)
    {
       goto  each_read_file_end;
    }

	while(next_file(&dir,&finfo) == FR_OK)
	{
		if(ret == FR_OK && finfo.fname[0] != 0)
		{
            #if 0
			os_printf("fname:%s\n",finfo.fname);
			btn = lv_list_add_btn(list, NULL, (const char *)finfo.fname);
			lv_group_add_obj(group, btn);
			lv_obj_add_event_cb(btn, MP3_replay, LV_EVENT_ALL, list);
            #endif
            fn(finfo.fname,param);
			
		}
		else
		{
			break;
		}
	}


    avi_close_dir(&dir);
each_read_file_end:
    os_printf("%s:%d\tret:%d\n",__FUNCTION__,__LINE__,ret);
    return ret;
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
        struct player_ui_s *ui_s = (struct player_ui_s *)lv_event_get_user_data(e);
        struct avi_list_param param;
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

        each_read_file(PLAY_UI_DIR,avi_list_show,(void*)&param);

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

//stream_self_cmd_func(jpg_de->parent_data_s->s,JPG_DECODE_ARG,(uint32_t)jpg_de->parent_data_s);

//这里要创建一个流,专门是控制avi播放流的,由jpg解码,发送到这里缓存起来,然后控制播放速度、暂停等
//这里使用lvgl的timer来代替workqueue





static uint32_t custom_get_data_len(void *data)
{
	struct data_structure  *data_s = (struct data_structure  *)data;
    return get_stream_real_data_len(data_s->data);
}

static void *custom_get_data(void *data)
{
	struct data_structure  *data_s = (struct data_structure  *)data;
    return get_stream_real_data(data_s->data);;
}


static uint32_t custom_get_data_time(void *data)
{
	struct data_structure  *data_s = (struct data_structure  *)data;
    return get_stream_data_timestamp(data_s->data);
}


static uint32_t forward_stream_cmd_func(stream *s,int opcode,uint32_t arg)
{
    struct data_structure *data = (struct data_structure *)arg;
    if(data->data)
    {
        struct data_structure *data_parent = (struct data_structure *)data->data;
        return stream_self_cmd_func(data_parent->s,YUV_ARG,(uint32_t)data_parent);
    }
    return 0;
}


//这个是转发的流,只是添加一些必要信息,所以这里获取的data信息要更换
static const stream_ops_func forward_stream_ops = 
{
	.get_data_len = custom_get_data_len,
    .get_data = custom_get_data,
    .get_data_time = custom_get_data_time,
};



static int32_t player_work(struct os_work *work)
{
    struct player_forward_stream_s *player_msg  = (struct player_forward_stream_s *)work;
    struct data_structure *data_s = NULL;
    uint32_t time;
	//static int count = 0;
    if(!player_msg->s->enable)
    {
        goto player_work_get_data;
    }
    
    if(!(player_msg->play_status&PLAY_STATUS_STOP_PLAY))
    {
        goto player_work_get_data;
    }


    if(player_msg->data_s)
    {

        time = get_stream_data_timestamp(player_msg->data_s);
        //先检查时间是否合适
        if(player_msg->start_time + time > os_jiffies())
        {
            if(player_msg->magic != player_msg->data_s->magic)
            {
                free_data(player_msg->data_s);
                player_msg->data_s = NULL;
            }
            goto player_work_end;
        }
        //再次检查是否符合需要播放(有可能是缓存或者快进导致进入这里)
        if(player_msg->magic != player_msg->data_s->magic)
        {
            free_data(player_msg->data_s);
            player_msg->data_s = NULL;
            _os_printf("=");
            goto player_work_end;
        }

        if(player_msg->play_status&PLAY_STATUS_PLAY_1FPS)
        {
            player_msg->play_status &= ~(PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);
        }
        


        //os_printf("time:%d\tstart_time:%d\tmagic:%d\n",time,player_msg->start_time,player_msg->data_s->magic);
        player_msg->last_fps_play_time = time;
        //判断一下时间是否需要播放
        data_s = get_src_data_f(player_msg->s);
        //准备填好显示、解码参数信息,然后发送到解码的模块去解码,主要如果没有成功要如何处理,这里没有考虑
        if(data_s)
        {
            data_s->type = player_msg->data_s->type;
            data_s->data = player_msg->data_s;
            //os_printf("time:%d\ttype:%X\n",time,player_msg->data_s->type);
            _os_printf("=");
            send_data_to_stream(data_s);
            player_msg->data_s = NULL;
        }
    }

player_work_get_data:
    if(!player_msg->data_s)
    {
        player_msg->data_s = recv_real_data(player_msg->s);
    }
    else
    {
        if(player_msg->magic != player_msg->data_s->magic)
        {
            free_data(player_msg->data_s);
            player_msg->data_s = NULL;
            goto player_work_end;
        }
    }
    
player_work_end:
    os_run_work_delay(work, 1);
	return 0;
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
            struct player_forward_stream_s *msg = (struct player_forward_stream_s *)STREAM_ZALLOC(sizeof(struct player_forward_stream_s));
            msg->s = s;
            s->priv = msg;
            msg->start_time = os_jiffies();
            msg->data_s = NULL;
            msg->magic = 0;
            msg->play_status = (PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);
            msg->last_fps_play_time = 0;
            register_stream_self_cmd_func(s,forward_stream_cmd_func);
            stream_data_dis_mem_custom(s);
            OS_WORK_INIT(&msg->work, player_work, 0);
            os_run_work_delay(&msg->work, 1);
            //enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
        case STREAM_DATA_DIS:
        {
            struct data_structure *data = (struct data_structure *)priv;
            data->ops = (stream_ops_func*)&forward_stream_ops;
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
            struct data_structure *data = (struct data_structure *)priv;
            struct player_forward_stream_s *msg = (struct player_forward_stream_s *)s->priv;
            //过滤一下没有用的数据包,只是接收play_time_start之后的数据包
            if(!(msg->magic == data->magic))
            {
                res = 1;
            }


        }
		break;

		case STREAM_DATA_FREE:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                //因为是转发,所以这里移除的话是交给原来的方法移除
                _os_printf("$");
                free_data(data->data);
                data->data = NULL;
            }
		}

		break;

        case STREAM_CLOSE_EXIT:
        {
            struct player_forward_stream_s *msg = (struct player_forward_stream_s *)s->priv;
            os_work_cancle2(&msg->work, 1);
            //清理不需要的资源
            if(msg->data_s)
            {
                os_printf("msg->data_s:%X\n",msg->data_s);
                free_data(msg->data_s);
            }
        }
        break;

        case STREAM_MARK_GC_END:
        {
            struct player_forward_stream_s *msg = (struct player_forward_stream_s *)s->priv;
            if(msg)
            {
                STREAM_FREE(msg);
            }
            
        }
            
        break;


		default:
		break;
	}
	return res;
}



//创建播放器的流,这个流主要实现的是控制速度转发作用,暂时控制的是视频
//如果要控制音频,需要计算时间来控制速度
static stream *player_stream(const char *name)
{
    stream *s = open_stream_available(name,16,16,opcode_func,NULL);
    return s;
}






static void player_locate_ctrl_ui(lv_event_t * e)
{
    struct player_ui_s *ui_s = (struct player_ui_s *)lv_event_get_user_data(e);
    struct player_forward_stream_s *player_msg = ui_s->player_s->priv;
    int32_t c = *((int32_t *)lv_event_get_param(e));
    //printf("c:%d\n",c);
    switch(c)
    {
        //后退
        case 'a':
        {

            os_work_cancle2(&player_msg->work, 1);
            uint32_t now_play_time = stream_self_cmd_func(ui_s->avi_s,NEWAVI_PLAYER_LOCATE_REWIND_INDEX,player_msg->last_fps_play_time);
            //重新开始播放,修改启动时间
            player_msg->last_fps_play_time = now_play_time;
            player_msg->start_time = os_jiffies() - now_play_time;
            player_msg->magic = os_jiffies();
            stream_self_cmd_func(ui_s->avi_s,NEWAVI_PLAYER_MAGIC,player_msg->magic);
            player_msg->play_status = (PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);

            OS_WORK_REINIT(&player_msg->work);
            os_run_work_delay(&player_msg->work, 1);
        }

        break;
        //前进
        case 'd':
        {
            os_work_cancle2(&player_msg->work, 1);
            uint32_t now_play_time = stream_self_cmd_func(ui_s->avi_s,NEWAVI_PLAYER_LOCATE_FORWARD_INDEX,player_msg->last_fps_play_time);
            //重新开始播放,修改启动时间
            player_msg->last_fps_play_time = now_play_time;
            player_msg->start_time = os_jiffies() - now_play_time;
            player_msg->magic = os_jiffies();
            stream_self_cmd_func(ui_s->avi_s,NEWAVI_PLAYER_MAGIC,player_msg->magic);
            player_msg->play_status = (PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);
            OS_WORK_REINIT(&player_msg->work);
            os_run_work_delay(&player_msg->work, 1);
        }

        break;
        
        default:
        break;
    }
}

static void player_ctrl_ui(lv_event_t * e)
{
    struct player_ui_s *ui_s = (struct player_ui_s *)lv_event_get_user_data(e);
    struct player_forward_stream_s *player_msg = ui_s->player_s->priv;
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    //根据播放状态设置
    if(player_msg->play_status&PLAY_STATUS_STOP_PLAY)
    {
        player_msg->play_status &= (~PLAY_STATUS_STOP_PLAY);
    }
    else
    {
        //重新开始播放,修改启动时间
        player_msg->start_time = os_jiffies() - player_msg->last_fps_play_time;
        player_msg->play_status |= PLAY_STATUS_STOP_PLAY;
        os_printf("################## stop time:%d\tlast_time:%d\n",player_msg->start_time,player_msg->last_fps_play_time);
    }
}
static void exit_player_ui(lv_event_t * e)
{
    struct player_ui_s *ui_s = (struct player_ui_s *)lv_event_get_user_data(e);
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        lv_timer_del(ui_s->timer);
        struct player_ui_s *ui_s = (struct player_ui_s *)lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        
        
        close_stream(ui_s->other_s);
        close_stream(ui_s->decode_s);
        close_stream(ui_s->player_s);
        close_stream(ui_s->avi_s);
        ui_s->other_s = NULL;
        ui_s->decode_s = NULL;
        ui_s->player_s = NULL;
        ui_s->avi_s = NULL;
        
        lv_obj_del(ui_s->now_ui);
        if(ui_s->play_name)
        {
            STREAM_FREE(ui_s->play_name);
            ui_s->play_name = NULL;
        }
        set_lvgl_get_key_func(NULL);
        os_printf("sram mem:%d\tpsram mem:%d\n",sysheap_freesize(&sram_heap),sysheap_freesize(&psram_heap));
        print_custom_sram();
        print_custom_psram();
    }

}

static void player_ui_timer(lv_timer_t *t)
{
    struct player_ui_s *ui_s = (struct player_ui_s *)t->user_data;
    if(ui_s->avi_s)
    {
        //获取当前播放器的流结构体
        struct player_forward_stream_s *player_msg = ui_s->player_s->priv;
        //os_printf("play time:%d\tstart_time:%d\n",player_msg->last_fps_play_time,player_msg->start_time);
        char show_time[64];
        sprintf(show_time,"%02d:%02d",(player_msg->last_fps_play_time/1000)/60,(player_msg->last_fps_play_time/1000)%60);
        lv_label_set_text(ui_s->label_time, show_time);
#if 0
        //判断文件已经播放完毕,如果需要重新播放,则重新建立播放文件
        int status = stream_self_cmd_func(ui_s->avi_s,NEWAVI_PLAYER_STATUS,0);
        if(status)
        {
            os_printf("status:%d\n",status);
        }
#endif
    }

}



static void enter_player_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct player_ui_s *ui_s = (struct player_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *base_ui = ui_s->base_ui;
    lv_obj_add_flag(base_ui, LV_OBJ_FLAG_HIDDEN);

    
    lv_obj_t *ui = lv_obj_create(lv_scr_act()); 
    ui_s->now_ui = ui; 
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
#if 0
    struct avi_list_param param;
    lv_obj_t *list = lv_list_create(ui_s->now_ui);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    param.ui_s = ui_s;
    param.ui = list;
    param.group = NULL;
    each_read_file("DCIM",avi_list_show,(void*)&param);
    return;
#endif


    //播放转发发送到P0,控制速度
    ui_s->player_s = player_stream(S_NEWPLAYER);
    //streamSrc_bind_streamDest(ui_s->player_s,R_VIDEO_P0);
    streamSrc_bind_streamDest(ui_s->player_s,R_SPEAKER);
    streamSrc_bind_streamDest(ui_s->player_s,SR_OTHER_JPG);
    enable_stream(ui_s->player_s,1);

    //SR_OTHER_JPG接收数据,然后发送到decode
    ui_s->other_s = other_jpg_stream_not_bind(SR_OTHER_JPG,320,240,320,240,YUV_P0);
    streamSrc_bind_streamDest(ui_s->other_s,S_JPG_DECODE);
    enable_stream(ui_s->other_s,1);

    //decode发送到S_NEWPLAYER控制速度后,转发到P0
    ui_s->decode_s = jpg_decode_stream_not_bind(S_JPG_DECODE);
    streamSrc_bind_streamDest(ui_s->decode_s,R_VIDEO_P0);
    //streamSrc_bind_streamDest(ui_s->decode_s,S_NEWPLAYER);
    enable_stream(ui_s->decode_s,1);

#if 0
    ui_s->avi_s = newavi_player2_init(S_NEWAVI,"0:DCIM/AVI1.avi");
    if(ui_s->avi_s)
    {
        streamSrc_bind_streamDest(ui_s->avi_s,S_NEWPLAYER);
        //streamSrc_bind_streamDest(ui_s->avi_s,SR_OTHER_JPG);
        //streamSrc_bind_streamDest(ui_s->avi_s,R_SPEAKER);
        enable_stream(ui_s->avi_s,1);
    }   
#endif

    //创建lvgl的timer,用于获取播放的时间

    ui_s->label_time = lv_label_create(ui);
    lv_label_set_text(ui_s->label_time, "00:00");
    lv_obj_align(ui_s->label_time,LV_ALIGN_TOP_MID,0,0);
    ui_s->timer = lv_timer_create(player_ui_timer, 100,  (void*)ui_s);
    

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_player_ui, LV_EVENT_KEY, ui_s); 
    //控制暂停、播放
    lv_obj_add_event_cb(ui, player_ctrl_ui, LV_EVENT_SHORT_CLICKED, ui_s); 

    //快进快退,一次尝试快进5秒,一次尝试快退5秒
    lv_obj_add_event_cb(ui, player_locate_ctrl_ui, LV_EVENT_KEY, ui_s); 

    //弹出菜单栏,用于切换分辨率
    lv_obj_add_event_cb(ui, show_filelist, LV_EVENT_KEY, ui_s);
}

lv_obj_t *player2_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct player_ui_s *ui_s = (struct player_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct player_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    ui_s->play_name = NULL;
    
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "player2");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_player_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}