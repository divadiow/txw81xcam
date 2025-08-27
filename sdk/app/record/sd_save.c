
#include "sys_config.h"	
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "utlist.h"
#include "stream_frame.h"
#include "jpgdef.h"
#include "fatfs/osal_file.h"
#include "osal/string.h"

#ifdef PSRAM_HEAP
#define SD_TMP_MAX_SIZE	1024

int no_frame_record_video_psram(void *fp,void *d,int flen)
{
    uint8_t *jpeg_buf_addr = NULL;
    void * get_f = (void *)d;
	uint32_t total_len;
	uint32_t write_len;
	uint32_t offset = 0;
	

	struct stream_jpeg_data_s *el,*tmp;

	struct data_structure  *data = (struct data_structure  *)d;
    struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(data);
    struct stream_jpeg_data_s *dest_list_tmp = dest_list;
    total_len = GET_DATA_LEN(data);
	uint32_t uint_len = GET_NODE_LEN(data);
	uint32_t uint_len_tmp = uint_len;

	uint32 jpg_len = total_len;

	int jpg_count = 0;
	

    LL_FOREACH_SAFE(dest_list,el,tmp)
    {
        if(dest_list_tmp == el)
        {
            continue;
        }
        //读取完毕删除
        //图片保存起来
		uint_len_tmp = uint_len;
		jpeg_buf_addr = (uint8_t *)GET_JPG_SELF_BUF(data,el->data);

		offset = 0;
		while(fp && total_len && uint_len_tmp)
        {
			if(SD_TMP_MAX_SIZE <= uint_len_tmp)
			{
				write_len = SD_TMP_MAX_SIZE;
			}
			else
			{
				write_len = uint_len_tmp;
			}

			if(total_len < write_len){
				write_len = total_len;
			}

            osal_fwrite((void*)(jpeg_buf_addr+offset),1,write_len,fp);
  
			uint_len_tmp -= write_len;
			total_len -= write_len;
			offset += write_len;
        }

		DEL_JPEG_NODE(get_f,el);
		jpg_count++;
    }
	return jpg_len;
}
#endif


#if 1//OPENDML_EN &&  SDH_EN && FS_EN
#include "openDML.h"
#include "osal_file.h"
#include "media.h"
#include "osal/sleep.h"
#include "osal/string.h"
#include "jpgdef.h"
#include "osal/task.h"
#include "frame.h"


#include "video_app.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "hal/scale.h"
#include "hal/jpeg.h"
#include "hal/vpp.h"
#include "hal/dma.h"
#include "lib/lcd/lcd.h"



/********************************************************
 * 录像使用
********************************************************/


#define RECORD_DIR "0:/DCIM"

/***************************************************
 * 音频写入的注册函数
 * fp:文件句柄
 * d:音频帧的结构体
 * flen:需要写入的音频数据长度
***************************************************/
int no_frame_record_audio2(void *fp,void *d,int flen)
{
	if(!d)
	{
		return 1;
	}
	struct data_structure *audio_f = (struct data_structure*)d;
	uint8_t *buf = (uint8_t*)GET_DATA_BUF(audio_f);
	osal_fwrite(buf, flen, 1, fp);
	return 0;
}

//获取jpg的第一帧
uint8_t *get_jpeg_buf(struct data_structure *data,uint32_t *len)
{
	struct stream_jpeg_data_s *el,*tmp;
    struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(data);
    struct stream_jpeg_data_s *dest_list_tmp = dest_list;
	*len = GET_NODE_LEN(data);
	uint8_t *jpeg_buf_addr = NULL;

    LL_FOREACH_SAFE(dest_list,el,tmp)
    {
        if(dest_list_tmp == el)
        {
            continue;
        }
        jpeg_buf_addr = (uint8_t *)GET_JPG_SELF_BUF(data,el->data);
    }
	return jpeg_buf_addr;
}
/***************************************************
 * 视频写入的注册函数
 * fp:文件句柄
 * d:视频帧的结构体
 * flen:需要写入的视频数据长度
***************************************************/
int no_frame_record_video2(void *fp,void *d,int flen)
{
    uint8_t *jpeg_buf_addr = NULL;
    void * get_f = (void *)d;
	uint32_t total_len;

	

	struct stream_jpeg_data_s *el,*tmp;

	struct data_structure  *data = (struct data_structure  *)d;
    struct stream_jpeg_data_s *dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(data);
    struct stream_jpeg_data_s *dest_list_tmp = dest_list;
    total_len = GET_DATA_LEN(data);
	uint32_t uint_len = GET_NODE_LEN(data);

	uint32 jpg_len = total_len;

	int jpg_count = 0;

    LL_FOREACH_SAFE(dest_list,el,tmp)
    {
        if(dest_list_tmp == el)
        {
            continue;
        }
        //读取完毕删除
        //图片保存起来

        if(fp && total_len)
        {
            jpeg_buf_addr = (uint8_t *)GET_JPG_SELF_BUF(data,el->data);
            if(total_len >= uint_len)
            {
                osal_fwrite(jpeg_buf_addr,1,uint_len,fp);
                total_len -= uint_len;
            }
            else
            {
                osal_fwrite(jpeg_buf_addr,1,total_len,fp);
                total_len = 0;
            }
        }

		DEL_JPEG_NODE(get_f,el);
		jpg_count++;
    }
	return jpg_len;
}








