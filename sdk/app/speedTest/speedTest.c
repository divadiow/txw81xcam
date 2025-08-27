#include "socket_module.h"
#include "lib/net/eloop/eloop.h"
#include "typesdef.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "event.h"
#include "osal/string.h"


#define UDP_RCV_SIZE 	1460
#define UDP_SND_SIZE 	UDP_RCV_SIZE

static uint8_t speedTest_buf[UDP_RCV_SIZE];

//udp定时发送结构
struct UdpSpeed
{
	struct sockaddr_in remote_addr;
	struct event *timer_e;
	struct event *udp_e;
	int fd;
};



static void udp_speed_test(int fd)
{
	int len;
	unsigned char *udp_data;
	struct sockaddr remote_addr;
	int retval;
	int ret;
	uint64 time_if;
	int len_total = 0;
	uint8_t *pt;
	uint8_t type_eloop = 0;
	
	retval = 16;
	
	udp_data = (unsigned char*)malloc(UDP_RCV_SIZE );
	time_if = os_jiffies();
	printf("udp_test_bandwidth start...\n");
	while(1){
		//len = sendto(send_socket_fd, (unsigned char *)udp_data, UDP_XMIT_SIZE, 0, &udp_remote_client, sizeof(struct sockaddr));
		type_eloop = 0;
		printf("test recv\r\n");
		while(type_eloop < 10){
			len = recvfrom(fd, udp_data, UDP_RCV_SIZE, MSG_WAITALL, &remote_addr, (socklen_t*)&retval);	
			len_total += len;
			if((os_jiffies() - time_if) > 2000){
				printf("recvfrom loop:%d\r\n",type_eloop);
				type_eloop++;
				time_if = os_jiffies();
				pt = (uint8_t*)&len_total;
				udp_data[0] =  pt[0]&0x0f;
				udp_data[1] = (pt[0]>>4)&0x0f;
				udp_data[2] = pt[1]&0x0f;
				udp_data[3] = (pt[1]>>4)&0x0f;
				udp_data[4] = pt[2]&0x0f;
				udp_data[5] = (pt[2]>>4)&0x0f;
				udp_data[6] = pt[3]&0x0f;
				udp_data[7] = (pt[3]>>4)&0x0f;			
				udp_data[8] = 100;
				printf("len_total::%x	 %x%x%x%x\r\n",len_total,udp_data[0],udp_data[1],udp_data[2],udp_data[3]);	
				do{
					ret = sendto(fd, (unsigned char *)udp_data, 12, 0, &remote_addr, sizeof(struct sockaddr));			
				}while(ret<0);	

				do{
					ret = sendto(fd, (unsigned char *)udp_data, 12, 0, &remote_addr, sizeof(struct sockaddr));			
				}while(ret<0);

				do{
					ret = sendto(fd, (unsigned char *)udp_data, 12, 0, &remote_addr, sizeof(struct sockaddr));			
				}while(ret<0);				
	
				len_total = 0;
			}
		}
		printf("test send\r\n");

		time_if = os_jiffies();
		//for(type_eloop = 0;type_eloop < 10;type_eloop++)
		//{
			type_eloop = 0;
			while(type_eloop < 10){
				udp_data[8] = 101;
				sendto(fd, (unsigned char *)udp_data, UDP_RCV_SIZE, 0, &remote_addr, sizeof(struct sockaddr));
				if((os_jiffies() - time_if) > 2000){
					
					printf("sendto loop:%d\r\n",type_eloop);
					time_if = os_jiffies();
					type_eloop++;
					udp_data[8] = 102;
					do{
						ret = sendto(fd, (unsigned char *)udp_data, 12, 0, &remote_addr, sizeof(struct sockaddr));			
					}while(ret<0);					
				}
				
			}		
		//}
		udp_data[8] = 103;

		do{
			ret = sendto(fd, (unsigned char *)udp_data, 12, 0, &remote_addr, sizeof(struct sockaddr));			
		}while(ret<0);		
		do{
			ret = sendto(fd, (unsigned char *)udp_data, 12, 0, &remote_addr, sizeof(struct sockaddr));			
		}while(ret<0);
		do{
			ret = sendto(fd, (unsigned char *)udp_data, 12, 0, &remote_addr, sizeof(struct sockaddr));			
		}while(ret<0);		
	}

	printf("\n\n\n [ENTER] %s, Line = %d \n\n\n",__func__, __LINE__);
}


