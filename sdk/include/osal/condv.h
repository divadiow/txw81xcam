#ifndef __OS_CONDV_H
#define __OS_CONDV_H
#include "osal/atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

struct os_condv {
    uint32 magic;
    atomic_t waitings;
    void    *sema;
};

int32 os_condv_init(struct os_condv *cond);

int32 os_condv_broadcast(struct os_condv *cond);
int32 os_condv_signal(struct os_condv *cond);

int32 os_condv_del(struct os_condv *cond);
int32 os_condv_wait(struct os_condv *cond, struct os_mutex *mutex, uint32 tmo_ms);

#ifdef __cplusplus
}
#endif
#endif

