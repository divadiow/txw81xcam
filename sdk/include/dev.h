#ifndef _HUGEIC_DEV_H_
#define _HUGEIC_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

struct dev_obj;

struct devobj_ops{
#ifdef CONFIG_SLEEP
    int32 (*suspend)(struct dev_obj *obj);
    int32 (*resume)(struct dev_obj *obj);
#endif
};

#ifndef OS_BLKLIST
#define OS_BLKLIST
struct os_blklist{
    void *hdl;
};
#endif

struct dev_obj{
    uint16 dev_id;
    uint8  busy; /* device 被使用状态，由驱动代码设置。
                    有8bit可用，驱动代码根据 read/write/... 设置不同的bit位 */
    uint8  suspend: 1, /* device进入suspend状态，由dev_suspend API设置 */
           rev: 7;
    struct dev_obj *next;
#ifdef CONFIG_SLEEP
    struct os_blklist blklist;
#endif
    const struct devobj_ops *ops;
};

#define DEV_BUSY(dev, val, set)      dev_busy((struct dev_obj *)(dev), val, set)
#define DEV_SUSPENDED(dev, suspend)  dev_suspended((struct dev_obj *)(dev), suspend)
#define HALDEV_SUSPENDED(dev)        if(dev_suspended((struct dev_obj *)dev, 1)) return RET_ERR

/**
 * dev core module initialize
 * @return return RET_OK if initialize sucess, else return RET_ERR.
 **/
extern int32 dev_init(void);

/**
 * get dev by dev_id
 * @param[in] dev_id device id
 * @return return device object if device exist, else return NULL.
 **/
extern struct dev_obj *dev_get(int32 dev_id);

/*
* set or clear device busy flag.
*/
extern void dev_busy(struct dev_obj *dev, uint8 busy, uint8 set);

/**
 * register device into dev manager.
 * @param[in] dev_id device id.
 * @param[in] device object to register.
 * @return return RET_OK if register sucess, else return RET_ERR.
 **/
extern int32 dev_register(uint32 dev_id, struct dev_obj *device);

/**
 * unregister device into dev manager.
 * @param[in] device object to unregister.
 * @return return RET_OK if register sucess, else return RET_ERR.
 **/
extern int32 dev_unregister(struct dev_obj *device);

/* 检查指定的device是否已经被suspend，
   参数suspend：表示如果device已经被suspend，是否suspend当前task
*/
int32 dev_suspended(struct dev_obj *dev, uint8 suspend);
/**
 * device suspend when system enter sleep.
 * @param[in] dsleep sleep or deep sleep.
 * @return return RET_OK if unregister sucess, else return RET_ERR.
 **/
extern int32 dev_suspend(uint16 type);
extern int32 dev_suspend_hook(struct dev_obj *dev, uint16 type);

/**
 * device resume when system resume.
 * @return return RET_OK if unregister sucess, else return RET_ERR.
 **/
extern int32 dev_resume(uint16 type, uint32 wkreason);
extern int32 dev_resume_hook(struct dev_obj *dev, uint16 type, uint32 wkreason);

extern int32 pin_func(int32 dev_id, int32 request);

#ifdef __cplusplus
}
#endif

#endif
