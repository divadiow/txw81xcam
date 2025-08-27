#ifndef _AEC_PROCESS_H_
#define _AEC_PROCESS_H_

#include "../modules/audio_processing/aecm/echo_control_mobile.h"

extern void *aecmInst;

extern int aec_init(uint32_t sampleRate, int16_t nMode);
extern int push_farbuf(short *data, unsigned int len, uint32_t sampleRate);
extern int aec_process(int16_t *near_frame, uint32_t inSampleCount, int16_t msInSndCardBuf, uint32_t sampleRate);

#endif
