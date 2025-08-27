#ifndef _AGC_PROCESS_H_
#define _AGC_PROCESS_H_

#include "../modules/audio_processing/agc/agc.h"

extern int agc_init(int16_t agcMode, uint32_t sampleRate);
extern int agc_process(int16_t *buffer, uint32_t sampleRate, size_t samplesCount);

#endif
