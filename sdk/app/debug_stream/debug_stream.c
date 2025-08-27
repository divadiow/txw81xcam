#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal_file.h"
extern void at_save_osd_thread(void *d);
#define FILTER_NAMES_NUM    (10)
static char *filter_names[FILTER_NAMES_NUM] = {NULL};
//保存多少张,用全局变量吧,监听的流越多,保存的越多
uint32_t g_save_num = 10;

struct os_task save_osd_task;
uint8_t osd_task_running = 0;
int32 demo_atcmd_save_osd(const char *cmd, char *argv[], uint32 argc)
{
    if(argc < 2)
    {
        os_printf("%s argc too small:%d,should more 2 arg\n",__FUNCTION__,argc);
        return 0;
    }
    
    if(!osd_task_running)
    {
        g_save_num = os_atoi(argv[0]);
        os_printf("%s:%d start,g_save_num:%d\n",__FUNCTION__,__LINE__,g_save_num);
        osd_task_running = 1;
        for(int i=1;i<argc;i++)
        {
            filter_names[i-1] = (char*)os_malloc(strlen(argv[i])+1);
            memcpy(filter_names[i-1] ,argv[i],strlen(argv[i])+1);
            if(i>=FILTER_NAMES_NUM)
            {
                os_printf("%s:%d overflow\n",__FUNCTION__,__LINE__);
            }
        }
        OS_TASK_INIT("at_save_osd", &save_osd_task, at_save_osd_thread, (uint32)filter_names, OS_TASK_PRIORITY_NORMAL, 1024);  
    }
    else
    {
        os_printf("%s:%d fail\n",__FUNCTION__,__LINE__);
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
            s->priv = priv;
            enable_stream(s,1);
		}
		break;
        case STREAM_RECV_FILTER_DATA:
        break;
        case STREAM_FILTER_DATA://R_OSD_ENCODE
        {
			struct data_structure *data = (struct data_structure *)priv;
            //os_printf("%s:%d\tdata:%X\n",__FUNCTION__,__LINE__,data);
            //只是比较前缀,
			if(s->priv )
			{
                res = 1;
                char **filter_names = (char **)s->priv;
                for(int i=0;i<FILTER_NAMES_NUM;i++)
                {
                    
                    if(filter_names[i] == NULL)
                    {
                        break;
                    }
                    if(memcmp(filter_names[i],data->s->name,strlen(filter_names[i])) == 0)
                    {
                        res = 0;
                        break;
                    }
                }
				
				break;
			}
        }
        break;
		case STREAM_OPEN_FAIL:
		break;
        case STREAM_CLOSE_EXIT:
            if(s->priv)
            {
                char **filter_names = (char **)s->priv;
                for(int i=0;i<FILTER_NAMES_NUM;i++)
                {
                    if(filter_names[i])
                    {
                        os_printf("%s:%d free name:%s\n",__FUNCTION__,__LINE__,filter_names[i]);
                        os_free(filter_names[i]);
                        filter_names[i] = NULL;
                    }
                    else
                    {
                        break;
                    }
                }
                s->priv = NULL;
            }
        break;
		default:
			//默认都返回成功
		break;
	}
	return res;
}






void at_save_osd_thread(void *d)
{
    struct data_structure *get_f = NULL;
    stream *s = NULL;
    uint32_t flen;
    void *fp = NULL;
    char filename[64] = {0};
    uint32_t err_count = 0;
    void *buf;
    os_printf("%s:%d start\n",__FUNCTION__,__LINE__);

    int save_count = 0;
    s = open_stream_available(R_DEBUG_STREAM,0,10,opcode_func,d);
    if(!s)
    {
        goto at_save_osd_thread_end;
    }

    while(1)
    {
        get_f = recv_real_data(s);
        if(get_f)
        {
            err_count = 0;
            os_sprintf(filename,"0:%s_%08d.rgb",get_f->s->name,(uint32_t)os_jiffies()%99999999);
            os_printf("filename:%s\n",filename);
            fp = osal_fopen(filename,"wb+");
            if(!fp)
            {
                goto at_save_osd_thread_end;
            }
            buf = get_stream_real_data(get_f);
            flen = get_stream_real_data_len(get_f);
            os_printf("buf:%X\tflen:%X\n",buf,flen);
            osal_fwrite(buf, flen, 1, fp);
            free_data(get_f);
            get_f = NULL;
            osal_fclose(fp);
            save_count++;
            if(save_count > g_save_num)
            {
                break;
            }
            

        }
        else
        {
            os_sleep_ms(1);
            if(err_count++ > 1000)
            {
                os_printf("%s:%d timeout\n",__FUNCTION__,__LINE__);
                goto at_save_osd_thread_end;
            }
        }
        
    }
    at_save_osd_thread_end:
    if(get_f)
    {
        free_data(get_f);
        get_f = NULL;
    }
    
    if(s)
    {
        close_stream(s);
    }
    os_printf("%s:%d exit\r\n",__FUNCTION__,__LINE__);
    osd_task_running = 0;
}