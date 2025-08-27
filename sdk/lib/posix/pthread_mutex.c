#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/pthread.h"

#ifdef TXWSDK_POSIX

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
#ifdef __MINILIBC__
    struct os_mutex *m = (struct os_mutex *)(mutex->mutex);
    if (m) {
        os_mutex_del(m);
        os_free(m);
    }

    mutex->initted = 0;
    mutex->mutex = NULL;
    mutex->attr = NULL;
#else
    struct os_mutex *m = (struct os_mutex *)(*mutex);
    if (m) {
        os_mutex_del(m);
        os_free(m);
    }
#endif
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    struct os_mutex *m;

#ifdef __MINILIBC__
    if (NULL == mutex) {
        return -EINVAL;
    }

    os_memset(mutex, 0, sizeof(pthread_mutex_t));
    m = os_zalloc(sizeof(struct os_mutex));
    if (m == NULL) {
        pthread_err("alloc fail\r\n");
        return -ENOMEM;
    }

    os_mutex_init(m);
    mutex->initted = 1;
    mutex->mutex = m;
    mutex->attr = (pthread_mutexattr_t *)attr;
#else
    if (mutex == NULL) {
        return -EINVAL;
    }

    m = os_zalloc(sizeof(struct os_mutex));
    if (m == NULL) {
        pthread_err("alloc fail\r\n");
        return -ENOMEM;
    }

    os_mutex_init(m);
    *mutex = (pthread_mutex_t)m;
#endif
    return RET_OK;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
#ifdef __MINILIBC__
    if (NULL == mutex) {
        return -EINVAL;
    }

    if (NULL == mutex->mutex) {
        pthread_mutex_init(mutex, NULL);
    }

    return os_mutex_lock((struct os_mutex *)(mutex->mutex), osWaitForever);
#else
    struct os_mutex *m;

    if (*mutex == 0) {
        pthread_mutex_init(mutex, NULL);
    }
    m = (struct os_mutex *)(*mutex);
    if (m) {
        return os_mutex_lock(m, osWaitForever);
    }
    return -EINVAL;
#endif
}

int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime)
{
#ifdef __MINILIBC__
    if (NULL == mutex) {
        return -EINVAL;
    }

    if (NULL == mutex->mutex) {
        pthread_mutex_init(mutex, NULL);
    }

    return os_mutex_lock((struct os_mutex *)(mutex->mutex), pthread_timespec_delta(abstime));
#else
    struct os_mutex *m;

    if (*mutex == 0) {
        pthread_mutex_init(mutex, NULL);
    }
    m = (struct os_mutex *)(*mutex);
    if (m) {
        return os_mutex_lock(m, pthread_timespec_delta(abstime));
    }
    return -EINVAL;
#endif
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
#ifdef __MINILIBC__
    if (NULL == mutex) {
        return -EINVAL;
    }

    if (NULL == mutex->mutex) {
        pthread_mutex_init(mutex, NULL);
    }

    return os_mutex_lock((struct os_mutex *)(mutex->mutex), 0);
#else
    struct os_mutex *m;

    if (*mutex == 0) {
        pthread_mutex_init(mutex, NULL);
    }
    m = (struct os_mutex *)(*mutex);
    if (m) {
        return os_mutex_lock(m, 0);
    }
    return -EINVAL;
#endif
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
#ifdef __MINILIBC__
    if (NULL == mutex) {
        return -EINVAL;
    }

    if (NULL == mutex->mutex) {
        pthread_mutex_init(mutex, NULL);
    }

    return os_mutex_unlock((struct os_mutex *)(mutex->mutex));
#else
    struct os_mutex *m;

    if (*mutex == 0) {
        pthread_mutex_init(mutex, NULL);
    }
    m = (struct os_mutex *)(*mutex);
    if (m) {
        return os_mutex_unlock(m);
    }
    return -EINVAL;
#endif
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    return 0;
}

#endif

