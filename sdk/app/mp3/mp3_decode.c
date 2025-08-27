#include "basic_include.h"
#include "csi_kernel.h"
#include "stream_define.h"
#include "stream_frame.h"
#include "custom_mem/custom_mem.h"
#include "osal_file.h"
#include "audio_dac.h"
#include "mad.h"
#include "mp3_decode.h"
#include "mp3_getInfo.h"

#define BUFF_SIZE   2048
#define MAX_MP3_DECODE_TXBUF    4

struct mp3_decode_struct {
	struct os_semaphore clean_sema;
    struct os_task task_hdl;
    stream *mp3_stream;
    uint8_t get_first_frame;
    uint8_t buf[BUFF_SIZE*2];
    uint32_t buf_size;
    uint32_t cur_sampleRate;
	mp3_read_func mp3_read;
};
static struct mp3_decode_struct *mp3_decode_s = NULL;

uint8_t mp3_play_finish = 1;
static void *mp3_fp = NULL;
static char *mp3_filename = NULL;
static uint8_t next_status = MP3_STOP;
static uint8_t current_status = MP3_STOP;
static uint32_t file_size = 0;

void mp3_decode_deinit(void);

uint8_t get_mp3_decode_status(void)
{
    return current_status;
}
   
void set_mp3_decode_status(uint8_t status)
{
    next_status = status; 
}

int32_t mp3_file_read(uint8_t *buf, uint32_t size)
{
    int32_t read_len = 0;
	
    read_len = osal_fread(buf, size, 1, mp3_fp);
    return read_len;
}

static uint16_t CRC_16(uint8_t *temp,uint16_t len)
{
	uint16_t i,j;
	uint16_t CRC_1 = 0xFFFF;
	
	for(i = 0;i < len;i++)
	{
		CRC_1 ^= temp[i];
		for(j = 0;j < 8;j++)
		{
			if(CRC_1 & 0x01)
			{
				CRC_1 >>=1;
				CRC_1 ^= 0xA001;
			}
			else 
			{
				CRC_1 >>=1;
			}
		}
	}
	return(CRC_1);
}

static enum mad_flow error(void *data,struct mad_stream *stream, struct mad_frame *frame)
{
    struct mp3_decode_struct *s = (struct mp3_decode_struct *)data;

    os_printf("decoding error 0x%04x (%s) at byte offset %u\n",
    stream->error, mad_stream_errorstr(stream),
    stream->this_frame - s->buf);

    /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow input(void *cb_data, struct mad_stream *stream)
{
    uint8_t *mp3_buf = NULL;
    int32_t read_len = 0;
    int32_t ret_code;
    uint32_t ID3V2_len = 0;
    int32_t ID3V2_offset = 0;
	uint32_t unproc_data_size = 0;    /*the unprocessed data's size*/
	struct mp3_decode_struct *s = (struct mp3_decode_struct *)cb_data;

	if(os_sema_down(&mp3_decode_s->clean_sema, 0))
		mad_stream_init(stream);
    mp3_buf = s->buf;
    while(!s->get_first_frame) {
        read_len = s->mp3_read(s->buf+unproc_data_size,BUFF_SIZE);
        if(read_len <= 0) {
			os_printf("%s %d err!\n",__FUNCTION__,__LINE__);
            return MAD_FLOW_STOP;
		}
        s->buf_size = read_len + unproc_data_size;
        if(os_strncmp(mp3_buf, "ID3", 3) == 0)
        {
            if (s->buf_size < 10) {
                unproc_data_size = s->buf_size;
                os_sleep_ms(5);
                continue;
            }
            ID3V2_len = (mp3_buf[6]&0x7F)*0x200000+ (mp3_buf[7]&0x7F)*0x4000 + 
                                            (mp3_buf[8]&0x7F)*0x80 +(mp3_buf[9]&0x7F);
            ID3V2_offset = (ID3V2_len+10);
            while(ID3V2_offset >= read_len) {
                os_sleep_ms(5);
                ID3V2_offset -= read_len;
                read_len = s->mp3_read(s->buf,BUFF_SIZE);
                s->buf_size = read_len;
            } 
            if(ID3V2_offset) {
                s->buf_size -= ID3V2_offset;
                os_memcpy(mp3_buf, mp3_buf+ID3V2_offset, (read_len-ID3V2_offset));
            }
            if(s->buf_size <= 1) {
                os_sleep_ms(5);
                unproc_data_size = s->buf_size;
                continue;
            }
        } 

        for(uint32_t i=0; i<(s->buf_size-1); i++) {
            if( ( (mp3_buf[i] << 8) | (mp3_buf[i+1] & 0xE0) ) == 0xFFE0 ) {
                s->get_first_frame = 1;
                unproc_data_size = s->buf_size - i;
                s->buf_size = unproc_data_size;
                mad_stream_buffer(stream, s->buf, s->buf_size);
                unproc_data_size = 0;
				ret_code = MAD_FLOW_CONTINUE;
				return ret_code;
            }
        }
        if(!s->get_first_frame) {
            unproc_data_size = 1;
            mp3_buf[0] = mp3_buf[s->buf_size-1];
        }
        os_sleep_ms(5);
    }
    unproc_data_size += stream->bufend - stream->next_frame;
    if(unproc_data_size)
        os_memcpy(s->buf, s->buf+s->buf_size-unproc_data_size, unproc_data_size);
    read_len = s->mp3_read(s->buf+unproc_data_size,BUFF_SIZE);
    if(read_len <= 0)
        ret_code = MAD_FLOW_STOP;
    else {
        s->buf_size = read_len + unproc_data_size;
        /*Hand off the buffer to the mp3 input stream*/
        mad_stream_buffer(stream, s->buf, s->buf_size);
        ret_code = MAD_FLOW_CONTINUE;
    }
	unproc_data_size = 0;
    return ret_code;
}

static enum mad_flow output(void *cb_data, struct mad_header const *header, struct mad_pcm *pcm)
{
	int16_t sample_val = 0;
	int16_t *data = NULL;
    uint32_t offset = 0;
    uint32_t nchannels, nsamples;
    uint32_t samplerate;
	int16_t *left_ch, *right_ch;
    struct data_structure *data_s = NULL;
    struct mp3_decode_struct *s = (struct mp3_decode_struct *)cb_data;
    stream *mp3_stream = s->mp3_stream;

