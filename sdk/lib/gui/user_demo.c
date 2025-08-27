/**
 * @file lv_demo_widgets.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "dev/lcdc/hglcdc.h"
#include "osal/semaphore.h"
#include "lib/lcd/lcd.h"
#include "lib/lcd/gui.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "hal/dma.h"
#include "video_app/video_app.h"
#include "resorce/language.h"
#include "lv_demo_widgets.h"
#include "openDML.h"
#include "osal/mutex.h"
#include "avidemux.h"
#include "avi/play_avi.h"
#include "playback/playback.h"
#include "lib/vef/video_ef.h"
#include "osal/task.h"
#if MP3_EN
#include "mp3/mp3_decode.h"
#endif

#if AMR_EN
#include "amr_app/amr_app.h"
#endif

#include "lvImg.h"

//#include "amrnb_app/amr_app.h"

extern gui_msg gui_cfg;
extern volatile vf_cblk g_vf_cblk;
extern lv_group_t * group_golop;
extern lv_indev_t * indev_keypad;

lv_group_t * group_cur;
lv_group_t * group0_golop;
lv_group_t * group1_golop;
lv_group_t * group2_golop;
lv_group_t * group3_golop;
lv_group_t * group4_golop;
lv_group_t * group5_golop;
lv_group_t * group6_golop;
lv_group_t * group7_golop;
lv_group_t * group8_golop;
lv_group_t * group9_golop;
lv_group_t * music_group;
lv_group_t * mp3_group;
lv_obj_t * cur_obj;
lv_obj_t * page0_obj;
lv_obj_t * page1_obj;
lv_obj_t * page2_obj;
lv_obj_t * page3_obj;
lv_obj_t * page4_obj;
lv_obj_t * page5_obj;
lv_obj_t * page6_obj;
lv_obj_t * page7_obj;
lv_obj_t * page8_obj;
lv_obj_t * page9_obj;
lv_obj_t * page15_obj;    //usb
lv_obj_t * ui_musicplay;
lv_obj_t * ui_musicplay;
lv_obj_t * ui_musicpause;
lv_obj_t * ui_musicnext;
lv_obj_t * ui_musicpre;
lv_obj_t * music_label;
lv_obj_t * back_btn;
lv_obj_t * record_btn;	
lv_obj_t * start_rec_btn;
lv_obj_t * photo_btn;
lv_obj_t * take_photo_btn;
lv_obj_t * setting_btn;
lv_obj_t * wifi_btn;
lv_obj_t * uvc_on_btn;
lv_obj_t * larger_btn;
lv_obj_t * rahmen_btn;
lv_obj_t * printer_btn;
lv_obj_t * haha_btn;
lv_obj_t * vf_sel_btn;
lv_obj_t * vf_sel_label;
lv_obj_t * next_btn;
lv_obj_t * label_uvc_on;
lv_obj_t * label_time;
lv_obj_t * label_rec;
//lv_obj_t * main_img;
lv_obj_t * playback_photo_btn;
lv_obj_t * playback_rec_btn;
lv_obj_t * playback_photo_printer_btn;
lv_obj_t * next_photo_btn;
lv_obj_t * next_rec_btn;
lv_obj_t * game_btn;
lv_obj_t * spi_btn;

lv_obj_t * rec_next_btn;
lv_obj_t * rec_back_btn;
lv_obj_t * photo_next_btn;
lv_obj_t * photo_back_btn;
lv_obj_t * wifi_next_btn;
lv_obj_t * wifi_back_btn;
lv_obj_t * setting_next_btn;
lv_obj_t * setting_back_btn;
lv_obj_t * playback_rec_next_btn;
lv_obj_t * playback_rec_back_btn;
lv_obj_t * playback_photo_next_btn;
lv_obj_t * playback_photo_back_btn;
lv_obj_t * game_start_btn;
lv_obj_t * game_back_btn;
lv_obj_t * game_next_btn;

lv_obj_t * spi_back_btn;
lv_obj_t * spi_next_btn;

lv_obj_t * list_setting_children;


lv_obj_t * setting_option_btn[LANGUAGE_STR_MAX];
lv_obj_t * resolution_btn[6];    //VGA,720P,1080P,2K,4K,8K
lv_obj_t * continous_btn[5];
lv_obj_t * game_list_btn[13];

extern uint8 *video_psram_mem;
extern uint8 *video_psram_mem1;
extern uint8 *video_psram_mem2;

extern uint8 *video_decode_mem;
extern uint8 *video_decode_mem1;
extern uint8 *video_decode_mem2;
extern uint8 video_decode_config_mem[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2];
extern uint8 video_decode_config_mem1[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2];
extern uint8 video_decode_config_mem2[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2];

extern uint8 video_psram_config_mem[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2];
extern uint8 video_psram_config_mem1[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2];
extern uint8 video_psram_config_mem2[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2];


extern int global_avi_running;
extern int global_avi_exit;
extern lv_font_t myfont;
extern uint8 *yuvbuf;
uint8_t page_cur = 0;
uint8_t page_num;
uint8_t uvc_start = 0;
extern uint8_t uvc_open;

uint8_t rec_open;
uint8_t enlarge_glo = 10;
uint8_t rahmen_open;
extern volatile uint8_t vfx_open;
extern volatile uint8_t vfx_pingpang; 


extern Vpp_stream photo_msg;
extern lcd_msg lcd_info;
extern uint32_t scsi_count;
extern uint8 spi_video_run;
uint8_t name_rec_photo[32];
struct lv_time {
	uint8 lv_hour;
	uint8 lv_min;
	uint8 lv_sec;
	uint8 res;
};

struct lv_time time_msg;


extern const unsigned char main_demo[118351];
extern const unsigned char menu_wifi[60464];
extern const unsigned char menu_usb[55191];


void yes_or_no_list();
void language_list();
void sound_list();
void ios_list();
void rec_list();
void game_list();
void takephoto_list();
void cycle_list();
void continous_shot_list();
void usbdev_list();
void lv_page_select(uint8_t page);
uint32_t file_mode (const char *mode);
void play_wav_page();
void start_record_thread(uint8_t video_fps,uint8_t audio_frq);
uint8_t send_stop_record_cmd();
void lv_demo_music_pause(void);


LV_IMG_DECLARE(photo);
LV_IMG_DECLARE(rec);
LV_IMG_DECLARE(DV_W);
LV_IMG_DECLARE(DV_R);
LV_IMG_DECLARE(srcbtn0);
LV_IMG_DECLARE(srcbtn0_1);
LV_IMG_DECLARE(menu1_0);
LV_IMG_DECLARE(menu1_1);
LV_IMG_DECLARE(menu1_2);
LV_IMG_DECLARE(menu2_0);
LV_IMG_DECLARE(menu2_1);
LV_IMG_DECLARE(menu2_2);
LV_IMG_DECLARE(menu3_0);
LV_IMG_DECLARE(menu3_1);
LV_IMG_DECLARE(menu3_2);
LV_IMG_DECLARE(menu4_0);
LV_IMG_DECLARE(menu4_1);
LV_IMG_DECLARE(menu4_2);
LV_IMG_DECLARE(menu5_0);
LV_IMG_DECLARE(menu5_1);
LV_IMG_DECLARE(menu5_2);
LV_IMG_DECLARE(menu6_0);
LV_IMG_DECLARE(menu6_1);
LV_IMG_DECLARE(menu6_2);
LV_IMG_DECLARE(menu_null);
LV_IMG_DECLARE(menu_test);
LV_IMG_DECLARE(menu_spi);
LV_IMG_DECLARE( icon_play);
LV_IMG_DECLARE( icon_last);
LV_IMG_DECLARE( icon_next);
LV_IMG_DECLARE( icon_pause);
LV_IMG_DECLARE( img_lv_demo_music_logo); 

void take_photo_thread_init(uint16_t w,uint16_t h,uint8_t continuous_spot);

void lv_time_reset(struct lv_time *time_now){
	static lv_style_t style;	
	time_now->lv_hour = 0;
	time_now->lv_min  = 0;
	time_now->lv_sec  = 0;

	lv_style_reset(&style);
	lv_style_init(&style);
	lv_style_set_text_font(&style,&lv_font_montserrat_24);
	
	if(label_time == NULL){	
		label_time = lv_label_create(page1_obj);
	}
	os_printf("label_time:%x\n",label_time);
	lv_obj_add_style(label_time,&style,LV_STATE_DEFAULT);
	lv_obj_align(label_time,LV_ALIGN_BOTTOM_MID,0,0);
	lv_label_set_recolor(label_time, 1);	
	
}


void lv_time_add(struct lv_time *time_now){
	if(time_now->lv_sec == 59){
		if(time_now->lv_min == 59){
			if(time_now->lv_hour >= 99){
				time_now->lv_hour = 0;
				time_now->lv_min  = 0;
				time_now->lv_sec  = 0;				
			}else{
				time_now->lv_hour++;
				time_now->lv_min  = 0;
				time_now->lv_sec  = 0;				
			}
		}else{
			time_now->lv_min++;
			time_now->lv_sec = 0;
		}
	}else{
		time_now->lv_sec++;
	}
}


void display_time(lv_obj_t * p_label,struct lv_time *time_now){
	static char time_str[20];
	printf("%02d:%02d:%02d\r\n",time_now->lv_hour,time_now->lv_min,time_now->lv_sec);
	sprintf(time_str,"#ff0088 %02d:%02d:%02d#",time_now->lv_hour,time_now->lv_min,time_now->lv_sec);
	lv_label_set_text(label_time,time_str);
	
}

void del_time_label(){
	lv_obj_del(label_time);
	label_time = NULL;
}

void dv_flash(uint8_t flash){
	lv_obj_t * img;
	lv_obj_del(label_rec);
	img = lv_img_create(page1_obj);
	label_rec = img;
	if(flash == 1)
		lv_img_set_src(img,&DV_W);
	else
		lv_img_set_src(img,&DV_R);
	
	lv_obj_set_pos(img, SCALE_WIDTH-80, 0);
}

static void setting_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[FORMAT_STR]){
			yes_or_no_list();
		}

		if(e->target == setting_option_btn[LANGUAGE_STR]){
			language_list();
		}
		
		if(e->target == setting_option_btn[SOUND_STR]){
			sound_list();
		}	

		if(e->target == setting_option_btn[ISO_STR]){
			ios_list();
		}

		if(e->target == setting_option_btn[RECORD_STR]){
			rec_list();
		}

		if(e->target == setting_option_btn[TAKEPHOTO_STR]){
			takephoto_list();
		}
		
		if(e->target == setting_option_btn[CYCLE_STR]){
			cycle_list();
		}

		if(e->target == setting_option_btn[CONTINUOUS_STR]){
			continous_shot_list();
		}

		if(e->target == setting_option_btn[USBDEV_STR]){
			usbdev_list();
		}

		if(e->target == setting_option_btn[EXIT_STR]){
			lv_page_select(0);
		}
		

	}

}








//创建一个新的MP3界面


uint32_t mp3_open_dir(DIR *avi_dir,char *dir_name)
{
    char filepath[64];
    os_sprintf(filepath,"0:%s",dir_name);
	os_printf("filepath:%s\n",filepath);
    return f_opendir(avi_dir,filepath);
}

uint32_t mp3_close_dir(DIR *avi_dir)
{
    f_closedir(avi_dir);
	return 0;
}
uint32_t get_dir_next_file(DIR *avi_dir,FILINFO* finfo)
{
    uint32_t ret = f_readdir(avi_dir,finfo);
    if(ret != FR_OK || finfo->fname[0] == 0)
    {
        return ~0;
    }
    return ret;
}
#if MP3_EN

#if 0
static void MP3_play_continue(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		set_mp3_decode_status(MP3_PLAY);
	}
}

static void MP3_pause(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		set_mp3_decode_status(MP3_PAUSE);
	}
}


//mp3界面资源释放
static void MP3_res_release(lv_event_t * e)
{
	lv_group_t *group = (lv_group_t *)lv_event_get_user_data(e);
	lv_indev_set_group(indev_keypad, group4_golop);	
	//删除mp3的group
	lv_group_del(group);
}

static void MP3_exit(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);
	if(code == LV_EVENT_CLICKED)
	{
		set_mp3_decode_status(MP3_STOP);
		lv_obj_del(obj);
	}
}
extern void mp3_decode_thread(void *d);
static void MP3_replay(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * list = (lv_obj_t *)lv_event_get_user_data(e);
	lv_obj_t * btn = (lv_obj_t *)e->target;
	if(code == LV_EVENT_CLICKED)
	{
		os_printf("%s name:%s\n",__FUNCTION__,lv_list_get_btn_text(list,btn));
		char filepath[64];
		os_sprintf(filepath,"0:mp3/%s",lv_list_get_btn_text(list,btn));
		os_printf("filepath:%s\n",filepath);
		mp3_decode_thread(filepath);
	}
}


static void MP3_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		lv_group_t * group = lv_group_create();
		lv_indev_set_group(indev_keypad, group);	


		lv_obj_t * obj = lv_obj_create(lv_scr_act());
		lv_obj_add_event_cb(obj, MP3_res_release, LV_EVENT_DELETE, group);
		lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
		lv_obj_t *list = lv_list_create(obj);
		lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
		lv_obj_t * btn = lv_list_add_btn(list, NULL, (const char *)"exit");
		//退出这个界面
		lv_obj_add_event_cb(btn, MP3_exit, LV_EVENT_ALL, obj);
		lv_group_add_obj(group, btn);

		//继续播放
		btn = lv_list_add_btn(list, NULL, (const char *)"play");
		lv_obj_add_event_cb(btn, MP3_play_continue, LV_EVENT_ALL, obj);
		lv_group_add_obj(group, btn);

		//暂停
		btn = lv_list_add_btn(list, NULL, (const char *)"pause");
		lv_obj_add_event_cb(btn, MP3_pause, LV_EVENT_ALL, obj);
		lv_group_add_obj(group, btn);



		//读取sd卡的内容,将文件的列表显示出来
		DIR  avi_dir;
		FILINFO finfo;
		int ret;
		ret = mp3_open_dir(&avi_dir,"mp3");
		os_printf("ret:%d\n",ret);
		while(get_dir_next_file(&avi_dir,&finfo) == FR_OK)
		{
			if(ret == FR_OK && finfo.fname[0] != 0)
			{
				os_printf("fname:%s\n",finfo.fname);
				btn = lv_list_add_btn(list, NULL, (const char *)finfo.fname);
				lv_group_add_obj(group, btn);
				lv_obj_add_event_cb(btn, MP3_replay, LV_EVENT_ALL, list);
				
			}
			else
			{
				break;
			}
		}


		mp3_close_dir(&avi_dir);
	}
}
#else

#define BAR_COLOR1          lv_color_hex(0xe9dbfc)
#define BAR_COLOR2          lv_color_hex(0x6f8af6)
#define BAR_COLOR3          lv_color_hex(0xffffff)
#define BAR_COLOR1_STOP     50
#define BAR_COLOR2_STOP     100
#define BAR_COLOR3_STOP     (230)
#define BAR_COUNT 			10
#define DEG_STEP            (180/BAR_COUNT)
#define BAND_CNT            4
#define BAR_PER_BAND_CNT    (BAR_COUNT / BAND_CNT)
#define INTRO_TIME          1000
#define ACTIVE_TRACK_CNT    3
static lv_obj_t * spectrum_obj;
static lv_obj_t * mp3_obj;
static volatile uint32_t spectrum_i = 0;
static volatile uint32_t spectrum_i_pause = 0;
static uint32_t spectrum_lane_ofs_start = 0;
//static uint32_t spectrum_len;
static uint32_t bar_rot = 0;
static uint32_t bar_ofs = 0;
//static uint32_t track_id;
static bool playing;
static bool start_anim = TRUE;
static lv_coord_t start_anim_values[10];
//static const uint16_t (* spectrum)[4];
static const uint16_t rnd_array[30] = {994, 285, 553, 11, 792, 707, 966, 641, 852, 827, 44, 352, 146, 581, 490, 80, 729, 58, 695, 940, 724, 561, 124, 653, 27, 292, 557, 506, 382, 199};
volatile uint16_t spectrum_dequeue[4096];
volatile int play_num = 1;;
uint32 repeat_count = 150;
int tem = 0;

extern uint32_t vad_tem_buff[5];
extern uint32 vad_buff_num;
extern uint32 num;
extern float average_energy_tem;
extern float smoothed_energy;
extern float smoothed_map_energy;
extern int sp_num;
extern int sd_num;
extern volatile uint32 vad_r;


static void MP3_play_continue(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		set_mp3_decode_status(MP3_PLAY);
	}
}

static void MP3_pause(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		lv_demo_music_pause();
		set_mp3_decode_status(MP3_PAUSE);
	}
}

//mp3界面资源释放
static void MP3_res_release(lv_event_t * e)
{
	lv_group_t *group = (lv_group_t *)lv_event_get_user_data(e);
	lv_indev_set_group(indev_keypad, music_group);	
	//删除mp3的group
	lv_group_del(group);
}

static void MP3_exit(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);
	if(code == LV_EVENT_CLICKED)
	{
		set_mp3_decode_status(MP3_STOP);
		lv_obj_del(obj);
	}
}


static void spectrum_anim_cb(void * a, int32_t v)
{
    lv_obj_t * obj = a;
	os_printf("spectrum_i:%d\r\n",v);
    if(start_anim) {
        lv_obj_invalidate(obj);
        return;
    }

	if(spectrum_i != v){
		spectrum_dequeue[++sp_num] = vad_r;
		play_num++;
		os_printf("play_num:%d spectrum_buff:%d\r\n",play_num,spectrum_dequeue[play_num]);
	}
    spectrum_i = v;
	printf("tem:%d sd_num:%d\n",tem,sd_num);
	if(tem == sd_num){
		playing = false;
		memset((void *)spectrum_dequeue,0,sizeof(spectrum_dequeue));
		spectrum_i_pause = 0;
		spectrum_i = 0;;
		sp_num = 1;
		sd_num = 1;
		play_num = 1;
		//repeat_count = 0;
		lv_anim_del(spectrum_obj, spectrum_anim_cb);
	} else {
		tem = sd_num;
	}


    lv_obj_invalidate(obj);

    static uint32_t bass_cnt = 0;
    static int32_t last_bass = -1000;
    static int32_t dir = 1;
    if(spectrum_dequeue[play_num] > 12) {
        if(spectrum_i - last_bass > 5) {
            bass_cnt++;
            last_bass = spectrum_i;
            if(bass_cnt >= 2) {
                bass_cnt = 0;
                spectrum_lane_ofs_start = spectrum_i;
                bar_ofs++;
            }
        }
    }
    if(spectrum_dequeue[play_num] < 4) bar_rot += dir;

}

static void start_anim_cb(void * a, int32_t v)
{
    lv_coord_t * av = a;
    *av = v;
    lv_obj_invalidate(spectrum_obj);
}

static void spectrum_end_cb(lv_anim_t * a)
{
	printf("2=2\n");
	playing = false;
	memset((void *)spectrum_dequeue,0,sizeof(spectrum_dequeue));
	sp_num = 1;
	play_num = 1;
	repeat_count = 150;
	set_mp3_decode_status(MP3_STOP);
    LV_UNUSED(a);
}

void lv_demo_music_pause(void)
{
    playing = false;
    spectrum_i_pause = spectrum_i;
    spectrum_i = 0;
	spectrum_dequeue[play_num] = 0;
	lv_anim_t * a = lv_anim_get(spectrum_obj, spectrum_anim_cb);
	repeat_count = lv_anim_get_repeat_count(a);
	printf("repeat_count:%d\n",repeat_count);
    lv_anim_del(spectrum_obj, spectrum_anim_cb);
    lv_obj_invalidate(spectrum_obj);
}

void lv_demo_music_resume(void)
{
    playing = true;
    spectrum_i = spectrum_i_pause;
    lv_anim_t a;
    lv_anim_init(&a);
	lv_anim_set_values(&a, spectrum_i, 2000);
    lv_anim_set_exec_cb(&a, spectrum_anim_cb);
    lv_anim_set_var(&a, spectrum_obj);
	lv_anim_set_time(&a, 2500);
	printf("repeat_count:%d\n",repeat_count);
	lv_anim_set_repeat_count(&a,repeat_count);
    lv_anim_set_ready_cb(&a, spectrum_end_cb);
    lv_anim_start(&a);

}

extern void mp3_decode_init(void *d, void *read_func);
static void MP3_replay(lv_event_t * e)
{
	//lv_obj_t * obj = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);
	bool ret = false;
	//os_printf("code:%d\n",code);
	if(code == LV_EVENT_CLICKED)
	{
		if(get_mp3_decode_status() == MP3_PAUSE){
			lv_demo_music_resume();
			set_mp3_decode_status(MP3_PLAY);
		}else{
			playing = false;
			memset((void *)spectrum_dequeue,0,sizeof(spectrum_dequeue));
			spectrum_i_pause = 0;
			spectrum_i = 0;;
			sp_num = 1;
			play_num = 1;
			//repeat_count = 0;
			ret = lv_anim_del(spectrum_obj, spectrum_anim_cb);
			if(ret == true)
				repeat_count = 150;
			lv_obj_invalidate(spectrum_obj);

			lv_demo_music_resume();
			os_printf("%s name:%s\n",__FUNCTION__,lv_label_get_text(label));
			char filepath[64];
			os_sprintf(filepath,"0:mp3/%s",lv_label_get_text(label));
			os_printf("filepath:%s\n",filepath);
			mp3_decode_init(filepath, NULL);
		}

	}
}


int32_t get_cos(int32_t deg, int32_t a)
{
    int32_t r = (lv_trigo_cos(deg) * a);

    r += LV_TRIGO_SIN_MAX / 2;
    return r >> LV_TRIGO_SHIFT;
	//return 
}

int32_t get_sin(int32_t deg, int32_t a)
{
    int32_t r = lv_trigo_sin(deg) * a;

    r += LV_TRIGO_SIN_MAX / 2;
    return r >> LV_TRIGO_SHIFT;

}

static void spectrum_draw_event_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_event_set_ext_draw_size(e, LV_VER_RES);
    }
    else if(code == LV_EVENT_COVER_CHECK) {
        lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
    }	
	else if(code == LV_EVENT_DRAW_POST)
	{
		lv_obj_t * obj = lv_event_get_target(e);
        lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);

        lv_opa_t opa = lv_obj_get_style_opa(obj, LV_PART_MAIN);
        if(opa < LV_OPA_MIN) {
			return;
		}

		lv_point_t poly[4];
        lv_point_t center;

        center.x = obj->coords.x1 + lv_obj_get_width(obj) / 2 - 130;
        center.y = obj->coords.y1 + lv_obj_get_height(obj) / 2;

		lv_draw_rect_dsc_t draw_dsc;
        lv_draw_rect_dsc_init(&draw_dsc);
        draw_dsc.bg_opa = LV_OPA_COVER;

        uint16_t r[64];
        uint32_t i;

        lv_coord_t min_a = 5;
        lv_coord_t r_in = 0;

        for(i = 0; i < BAR_COUNT; i++) r[i] = r_in + min_a;
	
        uint32_t s;
        for(s = 0; s < 4; s++) {
            uint32_t f;
            uint32_t band_w = 0;    /*Real number of bars in this band.这个波段的棒数*/
            switch(s) {
                case 0:
                    band_w = 8;
                    break;
                case 1:
                    band_w = 6;
                    break;
                case 2:
                    band_w = 4;
                    break;
                case 3:
                    band_w = 2;
                    break;
            }
            /* Add "side bars" with cosine characteristic.添加具有余弦特性的“侧边条”。*/
            for(f = 0; f < band_w; f++) {
				uint32_t ampl_main = spectrum_dequeue[play_num];
                int32_t ampl_mod = get_cos(f * 360 / band_w + 180, 180) + 180;
                int32_t t = BAR_PER_BAND_CNT * s - band_w / 2 + f;
                if(t < 0) t = BAR_COUNT + t;
                if(t >= BAR_COUNT) t = t - BAR_COUNT;
                r[t] += (ampl_main * ampl_mod) >> 9;
            }
        }

        uint32_t amax = 20;
        int32_t animv = spectrum_i - spectrum_lane_ofs_start;
        if(animv > amax) animv = amax;
        for(i = 0; i < BAR_COUNT; i++) {
            //uint32_t deg_space = 1;
            uint32_t j = (i + bar_rot + rnd_array[bar_ofs % 10]) % BAR_COUNT;
            uint32_t k = (i + bar_rot + rnd_array[(bar_ofs + 1) % 10]) % BAR_COUNT;

            uint32_t v = (r[k] * animv + r[j] * (amax - animv)) / amax;
            if(start_anim) {
                v = r_in + start_anim_values[i];
            }

            if(v < BAR_COLOR1_STOP) draw_dsc.bg_color = BAR_COLOR1;
            else if(v > BAR_COLOR3_STOP) draw_dsc.bg_color = BAR_COLOR3;
            else if(v > BAR_COLOR2_STOP) draw_dsc.bg_color = lv_color_mix(BAR_COLOR3, BAR_COLOR2,
                                                                              ((v - BAR_COLOR2_STOP) * 255) / (BAR_COLOR3_STOP - BAR_COLOR2_STOP));
            else draw_dsc.bg_color = lv_color_mix(BAR_COLOR2, BAR_COLOR1,
                                                      ((v - BAR_COLOR1_STOP) * 255) / (BAR_COLOR2_STOP - BAR_COLOR1_STOP));

            uint32_t di = 270;

			int32_t y_out = get_sin(di, v);
			if(y_out < -120 )
				y_out = -10;

			int32_t y_in  = get_sin(di,r_in);

            poly[0].x = center.x + i*15;
            poly[0].y = center.y + y_out;

            poly[1].x = center.x + i*15;
            poly[1].y = center.y + y_in;

            poly[2].x = center.x + 10 + i*15;
            poly[2].y = center.y + y_in;
            poly[3].x = center.x + 10 + i*15;
            poly[3].y = center.y + y_out;

            lv_draw_polygon(draw_ctx, &draw_dsc, poly, 4);
        }	

	}
}




