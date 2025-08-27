#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"


#include "custom_mem/custom_mem.h"
#include "stream_frame.h"

#include "interf_dec.h"
#include "sp_dec.h"
#include "typedef.h"

#include "amr_app.h"
#include "fatfs/osal_file.h"
#include "osal/string.h"


static uint32_t get_amrnb_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	return ((uint32_t)d->priv);
}


static uint32_t set_amrnb_data_len(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->priv = (void*)len;
	return (uint32_t)len;
}


static const stream_ops_func stream_sound_ops = 
{
	.get_data_len = get_amrnb_data_len,
	.set_data_len = set_amrnb_data_len,
    // .set_data_time = set_sound_data_time,
};

static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            stream_data_dis_mem_custom(s);
			streamSrc_bind_streamDest(s,R_SPEAKER);
            streamSrc_bind_streamDest(s,R_SAVE_STREAM_DATA);
            s->priv = priv;
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			//int data_num = (int)data->priv;
			data->type = SET_DATA_TYPE(SOUND,SOUND_AMRNB);//设置声音的类型
            data->priv = (void*)(AMRNB_TO_PCM_SIZE*2);
			//注册对应函数
			data->ops = (stream_ops_func*)&stream_sound_ops;
            //申请最大的音频size
			data->data = (void*)custom_malloc_psram((AMRNB_TO_PCM_SIZE*2));
		}
		break;

		case STREAM_DATA_DESTORY:
		{
            struct data_structure *data = (struct data_structure *)priv;
            if(data->data)
            {
                custom_free_psram(data->data);
            }
		}
		break;

		//如果释放空间,则删除所有的节点
		case STREAM_DATA_FREE:
		{
		}
		break;

        case STREAM_DATA_FREE_END:
        break;

		//每次即将发送到一个流,就调用一次
		case STREAM_SEND_TO_DEST:
		{

		}
		break;


		//数据发送完成
		case STREAM_SEND_DATA_FINISH:
		{

		}
		break;

		//退出,则关闭对应得流
		case STREAM_CLOSE_EXIT:
		{			
		}
		break;
        //这里移除priv(流名称动态申请)
        case STREAM_DEL:
        {
            if(s->priv)
            {
                custom_free(s->priv);
            }
        }

		default:
			//默认都返回成功
		break;
	}
	return res;
}


static struct amr_extern_read *g_amrnb_decode = NULL;
//读取当前amr解码的状态,如果播放之前,最好先判断一下
uint8_t get_amrnb_decode_status()
{
    //os_printf("g_amrnb_decode:%X\n",g_amrnb_decode);
    if(g_amrnb_decode)
    {
       // os_printf("g_amrnb_decode->inside_status:%d\n",g_amrnb_decode->inside_status);
        return g_amrnb_decode->inside_status;
    }
    return AMR_STOP;
}

//外部调用,可以控制mp3解码状态,但不是立刻生效,应该是等待解码下一帧开始生效
uint8_t set_amrnb_decode_status(uint8_t status)
{
    if(g_amrnb_decode)
    {
        g_amrnb_decode->status = status; 
    }
    return 1;
}



