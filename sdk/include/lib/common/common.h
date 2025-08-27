
#ifndef _HGIC_COMMON_H_
#define _HGIC_COMMON_H_

void do_global_ctors(void);
void do_global_dtors(void);

void cpu_loading_print(uint8 all, struct os_task_info *tsk_info, uint32 size);

void sys_wakeup_host(void);

void module_version_show(void);

extern uint32 sdk_version;
extern uint32 svn_version;
extern uint32 app_version;

#endif

