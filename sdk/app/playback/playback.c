#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "jpgdef.h"
#include "hal/jpeg.h"
#include "dev/scale/hgscale.h"
#include "osal/semaphore.h"
#include "openDML.h"
#include "osal/mutex.h"
#include "avidemux.h"
#include "avi/play_avi.h"
#include "custom_mem/custom_mem.h"
#include "osal/task.h"

#define WEBFILE_NODE_LEN 1400

extern uint8 psram_ybuf_src[IMAGE_H*IMAGE_W+IMAGE_H*IMAGE_W/2];
uint32_t photo_size[2];
extern FIL fp_jpg;
int global_avi_running = 0;
int global_avi_exit    = 0;
void jpg_decode_to_lcd(uint32 photo,uint32 jpg_w,uint32 jpg_h,uint32 video_w,uint32 video_h);
uint32_t file_mode (const char *mode);
int32 jpg_decode_is_finish();
uint32_t stop_play_avi_stream(stream* s);

void jpg_analyze(uint8_t *buf,uint8_t *size){
	uint32_t itk = 0;
	uint8_t *p;
	
	for(itk = 0; itk < 1024;itk++){
		if(buf[itk] == 0xff){
			if(buf[itk+1] == 0xc0){
				p = size;
				p[0] = buf[itk+6];
				p[1] = buf[itk+5];
				p = &size[4];
				p[0] = buf[itk+8];
				p[1] = buf[itk+7];	
				return;
			}
		}
	}
}


#define MAX(a,b) ( (a)>(b)?a:b )
static uint32_t a2i(char *str)
{
    uint32_t ret = 0;
    uint32_t indx =0;
    while(str[indx] != '\0'){
        if(str[indx] >= '0' && str[indx] <='9'){
            ret = ret*10+str[indx]-'0';
        }
        indx ++;
    }
    return ret;
}

uint8_t arrary_compare(uint8 *src,uint8 *dst,uint8 len){
	uint8 itk;
	for(itk = 0; itk < len;itk++){
		if(src[itk] != dst[itk]){
			return 0;
		}
	}
	return 1;
}
 
void jpeg_file_get(uint8* photo_name,uint32 reset,char* file)
{
	static uint32 num_cal = 0;
	char name[6];
	FRESULT ret;
	DIR  avi_dir;
	uint32 len;
	FILINFO finfo;
	
	uint32 cal = 0;
	uint32 file_num = 0;
	len = strlen(file);

	if(reset == 1){
		num_cal = 0;
	}
 
	ret = f_opendir(&avi_dir,"0:/DCIM");

	if(ret!=FR_OK)
	{
		return;
	}
	strcpy(name,file);
    while(1){
        ret = f_readdir(&avi_dir,&finfo);
        if(ret != FR_OK || finfo.fname[0] == 0) break;
		if(arrary_compare((uint8 *)finfo.fname,(uint8 *)name,len)){
			cal++;             //先统计有多少个符合的文件
		}
    }
    f_closedir(&avi_dir);

	if(cal == 0){
		strcpy((char *)photo_name,"");
		return;
	}
	file_num = (num_cal % cal);
	ret = f_opendir(&avi_dir,"0:/DCIM");
	cal = 0;
    while(1){
        ret = f_readdir(&avi_dir,&finfo);
		_os_printf("fname2--:%s  %d %d\r\n",finfo.fname,file_num,cal);
        if(ret != FR_OK || finfo.fname[0] == 0) break;
		if(arrary_compare((uint8 *)finfo.fname,(uint8 *)name,len)){
			if(file_num == cal){
				break;
			}
			cal++;
		}
    }
    f_closedir(&avi_dir);	
	
	_os_printf("==================fname:%s\r\n",finfo.fname);
	strcpy((char *)photo_name,(char *)finfo.fname);

	num_cal++;
	
}

