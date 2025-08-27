#ifndef _NS_PROCESS_H_
#define _NS_PROCESS_H_

#include "../modules/audio_processing/ns/noise_suppression_x.h"
extern int ns_init(uint32 samplerate);
extern int ns_process(int16_t *buffer, uint32_t sampleRate, uint64_t samplesCount);
#endif