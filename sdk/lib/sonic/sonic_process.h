#ifndef _SONIC_PROCESS_H_
#define _SONIC_PROCESS_H_

#include "sonic.h"

sonicStream sonicStream_init(int32_t samplerate, int32_t channels, float speed, float pitch);
void sonicStream_deinit(sonicStream stream);
void sonicStream_input_data(sonicStream stream,short *inBuffer,int samplesRead);
int sonicStream_output_data(sonicStream stream,short *outBuffer,int maxlen);
void sonic_flush_stream(sonicStream stream);
int sonicStream_output_available(sonicStream stream);
void sonicStream_set_pitch(sonicStream stream, float pitch);
void sonicStream_set_speed(sonicStream stream, float speed);
#endif