void jpeg_photo_explain(uint8* photo_name, uint32 scale_w, uint32 scale_h){

	uint32_t data_count;
	uint32_t read_count = 0;
	uint8_t *data_buf = NULL;
	uint32_t readLen;
	uint32_t count = 0;
	uint32_t data_len;
	void *fp;
	uint8_t *photo_sd_cache = NULL;
	uint32_t photo_sd_cache_size = 8*1024; 
	fp = osal_fopen((const char*)photo_name,"rb");
	if(!fp){
		_os_printf("f_open %s error\r\n",photo_name);
		goto jpeg_photo_explain_end;
	}
	data_len = osal_fsize(fp);
	_os_printf("data_len:%d\r\n",data_len);
	
	//从psram中申请一个空间
	data_buf = (uint8_t*)custom_malloc_psram(data_len);
	if(!data_buf)
	{
		goto jpeg_photo_explain_end;
	}

	while(!photo_sd_cache)
	{
		photo_sd_cache = (uint8_t*)custom_malloc(photo_sd_cache_size);
		if(!photo_sd_cache)
		{
			photo_sd_cache_size >>= 1;
			if(photo_sd_cache_size < 512)
			{
				goto jpeg_photo_explain_end;
			}
		}
	}
	os_printf("photo_sd_cache_size:%d\n",photo_sd_cache_size);
	data_count = 512;
	while(data_len){

		if(data_len > data_count)
		{
			data_count = photo_sd_cache_size;
		}
		else
		{
			data_count = data_len; 
		}
		readLen = osal_fread(photo_sd_cache,1,data_count,fp);
		if(readLen == 0)
		{
			goto jpeg_photo_explain_end;
	}
		memcpy(data_buf+read_count,photo_sd_cache,readLen);
		read_count += readLen;
		data_len -= readLen;		
	}
	osal_fclose(fp);
	fp = NULL;
	jpg_analyze(data_buf,(uint8_t *)photo_size);
	_os_printf("width:%d  high:%d\r\n",photo_size[0],photo_size[1]);
	jpg_decode_to_lcd((uint32)data_buf,photo_size[1],photo_size[0],scale_w,scale_h);
	while(!jpg_decode_is_finish() && count < 1000)
	{
		os_sleep_ms(1);
		count++;
	}
	os_printf("%s count:%d\n",__FUNCTION__,count);

jpeg_photo_explain_end:
	if(fp)
	{
		osal_fclose(fp);
		fp = NULL;
}

	if(data_buf)
	{
		custom_free_psram(data_buf);
		data_buf = NULL;
	}

	if(photo_sd_cache)
	{
		custom_free(photo_sd_cache);
	}

}




uint32_t rec_arg[2];
void rec_playback_thread_init(uint8* rec_name){
	//rec_arg[0] = (w&0xffff) | ((h<<16)&0xffff0000);
	//rec_arg[1] = continuous_spot;
	extern void  lvgl_recv_avi_thread(void *d);
	os_task_create("rec_playback_thread", lvgl_recv_avi_thread, (void*)rec_name, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
}






//使用新框架去实现sd卡回放功能
#include "stream_frame.h"
#include "utlist.h"
static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
			s->priv = NULL;
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		//过滤数据,文件发送过来的数据可能是jpeg也可能是sound
		case STREAM_FILTER_DATA:
		{
				struct data_structure *data = (struct data_structure *)priv;
				//os_printf("type:%d\tdata:%X\n",GET_DATA_TYPE1(data->type),data);
				if(GET_DATA_TYPE1(data->type) == JPEG && GET_DATA_TYPE2(data->type) == JPEG_FILE)
				{
					//正确的图片包
				}
				else
				{
					res = 1;
				}

		}
		break;
		case STREAM_RECV_DATA_FINISH:
		break;

		case STREAM_CLOSE_EXIT:

		break;

		//响应命令,可以进行停止线程(要考虑线程内部的内存被释放)
		case STREAM_SEND_CMD:
		{
			uint32_t cmd = (uint32_t)priv;
			//只是接受支持的命令
			if(GET_CMD_TYPE1(cmd) == CMD_JPEG_FILE)
			{
				s->priv = (void*)GET_CMD_TYPE2(cmd);
			}
			
		}
		break;
	
		default:
			//默认都返回成功
		break;
	}
	return res;
}

extern volatile uint8 scale2_finish ;

