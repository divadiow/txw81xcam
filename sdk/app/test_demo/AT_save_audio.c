/***************************************************
    该demo主要是使用AT命令控制录制音频,录制格式为WAV
***************************************************/
#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal_file.h"
void at_save_audio_thread(void *d);

typedef struct
{
char chRIFF[4];                 // "RIFF" 标志  
int  total_Len;                 // 文件长度      
char chWAVE[4];                 // "WAVE" 标志  
char chFMT[4];                  // "fmt" 标志 
int  dwFMTLen;                  // 过渡字节（不定）  一般为16
short fmt_pcm;                  // 格式类别  
short  channels;                // 声道数  
int fmt_samplehz;               // 采样率 
int fmt_bytepsec;               // 位速  
short fmt_bytesample;           // 一个采样多声道数据块大小  
short fmt_bitpsample;    // 一个采样占的 bit 数  
char chDATA[4];                 // 数据标记符＂data ＂  
int  dwDATALen;                 // 语音数据的长度，比文件长度小42一般。这个是计算音频播放时长的关键参数~  
}WaveHeader;


const unsigned char wav_header[] = {  
'R', 'I', 'F', 'F',      // "RIFF" 标志  
0, 0, 0, 0,              // 文件长度  
'W', 'A', 'V', 'E',      // "WAVE" 标志  
'f', 'm', 't', ' ',      // "fmt" 标志  
16, 0, 0, 0,             // 过渡字节（不定）  
0x01, 0x00,              // 格式类别  
0x01, 0x00,              // 声道数      
0, 0, 0, 0,              // 采样率  
0, 0, 0, 0,              // 位速  
0x01, 0x00,              // 一个采样多声道数据块大小  
0x10, 0x00,              // 一个采样占的 bit 数  
'd', 'a', 't', 'a',      // 数据标记符＂data ＂  
0, 0, 0, 0               // 语音数据的长度，比文件长度小42一般。这个是计算音频播放时长的关键参数~  
};  

struct AT_AUDIO
{
    uint32_t frq;
    struct os_task task;
    uint8_t filename_prefix[4];
    uint8_t running;
    uint32_t minute;

};

static struct AT_AUDIO *audio_s = NULL;

int32 demo_atcmd_save_audio(const char *cmd, char *argv[], uint32 argc)
{
    if(argc < 2)
    {
        os_printf("%s argc too small:%d,should more 2 arg\n",__FUNCTION__,argc);
        return 0;
    }

    if(os_atoi(argv[0]) == 1)
    {
        if(audio_s)
        {
            os_printf("%s already running\n");
            return 0;
        }
        else
        {
            audio_s = custom_malloc(sizeof(struct AT_AUDIO));
            
            if(audio_s)
            {  
                os_memset(audio_s,0,sizeof(struct AT_AUDIO));
                audio_s->frq = os_atoi(argv[1]);
                os_printf("frq:%d\n",audio_s->frq);
                if(argc > 2)
                {
                    audio_s->minute = os_atoi(argv[2]) ;
                    if(audio_s->minute == 0)
                    {
                        audio_s->minute = ~0;
                    }
                }
                else
                {
                    audio_s->minute = ~0;
                }
                if(argc > 3)
                {
                    int prefix_len = strlen(argv[3]);
                    if(prefix_len > 3)
                    {
                        os_memcpy(audio_s->filename_prefix,argv[3],3);
                    }
                    else
                    {
                        os_memcpy(audio_s->filename_prefix,argv[3],prefix_len);
                    }
                }
				else
				{
					os_memcpy(audio_s->filename_prefix,"def",3);
				}
                //创建录音频的任务
                OS_TASK_INIT("at_audio", &audio_s->task, at_save_audio_thread, (uint32)audio_s, OS_TASK_PRIORITY_NORMAL, 1024);  
            }
        }
    }
    else if(os_atoi(argv[0]) == 0)
    {
        if(!audio_s)
        {
            os_printf("%s not running\n");
            return 0;
        }
        else
        {
            //设置停止标志位
            audio_s->running = 0;
        }
    }
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



void at_save_audio_thread(void *d)
{
    struct AT_AUDIO *a_s = (struct AT_AUDIO *)d;
    WaveHeader header;
    os_memcpy(&header,wav_header,sizeof(WaveHeader));
    header.fmt_samplehz = a_s->frq;
    header.fmt_bytepsec = a_s->frq*2;
    
    char filename[64] = {0};
    a_s->running = 1;
    uint32_t count = 0;
    int w_len = 0;
    uint32_t flen;
    uint32_t w_count = 0;
    uint8_t *buf;
    void *fp  = NULL;
    struct data_structure *get_f = NULL;
    stream* s = NULL;
    uint32_t start_time = 0;
    s = open_stream_available(R_AT_SAVE_AUDIO,0,8,opcode_func,NULL);
    if(!s)
    {
        goto at_save_audio_thread_end;
    }

    os_printf("prefix:%s\n",a_s->filename_prefix);
    os_sprintf(filename,"0:audio/%s_%04d.wav",a_s->filename_prefix,(uint32_t)os_jiffies()%9999);
    os_printf("record name:%s\n",filename);
    fp = osal_fopen(filename,"wb+");
    if(!fp)
    {
        goto at_save_audio_thread_end;
    }
    //偏移头部信息
    osal_fseek(fp,sizeof(WaveHeader));
    start_time = os_jiffies();
    while(a_s->running && (os_jiffies()-start_time)/1000 < a_s->minute*60)
    {
        count++;
        if(count % 1000 == 0)
        {
            os_printf("%s:%d\t%d\trecord time:%d\n",__FUNCTION__,__LINE__,w_count,os_jiffies()-start_time);
        }
        get_f = recv_real_data(s);
        if(get_f)
        {
            buf = get_stream_real_data(get_f);
            flen = get_stream_real_data_len(get_f);
            w_len = osal_fwrite(buf, flen, 1, fp);
            free_data(get_f);
            get_f = NULL;
            if(w_len <= 0)
            {
                goto at_save_audio_thread_end;
            }
            w_count += w_len;

        }
        else
        {
            os_sleep_ms(1);
        }

    }
at_save_audio_thread_end:
    header.dwDATALen = w_count;
    header.total_Len = w_count + sizeof(WaveHeader) - 8;
    

    os_printf("%s end!!!!!!!!!!!!!\n",__FUNCTION__);
    os_printf("start_time:%d\tend_time:%d\n",start_time,os_jiffies());
    if(fp)
    {
        osal_fseek(fp,0);
        osal_fwrite(&header, sizeof(header), 1, fp);
        osal_fclose(fp);
    }
    if(s)
    {
        close_stream(s);
    }
    a_s->running = 0;
    custom_free((void*)a_s);
    audio_s = NULL;
    return;
}