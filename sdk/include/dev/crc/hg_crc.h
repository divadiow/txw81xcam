#ifndef _HG_CRC_H_
#define _HG_CRC_H_
#include "hal/crc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hg_crc {
    struct crc_dev          dev;
    void                   *hw;

    struct os_mutex         lock;
    struct os_semaphore     done;
    uint32                  irq_num;
    uint32                  flags;

#ifdef CONFIG_SLEEP
    uint32                 *regs;
#endif
};

int32 hg_crc_attach(uint32 dev_id, struct hg_crc *crc_c);

#ifdef __cplusplus
}
#endif

#endif /* _HG_CRC_H_ */