#ifdef PSRAM_HEAP
void  lvgl_recv_avi_thread(void *d)
{
	stream *s = NULL;
	stream *play_avi_s = NULL;
	uint32_t photo_size_msg[2] = {0};
	struct data_structure  *get_f = NULL;
	uint32_t node_len ;
	uint32_t count = 0;
	uint32_t *cmd;
	uint8_t pause_flag = 1;


	struct stream_jpeg_data_s *el,*tmp;
	uint8_t *jpeg_buf_addr = NULL;


	char *filepath = (char*)d;
	os_printf("filepath:%s\n",filepath);
 	global_avi_exit = 0;
	global_avi_running = 0;	
	//创建接收视频的流
	s = open_stream_available(R_LVGL_JPEG,0,2,opcode_func,NULL);
	if(!s)
	{
		goto lvgl_recv_avi_end;
	}
	cmd = (uint32_t *)&s->priv;
	play_avi_s = creat_play_avi_stream(S_PLAYBACK,filepath,R_LVGL_JPEG,R_SPEAKER);
	if(!play_avi_s)
	{
		goto lvgl_recv_avi_end;
	}
	open_stream_again(play_avi_s);
	pause_1FPS_avi_stream(play_avi_s);
	extern uint32_t start_play_avi_thread(stream *s);
	//启动回放视频的线程
	start_play_avi_thread(play_avi_s);

	uint32_t start_time = os_jiffies();
	uint32_t last_play_time = 0;
	//循环接收数据
	while(1)
	{
		if(!get_f)
		{
		get_f = recv_real_data(s);
		}
		//视频播放已经停止
		count++;
		if(count%1000 == 0)
		{
			os_printf("@@@@@@@@s->priv:%X\tcmd:%X\n",s->priv,*cmd);
		}
		if((*cmd == PLAY_AVI_CMD_STOP)&&(!get_f))
		{		
			goto lvgl_recv_avi_end;
		}
		if(!get_f)
		{
			os_sleep_ms(1);
			continue;
		}
		else
		{
			//可以播放
			if(os_jiffies()-start_time >= get_stream_data_timestamp(get_f))
			{

			}
			//检查状态,不播放,去检查是否要暂停
			else
			{
				os_sleep_ms(1);
				goto check_playback_status;
			}
		}

		struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)get_stream_real_data(get_f);
		struct stream_jpeg_data_s *dest_list_tmp = dest_list;
		LL_COUNT(dest_list_tmp,el,node_len);
		
		//在psram的情况下,node_len一定是2
		if(node_len == 2)
		{
			LL_FOREACH_SAFE(dest_list,el,tmp)
			{
				if(dest_list_tmp == el)
				{
					continue;
				}
				jpeg_buf_addr = (uint8_t *)GET_JPG_SELF_BUF(get_f,el->data);
				//os_printf("jpeg_buf_addr:%X\n",jpeg_buf_addr);
				
				
				jpg_analyze(jpeg_buf_addr,(uint8_t *)photo_size_msg);
				//os_printf("D");
				last_play_time = get_stream_data_timestamp(get_f);
				jpg_decode_to_lcd((uint32)jpeg_buf_addr,photo_size_msg[1],photo_size_msg[0],320,240);
				count = 0;
				while(!jpg_decode_is_finish() && count < 1000)
				{
					os_sleep_ms(1);
					count++;
				}
				break;
			}
		}

		free_data(get_f);
		get_f = NULL;
		check_playback_status:
		while(!global_avi_running)
		{
			if(!pause_flag)
			{
				pause_avi_stream(play_avi_s);
			}
			pause_flag = 1;
			os_sleep_ms(1);
			if(global_avi_exit)
				break;
		}
		if(pause_flag)
		{
			pause_flag = 0;
			start_time = os_jiffies() - last_play_time;
			play_avi_stream(play_avi_s);
		}
		
		if(global_avi_exit)
			break;		
	}

	
	lvgl_recv_avi_end:
	global_avi_exit = 1;

	if(get_f)
	{
		free_data(get_f);
	}
	if(s)
	{
		close_stream(s);
	}

	if(play_avi_s)
	{
		//自带流删除功能,所以如果调用这个,不需要主动调用close_stream
		os_printf("%s:%d!!!!!!!!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
		stop_avi_stream(play_avi_s);
	}

	os_printf("%s:%d end\n",__FUNCTION__,__LINE__);

	return;
}
#else
void  lvgl_recv_avi_thread(void *d)
{
	stream *s = NULL;
	stream *play_avi_s = NULL;
	uint32_t photo_size_msg[2] = {0};
	struct data_structure  *get_f = NULL;
	uint32_t node_len ;
	uint32_t count = 0;
	uint32_t *cmd;
	uint8_t pause_flag = 1;

	struct stream_jpeg_data_s *el,*tmp;
	uint8_t *jpeg_buf_addr = NULL;


	char *filepath = (char*)d;
	os_printf("filepath:%s\n",filepath);
 	global_avi_exit = 0;
	global_avi_running = 0;	
	//创建接收视频的流
	s = open_stream_available(R_LVGL_JPEG,0,2,opcode_func,NULL);
	if(!s)
	{
		goto lvgl_recv_avi_end;
	}
	cmd = (uint32_t *)&s->priv;
	play_avi_s = creat_play_avi_stream(S_PLAYBACK,filepath,R_LVGL_JPEG,R_SPEAKER);
	if(!play_avi_s)
	{
		goto lvgl_recv_avi_end;
	}
	open_stream_again(play_avi_s);
	pause_1FPS_avi_stream(play_avi_s);
	extern uint32_t start_play_avi_thread(stream *s);
	//启动回放视频的线程
	start_play_avi_thread(play_avi_s);

	uint32_t start_time = os_jiffies();
	uint32_t last_play_time = 0;
	//循环接收数据
	while(1)
	{
		if(!get_f)
		{
		get_f = recv_real_data(s);
		}
		//视频播放已经停止
		count++;
		if(count%1000 == 0)
		{
			os_printf("@@@@@@@@s->priv:%X\tcmd:%X\n",s->priv,*cmd);
		}
		if((*cmd == PLAY_AVI_CMD_STOP)&&(!get_f))
		{		
			goto lvgl_recv_avi_end;
		}
		if(!get_f)
		{
			os_sleep_ms(1);
			continue;
		}
		else
		{
			//可以播放
			if(os_jiffies()-start_time >= get_stream_data_timestamp(get_f))
			{

			}
			//检查状态,不播放,去检查是否要暂停
			else
			{
				os_sleep_ms(1);
				goto check_playback_status;
			}
		}
		struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)get_stream_real_data(get_f);
		struct stream_jpeg_data_s *dest_list_tmp = dest_list;
		LL_COUNT(dest_list_tmp,el,node_len);

		uint8_t node_num = 0;
		int jpg_buf_len = get_stream_real_data_len(get_f);
		jpeg_buf_addr = (uint8_t *)custom_malloc(jpg_buf_len);
		if(!jpeg_buf_addr) {
			os_printf("%s:%d malloc jpg buf err!\n",__FUNCTION__,__LINE__);
		}
		LL_FOREACH_SAFE(dest_list,el,tmp)
		{	
			if(dest_list_tmp == el)
			{
				continue;
			}
			node_num++;
			if(jpg_buf_len >= WEBFILE_NODE_LEN) {
				hw_memcpy(jpeg_buf_addr+(node_num-1)*WEBFILE_NODE_LEN, (uint8_t *)el->data,WEBFILE_NODE_LEN);
				jpg_buf_len -= WEBFILE_NODE_LEN;
			}
			else {
				hw_memcpy(jpeg_buf_addr+(node_num-1)*WEBFILE_NODE_LEN, (uint8_t *)el->data,jpg_buf_len);
				jpg_buf_len = 0;
			}
			
			if(node_num >= (node_len-1))
				break;
		}

		jpg_analyze(jpeg_buf_addr,(uint8_t *)photo_size_msg);
		jpg_decode_to_lcd((uint32)jpeg_buf_addr,photo_size_msg[1],photo_size_msg[0],320,240);
		while(!jpg_decode_is_finish() && count < 1000)
		{
			os_sleep_ms(1);
			count++;
		}
		custom_free(jpeg_buf_addr);
		jpeg_buf_addr = NULL;
		free_data(get_f);
		get_f = NULL;
		check_playback_status:
		while(!global_avi_running)
		{
			if(!pause_flag)
			{
				pause_avi_stream(play_avi_s);
			}
			pause_flag = 1;
			os_sleep_ms(1);
			if(global_avi_exit)
				break;
		}
		if(pause_flag)
		{
			pause_flag = 0;
			start_time = os_jiffies() - last_play_time;
			play_avi_stream(play_avi_s);
		}
		
		if(global_avi_exit)
			break;		
	}	
	lvgl_recv_avi_end:
	global_avi_exit = 1;
	if(get_f)
	{
		free_data(get_f);
	}
	if(s)
	{
		close_stream(s);
	}
	if(play_avi_s)
	{
		//自带流删除功能,所以如果调用这个,不需要主动调用close_stream
		os_printf("%s:%d!!!!!!!!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
		stop_avi_stream(play_avi_s);
	}
	os_printf("%s:%d end\n",__FUNCTION__,__LINE__);

	return;
}

#endif