/* 边框样式 */  
static lv_style_t border_style;
static lv_style_t mp3btn_style;  

void music_imgbtn_style_init()
{
	lv_style_init(&border_style);
	lv_style_set_width(&border_style, 30);
	lv_style_set_height(&border_style, 30);
	//lv_style_set_bg_opa(&btnImgStyle, 0);	
	lv_style_set_shadow_color(&border_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&border_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&border_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_width(&border_style, 0);	
	lv_style_set_border_width(&border_style, 0);
}

void music_btn_style_init()
{
	/****init  page_btn  style***/
	lv_style_init(&mp3btn_style);
	lv_style_set_width(&mp3btn_style, 30);
	lv_style_set_height(&mp3btn_style, 30);
	lv_style_set_bg_color(&mp3btn_style, lv_color_hex(0xFFFFFF));	//0x101018
	//lv_style_set_bg_opa(&pageBtnStyle, 0);	
	//lv_style_set_shadow_color(&mp3btn_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&mp3btn_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&mp3btn_style, lv_color_make(0x00, 0x00, 0x00));
	//lv_style_set_outline_opa(&pageBtnStyle, 0);	
	lv_style_set_outline_width(&mp3btn_style, 0);	
	lv_style_set_border_width(&mp3btn_style, 0);
	lv_style_set_pad_all(&mp3btn_style, 0);
	lv_style_set_pad_gap(&mp3btn_style,0);
}

int32_t now_file = 0;
FILINFO finfo;
char* mp3_get_file_name(int last_or_next) {  
    DIR avi_dir;  
    uint32_t ret;  
    uint32_t total_files = 0;   
    uint32_t current_file = 0; 
  
    ret = mp3_open_dir(&avi_dir, "mp3");  
    os_printf("ret:%d\n", ret);  
    if (ret != FR_OK) {  
        return NULL;   
    }  
  
    // 先遍历目录以计算文件总数  
    while (get_dir_next_file(&avi_dir, &finfo) == FR_OK) {  
        if (finfo.fname[0] != 0) {  
            total_files++;  
        } else {  
            break;  
        }  
    }  
    mp3_close_dir(&avi_dir);   
  
    ret = mp3_open_dir(&avi_dir, "mp3");  
    if (ret != FR_OK) {  
        return NULL;  
    }  
  
    if (last_or_next) {  
        now_file++;  
        if (now_file >= total_files) {   
            now_file = 0;  
        }  
    } else {  
        now_file--;  
        if (now_file < 0) {   
            now_file = total_files - 1;  
        }  
    }  
	os_printf("now_file:%d total_files:%d \n",now_file,total_files); 
    current_file = 0;  
    while (get_dir_next_file(&avi_dir, &finfo) == FR_OK) {  
        if (finfo.fname[0] != 0) {  
            if (current_file == now_file) {  
                mp3_close_dir(&avi_dir);  
                return finfo.fname;  
            }  
            current_file++;  
        } else {  
            break; 
        }  
    }  
  
    mp3_close_dir(&avi_dir);  
    return NULL; 
}

static void mp3_last_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);
	if(code == LV_EVENT_CLICKED)
	{
		playing = false;
		spectrum_i_pause = 0;
		spectrum_i = 0;
		play_num = 1;
		repeat_count = 150;
		memset((void *)spectrum_dequeue,0,sizeof(spectrum_dequeue));
		//lv_anim_t * a = lv_anim_get(spectrum_obj, spectrum_anim_cb);
		lv_anim_del(spectrum_obj, spectrum_anim_cb);
		lv_obj_invalidate(spectrum_obj);

		char *filepath;
		filepath = mp3_get_file_name(0);
		//os_printf("mp3 fname:%s\n",(const char*)mp3_get_file_name(0));
		printf("file:%s\n",filepath);
		set_mp3_decode_status(MP3_STOP);
		lv_label_set_text(label,(const char*)filepath);
	}	
}

static void mp3_next_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);
	if(code == LV_EVENT_CLICKED)
	{
		//os_printf("mp3 fname:%s\n",(const char*)mp3_get_file_name(1));
		playing = false;
		spectrum_i_pause = 0;
		spectrum_i = 0;
		play_num = 1;
		repeat_count = 150;
		memset((void *)spectrum_dequeue,0,sizeof(spectrum_dequeue));
		//lv_anim_t * a = lv_anim_get(spectrum_obj, spectrum_anim_cb);
		lv_anim_del(spectrum_obj, spectrum_anim_cb);
		lv_obj_invalidate(spectrum_obj);

		char *filepath;
		filepath =mp3_get_file_name(1);
		printf("file:%s\n",filepath);
		set_mp3_decode_status(MP3_STOP);
		lv_label_set_text(label,(const char*)filepath);
	}
}

static void stop_start_anim(lv_timer_t * t)
{
    LV_UNUSED(t);
    start_anim = false;
	lv_anim_del(spectrum_obj, start_anim_cb);
    lv_obj_refresh_ext_draw_size(spectrum_obj);
}

