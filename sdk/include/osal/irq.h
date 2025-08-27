#ifndef __OS_IRQ_H_
#define __OS_IRQ_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*irq_handle)(void *data);
struct sys_hwirq {
    void *data;
    irq_handle handle;
#ifdef SYS_IRQ_STAT
    uint32 trig_cnt;
    uint32 tot_cycle;
    uint32 max_cycle;
#endif
};

__dsleep_text void irq_enable(uint32 irq);
__dsleep_text void irq_disable(uint32 irq);
__dsleep_text uint32 disable_irq(void);
__dsleep_text void enable_irq(uint32 flag);
int32 request_irq(uint32 irq_num, irq_handle handle, void *data);
int32 release_irq(uint32 irq_num);
uint32 irq_status(void);
uint32 cpu_runtime_calc(uint8 start, uint32 val);

#ifdef __cplusplus
}
#endif
#endif
