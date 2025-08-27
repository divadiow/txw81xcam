/***************************************************
    该demo主要是使用AT命令拍一张照片,前提要将jpeg打开
***************************************************/
#include "sys_config.h"	
#include "tx_platform.h"
#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal_file.h"
#include "video_app/video_app.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/etharp.h"
#include "utlist.h"
#include "jpgdef.h"
#include "lib/lcd/lcd.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"
#include "dhcpd_eloop/dhcpd_eloop.h"
#include "lib/common/sysevt.h"
#include "syscfg.h"
#include <csi_kernel.h>

#include "video_app_bbm.h"

#define  HALF_DUPLEX    1
#define  FULL_DUPLEX    0

#define PROTOCOL_SERVER        1
#define PROTOCOL_CLIENT        0

#define PROTOCOL_RECORD   0
#define ENABLE_CACHING   1

int udp_protocol_fd   =  - 1;
int handle_protocol_fd   =  - 1;

k_task_handle_t handle_task_lcd;
k_task_handle_t handle_task_recv;
k_task_handle_t handle_task_send;
k_task_handle_t protocol_task_recv;
k_task_handle_t protocol_task_send;
char *s_ptr_protocol = NULL;
char *r_ptr_protocol = NULL;
#if PROTOCOL_CLIENT || FULL_DUPLEX
char s_buf_for_protocol[2048]__attribute__ ((aligned(4)));
#endif
#if PROTOCOL_SERVER || FULL_DUPLEX
char r_buf_for_protocol[2048]__attribute__ ((aligned(4)));
#endif
uint8_t connect_protocol = 1;
uint8_t s_los_map[24];
uint8_t r_los_map[24];
uint8_t s_frame_id = 0;
uint8_t r_frame_id = 0;
uint8_t flash_lcd = 0;

uint8_t mcs_send     = 2;
uint8_t frmtype_send = 3;
uint8_t txmode = 0;          //0:typ0 mcs0   1: type0 mcs1   2: 5.5mbps   3:24 mclk
uint8_t send_mode = 0;    //0:typ0 mcs0   1: type0 mcs1   2: 5.5mbps   3:24 mclk

extern volatile uint8 scale2_finish ;
struct os_semaphore     r_connect_sem;
struct os_semaphore     s_connect_sem;

uint16_t rx_speed,tx_speed;
uint8_t dispnum;

uint8_t stop_heart_packet = 0;
typedef struct
{     
	uint32_t timeinfo;
	uint8_t  *addr;
	uint32_t data_len;
}room_msg;

#define PROOM_NUM 20  

#if LCD_EN 
room_msg proom[PROOM_NUM];
uint8_t proom_num = 0;
uint8 photo_decode_net[2][100*1024] __attribute__ ((aligned(4),section(".psram.src")));
uint8 photo_room[PROOM_NUM][30*1024] __attribute__ ((aligned(4),section(".psram.src")));
#endif

#if PROTOCOL_RECORD || LVGL_STREAM_ENABLE
#define STREAM_ROOM_NUM 4
bbm_net_jpeg_frame bbm_jpeg_frame[PROOM_NUM];
uint8 bbm_jpeg_room[STREAM_ROOM_NUM][30*1024] __attribute__ ((aligned(4),section(".psram.src")));
#endif

typedef struct
{     
	uint8_t  photo_state;   //bit7:start   bit6:end   bit[0-5]:node_num
	uint8_t  cnt;
	uint8_t  pack;
	uint8_t  mcs;
	uint32_t timeinfo;
	uint16_t tx_speed;
	uint8_t  frmtype;
	uint8_t  node_map[9];  //node has send,one bit for one node
}baby_head;

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

int usr_protocol_create(uint16_t port)
{
	int socket_c, err;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_len = sizeof(struct sockaddr_in);
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htons(INADDR_ANY);

	socket_c = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_c < 0)
	{
		printf("get socket err");
		return  - 1;
	} 

	err = bind(socket_c, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));

	if (err ==  - 1)
	{
		close(socket_c);
		return  - 1;

	}	
	return socket_c;
}

