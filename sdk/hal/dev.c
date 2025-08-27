#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/sleep.h"
#include "osal/task.h"
#include "osal/irq.h"

struct dev_mgr {
    struct dev_obj  *next;
    struct os_mutex  mutex;
};

__bobj static struct dev_mgr s_dev_mgr;

__init int32 dev_init()
{
    os_mutex_init(&s_dev_mgr.mutex);
    s_dev_mgr.next = NULL;
    return RET_OK;
}

struct dev_obj *dev_get(int32 dev_id)
{
    struct dev_obj *dev = NULL;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.next;
    while (dev) {
        if (dev->dev_id == dev_id) {
            break;
        }
        dev = dev->next;
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return dev;
}

void dev_busy(struct dev_obj *dev, uint8 busy, uint8 set)
{
    uint32 flag = disable_irq();
    if (set) {
        dev->busy |= busy;
    } else {
        dev->busy &= ~ busy;
    }
    enable_irq(flag);
}

__init int32 dev_register(uint32 dev_id, struct dev_obj *device)
{
    struct dev_obj *dev = dev_get(dev_id);
    if (dev) {
        return -EEXIST;
    }
    device->dev_id = dev_id;
#ifdef CONFIG_SLEEP
    os_blklist_init(&device->blklist);
#endif
    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    device->next   = s_dev_mgr.next;
    s_dev_mgr.next = device;
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}

int32 dev_unregister(struct dev_obj *device)
{
    struct dev_obj *dev = NULL;
    struct dev_obj *last = NULL;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.next;
    while (dev) {
        if (dev == device) {
            // check first dev
            if (device == s_dev_mgr.next) {
                s_dev_mgr.next = device->next;
            } else {
                last->next = device->next;
            }
#ifdef CONFIG_SLEEP
            os_blklist_del(&device->blklist);
#endif
            break;
        }
        last = dev;
        dev  = dev->next;
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}

#ifdef CONFIG_SLEEP
int32 dev_suspended(struct dev_obj *dev, uint8 suspend)
{
    if (dev->suspend && suspend && !__in_interrupt()) { /* 非中断上下文，直接挂起当前task */
        os_blklist_suspend(&dev->blklist, NULL);
    }
    return dev->suspend;
}

__weak int32 dev_suspend_hook(struct dev_obj *dev, uint16 type)
{
    return 1;
}
int32 dev_suspend(uint16 type)
{
    int32 loop;
    struct dev_obj *dev = NULL;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.next;
    while (dev) {
        if (dev->ops && dev->ops->suspend && !dev->suspend) {
            if (dev_suspend_hook(dev, type)) {
                loop = 0;
                dev->suspend = 1;
                while (dev->busy) {
                    os_sleep_ms(1);
                    if (loop++ > 100) {
                        loop = 0;
                        os_printf(KERN_WARNING"device %d busy !!!!\r\n", dev->dev_id);
                    }
                }
                dev->ops->suspend(dev);
            }
        }
        dev = dev->next;
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}

__weak int32 dev_resume_hook(struct dev_obj *dev, uint16 type, uint32 wkreason)
{
    return 1;
}
int32 dev_resume(uint16 type, uint32 wkreason)
{
    struct dev_obj *dev = NULL;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.next;
    while (dev) {
        if (dev->ops && dev->ops->resume && dev->suspend) {
            if (dev_resume_hook(dev, type, wkreason)) {
                dev->ops->resume(dev);
                dev->suspend = 0;
                os_blklist_resume(&dev->blklist);//唤醒所有被挂起的task
            }
        }
        dev = dev->next;
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}
#else
int32 dev_suspended(struct dev_obj *dev, uint8 suspend)
{
    return 0;
}

__weak int32 dev_suspend_hook(struct dev_obj *dev, uint16 type)
{
    return 1;
}
int32 dev_suspend(uint16 type)
{
    return RET_ERR;
}

__weak int32 dev_resume_hook(struct dev_obj *dev, uint16 type, uint32 wkreason)
{
    return 1;
}
int32 dev_resume(uint16 type, uint32 wkreason)
{
    return RET_ERR;
}

#endif