void amrnb_decode_file_thread(void *d)
{
    stream *s = NULL;
    uint32_t offset = 0;
    uint32_t readlen = 0;
    uint32_t read_size = 0;
    uint32_t timeout = 0;
    int frames = 0;
    int send_success = 0;
    struct data_structure  *data = NULL;
    uint32_t data_offset = 0;
    short *buf = NULL;
    short synth[160];

    unsigned char analysis[32];
    enum Mode dec_mode;

    char magic[8];
    const short block_size[16]={ 12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0 };


    struct amr_extern_read *amr = (struct amr_extern_read*)d;


    char *amr_stream_name = (char*)custom_malloc(64);
    os_sprintf(amr_stream_name,"%s_%d",S_AMRNB_AUDIO,(uint32_t)(os_jiffies()%1000));
    s = open_stream_available(amr_stream_name,8,0,opcode_func,amr_stream_name);
    if(!s)
    {
        goto amrnb_decode_file_thread_end;
    }

    int * destate;

    os_printf("%s:%d start\n",__FUNCTION__,__LINE__);
    


    //识别是否为正常的amrnb数据
   readlen = amr->amr_read( amr,magic, offset,sizeof( char )*strlen( AMRNB_MAGIC_NUMBER ));
   offset += readlen;
   if (strncmp( magic, AMRNB_MAGIC_NUMBER, strlen( AMRNB_MAGIC_NUMBER ) ) ) 
   {
        os_printf("%s:%d\tmagic:%s\n",__FUNCTION__,__LINE__,magic);
	   goto amrnb_decode_file_thread_end;
   }




    broadcast_cmd_to_destStream(s,SET_CMD_TYPE(CMD_AUDIO_DAC_MODIFY_HZ,8000));
    destate = Decoder_Interface_init();
   /* find mode, read file */
   while(amr->amr_read( amr,analysis, offset,sizeof (unsigned char)) > 0)
   {
     //这里判断一下是否暂停或者其他操作
     if(amr->status == AMR_PAUSE)
     {
        printf("%s pause\n",__FUNCTION__);
        amr->inside_status = amr->status;
        while(amr->status == AMR_PAUSE)
        {
            os_sleep_ms(1);
        }
        amr->inside_status = AMR_PLAY;
     }
        //退出
        if(amr->status == AMR_STOP)
        {
            printf("%s stop\n",__FUNCTION__);
            //mad_read->inside_status = mad_read->status;
            break;
        }
     offset += sizeof (unsigned char);
      dec_mode = (analysis[0] >> 3) & 0x000F;

	  read_size = block_size[dec_mode];

      readlen = amr->amr_read( amr,&analysis[1], offset,read_size);
      if(readlen <= 0)
      {
        break;
      }
      offset += readlen;


      frames ++;

      /* call decoder */
      //可以设置正确的buf位置
      Decoder_Interface_Decode(destate, analysis, synth, 0);

      //这里进行拷贝数据到节点,然后发送,先获取data,然后设置copy  buf的空间
      timeout = 0;
      while(!data && timeout < 1000)
      {
        data = get_src_data_f(s);
        if(!data)
        {
            os_sleep_ms(1);
            timeout++;
        }
        else
        {
            //数据拷贝偏移清0
            data_offset = 0;
        }
      }

      //timeout超时,可能由异常
      if(timeout >= 1000)
      {
        break;
      }

      //这里是拷贝数据
      buf = (short*)get_stream_real_data(data);
      memcpy(buf+data_offset,synth,160*2);
      data_offset+=160;

      

      //实际这里一定不能大于,如果大于就是越界了，当data_offset == AMRNB_TO_PCM_SIZE,代表这个音频数据
      //写满,可以发送出去进行播放 
      if(data_offset >= AMRNB_TO_PCM_SIZE)
      {

        //先hold住,因为可能不会立刻发送成功
        hold_data(data);

        //尝试发送
        timeout = 0;


        set_stream_real_data_len(data,AMRNB_TO_PCM_SIZE*2);
        send_success = 0;
        while(!send_success && timeout < 1000)
        {
            send_success = send_data_to_stream(data);
            if(!send_success)
            {
                os_sleep_ms(1);
                timeout++;
            }

        }
        //发送成功或者超时,则释放
        free_data(data);
        //os_printf("%s2 data:%X\n",__FUNCTION__,data);
        data = NULL;

        //timeout超时,可能由异常
        if(timeout >= 1000 || send_success == 0)
        {
            os_printf("%s:%d timeout:%d\t send_success:%d err ~~~~~~~~~~~~~~~~~~\n",__FUNCTION__,__LINE__,timeout,send_success);
            break;
        }

        data_offset = 0;

      }

      

      //osal_fwrite( synth, sizeof (short), 160, file_speech );
   }
    if(data)
    {

        if(data_offset > 0)
        {
            //这里将剩余数据清0
            buf = (short*)get_stream_real_data(data);
            memset(buf+data_offset,0,(AMRNB_TO_PCM_SIZE-data_offset)*2);
            set_stream_real_data_len(data,AMRNB_TO_PCM_SIZE*2);

            send_success = 0;
            timeout = 0;
            hold_data(data);
            while(!send_success && timeout < 1000)
            {
                send_success = send_data_to_stream(data);
                if(!send_success)
                {
                    os_sleep_ms(1);
                    timeout++;
                }

            }
            //发送成功或者超时,则释放
            free_data(data);
        }
        else
        {
            force_del_data(data);
        }
        //os_printf("%s1 data:%X\n",__FUNCTION__,data);
        data = NULL;
    }
   
    Decoder_Interface_exit(destate);
    os_printf("frames:%d\n",frames);
    os_printf("%s:%d end\n",__FUNCTION__,__LINE__);
    
    
    amrnb_decode_file_thread_end:

    amr->inside_status = AMR_STOP;
    amr->amr_close(amr,NULL);
    if(s)
    {
        close_stream(s);
    }
}