static void udp_server_speed_accept(struct event_info *e, void *d)
{
	char *tcp_rcv_buff;
	struct sockaddr remote_addr;
	int retval, size;
	retval = 16;
	int udp_server_fd = (int)d;
	
	tcp_rcv_buff = (char*)malloc(UDP_RCV_SIZE + 4);

	printf("udp server fd: %d\n", udp_server_fd);
	size = recvfrom(udp_server_fd, tcp_rcv_buff, UDP_RCV_SIZE, MSG_DONTWAIT, &remote_addr, (socklen_t*)&retval);
	printf("size:%d %s\n",size,tcp_rcv_buff);
	printf("udp client ip: %x\n", ((struct sockaddr_in *)&remote_addr)->sin_addr.s_addr);
	if(size > 0) {
			udp_speed_test(udp_server_fd);
	}
	
	free(tcp_rcv_buff);
}



void udp_client_speed_send(struct event_info *e, void *d)
{
	static char *send_buf = NULL;
	int size;
	if(!send_buf)
	{
		send_buf = (char*)malloc(UDP_RCV_SIZE);
		memset(send_buf,0,UDP_RCV_SIZE);
	}
    uint32_t  sec, usec,spcount;
	static uint32_t pcount = 0;
	++pcount;
	uint64 times = os_jiffies();
	sec = times/1000;
	
	sec = htonl(times/1000);
	usec = htonl((times%1000)*1000);
	spcount = htonl(pcount);


	memcpy(send_buf, &spcount, sizeof(spcount));
	memcpy(send_buf+4, &sec, sizeof(sec));
	memcpy(send_buf+8, &usec, sizeof(usec));
	


	
	struct UdpSpeed *udp_struct = (struct UdpSpeed *)d;
	struct sockaddr *remote_addr = (struct sockaddr*)&udp_struct->remote_addr;
	int udp_client_fd = (int)udp_struct->fd;
	size = sendto(udp_client_fd, (unsigned char *)send_buf, UDP_RCV_SIZE, 0, remote_addr, sizeof(struct sockaddr));	
	if(size < 0)
	{
		pcount--;
	}
}

void udp_client_speed_recv(struct event_info *e, void *d)
{
	static char *send_buf = NULL;
	int size;
	if(!send_buf)
	{
		send_buf = (char*)malloc(UDP_RCV_SIZE);
		memset(send_buf,0,UDP_RCV_SIZE);
	}
	struct sockaddr remote_addr;
	int retval;
	retval = 16;

	struct UdpSpeed *udp_struct = (struct UdpSpeed *)d;
	int udp_client_fd = (int)udp_struct->fd;
	size = recvfrom(udp_client_fd, send_buf, UDP_RCV_SIZE, MSG_DONTWAIT, &remote_addr, (socklen_t*)&retval);
	if(size > 0)
	{
		printf("................................size:%d\tsendbuf[0]:%X\n",size,send_buf[0]);
	}


}


void speedTest_udp_server(int port)
{

	int udp_server_fd;
	//uint16_t port = 8060;
	
	printf("> init udp server\nudp port:%d\n", port);

	udp_server_fd = creat_udp_socket(port);
	eloop_add_fd( udp_server_fd, EVENT_READ, EVENT_F_ENABLED, (void*)udp_server_speed_accept, (void*)udp_server_fd );
}