static void MP3_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED)
	{

		mp3_group = lv_group_create();
		lv_indev_set_group(indev_keypad, mp3_group);
		group_cur = mp3_group;

		lv_obj_t * obj = lv_obj_create(lv_scr_act());
		mp3_obj = obj;
		//spectrum_obj = lv_obj_create(mp3_obj);
		spectrum_obj = obj;
		lv_obj_set_size(obj,320,240);
		lv_obj_clear_flag(spectrum_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_event_cb(spectrum_obj,spectrum_draw_event_cb,LV_EVENT_ALL,NULL);
		lv_obj_refresh_ext_draw_size(spectrum_obj);
		//spectrum = spectrum_3;
		//spectrum_len = sizeof(spectrum_3) / sizeof(spectrum_3[0]) * 50;		

		start_anim = true;

		lv_timer_t * timer = lv_timer_create(stop_start_anim, INTRO_TIME + 4000, NULL);
    	lv_timer_set_repeat_count(timer, 1);		
		
		lv_anim_t a;
		lv_anim_init(&a);
		lv_anim_set_path_cb(&a, lv_anim_path_bounce);

		uint32_t i;
		lv_anim_set_exec_cb(&a, start_anim_cb);

		printf("LV_VER_RES:%d LV_HOR_RES:%d\n",LV_VER_RES,LV_HOR_RES);

		for(i = 0; i < BAR_COUNT; i++) {
			lv_anim_set_values(&a, LV_VER_RES, 5);
			lv_anim_set_delay(&a, INTRO_TIME - 200 + rnd_array[i] % 200);
			lv_anim_set_time(&a, 2500 + rnd_array[i] % 500);
			lv_anim_set_var(&a, &start_anim_values[i]);
			lv_anim_start(&a);
		}

		//lv_group_add_obj(mp3_group,spectrum_obj);

		music_imgbtn_style_init();
		music_btn_style_init();


//label
		music_label = lv_label_create(mp3_obj);
		lv_obj_set_width(music_label, LV_SIZE_CONTENT);
		lv_obj_set_height(music_label, LV_SIZE_CONTENT);
		lv_obj_align(music_label,LV_ALIGN_CENTER,0,35);
		lv_label_set_text(music_label,(const char*)mp3_get_file_name(1));
		//lv_obj_set_style_text_color(music_label, lv_color_hex(0xFCF9F9), LV_PART_MAIN | LV_STATE_DEFAULT );
		//lv_obj_set_style_text_opa(music_label, 255, LV_PART_MAIN| LV_STATE_DEFAULT);

//play
		ui_musicplay = lv_btn_create(mp3_obj);
		lv_obj_t *ui_musicplayimg = lv_obj_create(ui_musicplay);
		
		lv_obj_set_style_bg_img_src(ui_musicplayimg,&icon_play,0);
		lv_obj_add_style(ui_musicplay,&mp3btn_style,0);
		lv_obj_add_style(ui_musicplayimg,&border_style,0);
		//lv_obj_add_flag(ui_musicplay,LV_OBJ_FLAG_CLICKABLE);

		lv_obj_align(ui_musicplay,LV_ALIGN_CENTER,20,80);
		lv_obj_add_event_cb(ui_musicplay, MP3_replay, LV_EVENT_ALL, music_label);
		lv_group_add_obj(mp3_group, ui_musicplay);

//pause
		ui_musicpause = lv_btn_create(mp3_obj);
		lv_obj_t *ui_musicpauseimg = lv_obj_create(ui_musicpause);
		
		lv_obj_set_style_bg_img_src(ui_musicpauseimg,&icon_pause,0);
		lv_obj_add_style(ui_musicpause,&mp3btn_style,0);
		lv_obj_add_style(ui_musicpauseimg,&border_style,0);
		//lv_obj_add_flag(ui_musicplay,LV_OBJ_FLAG_CLICKABLE);

		lv_obj_align(ui_musicpause,LV_ALIGN_CENTER,-20,80);
		lv_obj_add_event_cb(ui_musicpause, MP3_pause, LV_EVENT_ALL, music_label);
		lv_group_add_obj(mp3_group, ui_musicpause);


//next
		ui_musicnext = lv_btn_create(mp3_obj);
		lv_obj_t *ui_musicnextimg = lv_obj_create(ui_musicnext);
		lv_obj_set_style_bg_img_src(ui_musicnextimg,&icon_next,0);
		lv_obj_add_style(ui_musicnext, &mp3btn_style, 0);
		lv_obj_add_style(ui_musicnextimg, &border_style, 0);
		lv_obj_align(ui_musicnext,LV_ALIGN_RIGHT_MID,-30,80);
		lv_obj_add_event_cb(ui_musicnext, mp3_next_handler, LV_EVENT_ALL, music_label);
		lv_group_add_obj(mp3_group, ui_musicnext);


//last
		ui_musicpre = lv_btn_create(mp3_obj);
		lv_obj_t *ui_musicpreimg = lv_obj_create(ui_musicpre);
		lv_obj_set_style_bg_img_src(ui_musicpreimg,&icon_last,0);
		lv_obj_add_style(ui_musicpre, &mp3btn_style, 0);
		lv_obj_add_style(ui_musicpreimg, &border_style, 0);
		lv_obj_align(ui_musicpre,LV_ALIGN_LEFT_MID,30,80);
		lv_obj_add_event_cb(ui_musicpre, mp3_last_handler, LV_EVENT_ALL, music_label);
		lv_group_add_obj(mp3_group, ui_musicpre);


//image
		lv_obj_t * music_img = 	lv_img_create(mp3_obj);
		lv_img_set_src(music_img,&img_lv_demo_music_logo);
		//lv_obj_set_size(music_img, 100, 100);
		lv_img_set_antialias(music_img, false);
		lv_obj_align(music_img, LV_ALIGN_CENTER, 90, -45);
		//lv_obj_add_event_cb(music_img, album_gesture_event_cb, LV_EVENT_GESTURE, NULL);
		lv_obj_clear_flag(music_img, LV_OBJ_FLAG_GESTURE_BUBBLE);
		lv_obj_add_flag(music_img, LV_OBJ_FLAG_CLICKABLE);

		lv_obj_fade_in(ui_musicplay, 1000, 0);
		lv_obj_fade_in(ui_musicnext, 1000, 0);
		lv_obj_fade_in(ui_musicpre, 1000, 0);
		lv_obj_fade_in(music_label, 1000, 0);
		lv_obj_fade_in(music_img, 1000, 0);
	}
}


#endif



#endif


#if AMR_EN

static void amr_exit(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);
	if(code == LV_EVENT_CLICKED)
	{
		set_amrnb_decode_status(AMR_STOP);
		lv_obj_del(obj);
	}
}


static void amr_play_continue(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		set_amrnb_decode_status(AMR_PLAY);
	}
}


static void amr_replay(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * list = (lv_obj_t *)lv_event_get_user_data(e);
	lv_obj_t * btn = (lv_obj_t *)e->target;
	if(code == LV_EVENT_CLICKED)
	{
		os_printf("%s name:%s\n",__FUNCTION__,lv_list_get_btn_text(list,btn));
		char filepath[64];
		os_sprintf(filepath,"0:amr/%s",lv_list_get_btn_text(list,btn));
		os_printf("filepath:%s\n",filepath);
		amr_decode_thread(filepath);
	}
}


static void amr_pause(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		set_amrnb_decode_status(AMR_PAUSE);
	}
}



static void amr_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		lv_group_t * group = lv_group_create();
		lv_indev_set_group(indev_keypad, group);	


		lv_obj_t * obj = lv_obj_create(lv_scr_act());
		//可以复用MP3的回调函数
		lv_obj_add_event_cb(obj, MP3_res_release, LV_EVENT_DELETE, group);
		lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
		lv_obj_t *list = lv_list_create(obj);
		lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
		lv_obj_t * btn = lv_list_add_btn(list, NULL, (const char *)"exit");
		//退出这个界面
		lv_obj_add_event_cb(btn, amr_exit, LV_EVENT_ALL, obj);
		lv_group_add_obj(group, btn);

		//继续播放
		btn = lv_list_add_btn(list, NULL, (const char *)"play");
		lv_obj_add_event_cb(btn, amr_play_continue, LV_EVENT_ALL, obj);
		lv_group_add_obj(group, btn);

		//暂停
		btn = lv_list_add_btn(list, NULL, (const char *)"pause");
		lv_obj_add_event_cb(btn, amr_pause, LV_EVENT_ALL, obj);
		lv_group_add_obj(group, btn);



		//读取sd卡的内容,将文件的列表显示出来
		DIR  avi_dir;
		FILINFO finfo;
		int ret;
		ret = mp3_open_dir(&avi_dir,"amr");
		os_printf("ret:%d\n",ret);
		while(get_dir_next_file(&avi_dir,&finfo) == FR_OK)
		{
			if(ret == FR_OK && finfo.fname[0] != 0)
			{
				os_printf("fname:%s\n",finfo.fname);
				btn = lv_list_add_btn(list, NULL, (const char *)finfo.fname);
				lv_group_add_obj(group, btn);
				lv_obj_add_event_cb(btn, amr_replay, LV_EVENT_ALL, list);
				
			}
			else
			{
				break;
			}
		}


		mp3_close_dir(&avi_dir);
	}
}
#endif




#if LV_USE_FREETYPE
static void ttf_test_exit_handler(lv_event_t * e)
{
	lv_obj_t  *ui = (lv_obj_t *)lv_event_get_user_data(e);
	lv_obj_del(ui);
}

static void ttf_test_restore_group_handler(lv_event_t * e)
{
	lv_group_t  *group = (lv_group_t *)lv_event_get_user_data(e);
	lv_indev_set_group(indev_keypad, group);
	os_printf("group:%X\n",group);
}
static void ttf_test_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED)
	{
		lv_font_t  *font = (lv_font_t *)lv_event_get_user_data(e);
		lv_group_t * group = lv_group_create();
		lv_indev_set_group(indev_keypad, group);
		lv_obj_t *test_ui = lv_obj_create(lv_scr_act());
		lv_obj_set_size(test_ui, LV_PCT(100), LV_PCT(100));
		//测试显示某个字体的文本
		lv_obj_set_style_text_font(test_ui, font, 0);


		//退出按钮
		lv_obj_t * btn = lv_btn_create(test_ui);
		lv_obj_add_event_cb(btn, ttf_test_exit_handler, LV_EVENT_CLICKED, test_ui);
		lv_obj_add_event_cb(test_ui, ttf_test_restore_group_handler, LV_EVENT_DELETE, lv_obj_get_group(e->current_target));
		lv_group_add_obj(group, btn);

		//测试一下问题
		lv_obj_t *label = lv_label_create(test_ui);
		lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
		lv_obj_set_size(label, 320, 240);
		lv_label_set_text(label, "123测试一些文字,看看流畅度如何,随机抽查几个文字,比如:ABCDefghijk,现在打算测试许多文字,使用的是后台渲染方式,现在测试一下,看看流畅度如何,我也不知道输入什么字比较好,泰芯FPV工程的测试:现在从网上复制一段话:在一个遥远的，被神秘森林环绕的小镇上，有一位年轻的发明家，名叫艾米丽。她拥有一间摆满了齿轮和电线的工作室，那里总是回响着轻微的机械声。艾米丽以其独创性和对科技的热爱而著称，她最新的发明，一臺能够预测天气的机器，已近完工。这台机器不仅采用了最先进的传感技术来分析大气模式，还融合了人工智能来处理数据，预测未来的天气情况。艾米丽希望这个发明能够帮助农民预测灾害性天气，避免农作物损失。同时，这个工具也为小镇的人们提供了日常穿着和活动的指导。在机器即将完成之际，艾米丽遇到了一个挑战。一个异常强大的风暴突然在小镇上空形成，电力系统因此中断，她的工作室也陷入了黑暗。艾米丽知道，这是对她发明的终极测试。她急忙启动了机器，紧张地等待着结果。机器开始运转，复杂的算法在人工智能的辅助下迅速处理数据。艾米丽紧盯着屏幕上的预测模型，风暴的路径和强度清晰可见。她的心跳加速，如果预测准确，这将证明她的机器的价值无可估量。终于，结果出来了。机器成功预测了风暴的精确路径，并且还指出了一条可能的暴雨带。艾米丽立即将结果通报给小镇的居民们，大家得以采取了及时的预防措施。随着镇上的居民安全地躲避风暴，艾米丽的脸上露出了释然的微笑。她知道，她的这项发明不仅仅是一项技术突破，更是对人类生活的一份珍贵贡献。");		lv_group_add_obj(group, label);
	}
}

static void ttf_font_del_handler(lv_event_t * e)
{
	lv_font_t  *font = (lv_font_t *)lv_event_get_user_data(e);
	os_printf("font:%X\n",font);
	lv_freetype_font_del(font);
}
#endif



#if NET_PAIR
extern uint8_t get_net_pair_status();
extern void set_pair_mode(uint8_t enable);

//检查是否配对完成,如果配对完成或者取消配对,则timer要移除
static void net_pair_timerevent_cb(lv_timer_t * timer)
{
	lv_obj_t * obj = (lv_obj_t *)timer->user_data;
	if(obj && !get_net_pair_status())
	{
		lv_obj_clear_state(obj, LV_STATE_CHECKED); 
		lv_timer_pause(timer);
	}
}

static void net_pair_remove_timer_event(lv_event_t * e)
{
	lv_timer_t * t = lv_event_get_user_data(e);
	lv_timer_del(t);
}
static void net_pair_switch_handler(lv_event_t * e)
{
	lv_obj_t * obj = lv_event_get_current_target(e);
	lv_timer_t * t = lv_event_get_user_data(e);
	if(lv_obj_has_state(obj, LV_STATE_CHECKED))
	{
		set_pair_mode(1);
		lv_timer_resume(t);
	}
	else
	{
		set_pair_mode(0);
		lv_timer_pause(t);
	}
}
#endif




static void format_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[YES_STR]){
			lv_page_select(6);
		}
		
		if(e->target == setting_option_btn[NO_STR]){
			lv_page_select(6);
		}
		lv_group_focus_obj(setting_option_btn[FORMAT_STR]);
	}
}


void nesStart(char* game);
extern const unsigned char acTanks[];
uint8_t game_sdh_cache[512];
extern FIL fp_jpg;
uint8 game_rom[1*1024] __attribute__ ((aligned(4),section(".psram.src")));
static void game_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	struct lcdc_device *lcd_dev;	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);		
	uint32_t res;
	uint32_t read_count = 0;
	uint32_t readLen;
	uint32_t data_len,data_count;
	uint32_t mod = file_mode ("rb");
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == game_list_btn[0]){
			res = f_open(&fp_jpg,"0:nes/1.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}		
		}
		
		if(e->target == game_list_btn[1]){
			res = f_open(&fp_jpg,"0:nes/2.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}
		}
		
		if(e->target == game_list_btn[2]){
			res = f_open(&fp_jpg,"0:nes/3.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}
		}		

		if(e->target == game_list_btn[3]){
			res = f_open(&fp_jpg,"0:nes/4.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}
		}

		if(e->target == game_list_btn[4]){
			res = f_open(&fp_jpg,"0:nes/5.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}
		}

		if(e->target == game_list_btn[5]){
			res = f_open(&fp_jpg,"0:nes/6.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}

		}
		
		if(e->target == game_list_btn[6]){
			res = f_open(&fp_jpg,"0:nes/7.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}

		}

		if(e->target == game_list_btn[7]){
			res = f_open(&fp_jpg,"0:nes/8.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}

		}		

		if(e->target == game_list_btn[8]){
			res = f_open(&fp_jpg,"0:nes/9.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}

		}

		if(e->target == game_list_btn[9]){
			res = f_open(&fp_jpg,"0:nes/10.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}

		}

		if(e->target == game_list_btn[10]){
			res = f_open(&fp_jpg,"0:nes/11.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}

		}

		if(e->target == game_list_btn[11]){
			res = f_open(&fp_jpg,"0:nes/12.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}

		}

		if(e->target == game_list_btn[12]){
			res = f_open(&fp_jpg,"0:nes/13.nes",mod);
			if(res){
				printf("f_open error\r\n");
				return;
			}

		}
		
		data_len = f_size(&fp_jpg); 
		if(data_len > 332*1024){
			os_printf("f_size too big error\r\n");
			return;
		}
		data_count = 512;
		while(data_len){
			res = f_read(&fp_jpg,game_sdh_cache,data_count,&readLen);
			memcpy(game_rom+read_count,game_sdh_cache,data_count);
			read_count += data_count;
			if(data_len > 512){
				data_len = data_len-data_count;
			}else{
				data_count = data_len; 
				data_len = 0;
			}
			
		}
		res = f_read(&fp_jpg,game_sdh_cache,data_count,&readLen);
		memcpy(game_rom+read_count,game_sdh_cache,data_count);	
		
		f_close(&fp_jpg);		
		lcdc_set_osd_en(lcd_dev,0);
		printf("rom:%02x%02x%02x%02x\r\n",game_rom[0],game_rom[1],game_rom[2],game_rom[3]);
		//nesStart((char*)game_rom); 

	}

}


static void language_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[LANGUAGE_EN_STR]){
			language_cur = 0;
			lv_page_select(6);
		}
		
		if(e->target == setting_option_btn[LANGUAGE_CN_STR]){
			language_cur = 1;
			lv_page_select(6);
		}
		lv_group_focus_obj(setting_option_btn[LANGUAGE_STR]);
	}
}

static void sound_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[OPEN_STR]){
			gui_cfg.sound_en = 1;
			lv_page_select(6);
		}
		
		if(e->target == setting_option_btn[CLOSE_STR]){
			gui_cfg.sound_en = 0;
			lv_page_select(6);
		}
		lv_group_focus_obj(setting_option_btn[SOUND_STR]);
	}
}

static void usbdev_handler(lv_event_t * e)
{
	void hgusb_dev_recfg(uint8_t dev_tpye);
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[UDISK_STR]){
			hgusb_dev_recfg(1);
			lv_page_select(6);
		}
		
		if(e->target == setting_option_btn[UVC_STR]){
			hgusb_dev_recfg(2);
			lv_page_select(6);
		}
		lv_group_focus_obj(setting_option_btn[USBDEV_STR]);
	}
}


static void ios_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[OPEN_STR]){
			gui_cfg.iso_en = 1;
			lv_page_select(6);
		}
		
		if(e->target == setting_option_btn[CLOSE_STR]){
			gui_cfg.iso_en = 0;
			lv_page_select(6);
		}
		lv_group_focus_obj(setting_option_btn[ISO_STR]);
	}
}

