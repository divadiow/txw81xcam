#ifndef _VAD_PROCESS_H_
#define _VAD_PROCESS_H_

#include "../modules/audio_processing/vad/vad.h"

extern int vad_init(int16_t vad_mode);
extern int vad_process(int16_t *buffer, uint32_t sampleRate, size_t samplesCount, int per_ms_frames);

#endif