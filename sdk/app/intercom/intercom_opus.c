#include "intercom.h"
#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal/work.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lib/net/eloop/eloop.h"
#include "netif/ethernetif.h"
#include "syscfg.h"
#include "dev/audio/components/vad/auvad.h"
#include "hal/gpio.h"
#include "../lib/opus/src/opus_private.h"
#include "keyWork.h"
#include "keyScan.h"
#include "hal/timer_device.h"

#define PIN_SPK_MUTE   PA_12

#define MUTE_SPEAKER    0
#define SERVER_RECORD   0
#define SERVER_RECORD_NODE_NUM	70

#define DUPLEX_TYPE    FULL_DUPLEX

#define PLC_PROCESS    1
#define ZERO_ORDER_HOLD        1

#define SAMPLERATE             8000
#define CHANNELS               1
#define APPLICATION            OPUS_APPLICATION_VOIP

#define INTERCOM_SERVER        1
#define INTERCOM_CLIENT        0 
#define INTERCOM_PORT          5008

#define ENCODED_BUF_NUM        15
#define SUBLIST_NUM            50
#define ENCODED_RINGBUF_LEN    1500
#define SOFTBUF_LEN            4000

#define DECODED_DATA_LEN	   320  
#define CODE_MODE              40 
#define NODE_DATA_LEN          10  
#define MAX_ENCODED_LEN        280

#define NUM_OF_FRAME           5
#define RESERVE                16

#define HW_TIMER_SYNC_CODE        0
#define SW_TIMER_SYNC_CODE        1

TYPE_INTERCOM_STRUCT *intercom = NULL;
audio_node audio_node_src[SOFTBUF_LEN/NODE_DATA_LEN] __attribute__ ((aligned(4),section(".psram.src")));
sublist sublist_src[SUBLIST_NUM] __attribute__ ((aligned(4),section(".psram.src")));
ringbuf_manage ringbuf_mana[ENCODED_BUF_NUM] __attribute__ ((aligned(4),section(".psram.src")));
struct list_head sublist_head;
struct list_head ringbuf_manage_src;
struct list_head ringbuf_manage_use;
struct list_head *manage_cur_push;
struct list_head *manage_cur_pop;


TYPE_RINGBUF *encoded_ringbuf = NULL;
char *sort_buf = NULL;
uint32 del_node_num = 0;
uint32 g_intercom_start = 0;

struct os_mutex send_mutex;
struct os_mutex list_mutex;
static struct os_semaphore encode_sem;
static struct os_semaphore decode_sem;
struct os_task intercom_server_handle_task;
struct os_task intercom_client_handle_task;

volatile uint16 play_sort = 0;
uint16 g_current_sort = 1;
uint8 g_numofcached = 0;
uint8 g_send_sequence = 0;
uint32 g_send_sort = 0;
uint32 g_s_identify_num = 0;
int now_dac_filter_type = 0;

static struct data_structure *current_adc_data = NULL;
static struct data_structure *current_dac_data = NULL;

#if DEALY_ADJUSTMENT
extern uint8 txmode;
extern uint8_t send_mode;
#endif

uint32 playofwait = 29;
volatile uint8 send_start_flag = 0;  //发送端缓存到一定数量再开启发送; 
uint8 play_start_flag = 2;           //接收到缓存到一定数量再开始解码和播放；
uint8 intercom_encode_flag = 0x03;   //编码控制开关；bit0:手动控制，bit1：软件控制；

#if INTERCOM_SERVER
uint8 g_transfer_mode = RECV_MODE;
#elif INTERCOM_CLIENT
uint8 g_transfer_mode = SEND_MODE;
#endif

uint8 g_switch_recv_success = 1;
uint8 g_code_sema_init = 0;

uint32 interval_time = CODE_MODE;

OpusEncoder *Enc_Inst = NULL;
OpusDecoder *Dec_Inst = NULL;

void intercom_server_handle(void *d);
void intercom_client_handle(void *d);
void intercom_recv(void *d);
void intercom_encoded_handle(void *d);
void intercom_decoded_handle(void *d);
void retransfer_check(void *d);

int intercom_server_init(void);
int intercom_client_init(void);
void intercom_send(uint8 num);
void lose_packet_check(void);

#if HW_TIMER_SYNC_CODE
static struct timer_device *ctl_timer = NULL;
void decode_sem_up(uint32 args, uint32 flags);
#elif SW_TIMER_SYNC_CODE
static struct os_timer ctl_timer;
void decode_sem_up(uint32 *args);
#endif


void intercom_init(void)
{
#if INTERCOM_SERVER
	intercom_server_init();
#elif INTERCOM_CLIENT
	intercom_client_init();
#endif	
}

static void intercom_struct_init(void)
{
	for(uint8 i=0; i<4; i++)
	{
		*((&intercom->udp_sfd)+i) = -1;
	}
	for(uint8 i=0; i<4; i++) {
		*((&intercom->encoded_task_stack)+i) = NULL;
	}
}

static void srcList_init(void)
{
	for(uint32 i=0; i<(SOFTBUF_LEN/NODE_DATA_LEN); i++)
	{
		audio_node_src[i].buf_addr = (uint8*)(sort_buf+(NODE_DATA_LEN*i));
		list_add_tail((struct list_head *)&audio_node_src[i].list,(struct list_head *)&intercom->srcList_head); 
	}
}

static void sublist_init(void)
{
	for(uint32 i=0; i<SUBLIST_NUM; i++)
	{
		sublist_src[i].node_head.next = &(sublist_src[i].node_head);
		sublist_src[i].node_head.prev = &(sublist_src[i].node_head);
		list_add_tail((struct list_head *)&sublist_src[i].list,(struct list_head *)&sublist_head); 
	}
}

static void ringbuf_manage_init(void)
{
	for(uint32 i=0; i<ENCODED_BUF_NUM; i++)
	{
		list_add_tail((struct list_head *)&ringbuf_mana[i].list,(struct list_head *)&ringbuf_manage_src); 
	}
	manage_cur_pop = &(ringbuf_manage_use);
	manage_cur_pop = &(ringbuf_manage_use);
}

int intercom_room_init(void)
{	
	encoded_ringbuf = (TYPE_RINGBUF*)intercom_malloc(sizeof(TYPE_RINGBUF));
	if(ringbuf_Init(encoded_ringbuf,(ENCODED_RINGBUF_LEN)) == -1)
		return -1;
	encoded_ringbuf->data = (uint8*)intercom_zalloc( (encoded_ringbuf->size)*sizeof(uint8) );
	if(!(encoded_ringbuf->data)) {
		os_printf("malloc encode_ringbuf->data err\n");
		return -1;
	}	
	
	sort_buf = (char*)intercom_zalloc(SOFTBUF_LEN* sizeof(uint8));
	if(!sort_buf) {
		os_printf("malloc sort_buf err\n");
		return -1;
	}	

	INIT_LIST_HEAD((struct list_head *)&intercom->srcList_head);
	INIT_LIST_HEAD((struct list_head *)&intercom->checkList_head);
	INIT_LIST_HEAD((struct list_head *)&intercom->useList_head);
	INIT_LIST_HEAD((struct list_head *)&sublist_head);
	INIT_LIST_HEAD((struct list_head *)&ringbuf_manage_src);
	INIT_LIST_HEAD((struct list_head *)&ringbuf_manage_use);
	srcList_init();
	sublist_init();
	ringbuf_manage_init();
	os_printf("intercom_room_init succes\n");
	return 0;
}

static void close_socket(void)
{
	for(uint8 i=0; i<4; i++) {
		if(*((&intercom->udp_sfd)+i) != -1)
			close(*((&intercom->udp_sfd)+i));
	}
}

static void free_room(void)
{
	if(encoded_ringbuf->data) {
		intercom_free(encoded_ringbuf->data);
		encoded_ringbuf->data = NULL;
	}
	ringbuf_del(encoded_ringbuf);

	if(sort_buf) {
		intercom_free(sort_buf);
		sort_buf = NULL;
	}
}

static void free_task_stack(void)
{
	for(uint8 i=0; i<4; i++) {
		if(*((&intercom->encoded_task_stack)+i)) {
			custom_free_psram(*((&intercom->encoded_task_stack)+i));
			*((&intercom->encoded_task_stack)+i) = NULL;
		}
	}
}