///////client
uint32_t addrip_return();
void read_handle_thread(){	
	int retval;
	int ret;
	//uint8_t itk = 0;
	uint8_t handlebuf[24];
	struct sockaddr remote_addr;
	retval = 16;

	while(1){
		ret = recvfrom (handle_protocol_fd, handlebuf, 24, 0, &remote_addr, (socklen_t*)&retval);
		connect_protocol = 1;
		if(handlebuf[0] == 1){

		}else if(handlebuf[0] == 2){
			//os_printf("buf:");
			//for(itk = 1;itk < 20;itk++){
			//	printf("%02x ",handlebuf[itk]);
				//if(handlebuf[itk] != 0){
				//	break;
				//}
			//}
			//printf("\r\n");
			memcpy(s_los_map,handlebuf,24);
			if(s_los_map[1] == s_frame_id)
				os_sema_up(&s_connect_sem);
		}
	}
}
///////client
extern struct system_status sys_status;
extern int32 dvp_set_baudrate(struct dvp_device *p_dvp, uint32 mclk);
void send_photo_thread()
{
	int  len;	
	struct sockaddr_in remote_addr;// = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	struct data_structure *get_f = NULL;
    stream *r_stream = NULL;
	uint32_t flen;
	uint16_t tx_speed = 0;
	uint32_t tx_data_count = 0;
	uint32_t old_time;
	uint32_t node_len;
	uint32_t snode_len;
	uint32_t sofset;
	uint8_t node_num;
	uint8_t jpg_num = 0;
	uint8_t count;
	uint8_t retry;
	uint8_t map_num,bit_num;
    struct stream_jpeg_data_s *dest_list;
    struct stream_jpeg_data_s *dest_list_tmp;
	struct stream_jpeg_data_s *el,*tmp;
	uint8_t *jpeg_buf_addr = NULL;
	baby_head *msg_head;
	int32  ret_sema        = 0;
	uint8_t lost_pack[20];
	uint8_t itk,jtk,ktk;
	uint8_t los_num;
	uint8_t mclk_set;
	struct dvp_device *dvp_dev;
	dvp_dev = (struct dvp_device *)dev_get(HG_DVP_DEVID);
	uint8 wait_cnt = 0;

    #if PROTOCOL_CLIENT
        #if 0
        while(1){
            if((uint32_t)sys_network_get_ip() == 0x101a8c0){
                os_sleep_ms(50);
                _os_printf("^");
            }else{
                break;
            }
        }
        #else
        while(!sys_status.wifi_connected){
            os_sleep_ms(10);
            if(wait_cnt++ >= 10){
                wait_cnt = 0;
                _os_printf("^");
            }
        }
        #endif
    #endif
	
    r_stream = open_stream_available(R_RTP_JPEG,0,8,opcode_func,NULL);
	old_time = os_jiffies();
	memset(&remote_addr,0,sizeof(struct sockaddr_in));
	remote_addr.sin_family=AF_INET;
#if PROTOCOL_SERVER
	remote_addr.sin_addr.s_addr=inet_addr("192.168.1.100");
#elif PROTOCOL_CLIENT
    remote_addr.sin_addr.s_addr=inet_addr("192.168.1.1");
#endif
	remote_addr.sin_port=htons(11011);
	msg_head = (baby_head *)s_ptr_protocol;

#if	LVGL_STREAM_ENABLE || LCD_EN
	start_jpg_stream(640,480,320,240);
#else
	start_jpeg();
#endif
	while(1){
		if(connect_protocol == 1){
			if((os_jiffies() - old_time) > 1000){
				old_time = os_jiffies();
				tx_speed = tx_data_count/1024;
				tx_data_count = 0;
			}else{
				if(os_jiffies() < old_time){  //os_jiffies 溢出处理
					old_time = os_jiffies();
				}
			}
			
			get_f = recv_real_data(r_stream);
			if(get_f){
				
				dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(get_f);
				dest_list_tmp = dest_list;
				flen = get_stream_real_data_len(get_f);
				snode_len = GET_NODE_LEN(get_f);
				node_len = 1308;//GET_NODE_LEN(get_f);
				memset(msg_head,0,sizeof(baby_head));
				node_num = (flen+node_len-1)/node_len;
				jpg_num++;
				
				if(jpg_num > 63)
					jpg_num = 0;

				//os_printf("send(%d   %d)..\r\n",flen,jpg_num);
				msg_head->photo_state = BIT(7)|jpg_num;
				msg_head->tx_speed = tx_speed;
				msg_head->timeinfo = (uint32)os_jiffies();
				msg_head->mcs      = mcs_send;
				msg_head->frmtype  = frmtype_send;
                
                #if 1
                    if(msg_head->mcs <= 1){
                        send_mode = 0;
                    }else if(msg_head->mcs <= 3){
                        send_mode = 1;
                    }else if(msg_head->mcs <= 5){
                        send_mode = 2;
                    }else{
                        send_mode = 3;
                    }
                #else
                
				if((msg_head->frmtype == 0)&&(msg_head->mcs == 0)){
					send_mode = 0;
				}else if((msg_head->frmtype == 0)&&(msg_head->mcs == 1)){
					send_mode = 1;
				}else if(((msg_head->frmtype == 0)&&(msg_head->mcs == 2)) || 
						 ((msg_head->frmtype == 1)&&(msg_head->mcs == 0))  || 
						 ((msg_head->frmtype == 2)&&(msg_head->mcs == 0)))
				{
					send_mode = 2;
				}else{
					send_mode = 3;
				}
				#endif
                
				count = 0;
				msg_head->pack        = node_num;
				LL_FOREACH_SAFE(dest_list,el,tmp){
					if(dest_list_tmp == el)
					{
						continue;
					}
					//读取完毕删除
					//图片保存起来
					s_frame_id = jpg_num;
					if(flen)
					{
						jpeg_buf_addr = (uint8_t *)GET_JPG_SELF_BUF(get_f,el->data);
						for(sofset = 0;sofset < snode_len;sofset=sofset+node_len){
							
							if(flen >= node_len)
							{
								flen -= node_len;
								map_num = count/8;
								bit_num = count%8;
								msg_head->node_map[8-map_num] |= BIT(bit_num);
								if(count){
									msg_head->photo_state = jpg_num;
								}
								msg_head->cnt = count;
							
								
								count++;
								hw_memcpy(s_ptr_protocol+20,(char*)jpeg_buf_addr+sofset,node_len);
								if(flen == 0){
									msg_head->photo_state = BIT(6)|jpg_num;
								}
								len = sendto(udp_protocol_fd, s_ptr_protocol, node_len+20, MSG_DONTWAIT, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
								if(len == -1){
									os_printf("send err:%d\r\n",count);
								}
								if(send_mode == 0){
									os_sleep_ms(40);
								}else if(send_mode == 1){
									os_sleep_ms(10);
								}else if(send_mode == 2){
									os_sleep_ms(2);
								}else if(send_mode == 3){
									os_sleep_ms(1);
								}
								tx_data_count += (node_len+20);
							}
							else
							{
								map_num = count/8;
								bit_num = count%8;
								msg_head->node_map[8-map_num] |= BIT(bit_num);
								msg_head->cnt = count;
								count++;
								hw_memcpy(s_ptr_protocol+20,(char*)jpeg_buf_addr+sofset,flen);
								msg_head->photo_state = BIT(6)|jpg_num;
								len = sendto(udp_protocol_fd, s_ptr_protocol, flen+20, MSG_DONTWAIT, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
								if(len == -1){
									os_printf("send err:%d\r\n",count);
								}
								if(send_mode == 0){
									os_sleep_ms(40);
								}else if(send_mode == 1){
									os_sleep_ms(10);
								}else if(send_mode == 2){
									os_sleep_ms(2);
								}else if(send_mode == 3){
									os_sleep_ms(1);
								}							
								tx_data_count += (flen+20);
								flen = 0;							
								//os_printf("-");
								break;
							}
						}
					}
				
					//DEL_JPEG_NODE(get_f,el);				
				}

				if(send_mode == 3){
					if(mclk_set != 12){
						dvp_set_baudrate(dvp_dev,12000000);
					}
					mclk_set = 12;
					ret_sema = os_sema_down(&s_connect_sem, 20);
				}else if(send_mode == 2){
					if(mclk_set != 10){
						dvp_set_baudrate(dvp_dev,10000000);
					}					
					mclk_set = 10;
					ret_sema = os_sema_down(&s_connect_sem, 60);
				}else if(send_mode == 1){
					if(mclk_set != 8){
						dvp_set_baudrate(dvp_dev,8000000);
					}					
					mclk_set = 8;
					ret_sema = os_sema_down(&s_connect_sem, 60);
				}else{
                    if(mclk_set != 6){
						dvp_set_baudrate(dvp_dev,6000000);
					}					
					mclk_set = 6;
					ret_sema = os_sema_down(&s_connect_sem, 60);
                }
				//printf("msg_head->mcs:%d\r\n",msg_head->mcs);
#if 1				
				if(ret_sema){
					//重发
					
					dest_list = dest_list_tmp;
					count = 0;
					jtk   = 0;
					flen = get_stream_real_data_len(get_f);
					memset(msg_head,0,sizeof(baby_head));
					msg_head->tx_speed = tx_speed;
					msg_head->pack        = node_num;
					msg_head->timeinfo = (uint32)os_jiffies();
					msg_head->mcs      = mcs_send;
					msg_head->frmtype  = frmtype_send;

                    #if 1
                        if(msg_head->mcs <= 1){
                            send_mode = 0;
                        }else if(msg_head->mcs <= 3){
                            send_mode = 1;
                        }else if(msg_head->mcs <= 5){
                            send_mode = 2;
                        }else{
                            send_mode = 3;
                        }
                    #else
					if((msg_head->frmtype == 0)&&(msg_head->mcs == 0)){
						send_mode = 0;
					}else if((msg_head->frmtype == 0)&&(msg_head->mcs == 1)){
						send_mode = 1;
					}else if(((msg_head->frmtype == 0)&&(msg_head->mcs == 2)) || 
							 ((msg_head->frmtype == 1)&&(msg_head->mcs == 0))  || 
							 ((msg_head->frmtype == 2)&&(msg_head->mcs == 0)))
					{
						send_mode = 2;
					}else{
						send_mode = 3;
					}
                    #endif

					
					memset(lost_pack,0,20);
					for(itk = 2;itk < 22;itk++){
						if(s_los_map[itk]!=0xff){
							if(s_los_map[itk]!=0){
								lost_pack[jtk] = s_los_map[itk];
								jtk++;
							}
						}else{
							los_num = node_num - s_los_map[itk+1];
							for(ktk = 0;ktk<los_num;ktk++){
								lost_pack[jtk] = s_los_map[itk+1]+ktk+1;
								jtk++;
							}
							break;
						}
					}
					retry = 1;
					if(node_num <= (jtk*2)){   //lost half
						retry = 3;
					}
					
					ktk = 0;
					LL_FOREACH_SAFE(dest_list,el,tmp){
						if(dest_list_tmp == el)
						{
							continue;
						}
						//读取完毕删除
						//图片保存起来
						
						if(flen)
						{
							jpeg_buf_addr = (uint8_t *)GET_JPG_SELF_BUF(get_f,el->data);

							for(sofset = 0;sofset < snode_len;sofset=sofset+node_len){
								if(flen >= node_len)
								{
									flen -= node_len;
									
									map_num = count/8;
									bit_num = count%8;
									msg_head->node_map[8-map_num] |= BIT(bit_num);
									if(count){
										msg_head->photo_state = jpg_num;
									}
									msg_head->cnt = count;									
									count++;
									hw_memcpy(s_ptr_protocol+20,(char*)jpeg_buf_addr+sofset,node_len);
									
									msg_head->photo_state = BIT(6)|BIT(7)|jpg_num;
									
									if(lost_pack[ktk] == count){
										ktk++;
										//printf("s(%d)",count);
										for(itk = 0;itk < retry;itk++){
											len = sendto(udp_protocol_fd, s_ptr_protocol, node_len+20, MSG_DONTWAIT, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));			
											if(len == -1){
												os_printf("rsend err:%d\r\n",count);
											}
											if(send_mode == 0){
												os_sleep_ms(40);
											}else if(send_mode == 1){
												os_sleep_ms(10);
											}else if(send_mode == 2){
												os_sleep_ms(2);
											}else if(send_mode == 3){
												os_sleep_ms(1);
											}
											tx_data_count += (node_len+20);
										}
											
										
									}
								
								}
								else
								{
									map_num = count/8;
									bit_num = count%8;
									msg_head->node_map[8-map_num] |= BIT(bit_num);
									msg_head->cnt = count;
									count++;
									hw_memcpy(s_ptr_protocol+20,(char*)jpeg_buf_addr+sofset,flen);
									msg_head->photo_state = BIT(6)|BIT(7)|jpg_num;
									if(lost_pack[ktk] == count){
										ktk++;
										//printf("s(%d)",count);
										for(itk = 0;itk < retry;itk++){
											len = sendto(udp_protocol_fd, s_ptr_protocol, flen+20, MSG_DONTWAIT, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
											if(len == -1){
												os_printf("rsend err:%d\r\n",count);
											}
											if(send_mode == 0){
												os_sleep_ms(40);
											}else if(send_mode == 1){
												os_sleep_ms(10);
											}else if(send_mode == 2){
												os_sleep_ms(2);
											}else if(send_mode == 3){
												os_sleep_ms(1);
											}
											tx_data_count += (flen+20);
										}
										
									}
								
									flen = 0;							
								}

							}
						}
					
						DEL_JPEG_NODE(get_f,el);				
					}
					
				}else{
					//超时，没数据要重发
				}
#endif				
			    free_data(get_f);
            	get_f = NULL;
			}else{
				os_sleep_ms(1);	
			}

		}else{
			os_sleep_ms(20);
		}

	}
}

///////server
void handle_send_thread()
{
	int  len;	
//	uint8_t itk;
	char buf[12];
	int32  ret_sema        = 0;
	struct sockaddr_in remote_addr;
	memset(&remote_addr,0,sizeof(struct sockaddr_in));
	remote_addr.sin_family=AF_INET;
#if PROTOCOL_SERVER
	remote_addr.sin_addr.s_addr=inet_addr("192.168.1.100");
#elif PROTOCOL_CLIENT
    remote_addr.sin_addr.s_addr=inet_addr("192.168.1.1");
#endif
	remote_addr.sin_port=htons(11012);
	while(1){
		ret_sema = os_sema_down(&r_connect_sem, 1000);
		if(stop_heart_packet == 0) {
			if (!ret_sema){
				buf[0] = 1;
				len = sendto(handle_protocol_fd, (char*)buf, 12, MSG_DONTWAIT, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
			}else{
				r_los_map[0] = 2;
				r_los_map[1] = r_frame_id;
				len = sendto(handle_protocol_fd, (char*)r_los_map, 24, MSG_DONTWAIT, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
				//os_printf("los:");
				//for(itk = 0;itk < 24;itk++){
				//	printf("%02x ",los_map[itk]);
				//}
				//printf("\r\n");
			}
		}
	}
}

///////server
bbm_net_jpeg_frame* bbm_put_frame(uint8_t *displayadr, uint32_t data_len, uint32_t timeinfo, uint8_t grab)
{
#if PROTOCOL_RECORD  || LVGL_STREAM_ENABLE
	uint8_t frame_num = 0;
	uint8_t i = 0;
	uint8_t count = 0;
	bbm_net_jpeg_frame get_f;

	get_f.state = 3;
	get_f.frame_len = data_len;
	get_f.timeinfo = timeinfo;

	for(frame_num = 0;frame_num < STREAM_ROOM_NUM; frame_num++) {
		if(bbm_jpeg_frame[frame_num].state == 0 || bbm_jpeg_frame[frame_num].state == 2)
		{
			bbm_jpeg_frame[frame_num] = get_f;
			hw_memcpy(bbm_jpeg_room[frame_num],displayadr,data_len);
			bbm_jpeg_frame[frame_num].data = bbm_jpeg_room[frame_num];
			// os_printf("%s timeinfo:%d data:%x len:%d\n",__FUNCTION__,get_f.timeinfo,bbm_jpeg_frame[frame_num].data,bbm_jpeg_frame[frame_num].frame_len);
			bbm_jpeg_frame[frame_num].state = 1;
			return &bbm_jpeg_frame[frame_num];
		}
	}

	if(grab)
	{
		for(frame_num = 0;frame_num < STREAM_ROOM_NUM; frame_num++) {
			if(bbm_jpeg_frame[frame_num].state == 1)
			{
				bbm_jpeg_frame[frame_num] = get_f;
				hw_memcpy(bbm_jpeg_room[frame_num],displayadr,data_len);
				bbm_jpeg_frame[frame_num].data = bbm_jpeg_room[frame_num];
				// os_printf("%s timeinfo:%d data:%x len:%d\n",__FUNCTION__,get_f.timeinfo,bbm_jpeg_frame[frame_num].data,bbm_jpeg_frame[frame_num].frame_len);
				bbm_jpeg_frame[frame_num].state = 1;
				return &bbm_jpeg_frame[frame_num];
			}
		}		
	}
#endif
	return 0;
}

bbm_net_jpeg_frame* bbm_get_frame()
{
#if PROTOCOL_RECORD || LVGL_STREAM_ENABLE
	uint8_t  displaynum = 255;
	uint8_t frame_num = 0;
	uint32_t tmr = 0x3fffffff;

	for(frame_num = 0;frame_num < STREAM_ROOM_NUM; frame_num++) {
		if(bbm_jpeg_frame[frame_num].state == 1)
		{
			if(bbm_jpeg_frame[frame_num].timeinfo < tmr)
			{
				tmr = bbm_jpeg_frame[frame_num].timeinfo;
				displaynum = frame_num;
			}

		}
	}

	if(displaynum != 255)
	{
		bbm_jpeg_frame[displaynum].state = 3;
		return &bbm_jpeg_frame[displaynum];
	}
#endif
	return 0;
}

void bbm_del_frame(bbm_net_jpeg_frame* get_f)
{
	get_f->state = 2;
	get_f->data = NULL;
	get_f->frame_len = 0;
}

void read_protocol_thread(){	
	int t;
	//fd_set rfds;
	int retval;
	int ret;
	int itk = 0;
	int jtk = 0;
	uint8_t tkt = 0;
	int buf_recv = 0;
	int frame_state  = 0;    //0: no action    1:new frame   2:wait check  3:updata lcd  4:already updata lcd
	uint32_t timeout;
 
	uint8_t mcs = 0;
	uint8_t frmtype = 0;
	uint8_t txmode_lock = 255;
	uint32_t old_time;
	uint32_t frame_time;
	uint32_t rx_count = 0;
	uint16_t tx_msg = 0;
	uint8_t run_lcd = 0;
	uint8_t errframe = 0;
	uint8_t count = 0;
//	uint8_t map_num,bit_num;
	uint8_t pingpang = 0;
	struct sockaddr remote_addr;
	uint8_t node_map_cache[20];
	uint16_t node_len = 1308;
	uint8_t f_num = 0;
	uint32_t total_len = 0;
//	uint8_t cnt_rom;
	uint8_t pack_num=0;
	retval = 16;
	baby_head *msg_head;
	msg_head = (baby_head *)r_ptr_protocol;
	timeout = 10000;
	static uint32_t recv_timeout_cnt = 0;

	uint8_t *displayadr = NULL;

	t = 10;         //10ms超时时间
	setsockopt(udp_protocol_fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&t,sizeof(struct timeval));
	old_time = os_jiffies();
	while(1){
		ret = recvfrom (udp_protocol_fd, r_ptr_protocol, 2048, 0, &remote_addr, (socklen_t*)&retval);	
		if(ret <= 0) {
			recv_timeout_cnt++;
			/*连续超过2s没有收到数据，则认为摄像头端进入低功耗，不再发心跳包*/
			if(recv_timeout_cnt >= 200) {
				recv_timeout_cnt = 200;
				stop_heart_packet = 1;
			}
		}
		else {
			recv_timeout_cnt = 0;
			stop_heart_packet = 0;
		}
		if((os_jiffies() - old_time) > 1000){
			old_time = os_jiffies();
			rx_speed= rx_count/1024;
			tx_speed= tx_msg;
			dispnum = run_lcd;
			run_lcd = 0;
			rx_count = 0;
			//printf("rx_speed:%d  tx_speed:%d  dispnum:%d \r\n",rx_speed,tx_speed,dispnum);
		}else{
			if(os_jiffies() < old_time){  //os_jiffies 溢出处理
				
				old_time = os_jiffies();
			}
		}
	
		if(ret != -1){
#if LCD_EN
			rx_count += ret;
			//os_printf("&(%d==%d  n:%02x%02x    %d)\r\n",msg_head->photo_state&0x3f,msg_head->pack,msg_head->node_map[7],msg_head->node_map[8],frame_state);
			r_frame_id = msg_head->photo_state&0x3f;
			frame_time = msg_head->timeinfo;
			tx_msg   = msg_head->tx_speed;
			if((msg_head->photo_state&(BIT(7)|BIT(6))) == (BIT(7)|BIT(6))){
				//printf("map:");
				//for(itk = 0;itk < 9;itk++){
				//	printf("%02x ",msg_head->node_map[itk]);
				//}
				//printf("\r\n");
				//printf("add...(f:%d %d)\r\n",msg_head->photo_state&0x3f,msg_head->cnt);
				node_map_cache[msg_head->cnt] = msg_head->cnt+1;
			}else if(msg_head->photo_state&BIT(7)){
				//新帧处理
			
				pack_num = msg_head->pack; 
				buf_recv = 0;
				count = 0;
				frame_state  = 1;
				errframe = 0;
				total_len = 0;
				for(itk = 0;itk < 20;itk++){
					node_map_cache[itk] = 0;
				}	
				node_map_cache[msg_head->cnt] = msg_head->cnt+1;
				node_len = ret-20;;
				f_num = msg_head->photo_state&0x3f;
				frmtype = msg_head->frmtype;
				mcs     = msg_head->mcs;

                mcs_send = mcs;
                frmtype_send = frmtype;

                #if 1
                    if(msg_head->mcs <= 1){
                        txmode = 0;
                    }else if(msg_head->mcs <= 3){
                        txmode = 1;
                    }else if(msg_head->mcs <= 5){
                        txmode = 2;
                    }   else{
                        txmode = 3;
                    }
                #else
				if((frmtype == 0)&&(mcs == 0)){
					txmode = 0;
				}else if((frmtype == 0)&&(mcs == 1)){
					txmode = 1;
				}else if(((frmtype == 0)&&(mcs == 2)) || 
						 ((frmtype == 1)&&(mcs == 0))  || 
						 ((frmtype == 2)&&(mcs == 0)))
				{
					txmode = 2;
				}else{
					txmode = 3;
				}
				#endif
				
				if(txmode_lock != txmode){
					txmode_lock = txmode;
					if(txmode > 2){
						if(t != 10){
							t = 10;
							setsockopt(udp_protocol_fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&t,sizeof(struct timeval));
						}						
					}else if(txmode > 0){
						if(t != 30){
							t = 30;
							setsockopt(udp_protocol_fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&t,sizeof(struct timeval));
						}
					}else{
						if(t != 50){
							t = 50;
							setsockopt(udp_protocol_fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&t,sizeof(struct timeval));
						}
					}
				}
			}else{
				if(f_num != (msg_head->photo_state&0x3f)){             //收到数据后发现换帧了
					
					//开启下一帧图像的初始化
					pack_num = msg_head->pack; 
					//recover
					buf_recv = 0;
					errframe = 0;
					count = 0;
					frame_state  = 1;
					for(itk = 0;itk < 20;itk++){
						node_map_cache[itk] = 0;
					}
					
					node_map_cache[msg_head->cnt] = msg_head->cnt+1;

					count = msg_head->cnt;
					f_num = msg_head->photo_state&0x3f;
				}
				
				node_map_cache[msg_head->cnt] = msg_head->cnt+1;
			}

			tkt = 0;
			for(itk = 0;itk < 9;itk++){
				if(msg_head->node_map[itk] != 0){
					for(jtk = 0;jtk < 8;jtk++){
						if((msg_head->node_map[itk]&BIT(jtk)) == 0){
							break;	
						}
						tkt++;
					}
					//printf("itk:%d  tkt:%d  itk*8+tkt-1:%d\r\n",itk,tkt,itk*8+tkt-1);
					buf_recv = ((8-itk)*8+tkt-1)*node_len;
					break;
				}
			}
			
			count++;
			
			
			if((msg_head->photo_state&0x3f) == f_num){
				if((r_ptr_protocol[20] == 0xff) && (r_ptr_protocol[21] == 0xd8)){
					// printf("head:%d %d\r\n",f_num,buf_recv);
				}
					if(pingpang == 0){
						// printf("B0:%d len:%d\n",buf_recv,ret-20);
						total_len = total_len + ret - 20;
						hw_memcpy(photo_decode_net[0]+buf_recv,r_ptr_protocol+20,ret-20);		
					}
					else{			
						//printf("B1:%d len:%d\n",buf_recv,ret-20);
						total_len = total_len + ret - 20;
						hw_memcpy(photo_decode_net[1]+buf_recv,r_ptr_protocol+20,ret-20);
					}

			}
	
	
			
			if((msg_head->photo_state&BIT(6))&&(frame_state == 1)){
				//检查上一帧是否正常，正常就推送，不正常就丢掉
				for(jtk = 0;jtk<pack_num;jtk++){
					if(node_map_cache[jtk] != (jtk+1)){
						errframe = 1;
					}
				}
//				os_printf("end frame...:%d  frame_state:%d\r\n",errframe,frame_state);
				if(errframe == 0){			
					if(frame_state !=4 ){
						frame_state = 4;
					}else{
						continue;
					}
					run_lcd++;

//					os_printf("MTPN:%d  %x\r\n",frame_time,photo_room[proom_num]);
					if(pingpang == 0){
						//printf("proom_num:%d pingpang:%d f_num:%d total_len:%d\n",proom_num,pingpang,f_num,total_len);
						if(photo_decode_net[0][2616] == 0xff && photo_decode_net[0][2617] == 0xd8)
						{
							printf("!!a!!!!!!b!!!!!b!!!!b!!b!!!b!b!!!\n");
						}
						hw_memcpy(photo_room[proom_num],photo_decode_net[0],30*1024);
						proom[proom_num].addr = photo_room[proom_num];	
						proom[proom_num].timeinfo = frame_time;
						proom[proom_num].data_len = total_len;

						displayadr = proom[proom_num].addr;

					#if !LVGL_STREAM_ENABLE && PROTOCOL_RECORD
						bbm_put_frame(proom[proom_num].addr, proom[proom_num].data_len, proom[proom_num].timeinfo, 1);
					#endif
						if(displayadr[2616] == 0xff && displayadr[2617] == 0xd8)
						{
							printf("!!c!!!!!!c!!!!!c!!!!c!!c!!!c!c!!!\n");
						}


						proom_num++;
						if(proom_num == PROOM_NUM){
							proom_num = 0;
						}	
						pingpang = 1;
						total_len = 0;
					}else{
						//printf("proom_num:%d pingpang:%d f_num:%d total_len:%d\n",proom_num,pingpang,f_num,total_len);
						if(photo_decode_net[1][2616] == 0xff && photo_decode_net[1][2617] == 0xd8)
						{
							printf("!!a1!!!!!!b!!!!!b!!!!b!!b!!!b!b!!!\n");
						}						
						hw_memcpy(photo_room[proom_num],photo_decode_net[1],30*1024);
						proom[proom_num].addr = photo_room[proom_num];	
						proom[proom_num].timeinfo = frame_time;
						proom[proom_num].data_len = total_len;

						displayadr = proom[proom_num].addr;

					#if !LVGL_STREAM_ENABLE && PROTOCOL_RECORD
						bbm_put_frame(proom[proom_num].addr, proom[proom_num].data_len, proom[proom_num].timeinfo, 1);
					#endif
						if(displayadr[2616] == 0xff && displayadr[2617] == 0xd8)
						{
							printf("!!e!!!!!!e!!!!!e!!!!e!!e!!!e!e!!!\n");
						}

						proom_num++;
						if(proom_num == PROOM_NUM){
							proom_num = 0;
						}
						
						pingpang = 0;
						total_len = 0;
					}
				}else{
					memset(r_los_map+2,0,20);
					itk = 0;
					for(jtk = 0;jtk<pack_num;jtk++){
						if(node_map_cache[jtk] != (jtk+1)){
							//printf("lose==>%d\r\n",jtk);
							r_los_map[itk+2] = jtk+1;
							itk++;
						}
					}
					os_sema_up(&r_connect_sem);			//发送当前包情况，告诉主机要重传哪些数据
					frame_state = 3;
				}
			}
#endif		
		}else{
			//printf("O(%d P:%d)",scale2_finish,pingpang);
			if(frame_state == 3)
			{				
				for(jtk = 0;jtk<pack_num;jtk++){
					if(node_map_cache[jtk] != (jtk+1)){
						errframe = 1;
					}
				}
#if LCD_EN
				if(errframe == 0){			
					if(frame_state !=4 ){
						frame_state = 4;
					}else{
						continue;
					}	
					run_lcd++;
					//if(scale2_finish){
					//	scale2_finish = 0;
//					os_printf("MTP:%d  %d\r\n",frame_time,t);
					if(pingpang == 0){
						//sys_dcache_clean_range((uint32_t *)photo_decode_net[0], buf_recv);
						//jpg_decode_to_lcd((uint32)photo_decode_net[0],320,240,320,240);
						//printf("2 proom_num:%d pingpang:%d f_num:%d total_len:%d\n",proom_num,pingpang,f_num,total_len);
						hw_memcpy(photo_room[proom_num],photo_decode_net[0],20*1024);
						proom[proom_num].addr = photo_room[proom_num];  
						proom[proom_num].timeinfo = frame_time;
						proom[proom_num].data_len = total_len;

						displayadr = proom[proom_num].addr;

					#if !LVGL_STREAM_ENABLE && PROTOCOL_RECORD
						bbm_put_frame(proom[proom_num].addr, proom[proom_num].data_len, proom[proom_num].timeinfo, 1);
					#endif
						// if(displayadr[2616] == 0xff && displayadr[2617] == 0xd8)
						// {
						// 	printf("!!c!!!!!!c!!!!!c!!!!c!!c!!!c!c!!!\n");
						// }

						proom_num++;
						if(proom_num == PROOM_NUM){
							proom_num = 0;
						}	
						pingpang = 1;
						total_len = 0;
					}else{
						//sys_dcache_clean_range((uint32_t *)photo_decode_net[1], buf_recv);					
						//jpg_decode_to_lcd((uint32)photo_decode_net[1],320,240,320,240);
						//printf("2 proom_num:%d pingpang:%d f_num:%d total_len:%d\n",proom_num,pingpang,f_num,total_len);
						hw_memcpy(photo_room[proom_num],photo_decode_net[1],20*1024);
						proom[proom_num].addr = photo_room[proom_num];  
						proom[proom_num].timeinfo = frame_time;
						proom[proom_num].data_len = total_len;

						displayadr = proom[proom_num].addr;
					#if !LVGL_STREAM_ENABLE && PROTOCOL_RECORD
						bbm_put_frame(proom[proom_num].addr, proom[proom_num].data_len, proom[proom_num].timeinfo, 1);
					#endif
						// if(displayadr[2616] == 0xff && displayadr[2617] == 0xd8)
						// {
						// 	printf("!!e!!!!!!e!!!!!e!!!!e!!e!!!e!e!!!\n");
						// }

						proom_num++;
						if(proom_num == PROOM_NUM){
							proom_num = 0;
						}
						
						pingpang = 0;
						total_len = 0;
					}
				}
#endif				
				frame_state = 0;
			}else if(frame_state == 2){

			}else{
				if(frame_state == 1){
					//丢尾包
					
					memset(r_los_map+2,0,20);
					itk = 0;
					for(jtk = 0;jtk<pack_num;jtk++){
						if(node_map_cache[jtk] != (jtk+1)){
							//printf("lose==>%d\r\n",jtk);
							r_los_map[itk+2] = jtk+1;
							itk++;
						}
					}
					//os_printf("S"); 			
					os_sema_up(&r_connect_sem);			
					frame_state = 3;		
					
				}else{
					frame_state = 0;
				}

			}			
		}

	}
}


//五帧做一次归纳平均
void lcd_play_thread(){
	uint8_t  itk = 0;
	uint8_t  displaynum;
	uint8_t  displaynum_g;
	uint8_t  num;
	uint8_t  num_old = 255;
	uint8_t  *displayadr;
	uint32_t tmr = 0x3fffffff;
	uint32_t delaytm;
	uint32_t timeout = 0;
	uint32_t tmr0,tmr1,tmr2,tmr3,tmr4;
	uint32_t timer_avg;
	uint32_t times_temp = 0;
#if LCD_EN
	while(1){
		num = 0;
		delaytm = 0;
		for(itk = 0;itk < PROOM_NUM;itk++){
			if(proom[itk].timeinfo != 0){     //观察有多少帧图像在缓存池
				num++;
			} 
		}	
		// os_printf("num:%d\n",num);
		if(num_old != num){
			// os_printf("num:%d\n",num);
			num_old = num;
		}
	#if ENABLE_CACHING	
		if(txmode == 3){
	#else
		if(1){
	#endif
			if(num == 0){
				os_sleep_ms(10);
			}
		}else{
			if(num < 5){
				os_sleep_ms(10);
				timeout++;
				if(timeout == 20){	//超过200ms就得显示一下更新图像
					displaynum = 255;
					timeout = 0;
					tmr = 0x3fffffff;
					for(itk = 0;itk < PROOM_NUM;itk++){
						if(proom[itk].timeinfo != 0){
							if(proom[itk].timeinfo < tmr){
								tmr = proom[itk].timeinfo;
								displayadr = proom[itk].addr; 
								displaynum = itk;
							}
						} 
					}
					if(displaynum != 255){
					#if LVGL_STREAM_ENABLE
						bbm_put_frame(proom[proom_num].addr, proom[proom_num].data_len, proom[proom_num].timeinfo, 1);
					#else
						jpg_decode_to_lcd((uint32)displayadr,320,240,320,240);
					#endif
						proom[displaynum].timeinfo = 0;
					}
				}
				
				continue;
			}else if(num < 6){
				delaytm = 20;
			}else if(num < 8){
				delaytm = 10;
			}else if(num < 10){
				delaytm = 5;
			}
			timeout = 0;
		}

		tmr = 0x3fffffff;
		for(itk = 0;itk < PROOM_NUM;itk++){
			if(proom[itk].timeinfo != 0){
				if(proom[itk].timeinfo < tmr){
					tmr = proom[itk].timeinfo;
					displayadr = proom[itk].addr; 
					displaynum = itk;
				}
			} 
		}

		displaynum_g = displaynum;
	#if ENABLE_CACHING
		if(txmode != 3){
	#else
		if(0){
	#endif
			tmr0 = proom[displaynum].timeinfo;
			displaynum++;
			if(displaynum == PROOM_NUM){
				displaynum = 0;
			}
			tmr1 = proom[displaynum].timeinfo;
			displaynum++;
			if(displaynum == PROOM_NUM){
				displaynum = 0;
			}
			tmr2 = proom[displaynum].timeinfo;
			displaynum++;
			if(displaynum == PROOM_NUM){
				displaynum = 0;
			}
			tmr3 = proom[displaynum].timeinfo;
			displaynum++;
			if(displaynum == PROOM_NUM){
				displaynum = 0;
			}
			tmr4 = proom[displaynum].timeinfo;
			
			timer_avg = (tmr4 - tmr0)/4;
		}else{
			timer_avg = 10;
		}

		times_temp = proom[displaynum_g].timeinfo;		
		proom[displaynum_g].timeinfo = 0;
		//printf("displayadr:%x displaynum_g:%d\n",displayadr,displaynum_g);

		if(displayadr[2616] == 0xff && displayadr[2617] == 0xd8)
		{
			printf("!!b!!!!!!b!!!!!b!!!!b!!b!!!b!b!!!\n");
		}
	#if LVGL_STREAM_ENABLE	
		bbm_put_frame(proom[proom_num].addr, proom[proom_num].data_len, proom[proom_num].timeinfo, 1);
	#else
		jpg_decode_to_lcd((uint32)displayadr,320,240,320,240);
	#endif

		if(displayadr[2616] == 0xff && displayadr[2617] == 0xd8)
		{
			printf("!!a!!!!!!a!!!!!a!!!!a!!a!!!a!a!!!\n");
		}

		// os_printf("tmr0:%d,tmr1:%d,tmr2:%d,tmr3:%d,tmr4:%d\r\n",tmr0,tmr1,tmr2,tmr3,tmr4);
		
		// os_printf("timer_avg:%d delaytm:%d displayadr:%x num:%d\n",timer_avg,delaytm,displayadr,num);
		if(timer_avg > 300)
			timer_avg = 300;
		
		if(num > 17){
			os_sleep_ms(timer_avg-5);
		}else{
			os_sleep_ms(timer_avg+delaytm);
		}		
	}
#endif
}

void udp_protocol_net_server_init()
{
	uint16_t port = 11011;
	udp_protocol_fd = usr_protocol_create(port);	
	csi_kernel_task_new((k_task_entry_t)read_protocol_thread, "protocol_udp_read", 0, 15, 0, NULL, 1024, &protocol_task_recv);
#if FULL_DUPLEX
    csi_kernel_task_new((k_task_entry_t)send_photo_thread, "protocol_udp_send", 0, 15, 0, NULL, 2048, &protocol_task_send);
#endif
}

void udp_handle_server_init()
{
	uint16_t port = 11012;   
    handle_protocol_fd = usr_protocol_create(port); 
	csi_kernel_task_new((k_task_entry_t)handle_send_thread, "handle_udp_send", 0, 15, 0, NULL, 1024, &handle_task_send);
#if FULL_DUPLEX
    csi_kernel_task_new((k_task_entry_t)read_handle_thread, "handle_udp_read", 0, 15, 0, NULL, 1024, &handle_task_recv);
#endif
}

void lcd_display_init()
{
	csi_kernel_task_new((k_task_entry_t)lcd_play_thread, "handle_lcd", 0, 15, 0, NULL, 1024, &handle_task_lcd);
}

void udp_protocol_net_client_init()
{
	uint16_t port = 11011;
	udp_protocol_fd = usr_protocol_create(port);	
	csi_kernel_task_new((k_task_entry_t)send_photo_thread, "protocol_udp_send", 0, 15, 0, NULL, 2048, &protocol_task_send);
#if FULL_DUPLEX
    csi_kernel_task_new((k_task_entry_t)read_protocol_thread, "protocol_udp_read", 0, 15, 0, NULL, 1024, &protocol_task_recv);
#endif
}

void udp_handle_client_init()
{
	uint16_t port = 11012;
	handle_protocol_fd = usr_protocol_create(port); 
	csi_kernel_task_new((k_task_entry_t)read_handle_thread, "handle_udp_read", 0, 15, 0, NULL, 1024, &handle_task_recv);
#if FULL_DUPLEX
    csi_kernel_task_new((k_task_entry_t)handle_send_thread, "handle_udp_send", 0, 15, 0, NULL, 1024, &handle_task_send);
#endif
}

#if PROTOCOL_CLIENT
void protocol_client_init(){
#if FULL_DUPLEX
	r_ptr_protocol = r_buf_for_protocol;
	os_sema_init(&r_connect_sem, 0);
    lcd_display_init();
    #if PROTOCOL_RECORD || LVGL_STREAM_ENABLE
       bbm_net_jpeg_init();
    #endif 
#endif
	s_ptr_protocol = s_buf_for_protocol;
	os_sema_init(&s_connect_sem, 0);
	udp_protocol_net_client_init();
	udp_handle_client_init();
}
#endif

#if PROTOCOL_SERVER
void protocol_server_init(){
#if FULL_DUPLEX
	s_ptr_protocol = s_buf_for_protocol;
	os_sema_init(&s_connect_sem, 0);
#endif
	r_ptr_protocol = r_buf_for_protocol;
	os_sema_init(&r_connect_sem, 0);
	udp_protocol_net_server_init();
	udp_handle_server_init();
	lcd_display_init();
#if PROTOCOL_RECORD || LVGL_STREAM_ENABLE
	bbm_net_jpeg_init();
#endif
}
#endif

stream *prc_stream(const char *name,stream *d_s);
void start_jpg_stream(uint32_t dvp_w, uint32_t dvp_h, uint32_t jpg_w, uint32_t jpg_h)
{
    stream *scale_s = scale3_stream_not_bind(S_PREVIEW_ENCODE_QVGA,dvp_w,dvp_h,jpg_w,jpg_h,YUV_P0);
    //绑定流到YUV_TO_JPG,意思就是将320x240数据给到yuv_to_jpg,然后yuv_to_jpg启动jpg0编码,得到320x240的jpg数据
    streamSrc_bind_streamDest(scale_s,R_YUV_TO_JPG);
    enable_stream(scale_s,1);

    //先创建jpg_s,然后绑定流(jpg给到谁用),最后使能
    stream *jpg_s = new_video_app_stream(S_JPEG);
    //给到图传测试使用
    streamSrc_bind_streamDest(jpg_s,R_RTP_JPEG);
    enable_stream(jpg_s,1);

    stream * yuv2jpg_s = prc_stream(R_YUV_TO_JPG,jpg_s);
    enable_stream(yuv2jpg_s,1);		
}

sysevt_hdl_res sysevt_lmac_event(uint32 event_id, uint32 data, uint32 priv){
	uint8_t *pbuf;
    switch(event_id&0xffff){
        case SYSEVT_LMAC_TX_STATUS:
			pbuf = &data; 
			//mcs_send     = pbuf[3]&0x0f;
            // if(mcs_send >= 6){
            //     mcs_send = 6;
            // }else{
			//     mcs_send = 4;
            // }
            mcs_send = 4;
			frmtype_send = (pbuf[3]&0xf0)>>4;
			//os_printf("SYSEVT_LMAC_TX_STATUS(%x  %x)...\r\n",data,mcs_send);
            break;
        default:
            os_printf("no this levent(%x)...\r\n",event_id);
            break;
    }
    return SYSEVT_CONTINUE;
}

void user_protocol()
{
#if PROTOCOL_CLIENT || FULL_DUPLEX
	sys_event_take(SYS_EVENT(SYS_EVENT_LMAC, 0),sysevt_lmac_event,0);
#endif
#if PROTOCOL_SERVER
    protocol_server_init();
#elif PROTOCOL_CLIENT
    protocol_client_init();
#endif
}