static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	//_os_printf("%s:%d\topcode:%d\n",__FUNCTION__,__LINE__,opcode);
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;



		default:
			//默认都返回成功
		break;
	}
	return res;
}


extern int new_sd_save_avi2(int *exit_flag,stream *s,stream *audio_s,int fps,int audiofrq,int settime,int pic_w,int pic_h);
struct sd_save_s
{
    int fps;
    int frq;
};

int sd_save_flag = 0;
uint8_t record_thread_status;
struct sd_save_s g_sd_save_msg;

uint8_t get_record_thread_status()
{
    return record_thread_status;
}

uint8_t send_stop_record_cmd()
{
    sd_save_flag = 0;
    return sd_save_flag;
}

void sd_save_thread(void *d)
{
	struct sd_save_s *ctrl = (struct sd_save_s *)d;
	stream *s = NULL;
	stream *audio_s = NULL;
	int err = 0;
	//打开视频
	jpeg_stream_init();

	s = open_stream(R_RECORD_JPEG,0,2,opcode_func,NULL);
	if(!s)
	{
		_os_printf("creat stream fail\n");
		goto sd_save_thread_end;
	}


	//如果需要录音,则打开录音的音频流
	if(ctrl->frq)
	{
		audio_s = open_stream(R_RECORD_AUDIO,0,8,opcode_func,NULL);

		if(!audio_s)
		{
			_os_printf("creat audio_s stream fail\n");
			goto sd_save_thread_end;
		}
	}
	sd_save_flag = 1;
    
    while(sd_save_flag)
    {
		if(s)
		{
			enable_stream(s,1);
		}
		
		if(audio_s)
		{
			enable_stream(audio_s,1);
		}
		
        err = new_sd_save_avi2(&sd_save_flag,s,audio_s,ctrl->fps,ctrl->frq,60*1,photo_msg.out0_w,photo_msg.out0_h);
		if(err == -2)
		{
			
			break;
		}
    }
    sd_save_thread_end:

	jpeg_stream_deinit();

	
	if(s)
	{
		close_stream(s);
	}

	if(audio_s)
	{
		close_stream(audio_s);
	}
	if(err == -2)
	{
		sd_save_flag = 0;
	}
	record_thread_status = 0;
}


void start_record_thread(uint8_t video_fps,uint8_t audio_frq)
{
	if(!record_thread_status)
	{
		int fps = video_fps;
		int frq = audio_frq*1000;
		record_thread_status = 1;
		//线程启动
		g_sd_save_msg.fps = fps;
		g_sd_save_msg.frq = frq;
		os_task_create("sd_save", sd_save_thread, (void*)&g_sd_save_msg, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024); 
	} 
}



