/***************************************************
    该demo主要是使用AT命令拍一张照片,前提要将jpeg打开
***************************************************/
#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal_file.h"
#include "video_app/video_app.h"

void at_save_photo_thread(void *d);
struct AT_PHOTO
{
    struct os_task task;
    uint32_t photo_num;
    uint8_t filename_prefix[4];
    uint8_t running;

};

static struct AT_PHOTO *photo_s = NULL;
int32 demo_atcmd_save_photo(const char *cmd, char *argv[], uint32 argc)
{
	#if OPENDML_EN &&  SDH_EN && FS_EN
    if(argc < 2)
    {
        os_printf("%s argc too small:%d,should more 2 arg\n",__FUNCTION__,argc);
        return 0;
    }
    if(os_atoi(argv[0]) == 0)
    {
        if(photo_s)
        {
            photo_s->running = 0;
        }
        else
        {
            os_printf("%s takephoto num err:%d\n",__FUNCTION__,os_atoi(argv[0]));
        }
        
        return 0;
    } 
    photo_s = custom_malloc(sizeof(struct AT_PHOTO));
    if(photo_s)
    {
        memset(photo_s,0,sizeof(struct AT_PHOTO));
        //连续拍照多少张
        photo_s->photo_num = os_atoi(argv[0]);
        int prefix_len = strlen(argv[1]);
        if(prefix_len > 3)
        {
            os_memcpy(photo_s->filename_prefix,argv[1],3);
        }
        else
        {
            os_memcpy(photo_s->filename_prefix,argv[1],prefix_len);
        }

        //创建拍照的任务
        OS_TASK_INIT("at_photo", &photo_s->task, at_save_photo_thread, (uint32)photo_s, OS_TASK_PRIORITY_NORMAL, 1024);  
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

extern int no_frame_record_video2(void *fp,void *d,int flen);
void at_save_photo_thread(void *d)
{
    struct data_structure *get_f = NULL;
    struct AT_PHOTO *p_s = (struct AT_PHOTO *)d;
    stream *s = NULL;
    uint32_t flen;
    char filename[64] = {0};
    s = open_stream_available(R_AT_SAVE_PHOTO,0,8,opcode_func,NULL);
    if(!s)
    {
        goto at_save_photo_thread_end;
    }
    p_s->running = 1;
    void *fp = NULL;
    uint32_t err_count = 0;
    start_jpeg();
    while(p_s->photo_num && p_s->running)
    {
        get_f = recv_real_data(s);
        if(get_f)
        {
            err_count = 0;
            os_sprintf(filename,"0:photo/%s_%04d.jpg",p_s->filename_prefix,(uint32_t)os_jiffies()%9999);
            os_printf("filename:%s\n",filename);
            fp = osal_fopen(filename,"wb+");
            if(!fp)
            {
                goto at_save_photo_thread_end;
            }
            flen = get_stream_real_data_len(get_f);
            no_frame_record_video2(fp,get_f,flen);
            free_data(get_f);
            get_f = NULL;
            osal_fclose(fp);
            p_s->photo_num--;
        }
        else
        {
            err_count++;
            os_sleep_ms(1);
            if(err_count++ > 1000)
            {
                goto at_save_photo_thread_end;
            }
        }
        
    }
    at_save_photo_thread_end:
    if(get_f)
    {
        free_data(get_f);
        get_f = NULL;
    }
    
    if(s)
    {
        stop_jpeg();
        close_stream(s);
    }
    p_s->running = 0;
    custom_free((void*)p_s);
    photo_s = NULL;
}