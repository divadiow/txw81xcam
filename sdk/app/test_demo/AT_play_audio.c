/***************************************************
    该demo主要是使用AT命令控制播放音频,录制格式为WAV
	AT命令格式：AT+PLAY_AUDIO=xxx.wav,mode
***************************************************/
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "string.h"

#include "osal/task.h"
#include "osal/string.h"
#include "hal/audac.h"
#include "stream_frame.h"
#include "osal_file.h"
#include "custom_mem/custom_mem.h"


#ifdef PSRAM_HEAP
#define PLAY_AUDIO_MALLOC	custom_malloc_psram
#define PLAY_AUDIO_FREE		custom_free_psram
#else
#define PLAY_AUDIO_MALLOC	custom_malloc
#define PLAY_AUDIO_FREE		custom_free
#endif


extern int get_audio_dac_set_filter_type(void);
extern void audio_dac_set_filter_type(int filter_type);
extern int audio_dac_get_samplingrate(void);
extern void audio_da_recfg(uint32_t hz);
#define AUDIO_LEN  (1024)
char audio_filePath[10];
void *audio_fp = NULL;

typedef struct _riff_chunk {
	uint8  ChunkID[4];
	uint32 ChunkSize;
	uint8  Format[4];
} TYPE_RIFF_CHUNK;
typedef struct _fmt_chunk {
	uint8  FmtID[4];
	uint32 FmtSize;
	uint16 FmtTag;
	uint16 FmtChannels;
	uint32 SampleRate;
	uint32 ByteRate;
	uint16 BlockAlign;
	uint16 BitsPerSample;
} TYPE_FMT_CHUNK;
typedef struct _data_chunk {
	uint8  DataID[4];
	uint32 DataSize;
} TYPE_DATA_CHUNK;
typedef struct _wave_head {
	TYPE_RIFF_CHUNK  riff_chunk;
	TYPE_FMT_CHUNK   fmt_chunk;
	TYPE_DATA_CHUNK  data_chunk;
} TYPE_WAVE_HEAD;

TYPE_WAVE_HEAD *wave_head = NULL;
uint32 audio_data_len;
static uint8_t play_mode = 0;
static uint8_t play_status = 0;
stream *wave_src = NULL;
struct os_task at_play_audio_task;



static uint32_t get_sound_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	return (uint32_t)d->priv;
}
static uint32_t set_sound_data_len(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->priv = (void*)len;
	return (uint32_t)len;
}
static const stream_ops_func stream_sound_ops = 
{
	.get_data_len = get_sound_data_len,
	.set_data_len = set_sound_data_len,
};
static uint8_t get_play_status(void)
{
	return play_status;
}
static void set_play_status(uint8_t sta)
{
	play_status = sta;
}
int at_play_audio_thread(void *d);
int32 demon_atcmd_play_audio(const char *cmd, char *argv[], uint32 argc)
{
	if(argc < 2)
    {
        os_printf("%s argc too small:%d,enter the path and mode\n",__FUNCTION__,argc);
        return 0;
    }

	if(argv[0])
	{
		memset(audio_filePath,0,sizeof(audio_filePath));
		memcpy(audio_filePath,argv[0],strlen(argv[0]));
		os_printf("audio path:%s\n",audio_filePath);
		play_mode = os_atoi(argv[1]);
		
		if(get_play_status() == 1)
		{
			set_play_status(0);
			while(wave_src != NULL)
				os_sleep_ms(100);
		}
		if((play_mode == 1) || (play_mode == 2)) {
			audio_fp = osal_fopen(audio_filePath,"rb");
			if(!audio_fp)
			{
				os_printf("\nopen audio file err");
				return 0;
			}
			if(wave_head == NULL)
				wave_head = (TYPE_WAVE_HEAD*)PLAY_AUDIO_MALLOC(sizeof(TYPE_WAVE_HEAD));
			else
				os_memset(wave_head, 0, sizeof(TYPE_WAVE_HEAD));
			if(!wave_head)
				return 0;
			osal_fread(wave_head,1,sizeof(TYPE_WAVE_HEAD),audio_fp);
			audio_data_len = wave_head->data_chunk.DataSize; 

			os_printf("SampleRate:%d FmtChannels:%d BitsPerSample:%d data_len:%d\r\n",
				wave_head->fmt_chunk.SampleRate,wave_head->fmt_chunk.FmtChannels,wave_head->fmt_chunk.BitsPerSample,audio_data_len);
			set_play_status(1);
			OS_TASK_INIT("at_play_audio", &at_play_audio_task, at_play_audio_thread, (void*)audio_fp, OS_TASK_PRIORITY_NORMAL, 1024);
		}
	}
	return 0;
}

static int opcode_func(stream *s,void *priv,int opcode)
{
	static uint8_t *audio_buf = NULL;
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{			
			audio_buf = PLAY_AUDIO_MALLOC(4 * AUDIO_LEN);
			if(audio_buf)
			{
				os_printf("at audio malloc:%x\n",audio_buf);
				stream_data_dis_mem(s,4);
			}
			streamSrc_bind_streamDest(s,R_SPEAKER);			
		}
		break;
		case STREAM_OPEN_FAIL:
		break;

		case STREAM_FILTER_DATA:
		break;

		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			int data_num = (int)data->priv;
			data->ops = (stream_ops_func*)&stream_sound_ops;
			data->data = audio_buf + (data_num)*AUDIO_LEN;
		}
		break;

		case STREAM_DATA_DESTORY:
		{
			if(audio_buf) {
				os_printf("at audio free:%x\n",audio_buf);
				PLAY_AUDIO_FREE(audio_buf);
				audio_buf = NULL;
			}
		}
		case STREAM_DATA_FREE:
			//_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
		break;


		//数据发送完成,可以选择唤醒对应的任务
		case STREAM_RECV_DATA_FINISH:
		break;

		default:
			//默认都返回成功
		break;
	}
	return res;	
}

