#ifndef _OS_TASK_H_
#define _OS_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*os_task_func_t)(void *arg);

enum OS_TASK_FLAGS{
    OS_TASK_FLAGS_LPRUN = BIT(31), //TASK在低功耗保活状态下需要调度运行
};

typedef enum  {
    OS_TASK_PRIORITY_IDLE         = 0,
    OS_TASK_PRIORITY_LOW          = 0x10,
    OS_TASK_PRIORITY_BELOW_NORMAL = 0x20,
    OS_TASK_PRIORITY_NORMAL       = 0x30,
    OS_TASK_PRIORITY_ABOVE_NORMAL = 0x40,
    OS_TASK_PRIORITY_HIGH         = 0x50,
    OS_TASK_PRIORITY_REALTIME     = 0x60,
    OS_TASK_PRIORITY_ISR          = 0xFF,
} OS_TASK_PRIORITY;

#define OS_TASK_INIT(name, task, func, data, prio, stksize) do { \
        os_task_init((const uint8 *)name, task, (os_task_func_t)func, (uint32)data); \
        os_task_set_stacksize(task, NULL, stksize); \
        os_task_set_priority(task, prio); \
        os_task_run(task);\
    }while(0)

#define OS_TASK_INIT2(name, task, func, data, prio, stack,stksize) do { \
        os_task_init((const uint8 *)name, task, (os_task_func_t)func, (uint32)data); \
        os_task_set_stacksize(task, stack, stksize); \
        os_task_set_priority(task, prio); \
        os_task_run(task);\
    }while(0)

#ifndef OS_BLKLIST
#define OS_BLKLIST
struct os_blklist{
    void          *hdl;
};
#endif

struct os_task {
    void          *hdl;
    os_task_func_t func;
    const char    *name;
    uint32_t       args;
    uint32_t       priority:8, stack_size:20, lprun:1, rev:3;
    void          *stack;
};

struct os_task_info {
    uint32 id;
    const  char *name;
    const  char *status;
    uint32 time;
    uint32 stack;
    uint32 prio;
    uint32 arg;
};


int32 os_task_init(const uint8 *name, struct os_task *task, os_task_func_t func, uint32 data);
int32 os_task_priority(struct os_task *task);
int32 os_task_stacksize(struct os_task *task);
int32 os_task_set_priority(struct os_task *task, uint32 priority);
int32 os_task_set_stacksize(struct os_task *task, void *stack, int32 stack_size);
int32 os_task_run(struct os_task *task);
int32 os_task_stop(struct os_task *task);
int32 os_task_del(struct os_task *task);
int32 os_task_runtime(struct os_task_info *tsk_times, int32 count);
void os_task_print(void);
void *os_task_current(void);
void *os_task_data(void *hdl);
int32 os_task_suspend(struct os_task *task);
int32 os_task_resume(struct os_task *task);
void os_task_dump(void *hdl, void *stack);
int32 os_task_count(void);
int32 os_task_yield(void);

int32 os_sched_disable(void);
int32 os_sched_enbale(void);
void os_lpower_mode(uint8 enable);

void *os_task_create(const char *name, os_task_func_t func, void *args, uint32 prio, uint32 time, void *stack, uint32 stack_size);
int32 os_task_destroy(void *hdl);

int32 os_task_suspend2(void *hdl);
int32 os_task_resume2(void  *hdl);

int32 os_blklist_init(struct os_blklist *blkobj);
void os_blklist_del(struct os_blklist *blkobj);
void os_blklist_suspend(struct os_blklist *blkobj, void *task_hdl);
void os_blklist_resume(struct os_blklist *blkobj);

#ifdef __cplusplus
}
#endif
#endif

