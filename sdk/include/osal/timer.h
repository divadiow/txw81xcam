
#ifndef __OS_TIMER_H_
#define __OS_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal/time.h"

typedef void (*os_timer_func_t)(void *arg);

enum OS_TIMER_MODE {
    OS_TIMER_MODE_ONCE,
    OS_TIMER_MODE_PERIODIC
};

struct os_timer {
    uint32_t magic;
    void    *hdl;
    os_timer_func_t cb;
    void        *data;
    uint8        mode;
    uint32       interval;
    uint32_t     trigger_cnt;
    uint32_t     total_time;
    uint32_t     max_time;
};

int os_timer_init(struct os_timer *timer, os_timer_func_t func, enum OS_TIMER_MODE mode, void *arg);
int os_timer_start(struct os_timer *timer, unsigned long expires);
int os_timer_stop(struct os_timer *timer);
int os_timer_del(struct os_timer *timer);
int os_timer_stat(struct os_timer *timer);

#ifdef __cplusplus
}
#endif
#endif

