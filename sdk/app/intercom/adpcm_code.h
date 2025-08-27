#ifndef _ADPCM_CODE_H_
#define _ADPCM_CODE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "ringbuf.h"
#include "lowcfe.h"

struct adpcm_channel {
    int32_t pcmdata;                        // current PCM value
    int32_t shaping_weight, error;          // for noise shaping
    int8_t index;                           // current index into step size table
};

struct adpcm_context {
    struct adpcm_channel channels [2];
    int num_channels, sample_rate, config_flags;
    int16_t *dynamic_shaping_array, last_shaping_weight;
    int static_shaping_weight;
};

typedef struct {
    struct adpcm_context *normal_cnxt;
    struct adpcm_context *rddancy_cnxt;
    TYPE_RINGBUF *ringbuf_original;
    TYPE_RINGBUF *ringbuf_encode;
    uint32_t rddancy_num;
}AdpcmEncoder;

typedef struct {
    uint8_t *rddancy_encbuf; 
    uint32_t rddancy_num;  
    uint32_t rddancy_encode_len;
    uint32_t rddancy_encbuf_num;
    uint32_t rddancy_encbuf_pos;
    uint32_t block_size;
    uint8_t plc_sta;
    LowcFE_c *lowcfe;
    int16_t *decode_buf;
}AdpcmDecoder;

typedef uint64_t rms_error_t;     // best if "double" or "uint64_t", "float" okay in a pinch
#define MAX_RMS_ERROR UINT64_MAX

#define NOISE_SHAPING_OFF       0       // flat noise (no shaping)
#define NOISE_SHAPING_STATIC    0x100   // static 1st-order shaping (configurable, highpass default)
#define NOISE_SHAPING_DYNAMIC   0x200   // dynamically tilted noise based on signal

#define LOOKAHEAD_DEPTH         0x0ff   // depth of search
#define LOOKAHEAD_EXHAUSTIVE    0x800   // full breadth of search (all branches taken)
#define LOOKAHEAD_NO_BRANCHING  0x400   // no branches taken (internal use only!)

#define CLIP(data, min, max) \
if ((data) > (max)) data = max; \
else if ((data) < (min)) data = min;

#define NIBBLE_TO_DELTA(b,n) ((n)<(1<<((b)-1))?(n)+1:(1<<((b)-1))-1-(n))
#define DELTA_TO_NIBBLE(b,d) ((d)<0?(1<<((b)-1))-1-(d):(d)-1)

#define NOISE_SHAPING_ENABLED   (NOISE_SHAPING_DYNAMIC | NOISE_SHAPING_STATIC)

#define adpcm_malloc malloc
#define adpcm_free   free

static void win_average_buffer (float *samples, int sample_count, int half_width);
int adpcm_encode(AdpcmEncoder *adpcm_enc, uint8_t *outbuf, int *outbufsize, int16_t *inbuf, int inbufcount, int bps);
int adpcm_decode(AdpcmDecoder *adpcm_dec, int16_t *outbuf, const uint8_t *inbuf, uint32_t inbufsize, int bps);
AdpcmEncoder *adpcm_encoder_create(int sample_rate, int lookahead, int noise_shaping, int block_size, int redundancy_num);
AdpcmDecoder *adpcm_decoder_create(int block_size, int redundancy_num);
int adpcm_decode_plc(AdpcmDecoder *adpcm_dec, int16_t *outbuf);
#endif