int16_t *wav_buf = NULL;
int at_play_audio_thread(void *d)
{
	int readLen = 0;
	int audio_len = 0;
	uint32 read_total_len = 0;
	struct data_structure *data = NULL;
	int32_t audio_temp = 0;
	int former_dac_priv = 0;
	int former_dac_samplingrate = 0;

	//struct audac_device *volume_dev = (struct audac_device *)dev_get(HG_AUDAC_DEVID);

	FIL *fp = (FIL*)d;	
	stream *wave_src = open_stream_available("file_audio", 4, 0, opcode_func,NULL);

	if(!wave_src)
	{
		osal_fclose(fp);
		PLAY_AUDIO_FREE(wave_head);
		os_printf("\nopen stream err");
		return 0;
	}
	former_dac_priv = get_audio_dac_set_filter_type();
	audio_dac_set_filter_type(SOUND_FILE);
	former_dac_samplingrate = audio_dac_get_samplingrate();
	if(former_dac_samplingrate != wave_head->fmt_chunk.SampleRate)
		broadcast_cmd_to_destStream(wave_src,SET_CMD_TYPE(CMD_AUDIO_DAC_MODIFY_HZ,wave_head->fmt_chunk.SampleRate));
	if(wave_head->fmt_chunk.BitsPerSample == 24) {
		audio_len = AUDIO_LEN*8/24;  //sample取整
		audio_len = audio_len*3/4;   //sample取偶
		audio_len *= 4;
	}
	else if(wave_head->fmt_chunk.BitsPerSample == 16) {
		audio_len = AUDIO_LEN;
	}
	else if(wave_head->fmt_chunk.BitsPerSample == 8) {
		audio_len = AUDIO_LEN/2;
	}
	while(1)
	{
		if(get_play_status() == 0)
			break;
		data = get_src_data_f(wave_src);
		if(data)
		{
			wav_buf = get_stream_real_data(data);
//			os_printf("wb:%x\n",wav_buf);
			if((read_total_len+audio_len) > audio_data_len) {
				readLen = osal_fread((uint8_t*)wav_buf, 1, (audio_data_len-read_total_len), fp);
			}
			else
				readLen = osal_fread((uint8_t*)wav_buf, 1, audio_len, fp);
			if(readLen > 0) {
				read_total_len += readLen;
				data->type = SET_DATA_TYPE(SOUND, SOUND_FILE);
				if(wave_head->fmt_chunk.BitsPerSample == 24) {
					for(uint32_t i=0; i<(readLen/3); i++) {						
						audio_temp = (int32_t)((*((uint8_t*)wav_buf+3*i))|(*((uint8_t*)wav_buf+3*i+1)<<8)|(*((uint8_t*)wav_buf+3*i+2))<<16)&0x00FFFFFF;
						// os_printf("former:%x %x %x %x ",(*((uint8_t*)wav_buf+3*i)),(*((uint8_t*)wav_buf+3*i+1)),(*((uint8_t*)wav_buf+3*i+2)),audio_temp);
						wav_buf[i] = audio_temp>>8;
						// printf("new:%x %d\n",(int16_t)wav_buf[i], read_total_len);
					}
					readLen = readLen*2/3;
				}
				else if(wave_head->fmt_chunk.BitsPerSample == 8) {
					os_memcpy(wav_buf+readLen/2, wav_buf, readLen);
					for(uint32_t i=0; i<readLen; i++) {
						wav_buf[i] = (int16_t)(*(((uint8_t*)wav_buf)+readLen+i)-128)<<8;
					}
					readLen = readLen*2;                
				}

				if(wave_head->fmt_chunk.FmtChannels == 1) {
					set_sound_data_len(data, readLen);
					send_data_to_stream(data);
				}
				else if(wave_head->fmt_chunk.FmtChannels == 2) {
					for(uint32_t i=0; i<(readLen/4); i++) {
						wav_buf[i] = ((wav_buf[2*i]+wav_buf[2*i+1])>>1);
					}
					readLen = readLen/2;
					set_sound_data_len(data, readLen);
					send_data_to_stream(data);
				}
			}	
			else if(readLen <= 0 )
			{
				os_printf("\nread audio data err");
				break;
			}
			if(read_total_len >= audio_data_len) {
				if(play_mode == 1)
					break;
				else if(play_mode == 2) {
					read_total_len = 0;
					osal_fseek(fp, sizeof(TYPE_WAVE_HEAD));
				}
			}		
		}
		else
			os_sleep_ms(1);
	}

	audio_dac_set_filter_type(former_dac_priv);
	osal_fclose(fp);
	if(former_dac_samplingrate != audio_dac_get_samplingrate())
		audio_da_recfg(former_dac_samplingrate);
	os_printf("\nplay wav end");
	close_stream(wave_src);
	wave_src = NULL;
	if(wave_head) {
		PLAY_AUDIO_FREE(wave_head);
		wave_head = NULL;
	}
	return 0;
}