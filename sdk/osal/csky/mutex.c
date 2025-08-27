#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "csi_kernel.h"

#define MUTEX_MAGIC (0xa8b4c2d5)

int32 os_mutex_init(struct os_mutex *mutex)
{
    if(mutex->magic == MUTEX_MAGIC){
        os_printf(KERN_WARNING"mutex repeat initialization ????\r\n");
    }
    mutex->hdl = csi_kernel_mutex_new();
    if(mutex->hdl) mutex->magic = MUTEX_MAGIC;
    return (mutex->hdl ? RET_OK : RET_ERR);
}

int32 os_mutex_lock(struct os_mutex *mutex, int32 tmo)
{
    ASSERT(mutex && mutex->hdl);
    return csi_kernel_mutex_lock(mutex->hdl, csi_kernel_ms2tick(tmo), (uint32_t)RETURN_ADDR());
}

int32 os_mutex_unlock(struct os_mutex *mutex)
{
    ASSERT(mutex && mutex->hdl);
    return csi_kernel_mutex_unlock(mutex->hdl);
}

int32 os_mutex_del(struct os_mutex *mutex)
{
    int32 ret = 0;
    ASSERT(mutex && mutex->hdl);
    ret = csi_kernel_mutex_del(mutex->hdl);
    mutex->hdl = NULL;
    mutex->magic = 0;
    return ret;
}

