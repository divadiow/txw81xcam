#include "udpFPV.h"
#include "sys_config.h"
#include "typesdef.h"
#include "osal/task.h"
#include "osal/string.h"
#include "socket_module.h"
#include "lib/net/eloop/eloop.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "jpgdef.h"
#include "utlist.h"
#include "video_app.h"
#ifdef CONFIG_SLEEP
#include "lib/lmac/lmac_dsleep.h"
#endif

struct udp_fpv_s
{
    //struct sockaddr_in remote_addr;
    uint32_t last_heat_time;
    struct os_task task_hdl;
    //void *e;
    stream *s;
    int fd;
    uint32_t running;
    uint8_t recv_buf[64];
    uint8_t send_buf[1500];

};

struct udp_jpg_s
{
    uint8_t type;
    union
    {
        uint8_t order;
        uint8_t status;
    };
    
    uint8_t total;
    uint8_t num;
};

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





static void udp_fpv_task(void *d)
{
	
    struct udp_fpv_s *udp_fpv = (struct udp_fpv_s *)d;
    int t_socket = (int)udp_fpv->fd;
	int retval = sizeof(struct sockaddr_in);
	struct sockaddr_in remote_addr;
	int nRecev;
    int order = 1,num = 0;
    struct data_structure *get_f;
    struct stream_jpeg_data_s *dest_list,*dest_list_tmp;
    struct stream_jpeg_data_s *el,*tmp;
    uint32_t already_send_size = 0;
    uint32_t node_len; 
    uint32_t jpg_size; 
    uint32_t send_size;
    uint8_t *jpeg_buf_addr ;
    struct system_sleep_param args;
    memset(&args,0,sizeof(struct system_sleep_param));
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    uint8_t *send_buf = udp_fpv->send_buf;
    struct udp_jpg_s *udp_jpg = (struct udp_jpg_s*)send_buf;
    
    struct udp_jpg_s *udp_status = (struct udp_jpg_s *)udp_fpv->recv_buf;
    while(1)
    {
        nRecev = recvfrom(t_socket, (char*)udp_fpv->recv_buf, sizeof(udp_fpv->recv_buf), MSG_DONTWAIT, (struct sockaddr*)&remote_addr, (socklen_t*)&retval);
        //接收数据来判断是否打开图传或者是否进入休眠状态
        if(nRecev > 0)
        {
            udp_fpv->recv_buf[nRecev] = 0;
            os_printf("udp fpv recv buf:%s\tudp_fpv->running:%d\n",udp_fpv->recv_buf,udp_fpv->running);
            //启动任务
            if(udp_fpv->recv_buf[0] == 'W')
            {
                if(!udp_fpv->running)
                {
                    
                    //OS_TASK_INIT("udp_fpv", &udp_fpv->task_hdl, user_eloop_run, NULL, OS_TASK_PRIORITY_NORMAL, 1024);
                    os_printf("wake up fpv task\n");
                    //创建对应的流
                    //打开mjpeg
                    
                    udp_fpv->s = open_stream_available(R_AT_SAVE_PHOTO,0,8,opcode_func,NULL);
                    if(udp_fpv->s)
                    {
                        udp_fpv->running = 1;
                        start_jpeg();
                    }

                    os_printf("%s:%d\ts:%X\n",__FUNCTION__,__LINE__,udp_fpv->s);
                }
                else
                {
                    os_printf("fpv task is already running\n");
                }

                //收到数据,就返回一个当前状态给到主机
                udp_status->type = 2;
                udp_status->status = udp_fpv->running;
                udp_status->total = 0;
                udp_status->num = 0;
                //发送5字节,实际只有2字节是有用的
                sendto(t_socket, (unsigned char *)udp_status, 5, 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
                
            }
            //需要关闭任务,等待任务删除
            else if(udp_fpv->recv_buf[0] == 'S')
            {
                
                if(udp_fpv->running)
                {
                    //停止mjpeg
                    stop_jpeg();
                    //关闭流
                    close_stream(udp_fpv->s);
                    udp_fpv->running = 0;

                    

                    //收到数据,就返回一个当前状态给到主机
                    udp_status->type = 2;
                    udp_status->status = udp_fpv->running;
                    udp_status->total = 0;
                    udp_status->num = 0;
                    //发送5字节,实际只有2字节是有用的
                    sendto(t_socket, (unsigned char *)udp_status, 5, 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));

                    //这里发送一下即将休眠的的命令,告诉对方,即将休眠了
                    
                    
                    udp_status->type = 3;
                    //多发送几次
                    sendto(t_socket, (unsigned char *)udp_status, 5, 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
                    sendto(t_socket, (unsigned char *)udp_status, 5, 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
                    sendto(t_socket, (unsigned char *)udp_status, 5, 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));

                    //500ms为资源释放
                    os_sleep_ms(500);
                    
                    //进入休眠
                    system_sleep(1, &args);
                    
                }
                //如果已经停止了,则返回状态给到客户端,告诉它,我们已经停止了
                else
                {
                    //收到数据,就返回一个当前状态给到主机
                    udp_status->type = 2;
                    udp_status->status = udp_fpv->running;
                    udp_status->total = 0;
                    udp_status->num = 0;
                    //发送5字节,实际只有2字节是有用的
                    sendto(t_socket, (unsigned char *)udp_status, 5, 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
                }

            }
        }

        //尝试图传发送
        if(udp_fpv->running && udp_fpv->s)
        {
            get_f = recv_real_data(udp_fpv->s);
            if(get_f)
            {
                //图像发送
                dest_list = (struct stream_jpeg_data_s *)get_stream_real_data(get_f);
                dest_list_tmp = dest_list;

                node_len = (uint32_t)stream_data_custom_cmd_func(get_f,CUSTOM_GET_NODE_LEN,NULL);
                jpg_size = (uint32_t) 	get_stream_real_data_len(get_f);
                //os_printf("GET_NODE_COUNT:%d\tjpg_size:%d\n",GET_NODE_COUNT(get_f),jpg_size);
                udp_jpg->total = GET_NODE_COUNT(get_f);
                udp_jpg->order = order++;
                udp_jpg->type = 1;
                num = 0;
                already_send_size = 0;

                //printf("udp_jpg_order:%d\n",udp_jpg->order);
                LL_FOREACH_SAFE(dest_list,el,tmp)
                {
                    if(el == dest_list_tmp)
                    {
                        //先发送图片的size
                        continue;
                    }


                    if(already_send_size + node_len > jpg_size)
                    {
                        send_size = jpg_size - already_send_size;
                    }
                    else
                    {
                        send_size = node_len;
                    }
                    //进行图片发送,
                    if(send_size)
                    {
                        jpeg_buf_addr = (uint8_t*)stream_data_custom_cmd_func(get_f,CUSTOM_GET_NODE_BUF,el->data);
                        num++;
                        udp_jpg->num = num;
                        memcpy(send_buf+sizeof(struct udp_jpg_s),jpeg_buf_addr,send_size);
						int result;
                        result = sendto(t_socket, (unsigned char *)send_buf, send_size+sizeof(struct udp_jpg_s), 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));	
                        //os_printf("send_size:%d\tresult:%d\n",send_size,result);
                    }

                    already_send_size += send_size;
                    //os_printf("already_send_size:%d\n",already_send_size);
                    stream_data_custom_cmd_func(get_f,CUSTOM_DEL_NODE,el);
                }

                free_data(get_f);
                get_f = NULL;


            }
        }
        os_sleep_ms(1);
    }

	return;
}
//统计接收udp的数据,timer定时打印
void udp_fpv_server(int port)
{
	int t_socket;
	t_socket = creat_udp_socket(port);
    struct udp_fpv_s *udp_fpv = (struct udp_fpv_s*)os_malloc(sizeof(struct udp_fpv_s));
    memset(udp_fpv,0,sizeof(struct udp_fpv_s));
    udp_fpv->fd = t_socket;
	//添加事件
	//udp_fpv->e = eloop_add_fd( t_socket, EVENT_READ, EVENT_F_ENABLED, (void*)udp_rx, (void*)udp_fpv );
    OS_TASK_INIT("udp_fpv", &udp_fpv->task_hdl, udp_fpv_task, udp_fpv, OS_TASK_PRIORITY_NORMAL, 1024);
    
}