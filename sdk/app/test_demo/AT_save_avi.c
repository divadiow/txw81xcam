/***************************************************
    该demo主要是录制AVI使用
***************************************************/
#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal_file.h"
#include "video_app/video_app.h"
struct AT_AVI
{
    struct os_task task;
    uint32_t fps;
    uint32_t frq;
    int running;
    uint8_t record_num;
    

};


extern void at_save_avi_thread(void *d);
static struct AT_AVI *avi_s = NULL;
int32 demo_atcmd_save_avi(const char *cmd, char *argv[], uint32 argc)
{
	#if OPENDML_EN &&  SDH_EN && FS_EN
    if(argc < 3)
    {
        os_printf("%s argc too small:%d,should more 2 arg\n",__FUNCTION__,argc);
        return 0;
    }

    if(os_atoi(argv[0]) == 0)
    {
		os_printf("avi_s:%X\n",avi_s);
        if(avi_s)
        {
            avi_s->running = 0;
        }
        else
        {
            os_printf("%s not running\n",__FUNCTION__);
        }
    }
    else if(os_atoi(argv[0]) == 1)
    {
        avi_s = custom_malloc(sizeof(struct AT_AVI));
        if(avi_s)
        {
            os_memset(avi_s,0,sizeof(struct AT_AVI));
            avi_s->fps = os_atoi(argv[1]);
            avi_s->frq = os_atoi(argv[2]);
            if(argc > 3)
            {
                avi_s->record_num = os_atoi(argv[3]);
            }
    
            if(avi_s->record_num == 0)
            {
                avi_s->record_num = 1;
            }
            os_printf("fps:%d\tfrq:%d\n",avi_s->fps,avi_s->frq);
            OS_TASK_INIT("at_avi", &avi_s->task, at_save_avi_thread, (uint32)avi_s, OS_TASK_PRIORITY_NORMAL, 1024);  
        }
    }
	#endif


    return 0;
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
void at_save_avi_thread(void *d)
{
    struct AT_AVI *func_avi_s = (struct AT_AVI *)d;
    stream *video_s = NULL;
    stream *audio_s = NULL;
    video_s = open_stream_available(R_AT_AVI_JPEG,0,8,opcode_func,NULL);
    if(!video_s)
    {
        goto at_save_avi_thread_end;
    }

    if(func_avi_s->frq)
    {
        audio_s = open_stream(R_AT_AVI_AUDIO,0,8,opcode_func,NULL);
    }

    jpeg_stream_init();

    func_avi_s->running = 1;
    os_printf("video_s:%X\taudio_s:%X\n",video_s,audio_s);
    while(func_avi_s->running && func_avi_s->record_num)
    {
        if(video_s)
        {
            enable_stream(video_s,1);
        }

        if(audio_s)
        {
            enable_stream(audio_s,1);
        }
        new_sd_save_avi2(&func_avi_s->running,video_s,audio_s,func_avi_s->fps,func_avi_s->frq,60*1,640,480);
        func_avi_s->record_num--;
    }
    

    at_save_avi_thread_end:
    
    if(video_s)
    {
        jpeg_stream_deinit();
        close_stream(video_s);
    }

    if(audio_s)
    {
        close_stream(audio_s);
    }
    func_avi_s->running = 0;
    custom_free((void*)func_avi_s);
    avi_s = NULL;
	os_printf("%s end\n",__FUNCTION__,__LINE__);
}