void speedTest_udp_client(const char *ip,int port)
{
	int udp_client_fd;
	udp_client_fd = creat_udp_socket(0);
	struct UdpSpeed *udp_struct = (struct UdpSpeed*)malloc(sizeof(struct UdpSpeed));
	struct sockaddr_in *addrServer = &udp_struct->remote_addr;
	memset(addrServer,0,sizeof(struct sockaddr_in));
	addrServer->sin_family=AF_INET;
	addrServer->sin_addr.s_addr=inet_addr(ip);
	addrServer->sin_port=htons(port);
	udp_struct->fd = udp_client_fd;
	//shutdown(udp_client_fd,SHUT_RD);

	//csi_kernel_task_new((k_task_entry_t)udp_client_speed_send, "speed", udp_struct, 1, 0, NULL, 1024, &eloop_task_handle);
	eloop_add_fd( udp_client_fd, EVENT_WRITE, EVENT_F_ENABLED, (void*)udp_client_speed_send, (void*)udp_struct );
	eloop_add_fd( udp_client_fd, EVENT_READ, EVENT_F_ENABLED, (void*)udp_client_speed_recv, (void*)udp_struct );
}



/****************************************************************TCP*****************************************************/

void TcpTX_event(struct event *ei, void *d)
{
	int fd = (int)d;
	int res;
	static uint8_t *send_buf = NULL;


	if(!send_buf)
	{
		send_buf = (uint8_t*)malloc(UDP_RCV_SIZE*10);
		
		if(!send_buf)
		{
			printf("%s no enough space\n",__FUNCTION__);
			eloop_remove_event(ei);
			if(fd > 0)
			{
				closesocket(fd);
			}
				send_buf = NULL;
		}

	}

	res = send(fd,send_buf,UDP_RCV_SIZE*10,0);

	if(res < 0)
	{
		printf("%s res:%d\n",__FUNCTION__,res);
		eloop_remove_event(ei);
		if(fd > 0)
		{
			closesocket(fd);
		}
		if(send_buf)
		{
			free(send_buf);
			send_buf = NULL;
		}
	}
	
}


void TcpTX_event2(void *d)
{
	int fd = (int)d;
	int res;
	uint8_t *send_buf = (uint8_t*)malloc(UDP_RCV_SIZE);


	if(!send_buf)
	{
		if(fd > 0)
		{
			closesocket(fd);
		}
		return;

	}
	while(1)
	{
		res = send(fd,send_buf,UDP_RCV_SIZE,0);

		if(res < 0)
		{

			if(fd > 0)
			{
				closesocket(fd);
			}
			free(send_buf);
			break;

		}
	}
	printf("%s end\n",__FUNCTION__);
	
}


void tcp_server_accept(struct event_info *e, void *d)
{
	struct sockaddr_in addr;
	socklen_t namelen = sizeof(addr);
	int server_fd = (int)d;
	int client_fd;
    printf("%s:%d\tserver_fd:%d\n",__FUNCTION__,__LINE__,server_fd);
    client_fd = accept(server_fd,(struct sockaddr *)&addr,&namelen);
	if(client_fd > 0)
	{
		//k_task_handle_t eloop_task_handle;
		//添加读事件
		//eloop_add_fd( client_fd , EVENT_WRITE, EVENT_F_ENABLED, (void*)TcpTX_event, (void*)client_fd);
		//csi_kernel_task_new((k_task_entry_t)TcpTX_event2, "tcpTX", (void*)client_fd, 10, 0, NULL, 2048, &eloop_task_handle);
		os_task_create("tcpTX", TcpTX_event2, (void*)client_fd, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);


	}

}

void speedTest_tcp_server(int port)
{

	int tcp_server_fd;
	//uint16_t port = 8060;
	
	printf("%s port:%d\n",__FUNCTION__,port);

	tcp_server_fd = creat_tcp_socket(port);
	printf("%s fd:%d\n",__FUNCTION__,tcp_server_fd);
	if(tcp_server_fd>=0)
	{
		listen(tcp_server_fd,3);
		eloop_add_fd( tcp_server_fd, EVENT_READ, EVENT_F_ENABLED, (void*)tcp_server_accept, (void*)tcp_server_fd );
	}
}









