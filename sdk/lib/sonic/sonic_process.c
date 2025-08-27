#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sonic_process.h"

#define AUDIO_LEN   1024

sonicStream sonicStream_init(int32_t samplerate, int32_t channels, float speed, float pitch)
{
    sonicStream stream;

    stream = sonicCreateStream(samplerate, channels);
    if(stream == NULL) {
        printf("sonicStream_init err!\n");
        return NULL;
    }
    sonicSetSpeed(stream, speed);
    sonicSetPitch(stream, pitch);
    sonicSetRate(stream, 1);
    sonicSetVolume(stream, 1);
    sonicSetQuality(stream, 0);\
	os_printf("sonicStream_init succwss,samplingrate:%d,channel:%d,speed:%f,pitch:%f",samplerate,channels,speed,pitch);
    return stream;
}

void sonicStream_deinit(sonicStream stream)
{
  sonicDestroyStream(stream);
}

void sonicStream_input_data(sonicStream stream,short *inBuffer,int samplesRead)
{
  sonicWriteShortToStream(stream, inBuffer, samplesRead);
}

int sonicStream_output_data(sonicStream stream,short *outBuffer,int maxlen)
{
  int samplesWritten;
  samplesWritten = sonicReadShortFromStream(stream, outBuffer,maxlen);
  return samplesWritten;
}

int sonicStream_output_available(sonicStream stream)
{
    return get_sonicReadLen(stream);
}

void sonic_flush_stream(sonicStream stream)
{
  sonicFlushStream(stream);
}

void sonicStream_set_pitch(sonicStream stream, float pitch)
{
	sonicSetPitch(stream, pitch);
}

void sonicStream_set_speed(sonicStream stream, float speed)
{
	sonicSetSpeed(stream, speed);
}