#ifndef _HG_SYSAES_V3_H_
#define _HG_SYSAES_V3_H_
#include "hal/sysaes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hg_sysaes_v3 {
    struct sysaes_dev   dev;
    void               *hw;
    struct os_mutex     lock;
    struct os_semaphore done;
    uint32              irq_num;
    uint32              flags;
#ifdef CONFIG_SLEEP
    uint32              *regs;
#endif    
};

int32 hg_sysaes_v3_attach(uint32 dev_id, struct hg_sysaes_v3 *sysaes);

#endif /* _HG_SYSAES_V3_H_ */