void tcp_rx_read(struct event_info *e, void *d)
{
	int t_socket = (int)d;
	int nRecev;
	nRecev = recv(t_socket,speedTest_buf,UDP_RCV_SIZE,0);
	if(nRecev <= 0)
	{
		closesocket(t_socket);
		eloop_remove_event(e);
		os_printf("%s exit\n",__FUNCTION__);
	}
}



void tcp_rx_server_accept(struct event_info *e, void *d)
{
	struct sockaddr_in addr;
	socklen_t namelen = sizeof(addr);
	int server_fd = (int)d;
	int client_fd;
    printf("%s:%d\tserver_fd:%d\n",__FUNCTION__,__LINE__,server_fd);
    client_fd = accept(server_fd,(struct sockaddr *)&addr,&namelen);
	if(client_fd > 0)
	{
		eloop_add_fd( client_fd, EVENT_READ, EVENT_F_ENABLED, (void*)tcp_rx_read, (void*)client_fd );
	}

}



void speedTest_tcp_rx_server(int port)
{

	int tcp_server_fd;
	//uint16_t port = 8060;
	
	printf("%s port:%d\n",__FUNCTION__,port);

	tcp_server_fd = creat_tcp_socket(port);
	printf("%s fd:%d\n",__FUNCTION__,tcp_server_fd);
	if(tcp_server_fd>=0)
	{
		listen(tcp_server_fd,3);
		eloop_add_fd( tcp_server_fd, EVENT_READ, EVENT_F_ENABLED, (void*)tcp_rx_server_accept, (void*)tcp_server_fd );
	}
}
























//udp定时发送结构
struct UdpToIp
{
	struct sockaddr_in remote_addr;
	struct event *timer_e;
	struct event *udp_e;
	int timer;
	int fd;
};
/*********************************************************************************************
定时用udp发送数据
*********************************************************************************************/


static void Udp_to_ip_send(struct event *ei, void *d)
{
	struct UdpToIp *udp_struct = (struct UdpToIp *)d;
	int serverfd = udp_struct->fd;
	struct sockaddr *remote_addr = (struct sockaddr *)&udp_struct->remote_addr;
	int retval, size;
	retval = 16;
	static uint8_t *udp_data = NULL;
	if(!udp_data)
	{
	 udp_data = (uint8_t*)malloc(UDP_RCV_SIZE );
	}
	size = sendto(serverfd,udp_data, UDP_RCV_SIZE, 0, remote_addr, sizeof(struct sockaddr));
	//printf("%s:%d\tsize:%d\tEINPROGRESS:%d\n",__FUNCTION__,__LINE__,size,EINPROGRESS);
	eloop_set_event_enabled(ei,EVENT_F_DISABLED);
}
static void Udp_to_ip_timer(void *ei, void *d)
{
	struct UdpToIp *udp_struct = (struct UdpToIp *)d;
	eloop_set_event_enabled(udp_struct->udp_e,EVENT_F_ENABLED);
}


static void udp_tx_start_listen(struct event *ei, void *d)
{
	struct UdpToIp *udp_struct = (struct UdpToIp *)d;
	int t_socket = (int)udp_struct->fd;
	int retval = sizeof(struct sockaddr_in);
	struct sockaddr_in remote_addr;
	struct sockaddr_in *recv_addr = &udp_struct->remote_addr;
	int nRecev;
	nRecev = recvfrom(t_socket, (char*)speedTest_buf, sizeof(speedTest_buf)-1, 0, (struct sockaddr*)&remote_addr, (socklen_t*)&retval);
	if(nRecev > 0)
	{
		speedTest_buf[nRecev] = 0;
		if(strcmp((const char*)speedTest_buf,"start") == 0 )
		{

			eloop_set_event_enabled(udp_struct->timer_e,1);
			memcpy(recv_addr,&remote_addr,sizeof(remote_addr));
		}
		else if(strcmp((const char *)speedTest_buf,"stop") == 0 )
		{
			eloop_set_event_enabled(udp_struct->timer_e,0);
			eloop_set_event_enabled(udp_struct->udp_e,0);
			os_printf("%s pause udp_tx\n",__FUNCTION__);
		}
		
	}
	return;
}

