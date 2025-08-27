#ifndef __RINGBUF_H
#define __RINGBUF_H

typedef struct ringbuf {
    void *data;
    volatile unsigned int front;
    volatile unsigned int rear;
    volatile unsigned int size;
} TYPE_RINGBUF;

extern int ringbuf_Init(TYPE_RINGBUF *buf, unsigned int buf_size);
extern int push_ringbuf(TYPE_RINGBUF *buf, void *data, unsigned int len);
extern int pop_ringbuf(TYPE_RINGBUF *buf, void *data, unsigned int len);
extern int pop_ringbuf_notmove(TYPE_RINGBUF *buf, void *data, unsigned int len);
extern int ringbuf_pop_available(TYPE_RINGBUF *buf);
extern int ringbuf_push_available(TYPE_RINGBUF *buf);
extern int ringbuf_del(TYPE_RINGBUF *buf);

#endif