    /* pcm->samplerate contains the sampling frequency */
    samplerate = pcm->samplerate;
    nchannels  = pcm->channels;
    nsamples   = pcm->length;
    left_ch    = pcm->samples[0];
    right_ch   = pcm->samples[1];

get_data_structure:	
    data_s = get_src_data_f(mp3_stream);
    if(data_s) {
        data = (int16_t*)get_stream_real_data(data_s);;
        while (nsamples--) {
            sample_val = (*left_ch++);
            data[offset] = sample_val;
            if (nchannels == 2) {
                sample_val = (*right_ch++);
                data[offset] = (data[offset]+sample_val)/2;
            }
            offset++;
        }
		set_stream_real_data_len(data_s, pcm->length*2);
		data_s->type = SET_DATA_TYPE(SOUND,SOUND_FILE);

		if(s->cur_sampleRate != samplerate)
		{
			s->cur_sampleRate = samplerate;
			audio_da_recfg(samplerate);
		}
		
		send_data_to_stream(data_s);
    }
    else {
        os_sleep_ms(1);
		goto get_data_structure;
	}

    if(next_status == MP3_PAUSE) 
    {
        current_status = MP3_PAUSE;
        while(next_status == MP3_PAUSE)
        {
            os_sleep_ms(1);
        }
    }

    if(next_status == MP3_STOP)
    {
        return MAD_FLOW_STOP;
    }
    current_status = MP3_PLAY;
  
    return MAD_FLOW_CONTINUE;
}

static int decode(struct mp3_decode_struct *s)
{
	int result;
    struct mad_decoder decoder;

    /* configure input, output, and error functions */
    mad_decoder_init(&decoder, s,
            input, 0 /* header */, 0 /* filter */, output,
            error, 0 /* message */);

    /* start decoding */
    result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

    /* release the decoder */
    mad_decoder_finish(&decoder);
    return result;
}

void mp3_decode_file_thread(void *d)
{
    int32_t former_dac_filter_type = 0;
    int32_t former_dac_samplingrate = 0;
    struct mp3_decode_struct *s = (struct mp3_decode_struct *)d;

	if(mp3_fp) {
		curmp3_info_init(mp3_filename);
		file_size = osal_fsize(mp3_fp);
		get_curmp3_size(file_size);	
		find_first_frame(mp3_fp);
	}
	if(cur_mp3_info) 
		osal_fseek(mp3_fp, cur_mp3_info->first_frame_offset);

	former_dac_filter_type = get_audio_dac_set_filter_type();
	audio_dac_set_filter_type(SOUND_FILE);
	former_dac_samplingrate = audio_dac_get_samplingrate();
    s->cur_sampleRate = former_dac_samplingrate;

	next_status = MP3_PLAY;
	current_status = MP3_PLAY;
    mp3_play_finish = 0;
    
    decode(s);

    clear_curmp3_info();
    mp3_decode_deinit();
    if(s->mp3_stream)
    {
        close_stream(s->mp3_stream);
    }
    while(mp3_play_finish != 1) {
        os_sleep_ms(1);
    }
    audio_dac_set_filter_type(former_dac_filter_type);	
	if(former_dac_samplingrate != audio_dac_get_samplingrate())
		audio_da_recfg(former_dac_samplingrate);
}

void mp3_decode_deinit(void)
{
	if(mp3_fp) {
		osal_fclose(mp3_fp);
		mp3_fp = NULL;
	}
	if(mp3_decode_s->clean_sema.hdl) 
		os_sema_del(&mp3_decode_s->clean_sema);
	next_status = MP3_STOP;
	current_status = MP3_STOP;	
}

static uint32_t get_mp3_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	return ((uint32_t)d->priv);
}