//定时向udp发送数据
void Udp_to_ip(char *ip,int port,int timer)
{
	int t_socket;
	t_socket = creat_udp_socket(30303);
	struct UdpToIp *udp_struct = (struct UdpToIp*)malloc(sizeof(struct UdpToIp));
	struct sockaddr_in *addrServer = &udp_struct->remote_addr;
	memset(addrServer,0,sizeof(struct sockaddr_in));;
	addrServer->sin_family=AF_INET;
	addrServer->sin_addr.s_addr=inet_addr(ip);
	addrServer->sin_port=htons(port);
	udp_struct->timer = timer;
	udp_struct->fd = t_socket;
	

	//添加事件,但是不启动,然后接收一个udp包数据后才开始发送
	udp_struct->udp_e = eloop_add_fd( t_socket, EVENT_WRITE, EVENT_F_NOACTION, (void*)Udp_to_ip_send, (void*)udp_struct );
	udp_struct->timer_e = eloop_add_timer(timer,EVENT_F_NOACTION,Udp_to_ip_timer,(void*)udp_struct);
	eloop_add_fd( t_socket, EVENT_READ, EVENT_F_ENABLED, (void*)udp_tx_start_listen, (void*)udp_struct );
}









static uint32_t speed_count = 0;

static void udp_rx(struct event *ei, void *d)
{
	int t_socket = (int)d;
	unsigned int retval = sizeof(struct sockaddr_in);
	struct sockaddr_in remote_addr;
	int nRecev;
	nRecev = recvfrom(t_socket, (char*)speedTest_buf, sizeof(speedTest_buf), 0, (struct sockaddr*)&remote_addr, (socklen_t*)&retval);
	if(nRecev > 0)
	{
		speed_count += nRecev;
	}
	return;
}
static void udp_rx_msg_timer(void *ei, void *d)
{
	static uint32_t start_time = 0;
	if(start_time == 0)
	{
		start_time = os_jiffies();
		return;
	}

	uint32_t speed = 0;
	uint32 time = (os_jiffies()-start_time);
	if(time){
		speed = speed_count/time;
		os_printf("speed:%d KB/s\n",speed);
	}else{
		os_printf("!!!!!!!!!!!!!!!!!!!!!time err %d\n",time);
	}


	//os_printf("count:%d\ttime:%d\t%d\tspeed:%d KB/s\n",speed_count,(os_jiffies()-start_time),speed,speed_count/(os_jiffies()-start_time));
	start_time = os_jiffies();
	speed_count = 0;
}

//统计接收udp的数据,timer定时打印
void Udp_rx_server(int port,int timer)
{
	int t_socket;
	t_socket = creat_udp_socket(port);

	

	//添加事件
	eloop_add_fd( t_socket, EVENT_READ, EVENT_F_ENABLED, (void*)udp_rx, (void*)t_socket );
	eloop_add_timer(timer,EVENT_F_ENABLED,udp_rx_msg_timer,(void*)NULL);

}


int get_tcp_connect(char *ip,int port)
{
	int t_socket = socket(AF_INET, SOCK_STREAM, 0);
 
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);


	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	fcntl(t_socket, F_SETFL, O_NONBLOCK);
	if (setsockopt(t_socket, SOL_SOCKET, LWIP_SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		// 错误处理
	}
    //连接服务器
	os_printf("ip:%s\tport:%d timeout\n",ip,port);
    int res = connect_tmo(t_socket, (struct sockaddr*)&server, sizeof(server),1000);
	os_printf("%s res:%d\n",__FUNCTION__,res);
	if(res <  0)
	{
		closesocket(t_socket);
		return -1;
	}

	int flag = 1; // 1 表示禁用 Nagle 算法，0 表示启用
	setsockopt(t_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
	return t_socket;

}