//暂时fps只能是30帧,audiofrq如果是0,则是不带音频
int new_sd_save_avi2(int *exit_flag,stream *s,stream *audio_s,int fps,int audiofrq,int settime,int pic_w,int pic_h)
{
	if((*exit_flag) == 0)
	{
		return -1;
	}
	struct data_structure *get_f = NULL;
	struct data_structure *audio_f = NULL;
	void *pavi = NULL;
	int insert = 0;
	float tim_diff = 0;
	int err = -1;
	int count = 0;
	uint8_t *headerbuf = NULL;
	AVI_INFO *odmlmsg = NULL;
	ODMLBUFF *odml_buff = NULL;
	int timeouts = 0;
	
	int flen;

	uint32_t audio_count = 0;
	uint32_t video_count = 0;


	_os_printf("fps:%d\tfrq:%d\tsettime:%d\tpic_w:%d\tpic_h:%d\n",fps,audiofrq,settime,pic_w,pic_h);

	odml_buff = malloc(sizeof(ODMLBUFF));
	odmlmsg = malloc(sizeof(AVI_INFO));
	headerbuf = malloc(_ODML_AVI_HEAD_SIZE__);

	if(!odml_buff || !odmlmsg || !headerbuf)
	{
		_os_printf("creat_odml_buff fail\n");
		os_sleep_ms(1);//等待内存释放
		goto sd_save_avi_end;
	}

	pavi = create_video_file(RECORD_DIR);


	if(!pavi)
	{
		_os_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%s:%d\n",__FUNCTION__,__LINE__);
		err = -2;
		goto sd_save_avi_end;
	}


	ODMLbuff_init(odml_buff);


	odml_buff->ef_time = 1000/fps;
	odml_buff->ef_fps = fps;
	odml_buff->vframecnt = 0;
	odml_buff->aframecnt = 0;
	odml_buff->aframeSample = 0;//这里不赋值,等到第一帧音频帧来了才赋值
	odml_buff->sync_buf = headerbuf;
	odmlmsg->audiofrq = audiofrq;

	if(audiofrq)
	{
		odmlmsg->pcm = 1;
	}
	else
	{
		odmlmsg->pcm = 0;
	}
	odmlmsg->win_w = pic_w;//
	odmlmsg->win_h = pic_h;//

	err = OMDLvideo_header_write(NULL, pavi,odmlmsg,(ODMLAVIFILEHEADER *)headerbuf);
	if(err < 0)
	{
		os_printf("%s opendml write head fail\n",__FUNCTION__);
		goto sd_save_avi_end;
	}
	//进入写卡
	while(1)
	{
		//get_f = (void*)GET_JPG_FRAME();
		get_f = recv_real_data(s);
		//_os_printf("get_f:%X\n",get_f);
		audio_f = recv_real_data(audio_s);
        if(!odml_buff->aframeSample && audio_f)
        {
            odml_buff->aframeSample = get_stream_real_data_len(audio_f);
        }

		if(get_f)
		{
			video_count++;
			timeouts = 0;
			_os_printf("&");
			odml_buff->cur_timestamp = os_jiffies();
			//err = no_framememt_write_opendml(odml_buff,pavi,GET_JPG_LEN(get_f),get_f);
			#ifndef PSRAM_HEAP  
            	err = opendml_write_video2(odml_buff,pavi,no_frame_record_video2,GET_DATA_LEN(get_f),get_f);
			#else
				err = opendml_write_video2(odml_buff,pavi,no_frame_record_video_psram,GET_DATA_LEN(get_f),get_f);
			#endif
			DEL_JPG_FRAME(get_f);
			get_f = NULL;
			if(err < 0)
			{
				os_printf("%s no_framememt_write_opendml fail\n",__FUNCTION__);
				goto sd_save_avi_end;
			}
			if(os_jiffies()-odml_buff->cur_timestamp > 500)
			{
				os_printf("time err:%d\n",os_jiffies()-odml_buff->cur_timestamp);
					
			}
			count ++;	
			insert = insert_frame(odml_buff,pavi,&tim_diff);
			odml_buff->last_timestamp = odml_buff->cur_timestamp ;
			count += insert;

			if((*exit_flag) == 0 || count>30*settime)
			{
				os_printf("exit_flag:%d\n",*exit_flag);
				os_printf("count:%d\tsettime:%d\n",count,settime);
				if(!audio_count)
				{
					odmlmsg->pcm = 0;
				}
				stdindx_updata(pavi,odml_buff);
				ODMLUpdateAVIInfo(pavi,odml_buff,odmlmsg->pcm,NULL,(ODMLAVIFILEHEADER *)headerbuf);
				osal_fclose(pavi);
				pavi = NULL;
				count = 0;
				break;
			}
		}



		//音频写入
		if(audio_f)
		{
			audio_count ++;
			flen = get_stream_real_data_len(audio_f);
			opendml_write_audio(odml_buff,pavi,no_frame_record_audio2,flen,audio_f);
			free_data(audio_f);
			audio_f = NULL;
			_os_printf("^");
		}


		if(!get_f && !audio_f)
		{
			os_sleep_ms(1);
			if(pavi)
			{
				timeouts++;
				if((*exit_flag) == 0  || timeouts>1000)
				{
					if(count > 0)
					{
						if(!audio_count)
						{
							odmlmsg->pcm = 0;
						}
						stdindx_updata(pavi,odml_buff);
						ODMLUpdateAVIInfo(pavi,odml_buff,odmlmsg->pcm,NULL,(ODMLAVIFILEHEADER *)headerbuf);
					}
					osal_fclose(pavi);
					pavi = NULL;

					break;
				}
			}
		}
	}

	sd_save_avi_end:
	os_printf("%s end\n",__FUNCTION__);
	if(odml_buff)
	{
		free(odml_buff);
		odml_buff = NULL;
	}

	if(odmlmsg)
	{
		free(odmlmsg);
		odmlmsg = NULL;
	}

	if(headerbuf)
	{
		free(headerbuf);
		headerbuf = NULL;
	}

	if(get_f)
	{
		DEL_JPG_FRAME(get_f);
	}

	if(audio_f)
	{
		DEL_FRAME(audio_f);
	}

	if(pavi)
	{
		osal_fclose(pavi);
		pavi = NULL;
	}

	if(s)
	{
		enable_stream(s,0);
	}

	if(audio_s)
	{
		enable_stream(audio_s,0);
	}
	return err;
}