#include "dec_if.h"
void amrwb_decode_file_thread(void *d)
{
    stream *s = NULL;
    uint32_t offset = 0;
    uint32_t readlen = 0;
    uint32_t timeout = 0;
    int frames = 0;
    int send_success = 0;
    struct data_structure  *data = NULL;
    uint32_t data_offset = 0;
    short *buf = NULL;
    Word16 synth[L_FRAME16k];              /* Buffer for speech @ 16kHz             */
    UWord8 serial[NB_SERIAL_MAX];

    //unsigned char analysis[32];

    char magic[16] = {0};
    const UWord8 block_size[16]= {18, 24, 33, 37, 41, 47, 51, 59, 61, 6, 6, 0, 0, 0, 1, 1};

    void *st;
    Word16 mode;


    struct amr_extern_read *amr = (struct amr_extern_read*)d;


    char *amr_stream_name = (char*)custom_malloc(64);
    os_sprintf(amr_stream_name,"%s_%d",S_AMRWB_AUDIO,(uint32_t)(os_jiffies()%1000));
    s = open_stream_available(amr_stream_name,8,0,opcode_func,amr_stream_name);
    if(!s)
    {
        goto amrwb_decode_file_thread_end;
    }



    os_printf("%s:%d start\n",__FUNCTION__,__LINE__);

    
    

    


    //识别是否为正常的amrnb数据
   readlen = amr->amr_read( amr,magic, offset,sizeof( char )*strlen( AMRWB_MAGIC_NUMBER ));
   offset += readlen;
   os_printf("magic:%s\n",magic);
   if (strncmp( magic, AMRWB_MAGIC_NUMBER, strlen( AMRWB_MAGIC_NUMBER ) ) ) 
   {
       
	   goto amrwb_decode_file_thread_end;
   }

   //修改一下硬件的采样率
   broadcast_cmd_to_destStream(s,SET_CMD_TYPE(CMD_AUDIO_DAC_MODIFY_HZ,16000));
   st = D_IF_init();





   /* find mode, read file */
   while(amr->amr_read( amr,serial, offset,sizeof (UWord8)) > 0)
   {
     //这里判断一下是否暂停或者其他操作
     if(amr->status == AMR_PAUSE)
     {
        printf("%s pause\n",__FUNCTION__);
        amr->inside_status = amr->status;
        while(amr->status == AMR_PAUSE)
        {
            os_sleep_ms(1);
        }
        amr->inside_status = AMR_PLAY;
     }
        //退出
        if(amr->status == AMR_STOP)
        {
            printf("%s stop\n",__FUNCTION__);
            //mad_read->inside_status = mad_read->status;
            break;
        }
     offset += sizeof (unsigned char);
      mode = (Word16)((serial[0] >> 3) & 0x0F);



      readlen = amr->amr_read( amr,&serial[1], offset,block_size[mode]-1);
      if(readlen <= 0)
      {
        break;
      }
      offset += readlen;


      frames ++;

      /* call decoder */
      //可以设置正确的buf位置
      D_IF_decode( st, serial, synth, _good_frame);

      //这里进行拷贝数据到节点,然后发送,先获取data,然后设置copy  buf的空间
      timeout = 0;
      while(!data && timeout < 1000)
      {
        data = get_src_data_f(s);
        if(!data)
        {
            os_sleep_ms(1);
            timeout++;
        }
        else
        {
            //数据拷贝偏移清0
            data_offset = 0;
        }
      }

      //timeout超时,可能由异常
      if(timeout >= 1000)
      {
        break;
      }

      //这里是拷贝数据
      buf = (short*)get_stream_real_data(data);
      memcpy(buf+data_offset,synth,L_FRAME16k*2);
      data_offset+=L_FRAME16k;

      

      //实际这里一定不能大于,如果大于就是越界了，当data_offset == AMRWB_TO_PCM_SIZE,代表这个音频数据
      //写满,可以发送出去进行播放 
      if(data_offset >= AMRWB_TO_PCM_SIZE)
      {

        //先hold住,因为可能不会立刻发送成功
        hold_data(data);

        //尝试发送
        timeout = 0;


        set_stream_real_data_len(data,AMRWB_TO_PCM_SIZE*2);
        send_success = 0;
        while(!send_success && timeout < 1000)
        {
            send_success = send_data_to_stream(data);
            if(!send_success)
            {
                os_sleep_ms(1);
                timeout++;
            }

        }
        //发送成功或者超时,则释放
        free_data(data);
        //os_printf("%s2 data:%X\n",__FUNCTION__,data);
        data = NULL;

        //timeout超时,可能由异常
        if(timeout >= 1000 || send_success == 0)
        {
            os_printf("%s:%d timeout:%d\t send_success:%d err ~~~~~~~~~~~~~~~~~~\n",__FUNCTION__,__LINE__,timeout,send_success);
            break;
        }

        data_offset = 0;

      }

      

      //osal_fwrite( synth, sizeof (short), 160, file_speech );
   }
    if(data)
    {

        if(data_offset > 0)
        {

            _os_printf("++++++++++++++++++++++\n");
            //这里将剩余数据清0
            buf = (short*)get_stream_real_data(data);
            memset(buf+data_offset,0,(AMRWB_TO_PCM_SIZE-data_offset)*2);
            set_stream_real_data_len(data,AMRWB_TO_PCM_SIZE*2);

            send_success = 0;
            timeout = 0;
            hold_data(data);
            while(!send_success && timeout < 1000)
            {
                send_success = send_data_to_stream(data);
                if(!send_success)
                {
                    os_sleep_ms(1);
                    timeout++;
                }

            }
            //发送成功或者超时,则释放
            free_data(data);
        }
        else
        {
            force_del_data(data);
        }

        //os_printf("%s1 data:%X\n",__FUNCTION__,data);

        data = NULL;
    }
   
    D_IF_exit(st);
    os_printf("frames:%d\n",frames);
    os_printf("%s:%d end\n",__FUNCTION__,__LINE__);
    
    
    amrwb_decode_file_thread_end:

    amr->inside_status = AMR_STOP;
    amr->amr_close(amr,NULL);
    if(s)
    {
        close_stream(s);
    }
}






