#ifndef _INTERCOM_H_
#define _INTERCOM_H_

#include "lwip/sockets.h"
#include "osal/task.h"
#include "osal/mutex.h"
#include "lib/net/eloop/eloop.h"
#include "stream_frame.h"
#include "ringbuf.h"
#include "csi_kernel.h"

#define iLBC_CODE   0
#define OPUS_CODE   0
#define ADPCM_CODE  1

#define  FULL_DUPLEX    0
#define  HALF_DUPLEX    1

#ifdef PSRAM_HEAP
#define intercom_malloc custom_malloc_psram
#define intercom_zalloc custom_zalloc_psram
#define intercom_free   custom_free_psram
#else
#define intercom_malloc custom_malloc
#define intercom_zalloc custom_zalloc
#define intercom_free   custom_free
#endif

typedef struct
{     
	struct list_head list;
	uint8* buf_addr;
}audio_node;

#if OPUS_CODE
typedef struct  
{
    struct list_head list;
    struct list_head node_head;
    uint8 seq;
    uint8 status;
    uint16 sort;
    uint32 timestamp;
    uint32 code_len;
	uint32 identify_num;
	uint32 node_cnt;
} __attribute__((packed)) sublist;

typedef struct 
{
    struct list_head list;
    uint8* buf_addr;
    uint32 data_len;
}ringbuf_manage;

#elif iLBC_CODE || ADPCM_CODE

typedef struct  
{
    struct list_head list;
    struct list_head node_head;
    uint8 seq;
    uint8 status;
    uint16 sort;
    uint32 timestamp;
	uint32 identify_num;
	uint32 node_cnt;
} __attribute__((packed)) sublist;

typedef struct 
{
    struct list_head list;
    uint8* buf_addr;
}ringbuf_manage;

#endif


typedef enum
{
    SEND_MODE,
    RECV_MODE
}transfer_mode;

typedef struct 
{
    int udp_sfd;
    int ack_sfd;
    int udp_cfd;
    int ack_cfd;

    struct sockaddr_in udp_s_addr;
    struct sockaddr_in ack_s_addr;
    struct sockaddr_in udp_c_addr;
    struct sockaddr_in ack_c_addr;


#if OPUS_CODE
	k_task_handle_t recv_task;
	k_task_handle_t retransfer_task;
	k_task_handle_t encoded_task;
	k_task_handle_t decoded_task;
	k_task_handle_t record_task;
#elif iLBC_CODE || ADPCM_CODE
    struct os_task recv_task;
    struct os_task retransfer_task;
    struct os_task encoded_task;
    struct os_task decoded_task;
    struct os_task record_task;
#endif

    volatile struct list_head srcList_head;
    volatile struct list_head checkList_head;
    volatile struct list_head useList_head;

#if OPUS_CODE
	char *encoded_task_stack;
	char *decoded_task_stack;
	char *recv_task_stack;
	char *retransfer_task_stack;
#endif

	stream *recv_s;
	stream *send_s;
} TYPE_INTERCOM_STRUCT;

extern TYPE_INTERCOM_STRUCT *intercom;
extern void intercom_init(void);
void encode_sema_up();
void intercom_encode_switch(uint8 enable);
#endif