static void cycle_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[OPEN_STR]){
			gui_cfg.cycle_rec_en = 1;
			lv_page_select(6);
		}
		
		if(e->target == setting_option_btn[CLOSE_STR]){
			gui_cfg.cycle_rec_en = 0;
			lv_page_select(6);
		}
		lv_group_focus_obj(setting_option_btn[CYCLE_STR]);
	}
}


static void rec_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);	
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == resolution_btn[0]){
			gui_cfg.rec_w = 640;
			gui_cfg.rec_h = 480;
			lv_page_select(6);
		}
		
		if(e->target == resolution_btn[1]){
			gui_cfg.rec_w = 1280;
			gui_cfg.rec_h = 720;
			lv_page_select(6);
		}
		
		if(e->target == resolution_btn[2]){
			gui_cfg.rec_w = 1920;
			gui_cfg.rec_h = 1080;			
			lv_page_select(6);
		}
		lv_group_focus_obj(setting_option_btn[RECORD_STR]);
	}
}

static void takephoto_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == resolution_btn[0]){
			gui_cfg.photo_w = 640;
			gui_cfg.photo_h = 480;
			lv_page_select(6);
		}
		
		if(e->target == resolution_btn[1]){
			gui_cfg.photo_w = 1280;
			gui_cfg.photo_h = 720;
			lv_page_select(6);
		}
		
		if(e->target == resolution_btn[2]){
			gui_cfg.photo_w = 1920;
			gui_cfg.photo_h = 1080;
			lv_page_select(6);
		}

		if(e->target == resolution_btn[3]){
			gui_cfg.photo_w = 2560;
			gui_cfg.photo_h = 1440;
			lv_page_select(6);
		}

		if(e->target == resolution_btn[4]){
			gui_cfg.photo_w = 3840;
			gui_cfg.photo_h = 2560;
			lv_page_select(6);
		}

		if(e->target == resolution_btn[5]){
			gui_cfg.photo_w = 7680;
			gui_cfg.photo_h = 4320;
			lv_page_select(6);
		}
		
		lv_group_focus_obj(setting_option_btn[TAKEPHOTO_STR]);
	}
}

static void continous_shot_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == continous_btn[0]){
			printf("take photo 1\r\n");
			gui_cfg.take_photo_num = 1;
			lv_page_select(6);
		}
		
		if(e->target == continous_btn[1]){
			printf("take photo 2\r\n");
			gui_cfg.take_photo_num = 2;
			lv_page_select(6);
		}
		
		if(e->target == continous_btn[2]){
			printf("take photo 5\r\n");
			gui_cfg.take_photo_num = 5;
			lv_page_select(6);
		}

		if(e->target == continous_btn[3]){
			printf("take photo 8\r\n");
			gui_cfg.take_photo_num = 8;
			lv_page_select(6);
		}

		if(e->target == continous_btn[4]){
			printf("take photo 10\r\n");
			gui_cfg.take_photo_num = 10;
			lv_page_select(6);
		}
		
		lv_group_focus_obj(setting_option_btn[CONTINUOUS_STR]);
	}
}

void yes_or_no_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[YES_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][YES_STR]);
	lv_obj_add_event_cb(setting_option_btn[YES_STR], format_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[YES_STR]);
	
	setting_option_btn[NO_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][NO_STR]);
	lv_obj_add_event_cb(setting_option_btn[NO_STR], format_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[NO_STR]);

}


void language_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[LANGUAGE_EN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][LANGUAGE_EN_STR]);
	lv_obj_add_event_cb(setting_option_btn[LANGUAGE_EN_STR], language_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[LANGUAGE_EN_STR]);
	
	setting_option_btn[LANGUAGE_CN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][LANGUAGE_CN_STR]);
	lv_obj_add_event_cb(setting_option_btn[LANGUAGE_CN_STR], language_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[LANGUAGE_CN_STR]);

}


void sound_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[OPEN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][OPEN_STR]);
	lv_obj_add_event_cb(setting_option_btn[OPEN_STR], sound_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[OPEN_STR]);
	
	setting_option_btn[CLOSE_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][CLOSE_STR]);
	lv_obj_add_event_cb(setting_option_btn[CLOSE_STR], sound_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[CLOSE_STR]);

}

void usbdev_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[UDISK_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][UDISK_STR]);
	lv_obj_add_event_cb(setting_option_btn[UDISK_STR], usbdev_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[UDISK_STR]);
	
	setting_option_btn[UVC_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][UVC_STR]);
	lv_obj_add_event_cb(setting_option_btn[UVC_STR], usbdev_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[UVC_STR]);

}


void ios_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[OPEN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][OPEN_STR]);
	lv_obj_add_event_cb(setting_option_btn[OPEN_STR], ios_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[OPEN_STR]);
	
	setting_option_btn[CLOSE_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][CLOSE_STR]);
	lv_obj_add_event_cb(setting_option_btn[CLOSE_STR], ios_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[CLOSE_STR]);

}

void rec_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	resolution_btn[0] = lv_list_add_btn(list, NULL, "VGA");
	lv_obj_add_event_cb(resolution_btn[0], rec_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[0]);
	
	resolution_btn[1] = lv_list_add_btn(list, NULL, "720P");
	lv_obj_add_event_cb(resolution_btn[1], rec_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[1]);
	
	resolution_btn[2] = lv_list_add_btn(list, NULL, "1080P");
	lv_obj_add_event_cb(resolution_btn[2], rec_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[2]);

}

void game_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page7_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	game_list_btn[0] = lv_list_add_btn(list, NULL, "赤影战士");
	lv_obj_add_event_cb(game_list_btn[0], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[0]);
	
	game_list_btn[1] = lv_list_add_btn(list, NULL, "魂斗罗");
	lv_obj_add_event_cb(game_list_btn[1], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[1]);
	
	game_list_btn[2] = lv_list_add_btn(list, NULL, "泡泡龙");
	lv_obj_add_event_cb(game_list_btn[2], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[2]);

	game_list_btn[3] = lv_list_add_btn(list, NULL, "热血格斗传说");
	lv_obj_add_event_cb(game_list_btn[3], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[3]);

	game_list_btn[4] = lv_list_add_btn(list, NULL, "热血物语");
	lv_obj_add_event_cb(game_list_btn[4], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[4]);

	game_list_btn[5] = lv_list_add_btn(list, NULL, "忍者龙剑传");
	lv_obj_add_event_cb(game_list_btn[5], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[5]);

	game_list_btn[6] = lv_list_add_btn(list, NULL, "忍者神龟");
	lv_obj_add_event_cb(game_list_btn[6], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[6]);	

	game_list_btn[7] = lv_list_add_btn(list, NULL, "神奇世界");
	lv_obj_add_event_cb(game_list_btn[7], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[7]);

	game_list_btn[8] = lv_list_add_btn(list, NULL, "双截龙3");
	lv_obj_add_event_cb(game_list_btn[8], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[8]);

	game_list_btn[9] = lv_list_add_btn(list, NULL, "松鼠大作战");
	lv_obj_add_event_cb(game_list_btn[9], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[9]);

	game_list_btn[10] = lv_list_add_btn(list, NULL, "西游记");
	lv_obj_add_event_cb(game_list_btn[10], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[10]);

	game_list_btn[11] = lv_list_add_btn(list, NULL, "雪人兄弟");
	lv_obj_add_event_cb(game_list_btn[11], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[11]);	

	game_list_btn[12] = lv_list_add_btn(list, NULL, "幽幽白书");
	lv_obj_add_event_cb(game_list_btn[12], game_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, game_list_btn[12]);		
}

void takephoto_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);	
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
	lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	resolution_btn[0] = lv_list_add_btn(list, NULL, "VGA");
	lv_obj_add_event_cb(resolution_btn[0], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[0]);
	
	resolution_btn[1] = lv_list_add_btn(list, NULL, "720P");
	lv_obj_add_event_cb(resolution_btn[1], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[1]);
	
	resolution_btn[2] = lv_list_add_btn(list, NULL, "1080P");
	lv_obj_add_event_cb(resolution_btn[2], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[2]);

	resolution_btn[3] = lv_list_add_btn(list, NULL, "2K");
	lv_obj_add_event_cb(resolution_btn[3], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[3]);

	resolution_btn[4] = lv_list_add_btn(list, NULL, "4K");
	lv_obj_add_event_cb(resolution_btn[4], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[4]);

	resolution_btn[5] = lv_list_add_btn(list, NULL, "8K");
	lv_obj_add_event_cb(resolution_btn[5], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[5]);

}

void cycle_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[OPEN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][OPEN_STR]);
	lv_obj_add_event_cb(setting_option_btn[OPEN_STR], cycle_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[OPEN_STR]);
	
	setting_option_btn[CLOSE_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][CLOSE_STR]);
	lv_obj_add_event_cb(setting_option_btn[CLOSE_STR], cycle_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[CLOSE_STR]);

}

void continous_shot_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);	
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
	lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	continous_btn[0] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][CLOSE_STR]);
	lv_obj_add_event_cb(continous_btn[0], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[0]);
	
	continous_btn[1] = lv_list_add_btn(list, NULL, "2");
	lv_obj_add_event_cb(continous_btn[1], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[1]);
	
	continous_btn[2] = lv_list_add_btn(list, NULL, "5");
	lv_obj_add_event_cb(continous_btn[2], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[2]);

	continous_btn[3] = lv_list_add_btn(list, NULL, "8");
	lv_obj_add_event_cb(continous_btn[3], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[3]);

	continous_btn[4] = lv_list_add_btn(list, NULL, "10");
	lv_obj_add_event_cb(continous_btn[4], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[4]);


}


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static void main_menu_key_analyze(lv_event_t * e){
	uint8_t key_code;
	key_code = lv_indev_get_key(lv_indev_get_act());
	if(e->target == record_btn){
		if(key_code == LV_KEY_RIGHT){
			lv_group_focus_obj(photo_btn);
		}

		if(key_code == LV_KEY_LEFT){
			lv_group_focus_obj(playback_rec_btn);
		}

		if(key_code == LV_KEY_UP){
			lv_group_focus_obj(playback_rec_btn);
		}	
		
		if(key_code == LV_KEY_DOWN){
			lv_group_focus_obj(setting_btn);
		}				
	}

	if(e->target == photo_btn){
		if(key_code == LV_KEY_RIGHT){
			lv_group_focus_obj(wifi_btn);
		}

		if(key_code == LV_KEY_LEFT){
			lv_group_focus_obj(record_btn);
		}

		if(key_code == LV_KEY_UP){
			lv_group_focus_obj(setting_btn);
		}	
		
		if(key_code == LV_KEY_DOWN){
			lv_group_focus_obj(playback_photo_btn);
		}				
	}	

	if(e->target == wifi_btn){
		if(key_code == LV_KEY_RIGHT){
			lv_group_focus_obj(setting_btn);
		}

		if(key_code == LV_KEY_LEFT){
			lv_group_focus_obj(photo_btn);
		}

		if(key_code == LV_KEY_UP){
			lv_group_focus_obj(playback_photo_btn);
		}	
		
		if(key_code == LV_KEY_DOWN){
			lv_group_focus_obj(playback_rec_btn);
		}				
	}			

	if(e->target == setting_btn){
		if(key_code == LV_KEY_RIGHT){
			lv_group_focus_obj(playback_photo_btn);
		}

		if(key_code == LV_KEY_LEFT){
			lv_group_focus_obj(wifi_btn);
		}

		if(key_code == LV_KEY_UP){
			lv_group_focus_obj(record_btn);
		}	
		
		if(key_code == LV_KEY_DOWN){
			lv_group_focus_obj(photo_btn);
		}				
	}

	if(e->target == playback_photo_btn){
		if(key_code == LV_KEY_RIGHT){
			lv_group_focus_obj(playback_rec_btn);
		}

		if(key_code == LV_KEY_LEFT){
			lv_group_focus_obj(setting_btn);
		}

		if(key_code == LV_KEY_UP){
			lv_group_focus_obj(photo_btn);
		}	
		
		if(key_code == LV_KEY_DOWN){
			lv_group_focus_obj(wifi_btn);
		}				
	}	

	if(e->target == playback_rec_btn){
		if(key_code == LV_KEY_RIGHT){
			lv_group_focus_obj(record_btn);
		}

		if(key_code == LV_KEY_LEFT){
			lv_group_focus_obj(playback_photo_btn);
		}

		if(key_code == LV_KEY_UP){
			lv_group_focus_obj(wifi_btn);
		}	
		
		if(key_code == LV_KEY_DOWN){
			lv_group_focus_obj(record_btn);
		}				
	}

		//特殊,直接跳转
		if(key_code == LV_KEY_END)
		{
			play_wav_page();
		}

}

void page0_analyze(lv_event_t * e){
	if(e->target == record_btn){
		lv_page_select(1);
		page_num = 1;
	}
	
	if(e->target == photo_btn){
		lv_page_select(2);
		page_num = 2;			
	}
	
	if(e->target == wifi_btn){
		lv_page_select(3);
		page_num = 3;
	}
	
	if(e->target == setting_btn){
		lv_page_select(6);
		page_num = 6;			
//		lv_page_select(4);
//		page_num = 4;
	}
	
	if(e->target == playback_photo_btn){
		lv_page_select(4);
		page_num = 4;		
//		lv_page_select(5);
//		page_num = 5;				
	}
	
	if(e->target == playback_rec_btn){
		lv_page_select(5);
		page_num = 5;
//		lv_page_select(6);
//		page_num = 6;				
	}			

	if(e->target == game_btn){
		lv_page_select(7);
		page_num = 7;				
	}	

	if(e->target == spi_btn){
		lv_page_select(8);
		page_num = 8;				
	}	
}

void zoomImg(unsigned char *psrc, int sw, int sh,int channels, unsigned char *pdst, int dw, int dh);
void vga_to_paper_size(uint8_t *src,uint8_t* des){


	uint16_t x=0,y=0;
	zoomImg(src,640,480,1,des,512,384);
	
	for(y=0;y<384;y++)
	{
		for(x=0;x<512;x++)
		{
			src[(x+1)*384-1-y] = des[x+y*512];//90°
		}
	}


}



unsigned char linearInterpolation(unsigned char* pline1, unsigned char* pline2, int channels,int u, int v){
	int pm3 = u * v;
	int pm2 = (u << 8) - pm3;
	int pm1 = (v << 8) - pm3;
	int pm0 = (1 << 16) - pm1 - pm2 - pm3;


    unsigned char result = (unsigned char)(((pm0*pline1[0] + pm2 * pline1[channels] + pm1 * pline2[0] + pm3* pline2[channels]) >> 16)&0xff);
	
	return result;
}


void zoomImg(unsigned char *psrc, int sw, int sh,int channels, unsigned char *pdst, int dw, int dh)
{
	if (!psrc || !pdst) return;
	if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
	if (channels != 1 && channels != 3) return;


	unsigned char *p = psrc,*q = pdst,*pt;
	int srcBytesPerLine = sw * channels,dstBytesPerLine = dw * channels;
	int srcy_16 = 0, v_8 = 0, srcx_16 = 0;
	int stepw = (((sw - 1) << 16) / dw +1);
	int steph = ((sh - 1) << 16) / dh + 1;
	//int stepc = ((channels - 1) << 16)/channels + 1;
	int i, j, m;


	srcy_16 = 0;
	for (j = 0; j < dh; j++)
	{
		v_8 = (srcy_16 & 0xFFFF) >> 8;
		p = psrc + srcBytesPerLine * (srcy_16 >> 16);
		srcx_16 = 0;
		for (i = 0; i < dstBytesPerLine; i+=channels)
		{
			pt = &p[(srcx_16>>16)*channels];


			for (m = 0; m < channels; m++) {
				q[i+m] = linearInterpolation(&pt[m], &pt[m] + srcBytesPerLine, channels, (srcx_16 & 0xFFFF) >> 8, v_8);
			}


			srcx_16 += stepw;
		}
		srcy_16 += steph;
	
		q += dstBytesPerLine;
	}
}