//需要根据MP3的读取长度配置
#define SD_TMP_SIZE (4096)
//这里只是读取一次文件,如果长度不对,不
uint32_t amr_read_func(struct amr_extern_read *e_read,void *ptr,uint32_t offset,uint32_t size)
{
	void *fp = e_read->fp;
    uint32_t len;
    uint32_t read_size = SD_TMP_SIZE;
    uint32_t read_offset = 0;
	//os_printf("read size:%d\tptr:%X\n",size,ptr);
    while(size)
    {
        if(size > SD_TMP_SIZE)
        {
            len = osal_fread(e_read->sd_buf,1,SD_TMP_SIZE,fp);
            read_size += SD_TMP_SIZE;
        }
        else
        {
            len = osal_fread(e_read->sd_buf,1,size,fp);
            read_size = size;
        }
        memcpy(ptr+read_offset,e_read->sd_buf,read_size);
        read_offset += read_size;
        size -= read_size;
    }

	return read_offset;
}

void amr_close_func(struct amr_extern_read *e_read,void *data)
{
	osal_fclose(e_read->fp);
    custom_free(e_read->sd_buf);
    e_read->fp = NULL;
    custom_free(e_read);
}

void amr_seek_func(struct amr_extern_read *e_read,uint32_t offset)
{
    osal_fseek(e_read->fp,offset);
}


char myamr_buf[4098];
void *amr_read_init(char *filename)
{
    //已经有使用该mp3解码,需要先等待解码结束才可以继续,可以去读取状态或者控制状态
 
	struct amr_extern_read *e_read = (struct amr_extern_read*)custom_malloc(sizeof(struct amr_extern_read));
    memset(e_read,0,sizeof(struct amr_extern_read));
	e_read->fp = osal_fopen(filename,"rb");
	os_printf("e_read->fp:%X\n",e_read->fp);
	e_read->readsize = osal_fsize(e_read->fp);
	os_printf("e_read->readsize:%d\n",e_read->readsize);
	e_read->amr_read = amr_read_func;
	e_read->amr_close = amr_close_func;
    e_read->amr_seek = amr_seek_func;
    //注意,这个要用sram的空间
    e_read->sd_buf = (uint8_t*)custom_malloc(SD_TMP_SIZE);
    //sd卡临时buf申请
    os_printf("sd_buf:%X\n",e_read->sd_buf);


    g_amrnb_decode = e_read;
	return (void*)e_read;
}



void amr_decode_thread(void *d)
{
    char *filename = (char *)d;

    uint32_t count = 0;
    uint8_t status = get_amrnb_decode_status();
    if(status != AMR_STOP)
    {
        set_amrnb_decode_status(AMR_STOP);
    }
    while((get_amrnb_decode_status() != AMR_STOP) && count < 1000)
    {
        os_sleep_ms(1);
        count ++;
    }
    if(count >= 1000)
    {
        os_printf("%s creat fail:%d\n",__FUNCTION__,get_amrnb_decode_status());
        return;
    }

    struct amr_extern_read *amr = (struct amr_extern_read*)amr_read_init(filename);

    //这里先判断是amrwb还是amrnb,根据不同去创建不同的线程
    
    if(amr)
    {
        char magic[16];
        amr->amr_read( amr,magic, 0,sizeof( char )*strlen( AMRWB_MAGIC_NUMBER ));
        amr->amr_seek(amr,0);
        //如果不是wb就是nb(即使不是nb,也会进去判断后退出)
        if (strncmp( magic, AMRWB_MAGIC_NUMBER, strlen( AMRWB_MAGIC_NUMBER ) ) == 0 ) 
        {
            os_task_create("amrwb", amrwb_decode_file_thread, (void*)amr, OS_TASK_PRIORITY_NORMAL, 0, NULL, 4*1024);
        }
        else
        {
            os_task_create("amrnb", amrnb_decode_file_thread, (void*)amr, OS_TASK_PRIORITY_NORMAL, 0, NULL, 3*1024);
        }
    }
}