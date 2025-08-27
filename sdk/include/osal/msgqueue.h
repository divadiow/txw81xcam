
#ifndef _OS_MSG_QUEUE_H_
#define _OS_MSG_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct os_msgqueue {
    uint32 magic;
    void  *hdl;
};

int32 os_msgq_init(struct os_msgqueue *msgq, int32 size);
uint32 os_msgq_get(struct os_msgqueue *msgq, int32 tmo_ms);
uint32 os_msgq_get2(struct os_msgqueue *msgq, int32 tmo_ms, int32 *err);
int32 os_msgq_put(struct os_msgqueue *msgq, uint32 data, int32 tmo_ms);
int32 os_msgq_put_head(struct os_msgqueue *msgq, uint32 data, int32 tmo_ms);
int32 os_msgq_del(struct os_msgqueue *msgq);
int32 os_msgq_cnt(struct os_msgqueue *msgq);

#ifndef OS_MSGQ_DEF
#define OS_MSGQ_DEF(name, size) struct os_msgqueue name;
#endif

#ifdef __cplusplus
}
#endif
#endif