uint8_t *vfx_linebuf = NULL;
uint8_t vga_room[2][640*480+640*480/2]__attribute__ ((aligned(4),section(".psram.src")));
extern void vf_open();
extern void vf_close();
extern void soft_to_jpg_cfg(uint32 w,uint32 h);
void page_rec_analyze(lv_event_t * e){
	struct vpp_device *vpp_dev;
	struct scale_device *scale_dev;

	struct dma_device *dma1_dev;
	uint32_t retval = 0;
	dma1_dev = (struct dma_device *)dev_get(HG_M2MDMA_DEVID); 
	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);

	if(e->target == start_rec_btn){
		if(rec_open == 0){
			printf("rec start\r\n");
			os_printf("===photo out0h:%d w:%d rec_h:%d rec_w:%d===\n",photo_msg.out0_h,photo_msg.out0_w,gui_cfg.rec_h,gui_cfg.rec_w);
			
			photo_msg.out0_h = gui_cfg.rec_h; 
			photo_msg.out0_w = gui_cfg.rec_w;

			if(vfx_open){
				os_printf("vfx_open..................................\r\n");
				vpp_request_irq(vpp_dev,ITP_DONE_ISR,(vpp_irq_hdl )&vpp_itp_vfx_done,0);
				soft_to_jpg_cfg(640,480);
				if(vfx_linebuf == NULL){
					vfx_linebuf = os_malloc(16*640+8*640);
				}
				scale_close(scale_dev);

				dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_LOCK, 0, 0); 
				while(retval != 1){
					os_sleep_ms(100);
					retval = dma_ioctl(dma1_dev, DMA_IOCTL_CMD_CHECK_DMA1_STATUS, 0, 0);
				}
				vfx_pingpang = 0;
				vpp_put_psram((uint32)vga_room,640,480);
				jpg_cfg(HG_JPG0_DEVID,SOFT_DATA);

			}else{
				if(photo_msg.out0_w > 640){
					scale_from_vpp_to_jpg(scale_dev,(uint32)yuvbuf,photo_msg.in_w,photo_msg.in_h,photo_msg.out0_w,photo_msg.out0_h);
					jpg_cfg(HG_JPG0_DEVID,SCALER_DATA);
				}else{
					jpg_cfg(HG_JPG0_DEVID,VPP_DATA0);
				}
					
			}
			
			#if 0
			jpg_start(HG_JPG0_DEVID);
			#else
			start_record_thread(30,8);
			#endif

			rec_open = 1;
			dv_flash(0);
			lv_time_add(&time_msg);
			lv_time_reset(&time_msg);
			display_time(page1_obj,&time_msg);
		}else{
			if(vfx_linebuf){
				vpp_set_itp_enable(vpp_dev,0);
				os_printf("vfx_open2..................................\r\n");
			}
		    scale_close(scale_dev);
			#if 0
			jpeg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID); 
			jpg_close(jpeg_dev);
			printf("rec close\r\n");
			#else
			send_stop_record_cmd();
			#endif
			os_sleep_ms(50);
			if(vfx_linebuf){
				dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_UNLOCK, 0, 0);
				os_free(vfx_linebuf);
				vfx_linebuf = NULL;				
				os_printf("vfx_open3..................................\r\n");
			}			
			rec_open = 0;
			dv_flash(1);
			del_time_label();
		}
	}
	
	if(e->target == rec_back_btn){
		send_stop_record_cmd();
		lv_page_select(0);
		page_num = 0;
	}
	
	if(e->target == rec_next_btn){
		page_num++;
		if(page_num == 9)
			page_num = 1;
		send_stop_record_cmd();
		lv_page_select(page_num);
	}			
	
	if(e->target == uvc_on_btn){	
		uvc_start ^= BIT(0);
		//lcdc_set_p0p1_enable(lcd_dev,1,uvc_start);
		if(uvc_start){
			lcd_info.lcd_p0p1_state = 3;
		}else{
			lcd_info.lcd_p0p1_state = 1;
		}
		if(uvc_start){
			extern uint8_t  enable_video_usb_to_lcd(uint8_t enable);
			lv_label_set_text(label_uvc_on, "uvc_off");
			uvc_open = 1;
			enable_video_usb_to_lcd(1);
		}
		else{
			extern uint8_t  enable_video_usb_to_lcd(uint8_t enable);
			lv_label_set_text(label_uvc_on, "uvc_on");				
			uvc_open = 0;		
			enable_video_usb_to_lcd(0);	
		}
		
	}
	
	if(e->target == larger_btn){
		enlarge_glo++;
		if(enlarge_glo == 21){
			enlarge_glo = 10;
		}
		gui_cfg.enlarge_lcd = enlarge_glo;			
	}

	if(e->target == rahmen_btn){
		if(rahmen_open == 0){
			rahmen_open = 1;
			vpp_set_ifp_en(vpp_dev,1);
		}else{
			rahmen_open = 0;
			vpp_set_ifp_en(vpp_dev,0);
		}		
	}

	char str[8] = {0};
	if(e->target == haha_btn){
		if(vfx_open == 0){
			vfx_open = 1;
			vf_open();
		}else if(vfx_open == 1){
			vfx_open = 0;
			vf_close();
		}
	}

	if(e->target == vf_sel_btn){
		g_vf_cblk.desp.ve_sel++;
		g_vf_cblk.desp.ve_sel = g_vf_cblk.desp.ve_sel % 6;
		if(g_vf_cblk.desp.ve_sel){
			sprintf(str,"VF%d", g_vf_cblk.desp.ve_sel);
		} else {
			sprintf(str,"VFX");
		}
		lv_label_set_text(vf_sel_label, str);
		// if(vfx_open == 0){
		// 	vfx_open = 1;
		// 	vf_open();
		// }else if(vfx_open == 1){
		// 	vfx_open = 0;
		// 	vf_close();
		// }
	}


}

volatile uint8  printer_action = 0;
extern volatile uint8_t itp_finish;
extern uint32_t get_takephoto_thread_status();
void page_takephoto_analyze(lv_event_t * e){
	struct vpp_device *vpp_dev;
	struct scale_device *scale_dev;
	struct dma_device *dma1_dev;
	uint32_t retval = 0;
	dma1_dev = (struct dma_device *)dev_get(HG_M2MDMA_DEVID); 
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	
	if(e->target == photo_back_btn){
		lv_page_select(0);
		page_num = 0;
	}
	
	if(e->target == take_photo_btn){
		printf("gui_cfg.take_photo_num:%d\r\n",gui_cfg.take_photo_num);
		if(!get_takephoto_thread_status())
		{
			take_photo_thread_init(gui_cfg.photo_w,gui_cfg.photo_h,gui_cfg.take_photo_num);
		}
		else
		{
			os_printf("%s err,get_takephoto_thread_status:%d\n",__FUNCTION__,get_takephoto_thread_status());
		}
	}
	
	if(e->target == photo_next_btn){
		page_num++;
		if(page_num == 9)
			page_num = 1;

		printf("take photo next(%d)...................................\r\n",page_num);
		lv_page_select(page_num);
	}

	if(e->target == rahmen_btn){
		if(rahmen_open == 0){
			rahmen_open = 1;
			vpp_set_ifp_en(vpp_dev,1);
		}else{
			rahmen_open = 0;
			vpp_set_ifp_en(vpp_dev,0);
		}		
	}

	if(e->target == printer_btn){
		vpp_request_irq(vpp_dev,ITP_DONE_ISR,(vpp_irq_hdl )&vpp_itp_done,0);
		scale_close(scale_dev);
		
		dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_LOCK, 0, 0); 
		while(retval != 1){
			os_sleep_ms(100);
			retval = dma_ioctl(dma1_dev, DMA_IOCTL_CMD_CHECK_DMA1_STATUS, 0, 0);
		}
		itp_finish = 0;
		vpp_put_psram((uint32)vga_room,640,480);
		while(itp_finish == 0){
			os_sleep_ms(1);
		}
		os_printf("itp   photo get....\r\n");
		dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_UNLOCK, 0, 0);
		vpp_set_itp_enable(vpp_dev,0);
		
		switch (g_vf_cblk.desp.ve_sel)
		{
			case 1:
				block_9((uint8*)vga_room, 0, 640, 480);
				break;
			case 2:
				block_4((uint8*)vga_room, 0, 640, 480);
				break;
			case 3:
				block_2_yinv((uint8*)vga_room, 0, 640, 480);
				break;
			case 4:
				block_2_xinv((uint8*)vga_room, 0, 640, 480);
				break;
			case 5 :
				hexagon_ve1((uint8*)vga_room, 0, 640, 480);
				break;
			case 6:
				uv_offset((uint8*)vga_room, 0, 640, 480, 30, 30);
				break;
			default:
				break;
		}
		
		vga_to_paper_size(vga_room[0],vga_room[1]);
		os_printf("deal end....\r\n");
		printer_action = 1;
	}

}

void page_wifi_analyze(lv_event_t * e){
	if(e->target == wifi_back_btn){
		lv_page_select(0);
		page_num = 0;
	}
	
	if(e->target == wifi_next_btn){
		page_num++;
		if(page_num == 9)
			page_num = 1;
		lv_page_select(page_num);
	}
}

void page_setting_analyze(lv_event_t * e){
	if(e->target == setting_back_btn){
		lv_page_select(0);
		page_num = 0;
	}
	
	if(e->target == setting_next_btn){
		page_num++;
		if(page_num == 9)
			page_num = 1;
		lv_page_select(page_num);
	}

}
  
void page_playback_photo_analyze(lv_event_t * e){
	uint8_t name[16];
	
	if(e->target == next_photo_btn){
		jpeg_file_get(name,0,"JPEG");
		sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",name);
		printf("name_rec_photo:%s\r\n",name_rec_photo);
		jpeg_photo_explain(name_rec_photo,320,240);
	}			
	
	if(e->target == playback_photo_back_btn){
		lv_page_select(0);
		page_num = 0;
	}
	
	if(e->target == playback_photo_next_btn){
		page_num++;
		if(page_num == 9)
			page_num = 1;
		lv_page_select(page_num);
	}

	if(e->target == playback_photo_printer_btn){
		os_printf("playback photo print start.....\r\n");
		NVIC_DisableIRQ(LCD_IRQn);
		jpg_dec_scale_del();
		set_lcd_photo1_config(640,480,0);
		jpg_decode_scale_config((uint32)vga_room);
		jpeg_photo_explain(name_rec_photo,640,480);
		vga_to_paper_size(vga_room[0],vga_room[1]);

		jpg_dec_scale_del();
		set_lcd_photo1_config(320,240,0);
		jpg_decode_scale_config((uint32)video_decode_mem);
		jpeg_photo_explain(name_rec_photo,320,240);
		NVIC_EnableIRQ(LCD_IRQn);
		os_printf("playback photo print...\r\n");
		printer_action = 1;
	}

}

void page_playback_rec_analyze(lv_event_t * e){
	uint8_t name[16];
	if(e->target == next_rec_btn){
		if(global_avi_exit){
			os_printf("replay\r\n\r\n");
			rec_playback_thread_init(name_rec_photo);
			os_sleep_ms(10);
		}
		global_avi_running ^= BIT(0);
	}			

	if(e->target == playback_rec_back_btn){
		global_avi_exit = 1;
		lv_page_select(0);
		page_num = 0;
	}
	
	if(e->target == playback_rec_next_btn){
		global_avi_exit = 1;
		page_num++;
		if(page_num == 9)
			page_num = 1;
		lv_page_select(page_num);
	}
	
	if(e->target == setting_option_btn[NEXT_REC_STR]){
		global_avi_exit = 1;
		os_sleep_ms(10);
		jpeg_file_get(name,0,"AVI");
		sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",name);
		rec_playback_thread_init(name_rec_photo);				
	}
}


void page_game_analyze(lv_event_t * e){
	
	if(e->target == game_start_btn){
		printf("play game...\r\n");

		game_list();
	}			
	
	if(e->target == game_back_btn){
		lv_page_select(0);
		page_num = 0;
	}
	
	if(e->target == game_next_btn){
		page_num++;
		if(page_num == 9)
			page_num = 1;
		lv_page_select(page_num);
	}

}

void page_spi_sensor_analyze(lv_event_t * e){
	struct lcdc_device *lcd_dev;
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
	
	if(e->target == spi_back_btn){
		spi_video_run = 0;
		lcdc_set_rotate_mirror(lcd_dev,0,LCD_ROTATE_90);
#if LCD_33_WVGA
		set_lcd_photo1_config(848,480,0);
#else	
		set_lcd_photo1_config(320,240,0);
#endif		
		lv_page_select(0);
		page_num = 0;
	}
	
	if(e->target == spi_next_btn){
		spi_video_run = 0;
		lcdc_set_rotate_mirror(lcd_dev,0,LCD_ROTATE_90);
		set_lcd_photo1_config(320,240,0);		
		page_num++;
		if(page_num == 9)
			page_num = 1;
		lv_page_select(page_num);
	}

} 

extern uint8_t  enable_video_usb_to_lcd(uint8_t enable);
//如果打开了uvc,也要停止
static void uvc_release(lv_event_t * e)
{
	if(uvc_start)
	{
		uvc_start ^= BIT(0);
		uvc_open = 0;		
		enable_video_usb_to_lcd(0);	

	}
}

//如果是录像,则要停止
static void rec_release(lv_event_t * e)
{
	os_printf("%s start\n",__FUNCTION__);
	if(rec_open)
	{
		struct scale_device *scale_dev;
		scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
		scale_close(scale_dev);
		rec_open = 0;
		send_stop_record_cmd();		
		del_time_label();
	}
}

static void event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	struct lcdc_device *lcd_dev;
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	if(code == LV_EVENT_KEY){
		if(page_cur == 0){
			main_menu_key_analyze(e);
		}
	}

	
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		if(page_cur == 0){
			page0_analyze(e);
		}else if(page_cur == 1){
			page_rec_analyze(e);
		}else if(page_cur == 2){
			page_takephoto_analyze(e);
		}else if(page_cur == 3){
			page_wifi_analyze(e);
		}else if(page_cur == 4){
			page_playback_photo_analyze(e);
		}else if(page_cur == 5){
			page_playback_rec_analyze(e);
		}else if(page_cur == 6){
			page_setting_analyze(e);
		}else if(page_cur == 7){
			page_game_analyze(e);
		}else if(page_cur == 8){
			page_spi_sensor_analyze(e);
		}
		
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

void lv_page_usb_config();
uint32_t hgusb_get_devtype();
void timer_event(){
	static uint8_t rec_num = 1;
	static uint32 timer_count = 0;
	static uint32 usb_tick = 0;
	static uint8_t usb_mode = 0;
	lv_obj_t *btn = NULL;
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);	
	if((timer_count%2) == 0){
		if(page_cur == 1){
			if(rec_open){
				lv_time_add(&time_msg);
				display_time(page1_obj,&time_msg);	
				dv_flash(rec_num&BIT(0));
				rec_num++;
			}
		}
	}

	if(page_cur == 0){
		if(group0_golop != NULL){
			btn = lv_group_get_focused(group0_golop);	
			if((timer_count%2)== 0){
				lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, &menu_null, &menu_null, &menu_null);	
			}
			else{
				lv_imgbtn_set_src(record_btn, LV_IMGBTN_STATE_RELEASED, &menu1_0, &menu1_1, &menu1_2);	
				lv_imgbtn_set_src(photo_btn, LV_IMGBTN_STATE_RELEASED, &menu2_0, &menu2_1, &menu2_2);	
				lv_imgbtn_set_src(wifi_btn, LV_IMGBTN_STATE_RELEASED, &menu3_0, &menu3_1, &menu3_2);	
				lv_imgbtn_set_src(setting_btn, LV_IMGBTN_STATE_RELEASED, &menu4_0, &menu4_1, &menu4_2);	
				lv_imgbtn_set_src(playback_photo_btn, LV_IMGBTN_STATE_RELEASED, &menu5_0, &menu5_1, &menu5_2);	
				lv_imgbtn_set_src(playback_rec_btn, LV_IMGBTN_STATE_RELEASED, &menu6_0, &menu6_1, &menu6_2);	
				lv_imgbtn_set_src(game_btn, LV_IMGBTN_STATE_RELEASED, &menu_test, &menu_test, &menu_test);	
				lv_imgbtn_set_src(spi_btn, LV_IMGBTN_STATE_RELEASED, &menu_spi, &menu_spi, &menu_spi);
			}
		}
	}
	if((timer_count%4) == 0){
		if(scsi_count != usb_tick){
			usb_tick = scsi_count;
			if(usb_mode == 0){
				lcd_info.lcd_p0p1_state = 2;
				lcdc_set_video_en(lcd_dev,0);
				if(hgusb_get_devtype() == 1){
					vpp_close(vpp_dev);
				}


				video_decode_mem  = video_psram_mem;
				video_decode_mem1 = video_psram_mem;
				video_decode_mem2 = video_psram_mem;
				jpg_dec_scale_del();
				set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
				jpg_decode_scale_config((uint32)video_decode_mem);
				memcpy(video_psram_mem1,menu_usb,sizeof(menu_usb));
				jpg_decode_to_lcd((uint32)video_psram_mem1,848,480,SCALE_WIDTH,SCALE_HIGH);
				lcdc_set_video_en(lcd_dev,1);
				page_num = 15;

				//lv_scr_load_anim(page15_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 0, 0, false);
				lv_page_usb_config();
				usb_mode = 1;
			}
		}else{
			if(usb_mode == 1){
				usb_mode = 0;
				lv_page_select(0);
			}
		}
	}

	timer_count++;
}