extern char video_name[50];
int bbm_sd_save_avi(int *exit_flag,stream *s,stream *audio_s,int fps,int audiofrq,int settime,int pic_w,int pic_h)
{
		if((*exit_flag) == 0)
	{
		return -1;
	}
	struct data_structure *get_f = NULL;
	struct data_structure *audio_f = NULL;
	void *pavi = NULL;
	int insert = 0;
	float tim_diff = 0;
	int err = -1;
	int count = 0;
	int count2 = 0;
	uint8_t *headerbuf = NULL;
	AVI_INFO *odmlmsg = NULL;
	ODMLBUFF *odml_buff = NULL;
	int timeouts = 0;
	
	int flen;

	uint32_t last_time = 0;
	uint32_t cur_time = 0;

	uint32_t video_get_frame_en = 1;
	uint32_t audio_get_frame_en = 0;
	uint32_t start_record_init = 1;

	uint32_t video_first_frame_time = 0;
	uint32_t audio_first_frame_time = 0;
	uint32_t video_now_time = 0;
	uint32_t audio_now_time = 0;

	uint32_t audio_last_timestamp = 0;
	uint32_t audio_cur_timestamp = 0;

	uint32_t audio_count = 0;
	uint32_t video_count = 0;

	_os_printf("fps:%d\tfrq:%d\tsettime:%d\tpic_w:%d\tpic_h:%d\n",fps,audiofrq,settime,pic_w,pic_h);

	odml_buff = malloc(sizeof(ODMLBUFF));
	odmlmsg = malloc(sizeof(AVI_INFO));
	headerbuf = malloc(_ODML_AVI_HEAD_SIZE__);

	if(!odml_buff || !odmlmsg || !headerbuf)
	{
		_os_printf("creat_odml_buff fail\n");
		os_sleep_ms(1);//等待内存释放
		goto sd_save_avi_end;
	}

	pavi = create_video_file(RECORD_DIR);


	if(!pavi)
	{
		_os_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%s:%d\n",__FUNCTION__,__LINE__);
		err = -2;
		goto sd_save_avi_end;
	}


	ODMLbuff_init(odml_buff);


	odml_buff->ef_time = 1000/fps;
	odml_buff->ef_fps = fps;
	odml_buff->vframecnt = 0;
	odml_buff->aframecnt = 0;
	odml_buff->aframeSample = 0;//这里不赋值,等到第一帧音频帧来了才赋值
	odml_buff->sync_buf = headerbuf;
	odmlmsg->audiofrq = audiofrq;

	if(audiofrq)
	{
		odmlmsg->pcm = 1;
		audio_get_frame_en = 1;
	}
	else
	{
		odmlmsg->pcm = 0;
		audio_get_frame_en = 0;
	}
	odmlmsg->win_w = pic_w;//
	odmlmsg->win_h = pic_h;//

	err = OMDLvideo_header_write(NULL, pavi,odmlmsg,(ODMLAVIFILEHEADER *)headerbuf);
	if(err < 0)
	{
		os_printf("%s opendml write head fail\n",__FUNCTION__);
		goto sd_save_avi_end;
	}




	//进入写卡
	while(1)
	{
		if(video_get_frame_en)
			get_f = recv_real_data(s);

		if(audio_get_frame_en)
			audio_f = recv_real_data(audio_s);

		
		if(get_f && video_count == 0)
		{
			video_first_frame_time = get_f->timestamp;
			video_get_frame_en = 0;
		}

		if(audio_f && audio_count == 0)
		{
			audio_first_frame_time = audio_f->timestamp;
			audio_get_frame_en = 0;
		}


		//同时获取到小于30ms的时间差的视频帧和音频帧才开始录卡
		if(start_record_init){
			if(video_first_frame_time == 0 || audio_first_frame_time == 0)
			{
				timeouts++;
				if(timeouts > 1000)
				{
					os_printf("no get video or audio frame, timeout!!!\n");
					osal_fclose(pavi);
					osal_unlink(video_name);
					pavi = NULL;
					goto sd_save_avi_end;
				}
				os_sleep_ms(1);
				continue;
			}else{
				os_printf("video_first_frame_time:%d audio_first_frame_time:%d diff:%d\n",video_first_frame_time,
																				audio_first_frame_time,
																				video_first_frame_time-audio_first_frame_time);
				if(video_first_frame_time >= audio_first_frame_time)
				{
					if(video_first_frame_time - audio_first_frame_time < 30)	//30ms 30帧
					{
						//正常
						video_get_frame_en = 1;
						audio_get_frame_en = 1;
						start_record_init = 0;
						timeouts = 0;
					}else
					{
						//视频帧比音频帧的时间快，先看到画面，再听到声音
						audio_get_frame_en = 1;
						DEL_FRAME(audio_f);
						audio_f = NULL;
						audio_first_frame_time = 0;
						os_sleep_ms(1);
						continue;
					}

				}else if(video_first_frame_time < audio_first_frame_time)
				{
					if(audio_first_frame_time - video_first_frame_time < 30)	//30ms 30帧
					{
						//正常
						video_get_frame_en = 1;
						audio_get_frame_en = 1;
						start_record_init = 0;
						timeouts = 0;
					}else
					{
						//音频帧比视频帧的时间快，先听到声音，再看到画面
						video_get_frame_en = 1;
						DEL_JPG_FRAME(get_f);
						get_f = NULL;
						video_first_frame_time = 0;
						os_sleep_ms(1);
						continue;
					}
				}
			}
		}



        if(!odml_buff->aframeSample && audio_f)
        {
            odml_buff->aframeSample = get_stream_real_data_len(audio_f);
        }

		if(get_f)
		{
			video_now_time = get_f->timestamp;

			video_count++;
			timeouts = 0;
			_os_printf("&");
			// os_printf("v:%d len:%d\n",video_now_time,GET_DATA_LEN(get_f));
			//odml_buff->cur_timestamp = os_jiffies();
			odml_buff->cur_timestamp = get_f->timestamp;
			//err = no_framememt_write_opendml(odml_buff,pavi,GET_JPG_LEN(get_f),get_f);

			last_time = os_jiffies();
			#ifndef PSRAM_HEAP  
				err = opendml_write_video2(odml_buff,pavi,no_frame_record_video2,GET_DATA_LEN(get_f),get_f);
			#else
				err = opendml_write_video2(odml_buff,pavi,no_frame_record_video_psram,GET_DATA_LEN(get_f),get_f);
			#endif
			DEL_JPG_FRAME(get_f);
			get_f = NULL;
			if(err < 0)
			{
				os_printf("%s no_framememt_write_opendml fail\n",__FUNCTION__);
				goto sd_save_avi_end;
			}

			count ++;	
			insert = insert_frame(odml_buff,pavi,&tim_diff);
			odml_buff->last_timestamp = odml_buff->cur_timestamp ;
			count += insert;
			video_count += insert;
			cur_time = os_jiffies();
			// os_printf("video write time:%d\n",cur_time-last_time);
			os_printf("insert %d len:%d\n",insert,GET_DATA_LEN(get_f));

			if((*exit_flag) == 0 || count>30*settime)
			{
				os_printf("exit_flag:%d\n",*exit_flag);
				os_printf("count:%d\tsettime:%d\n",count,settime);
				if(!audio_count)
				{
					odmlmsg->pcm = 0;
				}
				stdindx_updata(pavi,odml_buff);
				ODMLUpdateAVIInfo(pavi,odml_buff,odmlmsg->pcm,NULL,(ODMLAVIFILEHEADER *)headerbuf);
				osal_fclose(pavi);
				pavi = NULL;
				//count = 0;
				video_get_frame_en = 0;
				audio_get_frame_en = 0;
				break;
			}
			
		}



		//音频写入
		if(audio_f)
		{
			last_time = os_jiffies();

			audio_count ++;
			count2 ++;
			flen = get_stream_real_data_len(audio_f);
			audio_cur_timestamp = audio_f->timestamp;
			// os_printf("a:%d len:%d\n",audio_f->timestamp,flen);
			opendml_write_audio(odml_buff,pavi,no_frame_record_audio2,flen,audio_f);
			_os_printf("^");
			// os_printf("audio diff:%d cur_timestamp:%d last_timestamp:%d\n",audio_cur_timestamp - audio_last_timestamp,
			// 																audio_cur_timestamp,
			// 																audio_last_timestamp);
			audio_now_time = audio_f->timestamp;

			free_data(audio_f);
			audio_last_timestamp = audio_cur_timestamp;

			audio_f = NULL;
			cur_time = os_jiffies();
			// os_printf("audio write time:%d\n",cur_time-last_time);

		}


		if(!get_f && !audio_f)
		{
			os_sleep_ms(1);
			if(pavi)
			{
				timeouts++;

				if((*exit_flag) == 0  /*|| timeouts>1000*/)
				{
					if(count > 0)
					{
						if(!audio_count)
						{
							odmlmsg->pcm = 0;
						}
						stdindx_updata(pavi,odml_buff);
						ODMLUpdateAVIInfo(pavi,odml_buff,odmlmsg->pcm,NULL,(ODMLAVIFILEHEADER *)headerbuf);
					}
					osal_fclose(pavi);
					pavi = NULL;
					os_printf("%s %d timeouts:%d\n",__FUNCTION__,__LINE__,timeouts);
					video_get_frame_en = 0;
					audio_get_frame_en = 0;
					break;
				}
			}
		}
	}

	sd_save_avi_end:
	os_printf("now time diff:%d\n",video_now_time - audio_now_time);
	os_printf("=====%s end video_count:%d audio_count:%d\n",__FUNCTION__,count,count2);
	if(odml_buff)
	{
		free(odml_buff);
		odml_buff = NULL;
	}

	if(odmlmsg)
	{
		free(odmlmsg);
		odmlmsg = NULL;
	}

	if(headerbuf)
	{
		free(headerbuf);
		headerbuf = NULL;
	}

	if(get_f)
	{
		DEL_JPG_FRAME(get_f);
	}

	if(audio_f)
	{
		DEL_FRAME(audio_f);
	}

	if(pavi)
	{
		osal_fclose(pavi);
		pavi = NULL;
	}

	if(s)
	{
		enable_stream(s,0);
	}

	if(audio_s)
	{
		enable_stream(audio_s,0);
	}
	return err;
}

