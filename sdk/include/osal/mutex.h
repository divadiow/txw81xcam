#ifndef _OS_MUTEX_H_
#define _OS_MUTEX_H_

#ifdef __cplusplus
extern "C" {
#endif

struct os_mutex {
    uint32 magic;
    void  *hdl;
};

int32 os_mutex_init(struct os_mutex *mutex);
int32 os_mutex_lock(struct os_mutex *mutex, int32 tmo);
int32 os_mutex_unlock(struct os_mutex *mutex);
int32 os_mutex_del(struct os_mutex *mutex);

#ifdef __cplusplus
}
#endif

#endif