void lv_time_set(){
	static uint32_t user_data = 10;
	lv_timer_create(timer_event, 500,  &user_data);
}


static void fatfs_img_gc(lv_event_t * e)
{
	lv_img_dsc_t *gc = (lv_img_dsc_t *)lv_event_get_user_data(e);
	os_printf("%s gc:%X\n",__FUNCTION__,gc);
	if(gc)
	{
		fs_lv_img_free(gc);
	}
}

void lv_page_main_menu_config(){	
	static lv_style_t style5;

	group0_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group0_golop);
	group_cur = group0_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page0_obj = lv_obj_create(lv_scr_act());//lv_obj_create(lv_scr_act());	
	cur_obj = page0_obj;
	lv_obj_set_size(page0_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page0_obj, &style5, 0);
	lv_obj_set_style_text_font(page0_obj, &myfont, 0);

	lv_obj_t * btn1 = lv_imgbtn_create(page0_obj);
	record_btn = btn1;		
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_imgbtn_set_src(btn1, LV_IMGBTN_STATE_RELEASED, &menu1_0, &menu1_1, &menu1_2);
	lv_imgbtn_set_src(btn1, LV_IMGBTN_STATE_PRESSED, &menu1_0, &menu1_1, &menu1_2);
	lv_obj_align(btn1, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_set_size(btn1,60,60);
	//label = lv_label_create(btn1);
	//lv_label_set_text(label, language_switch[language_cur][RECORD_STR]);
	//lv_obj_center(label);
	lv_group_add_obj(group0_golop, btn1);
	
	lv_obj_t * btn3 = lv_imgbtn_create(page0_obj);
	photo_btn = btn3;		
	lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, NULL);
	lv_imgbtn_set_src(btn3, LV_IMGBTN_STATE_RELEASED, &menu2_0, &menu2_1, &menu2_2);
	lv_imgbtn_set_src(btn3, LV_IMGBTN_STATE_PRESSED, &menu2_0, &menu2_1, &menu2_2);
	lv_obj_align(btn3, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_size(btn3,60,60);
	lv_group_add_obj(group0_golop, btn3);
	
	lv_obj_t * btn4 = lv_imgbtn_create(page0_obj);
	wifi_btn = btn4;		
	lv_obj_add_event_cb(btn4, event_handler, LV_EVENT_ALL, NULL);
	lv_imgbtn_set_src(btn4, LV_IMGBTN_STATE_RELEASED, &menu3_0, &menu3_1, &menu3_2);
	lv_imgbtn_set_src(btn4, LV_IMGBTN_STATE_PRESSED, &menu3_0, &menu3_1, &menu3_2);
	lv_obj_align(btn4, LV_ALIGN_TOP_RIGHT, 0, 0);
	lv_obj_set_size(btn4,60,60);
	lv_group_add_obj(group0_golop, btn4);
	

	
	lv_obj_t * btn5 = lv_imgbtn_create(page0_obj);
	playback_photo_btn = btn5;	
	lv_obj_add_event_cb(btn5, event_handler, LV_EVENT_ALL, NULL);
	lv_imgbtn_set_src(btn5, LV_IMGBTN_STATE_RELEASED, &menu5_0, &menu5_1, &menu5_2);
	lv_imgbtn_set_src(btn5, LV_IMGBTN_STATE_PRESSED, &menu5_0, &menu5_1, &menu5_2);
	lv_obj_align(btn5, LV_ALIGN_TOP_LEFT, 0, 70);
	//lv_obj_align(btn5, LV_ALIGN_TOP_MID, 0, 80);
	lv_obj_set_size(btn5,60,60);
	lv_group_add_obj(group0_golop, btn5);
	
	lv_obj_t * btn6 = lv_imgbtn_create(page0_obj);
	playback_rec_btn = btn6;	
	lv_obj_add_event_cb(btn6, event_handler, LV_EVENT_ALL, NULL);
	lv_imgbtn_set_src(btn6, LV_IMGBTN_STATE_RELEASED, &menu6_0, &menu6_1, &menu6_2);
	lv_imgbtn_set_src(btn6, LV_IMGBTN_STATE_PRESSED, &menu6_0, &menu6_1, &menu6_2);
	lv_obj_align(btn6, LV_ALIGN_TOP_MID, 0, 70);
//	lv_obj_align(btn6, LV_ALIGN_TOP_RIGHT, 0, 80);
	lv_obj_set_size(btn6,60,60);
	lv_group_add_obj(group0_golop, btn6);

	lv_obj_t * btn2 = lv_imgbtn_create(page0_obj);
	setting_btn = btn2; 	
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_imgbtn_set_src(btn2, LV_IMGBTN_STATE_RELEASED, &menu4_0, &menu4_1, &menu4_2);
	lv_imgbtn_set_src(btn2, LV_IMGBTN_STATE_PRESSED, &menu4_0, &menu4_1, &menu4_2);
	//lv_obj_align(btn2, LV_ALIGN_TOP_LEFT, 0, 80);
	lv_obj_align(btn2, LV_ALIGN_TOP_RIGHT, 0, 70);
	lv_obj_set_size(btn2,60,60);
	lv_group_add_obj(group0_golop, btn2);


	
	lv_obj_t * btn7 = lv_imgbtn_create(page0_obj);
	game_btn = btn7;	
	lv_obj_add_event_cb(btn7, event_handler, LV_EVENT_ALL, NULL);
	lv_imgbtn_set_src(btn7, LV_IMGBTN_STATE_RELEASED, &menu_test, &menu_test, &menu_test);
	lv_imgbtn_set_src(btn7, LV_IMGBTN_STATE_PRESSED, &menu_test, &menu_test, &menu_test);
	lv_obj_align(btn7, LV_ALIGN_TOP_LEFT, 0, 140);
	lv_obj_set_size(btn7,60,60);
	lv_group_add_obj(group0_golop, btn7);



	lv_obj_t * btn8 = lv_imgbtn_create(page0_obj);
	spi_btn = btn8;	
	lv_obj_add_event_cb(btn8, event_handler, LV_EVENT_ALL, NULL);
	lv_imgbtn_set_src(btn8, LV_IMGBTN_STATE_RELEASED, &menu_spi, &menu_spi, &menu_spi);
	lv_imgbtn_set_src(btn8, LV_IMGBTN_STATE_PRESSED, &menu_spi, &menu_spi, &menu_spi);
	lv_obj_align(btn8, LV_ALIGN_TOP_MID, 0, 140);
	lv_obj_set_size(btn8,60,60);
	lv_group_add_obj(group0_golop, btn8);
	
	//使用文件系统的图标在lvgl上显示,这里是简单介绍使用方法
	
	lv_img_dsc_t *img;
	img = get_fs_lv_img_lzo("FLASH:4.lv");
	if(img)
	{
		lv_obj_t * bat_img = lv_img_create(page0_obj);
		lv_img_set_src(bat_img, img);
		lv_obj_align(bat_img, LV_ALIGN_TOP_RIGHT, 0, 0);
		lv_obj_set_size(bat_img, img->header.w, img->header.h);
		//释放资源文件
		lv_obj_add_event_cb(bat_img, fatfs_img_gc, LV_EVENT_DELETE, img);
	}

}

void lv_page_rec_config(){
	static lv_style_t style2;
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;
	static lv_style_t style6;
	static lv_style_t style7;
    lv_obj_t * label;
	lv_obj_t * img;
	lv_obj_t * img2;

	group1_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group1_golop);
	group_cur = group1_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page1_obj = lv_obj_create(lv_scr_act());//lv_obj_create(lv_scr_act());//
	cur_obj = page1_obj;
	lv_obj_set_size(page1_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page1_obj, &style5, 0);
	lv_obj_set_style_text_font(page1_obj, &myfont, 0);
	
	img = lv_img_create(page1_obj);
	lv_img_set_src(img,&rec);
	
	img2 = lv_img_create(page1_obj);
	label_rec = img2;
	lv_img_set_src(img2,&DV_W);
	lv_obj_set_pos(img2, SCALE_WIDTH-80, 0);
	
	
	lv_obj_t * btn1 = lv_btn_create(page1_obj);
	start_rec_btn = btn1;
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(btn1, rec_release, LV_EVENT_DELETE, NULL);

	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -100);
	lv_style_reset(&style2);
	lv_style_init(&style2);
	lv_style_set_bg_color(&style2, lv_color_make(0x0, 0x80, 0x00)); 
	lv_style_set_shadow_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_border_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_outline_color(&style2, lv_color_make(0xff, 0xff, 0xff));	
	lv_obj_add_style(btn1, &style2, 0);
	
	lv_group_add_obj(group1_golop, btn1);
	
	label = lv_label_create(btn1);
	lv_label_set_text(label, (const char *)language_switch[language_cur][RECORD_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn2 = lv_btn_create(page1_obj);
	rec_next_btn = btn2;
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, -55);	//参数三四为对齐后采取的偏移
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);
	
	lv_group_add_obj(group1_golop, btn2);
	lv_style_reset(&style3);
	lv_style_init(&style3);
	lv_style_set_radius(&style3,0); //圆边
	lv_style_set_bg_color(&style3, lv_color_make(0xc0, 0xe0, 0x12));	
	lv_style_set_shadow_color(&style3, lv_color_make(0xff, 0xff, 0xff));	
	//lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value)
	lv_obj_add_style(btn2, &style3, 0);
	label = lv_label_create(btn2);
	
	lv_label_set_text(label, (const char *)language_switch[language_cur][NEXT_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn3 = lv_btn_create(page1_obj);
	rec_back_btn = btn3;
	lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn3);
	lv_label_set_text(label, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn3, LV_ALIGN_CENTER, 0, -10);	  
	lv_obj_add_flag(btn3, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn3,80,40);
	lv_style_reset(&style4);
	lv_style_init(&style4);
	lv_style_set_radius(&style4,0); //圆边
	lv_style_set_bg_color(&style4, lv_color_make(0xb0, 0x50, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style4, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn3, &style4, 0);
	
	lv_group_add_obj(group1_golop, btn3);
	
	lv_obj_t * btn4 = lv_btn_create(page1_obj);
	uvc_on_btn = btn4;
	lv_obj_add_event_cb(btn4, event_handler, LV_EVENT_ALL, NULL);
	//设置当按键被删除的时候,要释放的资源
	lv_obj_add_event_cb(btn4, uvc_release, LV_EVENT_DELETE, NULL);
	label = lv_label_create(btn4);
	label_uvc_on = label;
	lv_label_set_text(label, "uvc_on");
	lv_obj_center(label);
	lv_obj_align(btn4, LV_ALIGN_CENTER, 0, 35);   
	lv_obj_add_flag(btn4, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn4,80,40);
	lv_style_reset(&style4);
	lv_style_init(&style4);
	lv_style_set_radius(&style4,0); //圆边
	lv_style_set_bg_color(&style4, lv_color_make(0xb0, 0x50, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style4, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn4, &style4, 0);
	lv_group_add_obj(group1_golop, btn4);
	
	
	lv_obj_t * btn5 = lv_btn_create(page1_obj);
	larger_btn = btn5;
	lv_obj_add_event_cb(btn5, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn5);
	lv_label_set_text(label, (const char *)language_switch[language_cur][LARGER_STR]);
	lv_obj_center(label);
	lv_obj_align(btn5, LV_ALIGN_CENTER, 0, 80);	  
	lv_obj_add_flag(btn5, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn5,80,40);
	lv_style_reset(&style6);
	lv_style_init(&style6);
	lv_style_set_radius(&style6,0); //圆边
	lv_style_set_bg_color(&style6, lv_color_make(0x70, 0x90, 0x22));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style6, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn5, &style6, 0);
	lv_group_add_obj(group1_golop, btn5);		


	lv_obj_t * btn6 = lv_btn_create(page1_obj);
	rahmen_btn = btn6;
	lv_obj_add_event_cb(btn6, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn6);
	lv_label_set_text(label, (const char *)language_switch[language_cur][RAHMEN_STR]);
	lv_obj_center(label);
	lv_obj_align(btn6, LV_ALIGN_CENTER, 90, -10);	  
	lv_obj_add_flag(btn6, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn6,80,40);
	lv_style_reset(&style7);
	lv_style_init(&style7);
	lv_style_set_radius(&style7,0); //圆边
	lv_style_set_bg_color(&style7, lv_color_make(0x70, 0x90, 0x22));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style7, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn6, &style7, 0);
	lv_group_add_obj(group1_golop, btn6);		
	

	lv_obj_t * btn7 = lv_btn_create(page1_obj);
	haha_btn = btn7;
	lv_obj_add_event_cb(btn7, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn7);
	// lv_label_set_text(label, (const char *)language_switch[language_cur][VFX_STR]);
	lv_label_set_text(label, "VFSW");
	lv_obj_center(label);
	lv_obj_align(btn7, LV_ALIGN_CENTER, 90, 35);	  
	lv_obj_add_flag(btn7, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn7,80,40);
	lv_style_reset(&style7);
	lv_style_init(&style7);
	lv_style_set_radius(&style7,0); //圆边
	lv_style_set_bg_color(&style7, lv_color_make(0x70, 0x90, 0x22));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style7, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn7, &style7, 0);
	lv_group_add_obj(group1_golop, btn7);

	lv_obj_t * btn8 = lv_btn_create(page1_obj);
	vf_sel_btn = btn8;
	lv_obj_add_event_cb(btn8, event_handler, LV_EVENT_ALL, NULL);
	vf_sel_label = lv_label_create(btn8);
	lv_label_set_text(vf_sel_label, "VFX");
	lv_obj_center(vf_sel_label);
	lv_obj_align(btn8, LV_ALIGN_CENTER, 90, 80);	  
	lv_obj_add_flag(btn8, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn8,80,40);
	lv_style_reset(&style7);
	lv_style_init(&style7);
	lv_style_set_radius(&style7,0); //圆边
	lv_style_set_bg_color(&style7, lv_color_make(0x70, 0x44, 0x22));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style7, lv_color_make(0x70, 0x44, 0x22));
	lv_obj_add_style(btn8, &style7, 0);
	lv_group_add_obj(group1_golop, btn8);

}

