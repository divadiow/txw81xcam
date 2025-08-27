#ifndef _HGIC_CPURPC_H_
#define _HGIC_CPURPC_H_

int32 cpu_rpc_init(uint32 addr, uint32 irq);
int32 cpu_rpc_call(uint32 func_id, uint32 *args, uint32 arg_cnt, uint32 sync);
void *cpu_rpc_func(uint32 func_id);

#endif