#define MIN(a,b) ( (a)>(b)?b:a )
#define MAX(a,b) ( (a)>(b)?a:b )
#define JPG_DIR "0:/DCIM"
#define JPG_FILE_NAME "JPG"

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

static void *creat_takephoto_file(char *dir_name)
{
    DIR  avi_dir;
    FRESULT ret;
    FILINFO finfo;
	uint32_t indx= 0;
	void *fp;
	char filepath[64];
    ret = f_opendir(&avi_dir,dir_name);
    if(ret != FR_OK){
        f_mkdir(dir_name);
        f_opendir(&avi_dir,dir_name);
    }
    while(1){
        ret = f_readdir(&avi_dir,&finfo);
        if(ret != FR_OK || finfo.fname[0] == 0) break;
        indx = MAX(indx,a2i(finfo.fname));
    }
    f_closedir(&avi_dir);    
    indx++;

	sprintf(filepath,"%s/jpeg%d.%s",dir_name,indx,JPG_FILE_NAME);

	fp = osal_fopen(filepath,"wb+");
	return fp;
}

extern Vpp_stream photo_msg;
extern uint8 *yuvbuf;
extern struct os_mutex m2m1_mutex;
extern uint8 psram_ybuf_src[IMAGE_H*IMAGE_W+IMAGE_H*IMAGE_W/2];
extern volatile uint8 scale_take_photo;
void take_photo_thread(void *d)
{
	uint16_t itp_w,itp_h;
	uint8_t  time_out = 0;
	uint8_t  continous_spon;
	struct dma_device *dma1_dev;
	struct scale_device *scale_dev;
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	struct jpg_device *jpeg_dev;
	uint32_t retval = 0;
	uint32* pixel_itp;
	stream *s = NULL;
	//stream *audio_s = NULL;
	//int err = 0;
	uint32 take_count = 0;
	//uint32_t timeouts = 0;
	void *fp;
	struct data_structure *get_f = NULL;
	os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	dma1_dev = (struct dma_device *)dev_get(HG_M2MDMA_DEVID); 
	jpeg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID); 
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	pixel_itp = (uint32*)d;
	itp_w = pixel_itp[0]&0xffff;
	itp_h = (pixel_itp[0]>>16)&0xffff;
	continous_spon = pixel_itp[1];	
	scale_close(scale_dev);
	jpg_close(jpeg_dev);
	dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_LOCK, 0, 0);	
	while(retval != 1){
		os_sleep_ms(100);
		retval = dma_ioctl(dma1_dev, DMA_IOCTL_CMD_CHECK_DMA1_STATUS, 0, 0);
	}
	
	jpg_cfg(HG_JPG0_DEVID,SCALER_DATA);
	//打开视频
	_os_printf("itp_w:%d itp_h:%d   continous_spon:%d\r\n",itp_w,itp_h,continous_spon);
	photo_msg.out0_w = itp_w;
	photo_msg.out0_h = itp_h;

	jpeg_stream_init();
	if(itp_h <= 1080){
		scale_from_vpp_to_jpg(scale_dev,(uint32)yuvbuf,photo_msg.in_w,photo_msg.in_h,itp_w,itp_h);
	}	

	
	s = open_stream_available(R_PHOTO_JPEG,0,1,opcode_func,NULL);
	if(!s)
	{
		os_printf("creat stream fail\n");
	}

	while(1){
		if(itp_h > 1080){
			//if(m2m1_mutex.magic == 0xa8b4c2d5)
			//	os_mutex_lock(&m2m1_mutex,osWaitForever);
			time_out = 0;
			lcdc_video_enable_auto_ks(lcd_dev,1);
			scale_from_soft_to_jpg(scale_dev,(uint32)psram_ybuf_src,photo_msg.in_w,photo_msg.in_h,itp_w,itp_h);
			scale_take_photo = 0;
			while(scale_take_photo == 0){
				os_sleep_ms(2);
				time_out++;
				if(time_out == 500)
					break;			//timeout
			}
			//if(m2m1_mutex.magic == 0xa8b4c2d5)
			//	os_mutex_unlock(&m2m1_mutex);
			
			os_sleep_ms(10);
			lcdc_video_enable_auto_ks(lcd_dev,0);
			vpp_open(vpp_dev);	
		}
		
		get_f = recv_real_data(s);
		if(get_f){
			//进行拍照
			fp = creat_takephoto_file(JPG_DIR);
			take_count++;
			os_printf("fp:%X\tlen:%d\n",fp,get_stream_real_data_len(get_f));
			if(fp)
			{
				no_frame_record_video_psram(fp,get_f,get_stream_real_data_len(get_f));

				osal_fclose(fp);
				free_data(get_f);
				get_f = NULL;
			}
			else
			{
				os_printf("%s:%d\n",__FUNCTION__,__LINE__);
				free_data(get_f);
				get_f = NULL;			
			}

			if(take_count == continous_spon)
				break;
		}

	}
	
	scale_close(scale_dev);

	if(s)
	{
		close_stream(s);
	}

	jpeg_stream_deinit();

	
	dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_UNLOCK, 0, 0);
	//代表线程关闭
	pixel_itp[2] = 0;
}

#endif