void lv_page_photo_config(){
	static lv_style_t style2;
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;
	static lv_style_t style6;
	lv_obj_t * label;
	lv_obj_t * img;

	group2_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group2_golop);
	group_cur = group2_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page2_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page2_obj;
	lv_obj_set_size(page2_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page2_obj, &style5, 0);
	lv_obj_set_style_text_font(page2_obj, &myfont, 0);
	img = lv_img_create(page2_obj);
	lv_img_set_src(img,&photo); 
	
	lv_obj_t * btn1 = lv_btn_create(page2_obj);
	take_photo_btn = btn1;
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -60);
	lv_style_reset(&style2);
	lv_style_init(&style2);
	lv_style_set_bg_color(&style2, lv_color_make(0x0, 0x80, 0x00)); 
	lv_style_set_shadow_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_border_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_outline_color(&style2, lv_color_make(0xff, 0xff, 0xff));	
	lv_obj_add_style(btn1, &style2, 0);
	
	lv_group_add_obj(group2_golop, btn1);
	
	label = lv_label_create(btn1);
	lv_label_set_text(label, (const char *)language_switch[language_cur][TAKEPHOTO_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn2 = lv_btn_create(page2_obj);
	photo_next_btn = btn2;
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 0);	//参数三四为对齐后采取的偏移
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);
	
	lv_group_add_obj(group2_golop, btn2);
	lv_style_reset(&style3);
	lv_style_init(&style3);
	lv_style_set_radius(&style3,0); //圆边
	lv_style_set_bg_color(&style3, lv_color_make(0xc0, 0xe0, 0x12));	
	lv_style_set_shadow_color(&style3, lv_color_make(0xff, 0xff, 0xff));	
	//lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value)
	lv_obj_add_style(btn2, &style3, 0);
	
	label = lv_label_create(btn2);
	
	lv_label_set_text(label, (const char *)language_switch[language_cur][NEXT_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn3 = lv_btn_create(page2_obj);
	photo_back_btn = btn3;
	lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn3);
	lv_label_set_text(label, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn3, LV_ALIGN_CENTER, 0, 60);   
	lv_obj_add_flag(btn3, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn3,80,40);
	lv_style_reset(&style4);
	lv_style_init(&style4);
	lv_style_set_radius(&style4,0); //圆边
	lv_style_set_bg_color(&style4, lv_color_make(0xb0, 0x50, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style4, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn3, &style4, 0);
	lv_group_add_obj(group2_golop, btn3);


	lv_obj_t * btn4 = lv_btn_create(page2_obj);
	rahmen_btn = btn4;
	lv_obj_add_event_cb(btn4, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn4);
	lv_label_set_text(label, (const char *)language_switch[language_cur][RAHMEN_STR]);
	lv_obj_center(label);
	lv_obj_align(btn4, LV_ALIGN_CENTER, 90, -10);	  
	lv_obj_add_flag(btn4, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn4,80,40);
	lv_style_reset(&style6);
	lv_style_init(&style6);
	lv_style_set_radius(&style6,0); //圆边
	lv_style_set_bg_color(&style6, lv_color_make(0x70, 0x90, 0x22));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style6, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn4, &style6, 0);
	lv_group_add_obj(group2_golop, btn4);	
			
	lv_obj_t * btn5 = lv_btn_create(page2_obj);
	printer_btn = btn5;
	lv_obj_add_event_cb(btn5, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn5);
	lv_label_set_text(label, (const char *)language_switch[language_cur][PRT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn5, LV_ALIGN_CENTER, 90, 60);	  
	lv_obj_add_flag(btn5, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn5,80,40);
	lv_style_reset(&style6);
	lv_style_init(&style6);
	lv_style_set_radius(&style6,0); //圆边
	lv_style_set_bg_color(&style6, lv_color_make(0x70, 0x90, 0x22));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style6, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn5, &style6, 0);
	lv_group_add_obj(group2_golop, btn5);

}

