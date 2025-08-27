#include <stdio.h>
#include <stdlib.h>
#include "ringbuf.h"
#include "osal/string.h"
#include "custom_mem/custom_mem.h"

int ringbuf_Init(TYPE_RINGBUF *buf, unsigned int buf_size)
{
    if(buf == NULL) {
        printf("Fail to init ringbuf!\r\n");
        return -1;
    }
    buf->front = 0;
    buf->rear = 0;
    buf->size = buf_size;
	printf("create ringbuf succes,size:%d\n",buf->size);
    return 0;
}

int push_ringbuf(TYPE_RINGBUF *buf, void *data, unsigned int len)
{             
    if(buf->data == NULL) {
        printf("Push ringbuf err!\r\n");
        return -1;
    } 
	if( (buf->rear + len) > (buf->size))
	{
		os_memcpy( (buf->data + buf->rear), data, (buf->size - buf->rear));
		os_memcpy( buf->data, (data+(buf->size - buf->rear)), (len - buf->size + buf->rear));
	}
	else
		os_memcpy( buf->data + buf->rear, data, len);
    buf->rear = (buf->rear+len)%(buf->size);  
    return 0; 
}

int pop_ringbuf(TYPE_RINGBUF *buf, void *data, unsigned int len)
{
    if(buf->data == NULL) {
        printf("Pop ringbuf err!!\r\n");
        return -1;
    } 
    if(buf->front == buf->rear) {
        printf("Ringbuf is empty!\r\n");
        return -1;        
    }

    if( (buf->front + len) > (buf->size) )
	{
		os_memcpy( data, (buf->data + buf->front), (buf->size - buf->front));
		os_memcpy( (data+(buf->size - buf->front)), buf->data, (len - buf->size + buf->front));		
	}
	else
		os_memcpy(data, (buf->data + buf->front), len);	
	buf->front = (buf->front+len)%(buf->size);
    return 0;
}
int pop_ringbuf_notmove(TYPE_RINGBUF *buf, void *data, unsigned int len)
{
    if(buf->data == NULL) {
        printf("Pop ringbuf err!!\r\n");
        return -1;
    } 
    if(buf->front == buf->rear) {
        printf("Ringbuf is empty!\r\n");
        return -1;        
    }

    if( (buf->front + len) > (buf->size) )
	{
		os_memcpy( data, (buf->data + buf->front), (buf->size - buf->front));
		os_memcpy( (data+(buf->size - buf->front)), buf->data, (len - buf->size + buf->front));		
	}
	else
		os_memcpy(data, (buf->data + buf->front), len);	
    return 0;
}
int ringbuf_pop_available(TYPE_RINGBUF *buf)
{
    if(buf == NULL) {
        printf(" Get ringbuf available err!\r\n");
        return -1;
    } 
    return ( ((buf->rear+buf->size)-buf->front) % (buf->size) );
}
int ringbuf_push_available(TYPE_RINGBUF *buf)
{
    if(buf == NULL) {
        printf(" Get ringbuf available err!\r\n");
        return -1;
    } 
    return ( ((buf->front+buf->size)-buf->rear-1) % (buf->size) );   
}
int ringbuf_del(TYPE_RINGBUF *buf)
{
    if(buf == NULL) {
        printf("Ringbuf del err!\r\n");
        return -1;
    } 
#ifdef PSRAM_HEAP
    custom_free_psram(buf);
#else
    custom_free(buf);
#endif
    buf = NULL;    
    return 0;
}
