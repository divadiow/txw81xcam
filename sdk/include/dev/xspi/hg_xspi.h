#ifndef _HG_XSPI_H_
#define _HG_XSPI_H_
#include "hal/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

enum hg_xspi_flags {
    hg_xspi_flags_ready,
    hg_xspi_flags_suspend,
};

struct hg_xspi {
    struct dev_obj          dev;
    void                   *hw;
    uint32                  irq_num;
    //qspi_irq_hdl            irq_hdl;
    uint32                  irq_data;
    uint32                  opened;
    uint32                  flags;
#ifdef CONFIG_SLEEP
    uint32                  sys_regs[6];
    uint32                  ospi_regs[256/4];
#endif    
};
extern struct hg_xspi xspi;



int32 hg_xspi_attach(uint32 dev_id, struct hg_xspi *p_spi);
void hg_xspi_sample_offset(uint32 ospi_base, int8 rx_offset, int8 tx_offset);

#ifdef __cplusplus
}
#endif

#endif 

