#ifndef _HGSHA_V0_H_
#define _HGSHA_V0_H_
#include "hal/sha.h"

#ifdef __cplusplus
extern "C" {
#endif



struct hgsha_v0 {
    struct sha_dev      dev;
    void                *hw;
    struct os_mutex     lock;
    struct os_semaphore done;
    uint32              irq_hw_num;
    uint32              irq_num;
    uint32              alg_type;
    uint8               buf[64];
    unsigned long long  bit_len;
    uint32              buf_cnt;
    uint32              blk_size;
    void *              task;
    void *              usr_p;
    volatile uint32     flags_intr_busy      :1,
                        flags_intr_handing   :1,
                        flags_intr_revs      :30;

    volatile uint32     flags_mod_started    :1,
                        flags_got_result     :1,
                        flags_redir_result   :1,
                        flags_revs           :29;
#ifdef CONFIG_SLEEP
    uint32 hash[8];
#endif
    // volatile uint32     ;
    void (*sha_irq_func)(void *args);
    void *irq_func_data;
};

int32 hgsha_v0_attach(uint32 dev_id, struct hgsha_v0 *sha);

#endif /* _hgsha256_H_ */