uint32_t intercom_push_key(struct key_callback_list_s *callback_list,uint32_t keyvalue,uint32_t extern_value)
{
	if( (keyvalue>>8) != AD_PRESS)
		return 0;
	uint32 key_val = (keyvalue & 0xff);
	if((key_val == KEY_EVENT_DOWN) || (key_val == KEY_EVENT_LDOWN) || (key_val == KEY_EVENT_REPEAT)) {
		g_transfer_mode = SEND_MODE;
	}
	else if((key_val == KEY_EVENT_SUP) || (key_val == KEY_EVENT_LUP)) {
		g_transfer_mode = RECV_MODE;
	}
	return 0;
}
int intercom_server_init(void)
{
	int time_out = CODE_MODE*2;
	intercom = (TYPE_INTERCOM_STRUCT*)custom_malloc(sizeof(TYPE_INTERCOM_STRUCT));
	if(!intercom)
		goto intercom_server_init_err;
	intercom_struct_init();
	
	intercom->udp_sfd = socket(AF_INET,SOCK_DGRAM,0);
	intercom->ack_sfd = socket(AF_INET,SOCK_DGRAM,0);

#if DUPLEX_TYPE == HALF_DUPLEX
	add_keycallback(intercom_push_key,NULL);
#endif
	
	if((intercom->udp_sfd==-1) || (intercom->ack_sfd==-1)) {
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_server_init_err;
	}

	if(setsockopt(intercom->udp_sfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&time_out,sizeof(int)) == -1) {
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_server_init_err;
	}

	OS_TASK_INIT("intercom_server_handle", &intercom_server_handle_task, intercom_server_handle,  NULL, OS_TASK_PRIORITY_NORMAL, 1024);
	return 0;
	
intercom_server_init_err:
	close_socket();	
	if(intercom)
		custom_free(intercom);
	intercom = NULL;
	return -1;
}

int intercom_client_init(void)
{
	intercom = (TYPE_INTERCOM_STRUCT*)custom_malloc(sizeof(TYPE_INTERCOM_STRUCT));
	if(!intercom)
		goto intercom_client_init_err;
	intercom_struct_init();

    intercom->udp_cfd = socket(AF_INET,SOCK_DGRAM,0);
	intercom->ack_cfd = socket(AF_INET,SOCK_DGRAM,0);

	if((intercom->udp_cfd==-1) || (intercom->ack_cfd==-1)) {
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_client_init_err;
	}

#if DUPLEX_TYPE == FULL_DUPLEX
	int time_out = CODE_MODE*2;
	if(setsockopt(intercom->udp_cfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&time_out,sizeof(int)) == -1) {
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_client_init_err;
	}
#endif

    OS_TASK_INIT("intercom_client_handle", &intercom_client_handle_task, intercom_client_handle,  NULL, OS_TASK_PRIORITY_NORMAL, 1024);
	return 0;
	
intercom_client_init_err:
	close_socket();	
	if(intercom)
		custom_free(intercom);
	intercom = NULL;
	return -1;
}

static void close_intercom_stream(void)
{
	for(uint8 i=0; i<2; i++)
	{
		if(*((&intercom->recv_s)+i))
			close_stream((stream*)*((&intercom->recv_s)+i));
	}
}

