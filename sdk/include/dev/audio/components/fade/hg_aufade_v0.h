#ifndef _HG_AUFADE_V0_H_
#define _HG_AUFADE_V0_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dev/audio/components/fade/aufade.h"
#include "hg_audio_v0_hw.h"

int32 hg_aufade_v0_init(void);
int32 hg_aufade_v0_open(struct aufade_device *aufade);
int32 hg_aufade_v0_close(struct aufade_device *aufade);
int32 hg_aufade_v0_start(struct aufade_device *aufade, enum aufade_mode mode);
int32 hg_aufade_v0_ioctl(struct aufade_device *aufade, enum aufade_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
int32 hg_aufade_v0_request_irq(struct aufade_device *aufade, enum aufade_irq_flag irq_flag, aufade_irq_hdl irq_hdl, uint32 irq_data);
int32 hg_aufade_v0_release_irq(struct aufade_device *aufade, enum aufade_irq_flag irq_flag);
int32 hg_aufade_v0_set_gain(struct hg_audio_hw_v0 *audio_hw, uint32 gain);
#ifdef __cplusplus
}
#endif

#endif /* _HG_AUFADE_V0_H_ */
