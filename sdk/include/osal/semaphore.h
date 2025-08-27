#ifndef __OS_SEMAPHORE_H
#define __OS_SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

struct os_semaphore {
    uint32 magic;
    void  *hdl;
};

int32 os_sema_init(struct os_semaphore *sem, int32 val);
int32 os_sema_del(struct os_semaphore *sem);
int32 os_sema_down(struct os_semaphore *sem, int32 tmo_ms);
int32 os_sema_up(struct os_semaphore *sem);
int32 os_sema_count(struct os_semaphore *sem);
void os_sema_eat(struct os_semaphore *sem);

#ifdef __cplusplus
}
#endif
#endif

