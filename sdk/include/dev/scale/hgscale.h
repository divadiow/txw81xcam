#ifndef HG_SCALE_H
#define HG_SCALE_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/scale.h"

typedef enum {
	FRAME_END,
	INBUF_OV,
	SCALE_IRQ_NUM
}SCALE_IRQ_E;
 

struct hgscale {
    struct scale_device   dev;
	uint32              hw;
    scale_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    uint32              opened:1, use_dma:1, dsleep:1;
	uint32 *cfg_backup;
};


int32 hgscale1_attach(uint32 dev_id, struct hgscale *scale);
int32 hgscale2_attach(uint32 dev_id, struct hgscale *scale);
int32 hgscale3_attach(uint32 dev_id, struct hgscale *scale);

#endif