void lv_page_wifi_config(){
	static lv_style_t style2;
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;

	lv_obj_t * label;

	group3_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group3_golop);
	group_cur = group3_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page3_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page3_obj;
	lv_obj_set_size(page3_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page3_obj, &style5, 0);
	lv_obj_set_style_text_font(page3_obj, &myfont, 0);
	
	lv_obj_t * btn1 = lv_btn_create(page3_obj);
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -60);
	lv_style_reset(&style2);
	lv_style_init(&style2);
	lv_style_set_bg_color(&style2, lv_color_make(0x0, 0x80, 0x00)); 
	lv_style_set_shadow_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_border_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_outline_color(&style2, lv_color_make(0xff, 0xff, 0xff));	
	lv_obj_add_style(btn1, &style2, 0);
	
	lv_group_add_obj(group3_golop, btn1);
	
	label = lv_label_create(btn1);
	lv_label_set_text(label, (const char *)language_switch[language_cur][WIFI_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn2 = lv_btn_create(page3_obj);
	wifi_next_btn = btn2;
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 0);	//参数三四为对齐后采取的偏移
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);
	
	lv_group_add_obj(group3_golop, btn2);
	lv_style_reset(&style3);
	lv_style_init(&style3);
	lv_style_set_radius(&style3,0); //圆边
	lv_style_set_bg_color(&style3, lv_color_make(0xc0, 0xe0, 0x12));	
	lv_style_set_shadow_color(&style3, lv_color_make(0xff, 0xff, 0xff));	
	//lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value)
	lv_obj_add_style(btn2, &style3, 0);
	
	label = lv_label_create(btn2);
	
	lv_label_set_text(label, (const char *)language_switch[language_cur][NEXT_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn3 = lv_btn_create(page3_obj);
	wifi_back_btn = btn3;
	lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn3);
	lv_label_set_text(label, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn3, LV_ALIGN_CENTER, 0, 60);   
	lv_obj_add_flag(btn3, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn3,80,40);
	lv_style_reset(&style4);
	lv_style_init(&style4);
	lv_style_set_radius(&style4,0); //圆边
	lv_style_set_bg_color(&style4, lv_color_make(0xb0, 0x50, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style4, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn3, &style4, 0);
	
	lv_group_add_obj(group3_golop, btn3);		

}

void lv_page_setting_config(){
	static lv_style_t style5;
	lv_obj_t *main_list;
	group4_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group4_golop);
	group_cur = group4_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page4_obj = lv_obj_create(lv_scr_act());//NULL
	cur_obj = page4_obj;
	lv_obj_set_size(page4_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page4_obj, &style5, 0);
	lv_obj_set_style_text_font(page4_obj, &myfont, 0);
	
	main_list = lv_list_create(page4_obj);
	lv_obj_add_flag(main_list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(main_list, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_size(main_list, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_center(main_list);		
	lv_obj_t * btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][SOUND_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[SOUND_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][ISO_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[ISO_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][RECORD_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[RECORD_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][TAKEPHOTO_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[TAKEPHOTO_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][FORMAT_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[FORMAT_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][CYCLE_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[CYCLE_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][BATTERY_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[BATTERY_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][LANGUAGE_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[LANGUAGE_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);

	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][CONTINUOUS_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[CONTINUOUS_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);

	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][USBDEV_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[USBDEV_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_add_event_cb(btn1, setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[EXIT_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);

#if MP3_EN
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)"mp3");
	lv_obj_add_event_cb(btn1, MP3_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group4_golop, btn1);
#endif
#if AMR_EN
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)"amr");
	lv_obj_add_event_cb(btn1, amr_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group4_golop, btn1);
#endif

//网络配对
#if NET_PAIR
	{
		lv_obj_t *net_pair_switch = lv_obj_create(main_list);
		//
		lv_obj_remove_style_all(net_pair_switch);
		lv_obj_set_flex_flow(net_pair_switch, LV_FLEX_FLOW_ROW);
		lv_obj_set_size(net_pair_switch, LV_PCT(100), LV_PCT(100));

		lv_obj_t* net_pair_label = lv_label_create(net_pair_switch);
		lv_label_set_text(net_pair_label, (const char *)"pair");
		lv_obj_set_flex_grow(net_pair_label, 4);
		//lv_obj_center(net_pair_label);
		//lv_obj_set_flex_grow(net_pair_switch, 1);
		btn1 = lv_switch_create(net_pair_switch);
		lv_obj_set_flex_grow(btn1, 1);
		if(get_net_pair_status())
		{
			lv_obj_add_state(btn1, LV_STATE_CHECKED);
		}

		//lv_obj_align(btn1, LV_ALIGN_OUT_TOP_RIGHT, 0, 0);
		//lv_obj_set_size(btn1, lv_pct(80), lv_pct(80));

		//创建检查网络配对状态
		lv_timer_t * t = lv_timer_create(net_pair_timerevent_cb, 100,  btn1);
		lv_obj_add_event_cb(btn1, net_pair_switch_handler, LV_EVENT_CLICKED, t);
		//移除timer事件
		lv_obj_add_event_cb(btn1, net_pair_remove_timer_event, LV_EVENT_DELETE, t);

		lv_group_add_obj(group4_golop, btn1);
	}
#endif
	
#if LV_USE_FREETYPE
	//extern uint8_t font_size_test;
	lv_font_t *font = lv_freetype_font_create("FLASH:3.ttf",16,LV_FREETYPE_FONT_STYLE_NORMAL);
	os_printf("font:%X\tmain_list:%X\n",font,main_list);
	if(font)
	{
		btn1 = lv_list_add_btn(main_list, NULL, (const char *)"测试");
		lv_obj_set_style_text_font(main_list, font, 0);
		lv_obj_add_event_cb(btn1, ttf_test_handler, LV_EVENT_ALL, font);
		//删除字体
		lv_obj_add_event_cb(main_list, ttf_font_del_handler, LV_EVENT_DELETE, font);
		lv_group_add_obj(group4_golop, btn1);
	}
#endif

}

void lv_page_playback_photo_config(){
	static lv_style_t style2;
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;
	static lv_style_t style6;

	lv_obj_t * label;

	group5_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group5_golop);
	group_cur = group5_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	//cur_obj = lv_obj_create(lv_scr_act());
	page5_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page5_obj;
	lv_obj_set_size(page5_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page5_obj, &style5, 0);
	lv_obj_set_style_text_font(page5_obj, &myfont, 0);
	
	
	lv_obj_t * btn1 = lv_btn_create(page5_obj);
	next_photo_btn = btn1;
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -90);
	lv_style_reset(&style2);
	lv_style_init(&style2);
	lv_style_set_bg_color(&style2, lv_color_make(0x0, 0x80, 0x00)); 
	lv_style_set_shadow_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_border_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_outline_color(&style2, lv_color_make(0xff, 0xff, 0xff));	
	lv_obj_add_style(btn1, &style2, 0);
	
	lv_group_add_obj(group5_golop, btn1);
	
	label = lv_label_create(btn1);
	lv_label_set_text(label, (const char *)language_switch[language_cur][PLAYBACK_PHOTO_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn2 = lv_btn_create(page5_obj);
	playback_photo_next_btn = btn2;
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, -30);	//参数三四为对齐后采取的偏移
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);
	
	lv_group_add_obj(group5_golop, btn2);
	lv_style_reset(&style3);
	lv_style_init(&style3);
	lv_style_set_radius(&style3,0); //圆边
	lv_style_set_bg_color(&style3, lv_color_make(0xc0, 0xe0, 0x12));	
	lv_style_set_shadow_color(&style3, lv_color_make(0xff, 0xff, 0xff));	
	//lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value)
	lv_obj_add_style(btn2, &style3, 0);
	
	label = lv_label_create(btn2);
	
	lv_label_set_text(label, (const char *)language_switch[language_cur][NEXT_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn3 = lv_btn_create(page5_obj);
	playback_photo_back_btn = btn3;
	lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn3);
	lv_label_set_text(label, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn3, LV_ALIGN_CENTER, 0, 30);   
	lv_obj_add_flag(btn3, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn3,80,40);
	lv_style_reset(&style4);
	lv_style_init(&style4);
	lv_style_set_radius(&style4,0); //圆边
	lv_style_set_bg_color(&style4, lv_color_make(0xb0, 0x50, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style4, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn3, &style4, 0);
	
	lv_group_add_obj(group5_golop, btn3);

	lv_obj_t * btn4 = lv_btn_create(page5_obj);
	playback_photo_printer_btn = btn4;
	lv_obj_add_event_cb(btn4, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn4);
	lv_label_set_text(label, (const char*)language_switch[language_cur][PRT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn4, LV_ALIGN_CENTER, 0, 80);
	lv_obj_add_flag(btn4, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn4, 80, 40);
	lv_style_reset(&style6);
	lv_style_init(&style6);
	lv_style_set_radius(&style6,0);
	lv_style_set_bg_color(&style6, lv_color_make(0x70, 0x90, 0x22));
	lv_style_set_shadow_color(&style6, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn4, &style6, 0);
	lv_group_add_obj(group5_golop, btn4);


}


void lv_page_playback_rec_config(){
	static lv_style_t style2;
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;
	static lv_style_t style6;
	lv_obj_t * label;
	group6_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group6_golop);
	group_cur = group6_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	//page6_obj = lv_obj_create(lv_scr_act());
	page6_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page6_obj;
	lv_obj_set_size(page6_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page6_obj, &style5, 0);
	lv_obj_set_style_text_font(page6_obj, &myfont, 0);
	
	
	lv_obj_t * btn1 = lv_btn_create(page6_obj);
	next_rec_btn = btn1;
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -100);
	lv_style_reset(&style2);
	lv_style_init(&style2);
	lv_style_set_bg_color(&style2, lv_color_make(0x0, 0x80, 0x00)); 
	lv_style_set_shadow_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_border_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_outline_color(&style2, lv_color_make(0xff, 0xff, 0xff));	
	lv_obj_add_style(btn1, &style2, 0);
	
	lv_group_add_obj(group6_golop, btn1);
	
	label = lv_label_create(btn1);
	lv_label_set_text(label, (const char *)language_switch[language_cur][PLAY_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn2 = lv_btn_create(page6_obj);
	playback_rec_next_btn = btn2;
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, -40);	//参数三四为对齐后采取的偏移
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);
	
	lv_group_add_obj(group6_golop, btn2);
	lv_style_reset(&style3);
	lv_style_init(&style3);
	lv_style_set_radius(&style3,0); //圆边
	lv_style_set_bg_color(&style3, lv_color_make(0xc0, 0xe0, 0x12));	
	lv_style_set_shadow_color(&style3, lv_color_make(0xff, 0xff, 0xff));	
	//lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value)
	lv_obj_add_style(btn2, &style3, 0);
	
	label = lv_label_create(btn2);
	
	lv_label_set_text(label, (const char *)language_switch[language_cur][NEXT_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn3 = lv_btn_create(page6_obj);
	playback_rec_back_btn = btn3;
	lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn3);
	lv_label_set_text(label, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn3, LV_ALIGN_CENTER, 0, 20);   
	lv_obj_add_flag(btn3, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn3,80,40);
	lv_style_reset(&style4);
	lv_style_init(&style4);
	lv_style_set_radius(&style4,0); //圆边
	lv_style_set_bg_color(&style4, lv_color_make(0xb0, 0x50, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style4, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn3, &style4, 0);
	
	lv_group_add_obj(group6_golop, btn3);		
	
	lv_obj_t * btn4 = lv_btn_create(page6_obj);
	setting_option_btn[NEXT_REC_STR] = btn4;
	lv_obj_add_event_cb(btn4, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn4);
	lv_label_set_text(label, (const char *)language_switch[language_cur][NEXT_REC_STR]);
	lv_obj_center(label);
	lv_obj_align(btn4, LV_ALIGN_CENTER, 0, 80);	  
	lv_obj_add_flag(btn4, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn4,80,40);
	lv_style_reset(&style6);
	lv_style_init(&style6);
	lv_style_set_radius(&style6,0); //圆边
	lv_style_set_bg_color(&style6, lv_color_make(0x70, 0x10, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style6, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn4, &style6, 0);
	
	lv_group_add_obj(group6_golop, btn4);		
	


}

void lv_page_usb_config(){	
	static lv_style_t style5;	
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page15_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page15_obj;
	lv_obj_set_size(page15_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page15_obj, &style5, 0);
}

void lv_page_game_config(){
	static lv_style_t style2;
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;
	
	lv_obj_t * label;

	group7_golop = lv_group_create();
	group_cur = group7_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page7_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page7_obj;
	lv_obj_set_size(page7_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page7_obj, &style5, 0);
	lv_obj_set_style_text_font(page7_obj, &myfont, 0);
	
	lv_obj_t * btn1 = lv_btn_create(page7_obj);
	game_start_btn = btn1;
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -60);
	lv_style_reset(&style2);
	lv_style_init(&style2);
	lv_style_set_bg_color(&style2, lv_color_make(0x0, 0x80, 0x00)); 
	lv_style_set_shadow_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_border_color(&style2, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_outline_color(&style2, lv_color_make(0xff, 0xff, 0xff));	
	lv_obj_add_style(btn1, &style2, 0);
	
	lv_group_add_obj(group7_golop, btn1);
	
	label = lv_label_create(btn1);
	lv_label_set_text(label, (const char *)language_switch[language_cur][GAME_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn2 = lv_btn_create(page7_obj);
	game_back_btn = btn2;
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 0);	//参数三四为对齐后采取的偏移
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);
	
	lv_group_add_obj(group7_golop, btn2);
	lv_style_reset(&style3);
	lv_style_init(&style3);
	lv_style_set_radius(&style3,0); //圆边
	lv_style_set_bg_color(&style3, lv_color_make(0xc0, 0xe0, 0x12));	
	lv_style_set_shadow_color(&style3, lv_color_make(0xff, 0xff, 0xff));	
	//lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value)
	lv_obj_add_style(btn2, &style3, 0);
	
	label = lv_label_create(btn2);
	
	lv_label_set_text(label, (const char *)language_switch[language_cur][NEXT_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn3 = lv_btn_create(page7_obj);
	game_next_btn = btn3;
	lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn3);
	lv_label_set_text(label, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn3, LV_ALIGN_CENTER, 0, 60);   
	lv_obj_add_flag(btn3, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn3,80,40);
	lv_style_reset(&style4);
	lv_style_init(&style4);
	lv_style_set_radius(&style4,0); //圆边
	lv_style_set_bg_color(&style4, lv_color_make(0xb0, 0x50, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style4, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn3, &style4, 0);
	
	lv_group_add_obj(group7_golop, btn3);

}


void lv_page_spi_config(){
	
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;
	
	lv_obj_t * label;

	group9_golop = lv_group_create();
	group_cur = group9_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page9_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page9_obj;
	lv_obj_set_size(page9_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page9_obj, &style5, 0);
	lv_obj_set_style_text_font(page9_obj, &myfont, 0);
	
	lv_obj_t * btn2 = lv_btn_create(page9_obj);
	spi_next_btn = btn2;
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 0);	//参数三四为对齐后采取的偏移
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);
	
	lv_group_add_obj(group9_golop, btn2);
	lv_style_reset(&style3);
	lv_style_init(&style3);
	lv_style_set_radius(&style3,0); //圆边
	lv_style_set_bg_color(&style3, lv_color_make(0xc0, 0xe0, 0x12));	
	lv_style_set_shadow_color(&style3, lv_color_make(0xff, 0xff, 0xff));	
	//lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value)
	lv_obj_add_style(btn2, &style3, 0);
	
	label = lv_label_create(btn2);
	
	lv_label_set_text(label, (const char *)language_switch[language_cur][NEXT_STR]);
	lv_obj_center(label);
	
	lv_obj_t * btn3 = lv_btn_create(page9_obj);
	spi_back_btn = btn3;
	lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, NULL);
	label = lv_label_create(btn3);
	lv_label_set_text(label, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_center(label);
	lv_obj_align(btn3, LV_ALIGN_CENTER, 0, 60);   
	lv_obj_add_flag(btn3, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_size(btn3,80,40);
	lv_style_reset(&style4);
	lv_style_init(&style4);
	lv_style_set_radius(&style4,0); //圆边
	lv_style_set_bg_color(&style4, lv_color_make(0xb0, 0x50, 0xc2));	
	//lv_style_set_shadow_width(&style4, 0);
	lv_style_set_shadow_color(&style4, lv_color_make(0xff, 0xff, 0xff));
	lv_obj_add_style(btn3, &style4, 0);
	
	lv_group_add_obj(group9_golop, btn3);		

}



int  play_wav_control(int control);
static void play_wav_handler(lv_event_t * e)
{

	uint8_t key_code;
	key_code = lv_indev_get_key(lv_indev_get_act()); 
	printf("button:%s\n",lv_list_get_btn_text(NULL,lv_event_get_target(e)));
	if(key_code == LV_KEY_RIGHT)
	{
		//播放暂停或者播放
		//play_wav_control2(5,lv_list_get_btn_text(NULL,lv_event_get_target(e)));
	}
	//播放或者停止
	else if(key_code == LV_KEY_LEFT)
	{
		//play_wav_control2(6,lv_list_get_btn_text(NULL,lv_event_get_target(e)));
	}
	else if(key_code == LV_KEY_END)
	{
		lv_page_select(0);
	}

}


void lv_page_wav_config()
{
	static lv_style_t style5;

	group8_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group8_golop);
	group_cur = group8_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page8_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page8_obj;
	lv_obj_set_size(page8_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page8_obj, &style5, 0);

	lv_obj_t *list1;
	list1 = lv_list_create(page8_obj);
	lv_obj_set_size(list1, LV_PCT(100), LV_PCT(100));
    lv_obj_t * btn,*list_text;

    list_text = lv_list_add_text(list1, "pause");


    btn = lv_list_add_btn(list1, NULL, "1.wav");
	lv_group_add_obj(group8_golop,btn);
	lv_obj_add_event_cb(btn, play_wav_handler, LV_EVENT_KEY, NULL);

    btn = lv_list_add_btn(list1, NULL, "2.wav");
	lv_group_add_obj(group8_golop,btn);
	lv_obj_add_event_cb(btn, play_wav_handler, LV_EVENT_KEY, NULL);
	



}

extern struct os_semaphore  vfx_sema;

extern void hexagon_ve(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void hexagon_ve1(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void block_9(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void block_4(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void block_2_yinv(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void block_2_xinv(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void uv_offset(uint8* in, uint8* tmp, uint32 w, uint32 h, int32 uoff, int32 voff);
extern void reduce_and_reflash(int s_w,int s_h,char *sbuf,int mode_type,int buf_type);
extern void soft_to_jpg_run(uint32 yuvbuf,uint32 linebuf,uint32 w,uint32 h);
extern volatile uint32 ve_sel;
extern volatile uint32 fr_down;
void vfx_run(){
	uint32 time_old;
	uint8* src;
	os_printf("vfx_run\r\n***********************");
	while(1){
		os_sema_down(&vfx_sema, osWaitForever);	
		if(vfx_linebuf == NULL) {
			os_printf("vfx_linebuf == NULL\r\n");
			continue;
		}
			
		time_old = os_jiffies();
		if((vfx_pingpang%2) == 1){
			src = (uint8_t*)&vga_room[0];
			vpp_put_psram((uint32_t)vga_room[1],640,480);
		}else{
			src = (uint8_t*)&vga_room[1];
			vpp_put_psram((uint32_t)vga_room[0],640,480);
		}
//		continue;
		if(fr_down) {
			fr_down--;
			continue;
		}
		switch(ve_sel)
		{
			case 1:
				block_9(src, 0, 640, 480);
				break;
			case 2:
				block_4(src, 0, 640, 480);
				break;
			case 3:
				block_2_yinv(src, 0, 640, 480);
				break;
			case 4:
				block_2_xinv(src, 0, 640, 480);
				break;
			case 5 :
				reduce_and_reflash(160,120,(char *)src,6,1); 	   //640*480	   ====> 160 * 120																		
				reduce_and_reflash(80,60,(char *)(src+640*480),6,1);
				reduce_and_reflash(80,60,(char *)(src+640*480+640*480/4),6,1);
				break;
			case 6:
				uv_offset(src, 0, 640, 480, 30, 30);
				break;
			default:
			break;
		}
		// _os_printf("[tick1 : %d]\r\n",os_jiffies() - time_old);
		soft_to_jpg_run((uint32)src,(uint32)vfx_linebuf,640,480);
		time_old = os_jiffies() - time_old;
		// _os_printf("[tick2 : %d]\r\n", time_old);
		if(time_old > 45) {
			fr_down += 2;
		}
		//os_sleep_ms(2);
	}
}




void lv_page_init(){
	uvc_open = 0;		
	enable_video_usb_to_lcd(0);	
	
	os_sema_init(&vfx_sema, 0);


	os_task_create("vfx_thread", vfx_run, (void*)NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);

}

extern const unsigned char test_mode[7830];

extern volatile uint8 scale2_finish;
void lv_page_select(uint8_t page)
{
	uint8_t name[16];

	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	struct jpg_device *jpg_dev;
	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);	
	page_cur = page;
	printf("page_cur:%d\r\n",page);
	if(page == 0){
		lcd_info.lcd_p0p1_state = 2;
		lcdc_set_video_en(lcd_dev,0);
		vpp_close(vpp_dev);
#if 0
		video_decode_mem  = vga_room;
		video_decode_mem1 = vga_room;
		video_decode_mem2 = vga_room;
		scale2_finish = 0;
		jpg_dec_scale_del();
		set_lcd_photo1_config(640,480,0);
		jpg_decode_scale_config((uint32)video_decode_mem);
		memcpy(video_psram_mem1,main_demo,sizeof(main_demo));
		jpg_decode_to_lcd((uint32)video_psram_mem1,960,480,640,480);
		while(scale2_finish == 0){
			;
		}
		printf("scale2_finish.....................................................................................\r\n");
#endif
		video_decode_mem  = video_psram_mem;
		video_decode_mem1 = video_psram_mem;
		video_decode_mem2 = video_psram_mem;
		lcdc_set_rotate_mirror(lcd_dev,0,LCD_ROTATE_90);
		jpg_dec_scale_del();
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
		jpg_decode_scale_config((uint32)video_decode_mem);
		memcpy(video_decode_config_mem,main_demo,sizeof(main_demo));
		jpg_decode_to_lcd((uint32)video_decode_config_mem,960,480,SCALE_WIDTH,SCALE_HIGH);
		
		lcdc_set_video_en(lcd_dev,1);
		page_num = 0;
		
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_main_menu_config();		
		//lv_scr_load_anim(page0_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
		lv_indev_set_group(indev_keypad, group0_golop);
	}else if(page == 1){
		lcdc_set_video_en(lcd_dev,0);
		video_decode_mem  = video_decode_config_mem;
		video_decode_mem1 = video_decode_config_mem1;
		video_decode_mem2 = video_decode_config_mem2;
		//video_decode_mem  = video_psram_mem;
		//video_decode_mem1 = video_psram_mem1;
		//video_decode_mem2 = video_psram_mem2;		
		jpg_dec_scale_del();
		set_lcd_photo1_config(160,120,0);
		jpg_decode_scale_config((uint32)video_decode_mem);
		rahmen_open = 0;
		vpp_set_ifp_en(vpp_dev,0);		
		vpp_open(vpp_dev);
		lcd_info.lcd_p0p1_state = 1;
		os_sleep_ms(200);
		
		lcdc_set_video_en(lcd_dev,1);
		uvc_start = 0;
		rec_open  = 0;
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_rec_config();			


		stream* src;
		resolution_parm priv = {
			.dvp_h = gui_cfg.dvp_h,
			.dvp_w = gui_cfg.dvp_w,
			.rec_h = gui_cfg.rec_h,
			.rec_w = gui_cfg.rec_w,
		};
		photo_msg.out0_h = gui_cfg.rec_h;
		photo_msg.out0_w = gui_cfg.rec_w;
		src = jpeg_stream_init();
		//发命令,内部去修改分辨率
		stream_self_cmd_func(src,SET_MJPEG_RESOLUTION_PARM_CMD,(uint32_t)&priv);
		jpeg_stream_deinit();
		os_sleep_ms(50);	

		//lv_scr_load_anim(page1_obj, LV_SCR_LOAD_ANIM_OUT_LEFT, 1000, 0, false);
		lv_indev_set_group(indev_keypad, group1_golop);
		
	}else if(page == 2){
		lcdc_set_video_en(lcd_dev,0);
		rahmen_open = 0;
		vpp_set_ifp_en(vpp_dev,0);
		vpp_open(vpp_dev);
		jpg_dec_scale_del();
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
		jpg_decode_scale_config((uint32)video_decode_mem);
		jpg_start(1);
		lcd_info.lcd_p0p1_state = 1;
		os_printf("lcd_p0p1_cur %d\r\n", lcd_info.lcd_p0p1_cur);
		os_sleep_ms(200);
		lcdc_set_video_en(lcd_dev,1);

		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_photo_config();		
		//lv_scr_load_anim(page2_obj, LV_SCR_LOAD_ANIM_MOVE_TOP, 1000, 0, false);
		lv_indev_set_group(indev_keypad, group2_golop);
	}else if(page == 3){
		lcd_info.lcd_p0p1_state = 2;
		lcdc_set_video_en(lcd_dev,0);
		
		vpp_close(vpp_dev);
		video_decode_mem  = video_psram_mem;
		video_decode_mem1 = video_psram_mem;
		video_decode_mem2 = video_psram_mem;
		jpg_dec_scale_del();
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
		jpg_decode_scale_config((uint32)video_decode_mem);
		memcpy(video_psram_mem1,menu_wifi,sizeof(menu_wifi));
		jpg_decode_to_lcd((uint32)video_psram_mem1,848,480,SCALE_WIDTH,SCALE_HIGH);
		lcdc_set_video_en(lcd_dev,1);		
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_wifi_config();

		//lv_scr_load_anim(page3_obj, LV_SCR_LOAD_ANIM_FADE_OUT, 1000, 0, false);
		lv_indev_set_group(indev_keypad, group3_golop);	
	}else if(page == 4){
		lcdc_set_video_en(lcd_dev,0);
		video_decode_mem  = video_decode_config_mem;
		video_decode_mem1 = video_decode_config_mem;
		video_decode_mem2 = video_decode_config_mem;

		jpeg_file_get(name,1,"JPEG");
		sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",name);
		printf("name_rec_photo:%s\r\n",name_rec_photo);

		jpg_dec_scale_del();
		set_lcd_photo1_config(320,240,0);
		jpg_decode_scale_config((uint32)video_decode_mem);
		jpeg_photo_explain(name_rec_photo,320,240);

		lcd_info.lcd_p0p1_state = 2;
		lcdc_set_video_en(lcd_dev,1);
		vpp_close(vpp_dev);
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_playback_photo_config();
		//lv_scr_load_anim(page5_obj, LV_SCR_LOAD_ANIM_OVER_LEFT, 1000, 0, false);		
		lv_indev_set_group(indev_keypad, group5_golop);	
	}else if(page == 5){
		lcdc_set_video_en(lcd_dev,0);
		video_decode_mem  = video_decode_config_mem;
		video_decode_mem1 = video_decode_config_mem1;
		video_decode_mem2 = video_decode_config_mem2;	
		jpg_dec_scale_del();
		set_lcd_photo1_config(320,240,0);
		jpg_decode_scale_config((uint32)video_decode_mem);
		
		
		jpeg_file_get(name,1,"AVI");
		sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",name);
		rec_playback_thread_init(name_rec_photo);
		lcd_info.lcd_p0p1_state = 2;
		lcdc_set_video_en(lcd_dev,1);
		vpp_close(vpp_dev);		
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_playback_rec_config();		
//		lv_scr_load_anim(page6_obj, LV_SCR_LOAD_ANIM_MOVE_LEFT, 1000, 0, false);
		lv_indev_set_group(indev_keypad, group6_golop);		
	}else if(page == 6){
		lcd_info.lcd_p0p1_state = 0;
		lcdc_set_video_en(lcd_dev,1);
		vpp_close(vpp_dev);
		if(list_setting_children){
			lv_obj_del(list_setting_children);
			list_setting_children = NULL;
		}
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}		
		lv_page_setting_config();
		//lv_scr_load_anim(page4_obj, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false);		
		lv_indev_set_group(indev_keypad, group4_golop);		
	}else if(page == 7){
		//lcd_info.lcd_p0p1_state = 0;

		lcd_info.lcd_p0p1_state = 1;		
	

		
		lcdc_set_video_en(lcd_dev,1);
		vpp_close(vpp_dev);
		
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_game_config();
		//lv_scr_load_anim(page7_obj, LV_SCR_LOAD_ANIM_MOVE_TOP, 0, 0, false);
		lv_indev_set_group(indev_keypad, group7_golop);
	}else if(page == 8){
		lcd_info.lcd_p0p1_state = 2;
		//lcdc_set_video_en(lcd_dev,0);
		
		vpp_close(vpp_dev);
		spi_video_run = 1;
		//jpg_dec_scale_del();
		lcdc_set_rotate_mirror(lcd_dev,1,LCD_ROTATE_0);
		set_lcd_photo1_config(240,320,0);
		os_sleep_ms(100);
		lcdc_set_video_en(lcd_dev,1);
	photo_msg.out0_h = 320;
	photo_msg.out0_w = 240;
		jpg_cfg(HG_JPG0_DEVID,PRC_DATA);
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		
		lv_page_spi_config();

		lv_indev_set_group(indev_keypad, group9_golop);	
	}
}

void play_wav_page()
{
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	lcd_info.lcd_p0p1_state = 0;
	lcdc_set_video_en(lcd_dev,1);
	vpp_close(vpp_dev);		
	lv_scr_load_anim(page8_obj, LV_SCR_LOAD_ANIM_MOVE_LEFT, 1000, 0, false);
	lv_indev_set_group(indev_keypad, group8_golop);


}