static uint32_t set_mp3_data_len(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->priv = (void*)len;
	return (uint32_t)len;
}

static stream_ops_func stream_sound_ops = 
{
	.get_data_len = get_mp3_data_len,
	.set_data_len = set_mp3_data_len,
};

static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_EXIT:
		{
            stream_data_dis_mem_custom(s);
			streamSrc_bind_streamDest(s,R_SPEAKER);
            s->priv = priv;
		}
		break;
		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
            data->priv = (void*)(1152*2);
			data->ops = &stream_sound_ops;
			data->data = (void*)MP3_DECODE_MALLOC((1152*2));
		}
		break;
		case STREAM_DATA_DESTORY:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                MP3_DECODE_FREE(data->data);
            }
		}
		break;
        case STREAM_DEL:
        {
            if(s->priv)
            {
                MP3_DECODE_FREE(s->priv);
            }
            MP3_DECODE_FREE(mp3_decode_s);
            mp3_decode_s = NULL;
            mp3_play_finish = 1;           
        }
        break;
		default:
		break;
	}
	return res;
}

void mp3_decode_clean_stream(void)
{
	if(mp3_decode_s->clean_sema.hdl)
		os_sema_up(&(mp3_decode_s->clean_sema));
}

void mp3_decode_init(void *d, void *read_func)
{
    uint8_t status = 0;
	uint32_t count = 0;
	mp3_read_func func = NULL;
    stream *mp3_stream = NULL;
    char *mp3_stream_name = NULL;

	status = get_mp3_decode_status();
    if(status != MP3_STOP)
    {
        set_mp3_decode_status(MP3_STOP);
    }
    while((get_mp3_decode_status() != MP3_STOP) && count < 1000)            
    {
        os_sleep_ms(1);
        count ++;
    }
    if(count >= 1000)
    {
        os_printf("%s creat fail:%d\n",__FUNCTION__,get_mp3_decode_status());
        return;
    }
	mp3_filename = (char*)d;
	if(mp3_filename){
		mp3_fp = osal_fopen(mp3_filename, "rb");
		if(!mp3_fp)
			return;	
		func = mp3_file_read;
	}
	else if(read_func) {
		func = (mp3_read_func)read_func;
	}
	else
		return;
		
	mp3_stream_name = (char*)MP3_DECODE_ZALLOC(64);
    os_sprintf(mp3_stream_name,"%s_%d",S_MP3_AUDIO,(int)(os_jiffies()%1000));
    mp3_stream = open_stream_available(mp3_stream_name,MAX_MP3_DECODE_TXBUF,0,opcode_func,mp3_stream_name);
	if(!mp3_stream) {
		os_printf("%s %d err!\n",__FUNCTION__,__LINE__);
		goto mp3_decode_init_err;
	}
      
	mp3_decode_s = (struct mp3_decode_struct *)MP3_DECODE_ZALLOC(sizeof(struct mp3_decode_struct));
	if(!mp3_decode_s) {
		os_printf("%s %d err!\n",__FUNCTION__,__LINE__);
		goto mp3_decode_init_err;
	}
	mp3_decode_s->mp3_stream = mp3_stream;
	mp3_decode_s->mp3_read = func;
	if(os_sema_init(&mp3_decode_s->clean_sema, 0) != RET_OK) {
		os_printf("%s %d err!\n",__FUNCTION__,__LINE__);
		goto mp3_decode_init_err;
	}
#if FORCE_MONO_CHANNEL
	os_task_create("mp3_decode_file_thread", mp3_decode_file_thread, (void*)mp3_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 6144);
#else
    os_task_create("mp3_decode_file_thread", mp3_decode_file_thread, (void*)mp3_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 8192);
#endif
	return;
	
mp3_decode_init_err:
	if(mp3_stream)
		close_stream(mp3_stream);
	if(mp3_fp)
		osal_fclose(mp3_fp);
}