static int opcode_func_r(stream *s,void *priv,int opcode)
{
	int res = 0;
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
static uint32_t get_sound_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	return (uint32_t)d->priv;
}
static uint32_t set_sound_data_len(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->priv = (void*)len;
	return len;
}
static stream_ops_func stream_sound_ops = 
{
	.get_data_len = get_sound_data_len,
	.set_data_len = set_sound_data_len,
};
static int opcode_func_s(stream *s,void *priv,int opcode)
{
	static uint8 *audio_buf = NULL;
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{			
			audio_buf = custom_malloc_psram(8 * ((DECODED_DATA_LEN+10)*2));
			if(audio_buf)
			{
				stream_data_dis_mem_custom(s);
			}
			streamSrc_bind_streamDest(s,R_SPEAKER);	
			#if SERVER_RECORD && INTERCOM_SERVER	
			streamSrc_bind_streamDest(s,R_INTERMEDIATE_DATA);
			#endif			
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
			data->ops = &stream_sound_ops;
			data->data = audio_buf + data_num*((DECODED_DATA_LEN+10)*2);
		}
		break;

		case STREAM_DATA_FREE:
		break;
        case STREAM_DATA_DESTORY:
            {
                if(audio_buf)
					custom_free_psram(audio_buf);
            }
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

#if SERVER_RECORD && INTERCOM_SERVER
static int opcode_func_record_s(stream *s,void *priv,int opcode)
{
	static uint8 *audio_buf = NULL;
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{			
			audio_buf = os_malloc_psram( SERVER_RECORD_NODE_NUM* ((DECODED_DATA_LEN+10)*2));
			if(audio_buf)
			{
				stream_data_dis_mem_custom(s);
			}
			// streamSrc_bind_streamDest(s,R_SPEAKER);	
			streamSrc_bind_streamDest(s,R_AT_AVI_AUDIO);
			streamSrc_bind_streamDest(s,R_AT_SAVE_AUDIO);			
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
			data->ops = &stream_sound_ops;
			data->data = audio_buf + data_num*((DECODED_DATA_LEN+10)*2);
		}
		break;

		case STREAM_DATA_FREE:
		{
		}
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

void intercom_record_thread(void *d)
{
	int flen = 0;
	int16 *send_record_stream_buf = NULL;
	struct data_structure *get_f_inter_r = NULL;
    struct data_structure *get_f_inter_s = NULL;
    stream* s_inter_s = NULL;
    stream* r_inter_s = NULL;
	uint8_t *buf = NULL;
	int ret ;
	uint32_t count = 0;
	uint32_t delay_count = 0;

	static uint32_t last_timestemp = 0;
	static uint32_t cur_timestemp = 0;

	s_inter_s = open_stream_available(S_INTERMEDIATE_DATA,SERVER_RECORD_NODE_NUM,0,opcode_func_record_s,NULL);
	r_inter_s = open_stream_available(R_INTERMEDIATE_DATA,0,SERVER_RECORD_NODE_NUM,opcode_func_r,NULL);
	if(!s_inter_s || !r_inter_s)
	{
		os_printf("open stream err\n");
		goto intercom_record_end;
	}

	while(1)
	{
		get_f_inter_r = recv_real_data(r_inter_s);

		if(get_f_inter_r)
		{
			get_f_inter_s = get_src_data_f(s_inter_s);
			if(get_f_inter_s)
			{
				count++;
				delay_count = 0;
				send_record_stream_buf = get_stream_real_data(get_f_inter_s);

				buf = (uint8_t*)get_stream_real_data(get_f_inter_r);


				flen = get_stream_real_data_len(get_f_inter_r);
				
				os_memcpy(send_record_stream_buf,buf,flen);

				set_stream_real_data_len(get_f_inter_s,flen);
                set_stream_data_time(get_f_inter_s,get_f_inter_r->timestamp);

				cur_timestemp = get_f_inter_r->timestamp;

				if((cur_timestemp > last_timestemp) && (last_timestemp != 0))
				{
					// os_printf("audio sleep:%d ms CUR:%d LAST:%d\n",(cur_timestemp-last_timestemp),cur_timestemp,last_timestemp);
				}

                ret = send_data_to_stream(get_f_inter_s);
				last_timestemp = cur_timestemp;
				free_data(get_f_inter_r);
				get_f_inter_r = NULL;

			}else
			{
				free_data(get_f_inter_r);
				os_sleep_ms(1);
				os_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");	//节点数量不足，不能保证肯定能获取到节点
			}
		}else{
			delay_count++;
			os_sleep_ms(1);
			// printf("M");
			if(delay_count == 90){
				//printf("!");
				//os_printf("================audio reset diff time==============\n");
				delay_count = 0;
				count = 0;
			}
		}
	}

intercom_record_end:

	if(get_f_inter_s)
	{
		force_del_data(get_f_inter_s);
	}

	if(get_f_inter_r)
	{
		free_data(get_f_inter_r);
	}

	if(r_inter_s)
	{
		close_stream(r_inter_s);
	}

	if(s_inter_s)
	{
		close_stream(s_inter_s);
	}
}
#endif

void intercom_server_handle(void *d)
{
    socklen_t  addrlen = sizeof(struct sockaddr_in);
	uint32_t ipaddr;
    int err = -1;

	if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
		do{
			ipaddr = lwip_netif_get_ip2("w0").addr;
			os_sleep_ms(100);
		}while((ipaddr&0xff000000) == 0x1000000);		
	}
	
	for(char i=0; i<2; i++)
	{
		(*((&intercom->udp_s_addr)+i)).sin_family = AF_INET;
		(*((&intercom->udp_s_addr)+i)).sin_port = htons(INTERCOM_PORT + i);
		(*((&intercom->udp_s_addr)+i)).sin_addr.s_addr = 0;	
		
		err = bind(*((&intercom->udp_sfd)+i), (struct sockaddr *)((&intercom->udp_s_addr)+i), addrlen);
		if(err == -1) {
			os_printf("\nerr:%s %d %d",__FUNCTION__,__LINE__,i);
			goto intercom_server_handle;
		}					
	}

	for(char i=0; i<2; i++)
	{
		(*((&intercom->udp_c_addr)+i)).sin_family = AF_INET;
		(*((&intercom->udp_c_addr)+i)).sin_port = htons(INTERCOM_PORT + i);
		(*((&intercom->udp_c_addr)+i)).sin_addr.s_addr = inet_addr("192.168.1.100");
	}

	err = intercom_room_init();
	if(err == -1)
	{
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_server_handle;			
	}

	intercom->recv_s = open_stream_available(R_INTERCOM_AUDIO, 0, 8, opcode_func_r, NULL);
	intercom->send_s = open_stream_available(S_INTERCOM_AUDIO, 8, 0, opcode_func_s, NULL);

	if( (!intercom->recv_s) || (!intercom->send_s) ) 
	{
		os_printf("\nopen stream R_INTERCOM_AUDIO err");
		goto intercom_server_handle;
	}

	Enc_Inst = opus_encoder_create(SAMPLERATE, CHANNELS, APPLICATION, &err);
	if (err != OPUS_OK) {
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_server_handle;	
	}
	
    opus_encoder_ctl(Enc_Inst, OPUS_SET_BITRATE(8000));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_VBR(1));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_VBR_CONSTRAINT(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_COMPLEXITY(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_INBAND_FEC(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_FORCE_CHANNELS(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_DTX(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_PACKET_LOSS_PERC(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_LSB_DEPTH(16));
	opus_encoder_ctl(Enc_Inst, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_ARG));
	opus_encoder_ctl(Enc_Inst, OPUS_SET_FORCE_MODE(MODE_SILK_ONLY));


	Dec_Inst = opus_decoder_create(SAMPLERATE, CHANNELS, &err);
	if (err != OPUS_OK) {
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_server_handle;	
	}

	os_mutex_init(&send_mutex);
	os_mutex_init(&list_mutex);
	os_sema_init(&encode_sem, 0);
	os_sema_init(&decode_sem, 0);
	g_code_sema_init = 1;

#if MUTE_SPEAKER == 1	
	gpio_set_mode(PIN_SPK_MUTE, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);
	gpio_set_dir(PIN_SPK_MUTE, GPIO_DIR_OUTPUT);
#endif

	intercom->encoded_task_stack = (char*)custom_zalloc_psram(25600);
	intercom->decoded_task_stack = (char*)custom_zalloc_psram(6144);
	intercom->recv_task_stack = (char*)custom_zalloc_psram(3072);
	intercom->retransfer_task_stack = (char*)custom_zalloc_psram(2048);
	
	csi_kernel_task_new((k_task_entry_t)intercom_encoded_handle, "intercom_encoded_handle", &encode_sem, 10, 0, 
												intercom->encoded_task_stack, 25600, &intercom->encoded_task);
	csi_kernel_task_new((k_task_entry_t)intercom_decoded_handle, "intercom_decoded_handle", &decode_sem, 10, 0, 
												intercom->decoded_task_stack, 6144, &intercom->decoded_task);
	csi_kernel_task_new((k_task_entry_t)intercom_recv, "intercom_recv", NULL, 31, 0, intercom->recv_task_stack, 3072, &intercom->recv_task);
	csi_kernel_task_new((k_task_entry_t)retransfer_check, "retransfer_check", NULL, 31, 0, intercom->retransfer_task_stack, 2048, &intercom->retransfer_task);
	#if SERVER_RECORD && INTERCOM_SERVER
	csi_kernel_task_new((k_task_entry_t)intercom_record_thread, "intercom_record_thread", NULL, 30, 0, NULL, 1024, &intercom->record_task);
	#endif
#if HW_TIMER_SYNC_CODE
	ctl_timer = (struct timer_device*)dev_get(HG_TIMER0_DEVID);
	err = timer_device_open(ctl_timer, TIMER_TYPE_PERIODIC, 0);
	if(err == -1)
		goto intercom_server_handle;
	err = timer_device_start(ctl_timer, 9600000, (timer_cb_hdl)decode_sem_up, (uint32_t)ctl_timer);		//40ms
	if(err == -1)
		goto intercom_server_handle;	
#elif SW_TIMER_SYNC_CODE
	os_timer_init(&ctl_timer, (os_timer_func_t)decode_sem_up, OS_TIMER_MODE_PERIODIC, &ctl_timer);
	os_timer_start(&ctl_timer, CODE_MODE);
#endif
	return;	
	
intercom_server_handle:
	close_socket();
	close_intercom_stream();	
	free_room();
	free_task_stack();
	if(intercom) {
		custom_free(intercom);
		intercom = NULL;
	}
	return;
}

void intercom_client_handle(void *d)
{
    socklen_t  addrlen = sizeof(struct sockaddr_in);
	int err = -1;
	uint32_t ipaddr;
	
	if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
		do{
			ipaddr = lwip_netif_get_ip2("w0").addr;
			os_sleep_ms(100);
		}while((ipaddr&0xff000000) == 0x1000000);
	}
	
	for(char i=0; i<2; i++)
	{
		(*((&intercom->udp_c_addr)+i)).sin_family = AF_INET;
		(*((&intercom->udp_c_addr)+i)).sin_port = htons(INTERCOM_PORT + i);
		(*((&intercom->udp_c_addr)+i)).sin_addr.s_addr = 0;
		
		err = bind(*((&intercom->udp_cfd)+i), (struct sockaddr *)(&(intercom->udp_c_addr)+i), addrlen);
		if(err == -1) {
			os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
			goto intercom_client_handle;
		}										
	}
	
	for(char i=0; i<2; i++)
	{
		(*((&intercom->udp_s_addr)+i)).sin_family = AF_INET;
		(*((&intercom->udp_s_addr)+i)).sin_port = htons(INTERCOM_PORT + i);
		(*((&intercom->udp_s_addr)+i)).sin_addr.s_addr = inet_addr("192.168.1.1");
	}
	
	err = intercom_room_init();
	if(err == -1)
	{
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_client_handle;			
	}

	intercom->recv_s = open_stream_available(R_INTERCOM_AUDIO, 0, 8, opcode_func_r, NULL);
	intercom->send_s = open_stream_available(S_INTERCOM_AUDIO, 8, 0, opcode_func_s, NULL);
	if( (!intercom->recv_s) || (!intercom->send_s) )
	{
		os_printf("\nopen stream R_INTERCOM_AUDIO err");
		goto intercom_client_handle;
	}

	Enc_Inst = opus_encoder_create(SAMPLERATE, CHANNELS, APPLICATION, &err);
	if (err != OPUS_OK) {
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_client_handle;	
	}
	
    opus_encoder_ctl(Enc_Inst, OPUS_SET_BITRATE(8000));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_VBR(1));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_VBR_CONSTRAINT(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_COMPLEXITY(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_INBAND_FEC(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_FORCE_CHANNELS(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_DTX(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_PACKET_LOSS_PERC(0));
    opus_encoder_ctl(Enc_Inst, OPUS_SET_LSB_DEPTH(16));

	Dec_Inst = opus_decoder_create(SAMPLERATE, CHANNELS, &err);
	if (err != OPUS_OK) {
		os_printf("\nerr:%s %d",__FUNCTION__,__LINE__);
		goto intercom_client_handle;	
	}

	os_mutex_init(&send_mutex);
	os_mutex_init(&list_mutex);
	os_sema_init(&encode_sem, 0);
	os_sema_init(&decode_sem, 0);
	g_code_sema_init = 1;

#if MUTE_SPEAKER == 1	
	gpio_set_mode(PIN_SPK_MUTE, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);
	gpio_set_dir(PIN_SPK_MUTE, GPIO_DIR_OUTPUT);
#endif

	intercom->encoded_task_stack = (char*)custom_zalloc_psram(25600);
	intercom->decoded_task_stack = (char*)custom_zalloc_psram(6144);
	intercom->recv_task_stack = (char*)custom_zalloc_psram(3072);
	intercom->retransfer_task_stack = (char*)custom_zalloc_psram(2048);
	
	csi_kernel_task_new((k_task_entry_t)intercom_encoded_handle, "intercom_encoded_handle", &encode_sem, 10, 0, 
												intercom->encoded_task_stack, 25600, &intercom->encoded_task);
	csi_kernel_task_new((k_task_entry_t)intercom_decoded_handle, "intercom_decoded_handle", &decode_sem, 10, 0, 
												intercom->decoded_task_stack, 6144, &intercom->decoded_task);
	csi_kernel_task_new((k_task_entry_t)intercom_recv, "intercom_recv", NULL, 31, 0, intercom->recv_task_stack, 3072, &intercom->recv_task);
	csi_kernel_task_new((k_task_entry_t)retransfer_check, "retransfer_check", NULL, 31, 0, intercom->retransfer_task_stack, 2048, &intercom->retransfer_task);

	#if SERVER_RECORD && INTERCOM_SERVER
	csi_kernel_task_new((k_task_entry_t)intercom_record_thread, "intercom_record_thread", NULL, 30, 0, NULL, 1024, &intercom->record_task);
	#endif
#if HW_TIMER_SYNC_CODE
	ctl_timer = (struct timer_device*)dev_get(HG_TIMER0_DEVID);
	err = timer_device_open(ctl_timer, TIMER_TYPE_PERIODIC, 0);
	if(err == -1)
		goto intercom_client_handle;
	err = timer_device_start(ctl_timer, 9600000, (timer_cb_hdl)decode_sem_up, (uint32_t)ctl_timer);		//40ms
	if(err == -1)
		goto intercom_client_handle;	
#elif SW_TIMER_SYNC_CODE
	os_timer_init(&ctl_timer, (os_timer_func_t)decode_sem_up, OS_TIMER_MODE_PERIODIC, &ctl_timer);
	os_timer_start(&ctl_timer, CODE_MODE);
#endif
	return;
	
intercom_client_handle:
	close_socket();
	close_intercom_stream();	
	free_room();
	free_task_stack();
	if(intercom) {
		custom_free(intercom);
		intercom = NULL;
	}
	return;
}

int get_audio_sublist_count(volatile struct list_head *head)
{
	int count = 0;
	struct list_head *list_n = (struct list_head *)head;
	int ret = os_mutex_lock(&list_mutex,10);
	while(list_n->next != head) {
		list_n = list_n->next;
		count++;
		if(count>SUBLIST_NUM)
		{
			count = 0;
			break;
		}
	}
	if(ret == 0)
		os_mutex_unlock(&list_mutex);
	return count;			
}
int get_audio_node_count(struct list_head *head)
{
	int count = 0;
	struct list_head *list_n = (struct list_head *)head;
	while(list_n->next != head) {
		list_n = list_n->next;
		count++;
		if(count>(SOFTBUF_LEN/NODE_DATA_LEN))
		{
			count = 0;
			break;
		}
	}
	return count;			
}
int get_ringbuf_manage_count()
{
	int count = 0;
	struct list_head *list_n = (struct list_head *)manage_cur_pop;
	struct list_head *head = (struct list_head *)manage_cur_push;
	while(list_n != head) {
		list_n = list_n->next;
		count++;
		if(count>(ENCODED_BUF_NUM))
		{
			count = 0;
			break;
		}
	}
	return count;			
}
static struct list_head *get_audio_sublist(struct list_head *head)
{
	int ret = os_mutex_lock(&list_mutex,10);
	if(list_empty_careful((const struct list_head *)&sublist_head)){
		if(ret == 0)
			os_mutex_unlock(&list_mutex);
		os_printf("sublist_head empty\n");
		return 0;
	}
	list_move_tail((struct list_head *)sublist_head.next,(struct list_head *)head);
	if(ret == 0)
		os_mutex_unlock(&list_mutex);
	return head->prev;			
}
static struct list_head *get_audio_node(volatile struct list_head *head, uint32 node_num)
{
	int ret = os_mutex_lock(&list_mutex,10);
	if(get_audio_node_count((struct list_head *)&intercom->srcList_head) < node_num){
		if(ret == 0)
			os_mutex_unlock(&list_mutex);
		os_printf("srcList empty\n");
		return 0;
	}

	for(uint32 i=0; i<node_num; i++)
		list_move_tail((struct list_head *)intercom->srcList_head.next,(struct list_head *)head);
	if(ret == 0)
		os_mutex_unlock(&list_mutex);
	return head->next;			
}
static void del_ringbuf_manage()
{
	int ret = os_mutex_lock(&list_mutex,10);
	list_move_tail((struct list_head *)ringbuf_manage_use.next,(struct list_head*)&ringbuf_manage_src);
	if(ret == 0)
		os_mutex_unlock(&list_mutex);
}
static struct list_head *get_ringbuf_manage(uint8 grab)
{
	int ret = os_mutex_lock(&list_mutex,10);
	if(list_empty_careful((const struct list_head *)&ringbuf_manage_src)) {
		if(grab)
			list_move_tail((struct list_head *)ringbuf_manage_use.next,(struct list_head *)&ringbuf_manage_use);
		else {
			if(ret == 0)
				os_mutex_unlock(&list_mutex);			
			return 0;
		}
	}
	else 
		list_move_tail((struct list_head *)ringbuf_manage_src.next,(struct list_head *)&ringbuf_manage_use);
	if(ret == 0)
		os_mutex_unlock(&list_mutex);
	return ringbuf_manage_use.prev;				
}
static void *get_audio_addr(struct list_head *list)
{
	audio_node *audio_n;
	audio_n = list_entry((struct list_head *)list,audio_node,list);
	return audio_n->buf_addr;
}
static void *get_ringbuf_manage_addr(struct list_head *list)
{
	ringbuf_manage *ringbuf_mana;
	ringbuf_mana = list_entry((struct list_head *)list,ringbuf_manage,list);
	return ringbuf_mana->buf_addr;
}
static uint32 get_ringbuf_manage_datalen(struct list_head *list)
{
	ringbuf_manage *ringbuf_mana;
	ringbuf_mana = list_entry((struct list_head *)list,ringbuf_manage,list);
	return (uint32)ringbuf_mana->data_len;
}

static void del_audio_node(volatile struct list_head *del)
{
	del->next->prev = intercom->srcList_head.prev;
	del->prev->next = (struct list_head*)(&(intercom->srcList_head));
	intercom->srcList_head.prev->next = del->next;
	intercom->srcList_head.prev = del->prev;
	del->next = (struct list_head *)del;
	del->prev = (struct list_head *)del;
}
static void del_audio_sublist(volatile struct list_head *del)
{
	int ret = os_mutex_lock(&list_mutex,10);
	sublist *sublist_n = list_entry((struct list_head *)del,sublist,list);
	if(sublist_n->node_cnt)
		del_audio_node(&(sublist_n->node_head));
	list_move_tail((struct list_head *)del,(struct list_head*)&sublist_head);
	if(ret == 0)
		os_mutex_unlock(&list_mutex);	
}

static void insert_into_useList(volatile struct list_head *del, volatile struct list_head *head)
{
	sublist *sublist_n;
	sublist *sublist_n_pos;
	sublist *npos;
	sublist_n = list_entry((struct list_head *)del,sublist,list);
	int prev_sort = 0;
	uint16 new_sort = 0;
	uint16 next_sort = 0;

	new_sort = sublist_n->sort;
	prev_sort = (play_start_flag&BIT(0))?play_sort:-1;
	if((prev_sort>=new_sort) && ((prev_sort-new_sort)<=20))
	{
		del_audio_sublist(del);
		return;					
	}

	int ret = os_mutex_lock(&list_mutex,10);
	if(list_empty_careful((struct list_head *)head)) {		
		list_move((struct list_head *)del,(struct list_head *)head);
		if(ret == 0)
			os_mutex_unlock(&list_mutex);	
		return;
	}
	if(ret == 0)
		os_mutex_unlock(&list_mutex);	
	list_for_each_entry_safe(sublist_n_pos, npos, (struct list_head *)head, list)
	{
		next_sort = sublist_n_pos->sort;
		if(new_sort == next_sort)
		{
			del_audio_sublist(del);
			return;
		}
		else if( (prev_sort < new_sort ) && (new_sort < next_sort) && ((next_sort-new_sort) < 60000) )     
		{
			ret = os_mutex_lock(&list_mutex,10);
			list_move((struct list_head *)del,(struct list_head *)(sublist_n_pos->list.prev));
			if(ret == 0)
				os_mutex_unlock(&list_mutex);	
			return;
		}
		else if( (prev_sort>60000) && ((prev_sort - new_sort)>60000) && (new_sort < next_sort) && (prev_sort - next_sort>60000) )    
		{
			ret = os_mutex_lock(&list_mutex,10);
			list_move((struct list_head *)del,(struct list_head *)(sublist_n_pos->list.prev));
			if(ret == 0)
				os_mutex_unlock(&list_mutex);
			return;			
		}
		prev_sort = next_sort;
	}	
	ret = os_mutex_lock(&list_mutex,10);
	list_move_tail((struct list_head *)del,(struct list_head *)head);
	if(ret == 0)
		os_mutex_unlock(&list_mutex);
	return;	
}

int recv_repeat_check(uint16 seq, uint16 sort)
{
	static uint16 seq_sort[ENCODED_BUF_NUM*2+1] = {0};

	if(sort == seq_sort[seq]) {
		return 1;
	}
	else {
		seq_sort[seq] = sort;
		return 0;
	}
}

void lose_packet_check(void)
{ 
	static char sequence = 0;
	char temp = 0;
	static uint32 lose_packet = 0;
	static char retrans_num[ENCODED_BUF_NUM*2+1] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_in);
	uint32 last_loop_lose = 0;
	uint32 new_loop_lose = 0;
	uint8 seq = 0;
	uint16 sort = 0;
	struct list_head *sublist_l = NULL;
	sublist *sublist_n = NULL;
	uint32 i = 0;
	uint16 status = 0;
	uint8 recv_timeout = 0;
	int prev_sort = 0;

	for(i=1; i<(ENCODED_BUF_NUM*2+1); i++)
	{
		if(lose_packet & BIT(i)) {
			retrans_num[i] +=1;
			if(retrans_num[i] >= 2) {
				retrans_num[i] = 0;
				lose_packet &= ~BIT(i);
			}
		}
	}

	sublist_l = intercom->checkList_head.next;
	sublist_n = list_entry((struct list_head *)sublist_l,sublist,list);
	status = sublist_n->status;
#if INTERCOM_CLIENT
	if(status == 2)
	{
		g_transfer_mode = SEND_MODE;
		del_audio_sublist(sublist_l);
	}
	else 
	{
		g_transfer_mode = RECV_MODE;
#endif
		temp = (sequence % (ENCODED_BUF_NUM*2))+1;
		seq = sublist_n->seq;
		if(os_abs(seq-temp) >= 8)
			temp = seq;
		sort = sublist_n->sort;
		prev_sort = (play_start_flag&BIT(0))?play_sort:-1;
		/*收到重复包，丢弃*/
		if(recv_repeat_check(seq, sort)) {
			del_audio_sublist(sublist_l);
		}
		else if((sort <= prev_sort) && (prev_sort-sort <10))
		{
			del_audio_sublist(sublist_l);
			lose_packet &= ~BIT(seq);
			retrans_num[seq] = 0;
		}	
		else
		{
			/*收到丢包，清掉该丢包位*/
			if( lose_packet & BIT(seq) )  
			{
				lose_packet &= ~BIT(seq);		
				retrans_num[seq] = 0;
				// printf("re:%d\n",sort);
			}
			else if( temp != seq) 
			{
				if(BIT(seq) > BIT(temp))
					lose_packet |= (BIT(seq) - BIT(temp));   
				else {
					last_loop_lose = BIT(ENCODED_BUF_NUM*2+1) - BIT(temp);
					new_loop_lose = BIT(seq) - BIT(1);
					lose_packet |= (last_loop_lose|new_loop_lose);
				}
				sequence = seq;
			}
			else
				sequence = temp;
			recv_timeout = 0;
			insert_into_useList(sublist_l,&intercom->useList_head);
			// os_printf("r:%d\n",sort);
		}
#if INTERCOM_CLIENT
	}
#endif
	if(lose_packet) 
	{
		if(now_dac_filter_type == SOUND_INTERCOM) {
		#if INTERCOM_SERVER
			sendto(intercom->ack_sfd, &lose_packet,4, 0, (struct sockaddr*)&(intercom->ack_c_addr), addrlen);
		#elif INTERCOM_CLIENT
			sendto(intercom->ack_cfd, &lose_packet, 4, 0, (struct sockaddr*)&(intercom->ack_s_addr), addrlen);
		#endif	
//			printf("lp:%d***********\n",lose_packet);
		}
	}
}

void losePacket_retransfer(uint8 *addr, uint32 len)
{
    if(encoded_ringbuf == NULL) {
        return;
    } 
	uint8 send_buf[1400] = {0};
	if((addr+len) > ((uint8*)(encoded_ringbuf->data+encoded_ringbuf->size))) {
		uint32 residue_len = (uint8*)(encoded_ringbuf->data+encoded_ringbuf->size)-addr;
		os_memcpy(send_buf, addr, residue_len);
		os_memcpy(send_buf+residue_len, encoded_ringbuf->data, len-residue_len);
	}
	else
		os_memcpy(send_buf, addr, len);	
#if INTERCOM_SERVER
	sendto(intercom->udp_sfd, send_buf, len, 0,
						(struct sockaddr*)&(intercom->udp_c_addr), sizeof(intercom->udp_c_addr));
#elif INTERCOM_CLIENT
	sendto(intercom->udp_cfd, send_buf, len, 0,
						(struct sockaddr*)&(intercom->udp_s_addr), sizeof(intercom->udp_s_addr));
#endif
}

void retransfer_check(void *d)
{
	int len = 0;
	char sequence = 0;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	uint32 lose_packet = 0;
	struct list_head *manage_p = NULL;
	uint8 *addr = NULL;
	uint32 send_totallen = 0;

	while(1)
	{
	#if INTERCOM_SERVER            
		len = recvfrom(intercom->ack_sfd, &lose_packet, 4, 0, (struct sockaddr*)&(intercom->ack_c_addr), &addrlen);
		if(g_transfer_mode == RECV_MODE)
			continue;
	#elif INTERCOM_CLIENT
		len = recvfrom(intercom->ack_cfd, &lose_packet, 4, 0, (struct sockaddr*)&(intercom->ack_s_addr), &addrlen);
	#endif
		if((play_start_flag&BIT(1)) == 0)
			continue;
		if(len > 0)
		{
			manage_p = ringbuf_manage_use.next;
			while(manage_p != &(ringbuf_manage_use)) {
				send_totallen = 0;
				addr = (uint8*)get_ringbuf_manage_addr(manage_p);
				sequence = *addr;
				if(BIT(sequence) & lose_packet)
				{	
//					printf("rs:%d\n",sequence);
					for(uint32 i=0; i<NUM_OF_FRAME; i++) {
						send_totallen += get_ringbuf_manage_datalen(manage_p);
						manage_p = manage_p->next;
						if(manage_p == (&ringbuf_manage_use))
							break;
					}
					if(os_mutex_lock(&send_mutex,-1) == 0)
					{						
						losePacket_retransfer(addr, send_totallen);		
						os_sleep_ms(5);
						os_mutex_unlock(&send_mutex);
					}
				}
				else
					manage_p = manage_p->next;
			}
		}
	}
}

void intercom_send(uint8 num)
{
    int slen = 0;
    socklen_t  addrlen = sizeof(struct sockaddr_in);
	int ret = -1;
	uint8 send_buf[1400] = {0};
	uint32 send_totallen = 0;
	uint32 data_len = 0;
	uint8 *addr = NULL;
	struct list_head *manage_p = NULL;

	uint32 numof_available = get_ringbuf_manage_count();
	
	if((numof_available >= num) && (send_start_flag==0)) {
		send_start_flag = 1;
	}
 
	if((send_start_flag) && (numof_available >= num))
	{
		ret = os_mutex_lock(&send_mutex,-1);
		if(ret == 0){
			manage_cur_pop = manage_cur_pop->next;
			manage_p = manage_cur_pop;
			addr = (uint8*)get_ringbuf_manage_addr(manage_cur_pop);
			for(uint32 i=0; i<num; i++) {
				data_len = get_ringbuf_manage_datalen(manage_p);
				send_totallen += data_len;
				manage_p = manage_p->next;
			}
			if((addr+send_totallen) > ((uint8*)(encoded_ringbuf->data+encoded_ringbuf->size))) {
				uint32 residue_len = (uint8*)(encoded_ringbuf->data+encoded_ringbuf->size)-addr;
				os_memcpy(send_buf, addr, residue_len);
				os_memcpy(send_buf+residue_len, encoded_ringbuf->data, send_totallen-residue_len);
			}
			else
				os_memcpy(send_buf, addr, send_totallen);

			if(send_start_flag == 1) {
			#if INTERCOM_SERVER
				slen = sendto(intercom->udp_sfd, send_buf, send_totallen, 0, (struct sockaddr*)&(intercom->udp_c_addr), addrlen);
			#elif INTERCOM_CLIENT
				slen = sendto(intercom->udp_cfd, send_buf, send_totallen, 0, (struct sockaddr*)&(intercom->udp_s_addr), addrlen);
			#endif				
			}
			os_mutex_unlock(&send_mutex);
		}
	}

}

void intercom_recv(void *d)
{
    int rlen = 0;
    socklen_t addrlen = sizeof(struct sockaddr_in);	
	uint8 recv_buf[1400] = {0};
	uint8 *node_addr = NULL;
	uint32 code_len = 0;
	uint32 offset = 0;
	uint32 node_num = 0;
	static int recv_timeout_cnt = 0;
#if INTERCOM_SERVER 
	uint8 encoded_buf[RESERVE] = {0};
	int ret = -1;
#endif
	static int r_identify_num = 0;
	while(1)
	{		
	#if INTERCOM_SERVER            
		rlen = recvfrom(intercom->udp_sfd, recv_buf, 1400, 0, (struct sockaddr*)&(intercom->udp_c_addr), &addrlen);
		if(rlen <= 0) {
		#if DUPLEX_TYPE == 	HALF_DUPLEX
			if((get_ringbuf_manage_count()<=0) && (g_transfer_mode == RECV_MODE) && (g_switch_recv_success==0))
			{	
				os_printf("intercom server send request\n");
				encoded_buf[1] = 2;
				*((uint32*)(encoded_buf+12)) = g_s_identify_num;
				ret = os_mutex_lock(&send_mutex,-1);
				if(ret == 0)
					sendto(intercom->udp_sfd, encoded_buf, RESERVE, 0, (struct sockaddr*)&(intercom->udp_c_addr), addrlen);	
				os_mutex_unlock(&send_mutex);
			}
		#elif DUPLEX_TYPE == FULL_DUPLEX
			recv_timeout_cnt++;
			if(recv_timeout_cnt>=20){
				recv_timeout_cnt = 20;
				if(send_start_flag==1)
					send_start_flag = 2; //认为对方进入低功耗或者掉线，停止发送，但是发送缓存区还需要更新；				
				intercom_encode_flag &= ~BIT(1); //停止编码
			}
		#endif			
			continue;
		}
		else {
		#if DUPLEX_TYPE == 	HALF_DUPLEX
			g_switch_recv_success = 1;
		#elif DUPLEX_TYPE == FULL_DUPLEX
			recv_timeout_cnt = 0;
			if(send_start_flag==2)
				send_start_flag = 1;
			intercom_encode_flag |= BIT(1);
		#endif
		}
	#elif INTERCOM_CLIENT
		rlen = recvfrom(intercom->udp_cfd, recv_buf, 1400, 0, (struct sockaddr*)&(intercom->udp_s_addr), &addrlen);
		if((play_start_flag&BIT(1)) == 0) {
			continue;
		}
	#if(DUPLEX_TYPE == FULL_DUPLEX)
		if(rlen <= 0) {			
			recv_timeout_cnt++;
			if(recv_timeout_cnt>=20){	
				recv_timeout_cnt = 20;		
				intercom_encode_flag &= ~BIT(1); //停止编码
			}			
		}
		else {
			recv_timeout_cnt = 0;
			intercom_encode_flag |= BIT(1);			
		}
	#endif						
	#endif
		if(rlen >= RESERVE)
		{
			offset = 0;
			while(rlen > offset)
			{
				struct list_head *sublist_l = get_audio_sublist((struct list_head*)(&intercom->checkList_head));
				if(!sublist_l)
					break;
				sublist *sublist_n = list_entry((struct list_head *)sublist_l,sublist,list);
				os_memcpy(&(sublist_n->seq), recv_buf+offset, RESERVE);
				if(r_identify_num != sublist_n->identify_num) {
					for(uint8 i=0; i<g_numofcached; i++)
					{
						struct list_head *sublist_l = intercom->useList_head.next;
						del_audio_sublist(sublist_l);
					}
					g_numofcached = 0;
					play_start_flag &= ~BIT(0);
					r_identify_num = sublist_n->identify_num;
					os_printf("\n******change intercom identify_num****\n");
				}
				offset += RESERVE;
				code_len = sublist_n->code_len;
				node_num = (code_len%NODE_DATA_LEN)?(code_len/NODE_DATA_LEN+1):(code_len/NODE_DATA_LEN);
				sublist_n->node_cnt = node_num;
				struct list_head *audio_n = get_audio_node(&(sublist_n->node_head), node_num);
				if(audio_n)
				{
					while(code_len > NODE_DATA_LEN) {
						node_addr = (uint8*)get_audio_addr(audio_n);
						os_memcpy(node_addr, recv_buf+offset, NODE_DATA_LEN);
						audio_n = audio_n->next;
						offset += NODE_DATA_LEN;
						code_len -= NODE_DATA_LEN;
					}
					if(code_len > 0) {
						node_addr = (uint8*)get_audio_addr(audio_n);
						os_memcpy(node_addr, recv_buf+offset, code_len);
						offset += code_len;
						code_len = 0;
					}
					lose_packet_check();
				}
				else {
					sublist_n->node_cnt = 0;
					del_audio_sublist(sublist_l);
				}
			}	
		}
	}
}

void mute_speaker(uint8 enable)
{
	gpio_set_val(PIN_SPK_MUTE, enable);
}

#if HW_TIMER_SYNC_CODE
void decode_sem_up(uint32 args, uint32 flags)
{
	static uint32_t time = CODE_MODE;
	static uint32_t period = 0;
	struct timer_device* timer_dev = (struct timer_device*)args;
	if(time != interval_time) {
		if(interval_time == CODE_MODE) 
			period = 9600000;
		else if(interval_time == (CODE_MODE-1))
			period = 9360000;
		else if(interval_time == (CODE_MODE+1))
			period = 9600000;
		timer_device_ioctl(timer_dev, TIMER_SET_PERIOD, period, 0);
		time =  interval_time;
	}
#elif  SW_TIMER_SYNC_CODE	
void decode_sem_up(uint32 *args)
{
	static uint32_t time = CODE_MODE;
	struct os_timer *timer = (struct os_timer *)args;
	if(time != interval_time) {
		os_timer_stop(timer);
		os_timer_start(timer, interval_time);
	}
#endif
	if(g_code_sema_init) {
		os_sema_up(&decode_sem);
	}
	return;
}

void encode_sema_up()
{
	if(g_code_sema_init) {
		os_sema_up(&encode_sem);
	}	
}

void intercom_encoded_start(void)
{
	intercom_encode_flag |= BIT(0);	
}
void intercom_emcoded_stop(void)
{
	intercom_encode_flag &= ~BIT(0);
}

void intercom_encoded_handle(void *d)
{
	int16 *recv_stream_buf = NULL;
    struct data_structure *get_f = NULL;		
	int code_len = 0;
	uint8_t encoded_buf[MAX_ENCODED_LEN] = {0};
	uint32 encoded_buf_len = 0;
	static uint16 sort = 0;
	uint32 timestamp = 0;
	static uint8 mute_sta = 0;
	static uint8 num_of_avaiable = 0;
	uint8 send_num = 0;
	ringbuf_manage *ringbuf_mana_n = NULL;
	struct list_head *ringbuf_mana_l = NULL;
	struct list_head *manage_l_del = NULL;
	ringbuf_manage *manage_n_del = NULL;
	struct os_semaphore *sem = (struct os_semaphore *)d;
	g_s_identify_num = 0;
	os_random_bytes((uint8*)(&g_s_identify_num), 4);
	os_printf("\n**********intercom ID:%d***********\n",g_s_identify_num);
	
	while(1)
	{
		os_sema_down(sem,-1);
		get_f = recv_real_data(intercom->recv_s);
		current_adc_data = get_f;
	#if DUPLEX_TYPE == HALF_DUPLEX
		if(g_transfer_mode == SEND_MODE) 
		{	
	#endif		
			if(get_f)
			{
				// os_printf("getf:%x\n",get_f);
			#if DUPLEX_TYPE == HALF_DUPLEX
				if((g_numofcached > 0) || (intercom_encode_flag!=0x03))
				{
			#elif DUPLEX_TYPE == FULL_DUPLEX
				if(intercom_encode_flag!=0x03)
				{
			#endif
					free_data(get_f);
					current_adc_data = NULL;
					g_send_sequence = (g_send_sequence % (ENCODED_BUF_NUM*2))+1;
					encoded_buf[0] = (uint8)g_send_sequence;
					encoded_buf[1] = 0;
					if(sort < 65535) sort++;
					else sort = 0;
					*((uint16*)(encoded_buf+2)) = sort;
					timestamp = os_jiffies();
					*((uint32*)(encoded_buf+4)) = timestamp;
					code_len = 0;
					*((uint32*)(encoded_buf+8)) = code_len;
					*((uint32*)(encoded_buf+12)) = g_s_identify_num;
					encoded_buf_len = code_len + RESERVE;
					ringbuf_mana_l = get_ringbuf_manage(0);
					if(!ringbuf_mana_l) {
						ringbuf_mana_l = ringbuf_manage_use.next;
						ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
						encoded_ringbuf->front = (encoded_ringbuf->front+(manage_n_del->data_len+RESERVE))%(encoded_ringbuf->size);
						if(manage_cur_pop == ringbuf_manage_use.next)
							manage_cur_pop = manage_cur_pop->next;
						ringbuf_mana_l = get_ringbuf_manage(1);
					}
					manage_cur_push = ringbuf_mana_l;
					ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
					ringbuf_mana_n->buf_addr = (uint8*)encoded_ringbuf->data + encoded_ringbuf->rear;
					ringbuf_mana_n->data_len = encoded_buf_len;
					while(ringbuf_push_available(encoded_ringbuf) < encoded_buf_len) {
						manage_l_del = ringbuf_manage_use.next;
						manage_n_del = list_entry(manage_l_del, ringbuf_manage, list);
						encoded_ringbuf->front = (encoded_ringbuf->front+(manage_n_del->data_len+RESERVE))%(encoded_ringbuf->size);
						if(manage_cur_pop == ringbuf_manage_use.next)
							manage_cur_pop = manage_cur_pop->next;
						del_ringbuf_manage();
					}
					push_ringbuf(encoded_ringbuf, encoded_buf, encoded_buf_len);
					send_num = 1;
					intercom_send(send_num);
					continue;
				}
				g_switch_recv_success = 0;

				mute_sta = 0;
				recv_stream_buf = get_stream_real_data(get_f);
				timestamp = os_jiffies();
				code_len = opus_encode(Enc_Inst, recv_stream_buf, DECODED_DATA_LEN, encoded_buf+RESERVE, MAX_ENCODED_LEN);	
				free_data(get_f);
				current_adc_data = NULL;
				g_send_sequence = (g_send_sequence % (ENCODED_BUF_NUM*2))+1;
				encoded_buf[0] = (uint8)g_send_sequence;
				encoded_buf[1] = 1;
				if(sort < 65535) sort++;
				else sort = 0;
				*((uint16*)(encoded_buf+2)) = sort;
				*((uint32*)(encoded_buf+4)) = timestamp;
				*((uint32*)(encoded_buf+8)) = code_len;
				*((uint32*)(encoded_buf+12)) = g_s_identify_num;
				encoded_buf_len = code_len + RESERVE;
				ringbuf_mana_l = get_ringbuf_manage(0);
				if(!ringbuf_mana_l) {
					ringbuf_mana_l = ringbuf_manage_use.next;
					ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
					encoded_ringbuf->front = (encoded_ringbuf->front+(manage_n_del->data_len+RESERVE))%(encoded_ringbuf->size);
					if(manage_cur_pop == ringbuf_manage_use.next)
						manage_cur_pop = manage_cur_pop->next;
					ringbuf_mana_l = get_ringbuf_manage(1);
				}
				manage_cur_push = ringbuf_mana_l;
				ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
				ringbuf_mana_n->buf_addr = (uint8*)encoded_ringbuf->data + encoded_ringbuf->rear;
				ringbuf_mana_n->data_len = encoded_buf_len;
				while(ringbuf_push_available(encoded_ringbuf) < encoded_buf_len) {
					manage_l_del = ringbuf_manage_use.next;
					manage_n_del = list_entry(manage_l_del, ringbuf_manage, list);
					encoded_ringbuf->front = (encoded_ringbuf->front+(manage_n_del->data_len+RESERVE))%(encoded_ringbuf->size);
					if(manage_cur_pop == ringbuf_manage_use.next)
						manage_cur_pop = manage_cur_pop->next;
					del_ringbuf_manage();
				}
				push_ringbuf(encoded_ringbuf, encoded_buf, encoded_buf_len);
				send_num = NUM_OF_FRAME;
				intercom_send(send_num);
			}
			else {
				if(send_start_flag)
				{
					if(mute_sta == 0) {
						num_of_avaiable = get_ringbuf_manage_count();
						mute_sta = 1;
					}

					if(num_of_avaiable > 0) {
						intercom_send(num_of_avaiable);
						num_of_avaiable--;
					}
					else {
						g_send_sequence = (g_send_sequence % (ENCODED_BUF_NUM*2))+1;
						encoded_buf[0] = (uint8)g_send_sequence;
						encoded_buf[1] = 0;
						if(sort < 65535) sort++;
						else sort = 0;
						*((uint16*)(encoded_buf+2)) = sort;
						timestamp = os_jiffies();
						*((uint32*)(encoded_buf+4)) = timestamp;
						code_len = 0;
						*((uint32*)(encoded_buf+8)) = code_len;
						*((uint32*)(encoded_buf+12)) = g_s_identify_num;
						encoded_buf_len = code_len + RESERVE;
						ringbuf_mana_l = get_ringbuf_manage(0);
						if(!ringbuf_mana_l) {
							ringbuf_mana_l = ringbuf_manage_use.next;
							ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
							encoded_ringbuf->front = (encoded_ringbuf->front+(manage_n_del->data_len+RESERVE))%(encoded_ringbuf->size);
							if(manage_cur_pop == ringbuf_manage_use.next)
								manage_cur_pop = manage_cur_pop->next;
							ringbuf_mana_l = get_ringbuf_manage(1);
						}
						manage_cur_push = ringbuf_mana_l;
						ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
						ringbuf_mana_n->buf_addr = (uint8*)encoded_ringbuf->data + encoded_ringbuf->rear;
						ringbuf_mana_n->data_len = encoded_buf_len;	
						while(ringbuf_push_available(encoded_ringbuf) < encoded_buf_len) {
							manage_l_del = ringbuf_manage_use.next;
							manage_n_del = list_entry(manage_l_del, ringbuf_manage, list);
							encoded_ringbuf->front = (encoded_ringbuf->front+(manage_n_del->data_len+RESERVE))%(encoded_ringbuf->size);
							if(manage_cur_pop == ringbuf_manage_use.next)
								manage_cur_pop = manage_cur_pop->next;
							del_ringbuf_manage();
						}
						push_ringbuf(encoded_ringbuf, encoded_buf, encoded_buf_len);
						send_num = 1;
						intercom_send(send_num);
					}
				}
			}
	#if DUPLEX_TYPE == HALF_DUPLEX
		}
		else if(g_transfer_mode == RECV_MODE){
			num_of_avaiable = get_ringbuf_manage_count();
			if(num_of_avaiable > 0) {
				intercom_send(num_of_avaiable);
				num_of_avaiable--;
			}
			if(get_f) {
				free_data(get_f);
				current_adc_data = NULL;
			}
		}
	#endif
	}
}
static uint32 rounding(float x, uint32 limit)
{
	uint32 res =  (x < ((float)(limit-0.5)))?(x+0.5):(limit-0.5);
	return res;
}
static void zero_order_hold(int16 *inbuf, int16 *outbuf, uint32 insize, uint32 outsize)
{
	float rate = (float)insize/outsize;

	for(uint32 i=0; i<outsize; i++)
	{
		*(outbuf+i) = *(inbuf+rounding((i*rate), insize));
	}
}
static int send_to_stream(uint8 cached) 
{
	int16 *send_stream_buf = NULL;
	struct data_structure *get_f = NULL;
	struct list_head *sublist_l = NULL;
	sublist *sublist_n = NULL;
	int code_len = 0;
	uint32 offset = 0;
	uint16 new_sort = 0;
	uint32_t timestamp = 0;
	int16 decode_data[DECODED_DATA_LEN] = {0};
	uint8 encode_data[MAX_ENCODED_LEN] = {0};
	uint8 packet_sta = 0;
	static uint32_t last_timestamp = 0;
	static uint32_t plc_cnt = 0;

	get_f = get_src_data_f(intercom->send_s);
	current_dac_data = get_f;
	if(get_f) {
		send_stream_buf = get_stream_real_data(get_f);
		play_sort = g_current_sort;
		if(list_empty((struct list_head*)&intercom->useList_head) == 0) {
			sublist_l = intercom->useList_head.next;
			sublist_n = list_entry((struct list_head*)sublist_l, sublist, list);
			new_sort = sublist_n->sort;
			if( (new_sort - g_current_sort) > 20 )
			 	g_current_sort = new_sort;
			// else if( ((g_current_sort - new_sort)>0) && ((g_current_sort - new_sort)<60000) )  
			//  	g_current_sort = new_sort;
			if(SUBLIST_NUM-cached<15)
				g_current_sort = new_sort;
			packet_sta = sublist_n->status;
			if(packet_sta == 0) {
			#if MUTE_SPEAKER == 1
				mute_speaker(1);
			#endif
				force_del_data(get_f);
				del_audio_sublist(sublist_l);
				if(cached<=(playofwait-2))
					interval_time = (CODE_MODE+1);
				else if(cached>=(playofwait+2))
					interval_time = (CODE_MODE-1);
				else
					interval_time = CODE_MODE-1;	
				g_intercom_start = 0;
				g_current_sort += 1;
				return 0;
			}
		#if MUTE_SPEAKER == 1
			mute_speaker(0);
		#endif
			if(g_current_sort == new_sort) {
				timestamp = sublist_n->timestamp;
				last_timestamp = timestamp;
				code_len = sublist_n->code_len;
				struct list_head *audio_n = sublist_n->node_head.next;
				uint8 *addr = NULL;
				while(code_len > NODE_DATA_LEN) {
					addr = (uint8*)get_audio_addr(audio_n);
					os_memcpy(encode_data+offset, addr, NODE_DATA_LEN);
					audio_n = audio_n->next;
					offset += NODE_DATA_LEN;
					code_len -= NODE_DATA_LEN;
				}
				if(code_len > 0) {
					addr = (uint8*)get_audio_addr(audio_n);
					os_memcpy(encode_data+offset, addr, code_len);
					offset += code_len;
					code_len = 0;
				}
				code_len = opus_decode(Dec_Inst, encode_data, sublist_n->code_len, decode_data, DECODED_DATA_LEN, 0);
				del_audio_sublist(sublist_l);
				plc_cnt = 0;
			}	
			else {
//				if((g_current_sort+1) == new_sort) {
//					code_len = sublist_n->code_len;
//					struct list_head *audio_n = sublist_n->node_head.next;
//					uint8 *addr = NULL;
//					while(code_len > NODE_DATA_LEN) {
//						addr = (uint8*)get_audio_addr(audio_n);
//						os_memcpy(encode_data+offset, addr, NODE_DATA_LEN);
//						audio_n = audio_n->next;
//						offset += NODE_DATA_LEN;
//						code_len -= NODE_DATA_LEN;
//					}
//					if(code_len > 0) {
//						addr = (uint8*)get_audio_addr(audio_n);
//						os_memcpy(encode_data+offset, addr, code_len);
//						offset += code_len;
//						code_len = 0;
//					}
//					if(opus_packet_has_lbrr(encode_data, sublist_n->code_len))
//						code_len = opus_decode(Dec_Inst, encode_data, sublist_n->code_len, decode_data, DECODED_DATA_LEN, 1);
//					else {
//					#if PLC_PROCESS == 1	
//						code_len = opus_decode(Dec_Inst, NULL, 0, decode_data, DECODED_DATA_LEN, 0);
//					#else
//						os_memset(decode_data, 0, DECODED_DATA_LEN*2);
//					#endif
//					}
//				}
				#if PLC_PROCESS == 1	
					code_len = opus_decode(Dec_Inst, NULL, 0, decode_data, DECODED_DATA_LEN, 0);
					timestamp = last_timestamp+interval_time;
					last_timestamp = timestamp;
				#else
					os_memset(decode_data, 0, DECODED_DATA_LEN*2);
				#endif	
					plc_cnt++;
				if(plc_cnt > 20) 
					play_start_flag &= ~BIT(0);				
			}
		}
		else
		{
		#if PLC_PROCESS == 1
			code_len = opus_decode(Dec_Inst, NULL, 0, decode_data, DECODED_DATA_LEN, 0);
			timestamp = last_timestamp+interval_time;
			last_timestamp = timestamp;
		#else
			os_memset(decode_data, 0, DECODED_DATA_LEN*2);
		#endif
			plc_cnt++;
			if(plc_cnt > 20) 
				play_start_flag &= ~BIT(0);
		}
		g_current_sort += 1;
	#if ZERO_ORDER_HOLD == 1
		if(cached<=(playofwait-2)) {
			zero_order_hold(decode_data, send_stream_buf, DECODED_DATA_LEN, (DECODED_DATA_LEN+8));
			set_sound_data_len(get_f, (2*(DECODED_DATA_LEN+8)));
			interval_time = (CODE_MODE+1);
		}
		else if(cached>=(playofwait+2)) {
			zero_order_hold(decode_data, send_stream_buf, DECODED_DATA_LEN, (DECODED_DATA_LEN-8));	
			set_sound_data_len(get_f, (2*(DECODED_DATA_LEN-8)));
			interval_time = (CODE_MODE-1);
		}
		else {
			os_memcpy(send_stream_buf, decode_data, (DECODED_DATA_LEN*2));
			set_sound_data_len(get_f, DECODED_DATA_LEN*2);
			interval_time = CODE_MODE;
		}			
	#else
		os_memcpy(send_stream_buf, decode_data, DECODED_DATA_LEN*2);
		set_sound_data_len(get_f, DECODED_DATA_LEN*2);
	#endif
		get_f->type = SET_DATA_TYPE(SOUND, SOUND_INTERCOM);
		set_stream_data_time(get_f,timestamp);
		send_data_to_stream(get_f);
		current_dac_data = NULL;
		g_intercom_start = 1;
	}
      
	return 0;
}
     
extern void audio_dac_set_filter_type(int filter_type);
extern int get_audio_dac_set_filter_type(void);

void intercom_decoded_handle(void *d)
{
	static int former_dac_filter_type = 0;
	uint8 numofwait = 0;     //等待缓存数量满足条件而暂缓播放的次数
	struct os_semaphore *sem = (struct os_semaphore *)d;

    former_dac_filter_type = get_audio_dac_set_filter_type();
	audio_dac_set_filter_type(SOUND_INTERCOM);

	struct list_head *sublist_l = NULL;
	sublist *sublist_n = NULL;

	while(1)
	{	
		os_sema_down(sem, -1);
		// os_printf("g_numofcached:%d %d %d\n",g_numofcached,g_current_sort,play_start_flag);
#if DEALY_ADJUSTMENT
	#if INTERCOM_SERVER
		if(txmode == 3)
	#elif INTERCOM_CLIENT
		if(send_mode == 3)
	#endif
			playofwait = 5;
		else
			playofwait = SORTBUF_NUM/2;
#endif
		g_numofcached = get_audio_sublist_count((struct list_head *)&intercom->useList_head);
		now_dac_filter_type = get_audio_dac_set_filter_type();
		if((now_dac_filter_type != SOUND_INTERCOM) && (!find_stream_enable(R_AT_AVI_AUDIO))) {
			for(uint8_t i=0; i<g_numofcached; i++) {
				struct list_head *sublist_l = intercom->useList_head.next;
				del_audio_sublist(sublist_l);
				g_current_sort += 1;
				play_sort = g_current_sort;				
			}
			play_start_flag &= ~BIT(0);
			numofwait = 0;
			os_sleep_ms(1);
			continue;
		}
		if((g_numofcached >= playofwait) && (!find_stream_enable(R_AT_AVI_AUDIO)) && (play_start_flag&BIT(0)))
		{
			for(uint8 i=0; i<del_node_num; i++)
			{
				struct list_head *sublist_l = intercom->useList_head.next;
				del_audio_sublist(sublist_l);
				g_current_sort += 1;
				play_sort = g_current_sort;
			}
		}
		del_node_num = 0;
		g_numofcached = get_audio_sublist_count((struct list_head *)&intercom->useList_head);
		/*缓存的帧数大于一定数量则开始播放*/
	#if DUPLEX_TYPE == 	HALF_DUPLEX
		if( ((play_start_flag&BIT(0)) == 0) && ((g_numofcached > playofwait) || (numofwait > playofwait) || ((g_transfer_mode == SEND_MODE)&&(g_numofcached>0))) )
	#else
		if( ((play_start_flag&BIT(0)) == 0) && ((g_numofcached > playofwait) || (numofwait > playofwait))) 
	#endif
		{
			play_start_flag |= BIT(0);
			numofwait = 0;
			sublist_l = intercom->useList_head.next;
			sublist_n = list_entry((struct list_head*)sublist_l, sublist, list);
			g_current_sort = sublist_n->sort;
			os_printf("intercom play start\n");
		}
		if( ((play_start_flag&BIT(0)) == 0) && (g_numofcached > 0) ) {
			numofwait++;
		}
		if(play_start_flag&BIT(0)) 
		{
			send_to_stream(g_numofcached);
		}
	#if DUPLEX_TYPE == 	HALF_DUPLEX
		if((g_numofcached <=0) && (g_transfer_mode == SEND_MODE) && (play_start_flag&BIT(0))) {
		#if MUTE_SPEAKER == 1
			mute_speaker(1);
		#endif
			interval_time = CODE_MODE;
			play_start_flag &= ~BIT(0);
			g_intercom_start = 0;
		}
	#endif		
	}
}

int32 demon_atcmd_intercom_encode(const char *cmd, char *argv[], uint32 argc)
{
	if(argc < 1)
    {
        os_printf("%s argc too less:%d,enter the switch\n",__FUNCTION__,argc);
        return 0;
    }	

	if(argv[0]) {
		intercom_encode_flag ^= 1;
		os_printf("\n********intercom_encode_flag:%d**********\n",intercom_encode_flag);
	}
	return 1;
}

void intercom_suspend(void)
{
#if HW_TIMER_SYNC_CODE
	struct timer_device* timer_dev = (struct timer_device*)dev_get(HG_TIMER0_DEVID);
	timer_device_close(timer_dev);
#elif SW_TIMER_SYNC_CODE
	os_timer_stop(&ctl_timer);
#endif
	g_code_sema_init = 0;
	play_start_flag &= ~BIT(1);
	os_sleep_ms(100);
	if(current_adc_data) {
		free_data(current_adc_data);
		current_adc_data = NULL;
	}
	if(current_dac_data) {
		force_del_data(current_dac_data);
		current_dac_data = NULL;		
	}
}

void intercom_resume(void)
{
#if HW_TIMER_SYNC_CODE
	int err = -1;
	struct timer_device* timer_dev = (struct timer_device*)dev_get(HG_TIMER0_DEVID);
	timer_device_open(timer_dev, TIMER_TYPE_PERIODIC, 0);
	err = timer_device_start(ctl_timer, 9600000, (timer_cb_hdl)decode_sem_up, (uint32_t)ctl_timer);
#elif SW_TIMER_SYNC_CODE
	os_timer_start(&ctl_timer,CODE_MODE);
#endif	
	g_numofcached = get_audio_sublist_count((struct list_head *)&intercom->useList_head);
	for(uint8 i=0; i<g_numofcached; i++)
	{
		struct list_head *sublist_l = intercom->useList_head.next;
		del_audio_sublist(sublist_l);
		
	}
	g_numofcached = 0;
	g_code_sema_init = 1;
	play_start_flag &= ~BIT(0);
	play_start_flag |= BIT(1);
	send_start_flag = 0;
	g_intercom_start = 0;
}