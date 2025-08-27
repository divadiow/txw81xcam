#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "adpcm_code.h"

/* step table */
static const uint16_t step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

/* step index tables */
static const int index_table[] = {
    /* adpcm data size is 4 */
    -1, -1, -1, -1, 2, 4, 6, 8
};

static const int index_table_3bit[] = {
    /* adpcm data size is 3 */
    -1, -1, 1, 2
};

static const int index_table_5bit[] = {
    /* adpcm data size is 5 */
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16
};

//void adpcm_downsamlpe_2(int16_t *buf, uint32_t size)
//{
//    int temp = 0;
//    for(uint32_t i=0; i<(size/2); i++) {
//        buf[i] = buf[2*i];
//    }
//}
//void adpcm_upsamlpe_2(int16_t *buf, uint32_t size)
//{
//    int temp = 0;
//    memcpy(buf+size,buf,size*2); 
//    for(uint32_t i=0; i<size; i++) {
//        buf[2*i] = buf[size+i];
//        buf[2*i+1] = buf[2*i];
//    }
//}

static inline int32_t noise_shape (struct adpcm_channel *pchan, int32_t sample)
{
    int32_t temp = -((pchan->shaping_weight * pchan->error + 512) >> 10);

    if (pchan->shaping_weight < 0 && temp) {
        if (temp == pchan->error)
            temp = (temp < 0) ? temp + 1 : temp - 1;

        pchan->error = -sample;
        sample += temp;
    }
    else
        pchan->error = -(sample += temp);

    return sample;
}

static rms_error_t min_error_4bit (const struct adpcm_channel *pchan, int nch, int32_t csample, const int16_t *psample, int flags, int *best_nibble, rms_error_t max_error)
{
    int32_t delta = csample - pchan->pcmdata, csample2;
    struct adpcm_channel chan = *pchan;
    uint16_t step = step_table[chan.index];
    uint16_t trial_delta = (step >> 3);
    int nibble, testnbl;
    rms_error_t min_error;

    // this odd-looking code always generates the nibble value with the least error,
    // regardless of step size (which was not true previously)

    if (delta < 0) {
        int mag = ((-delta << 2) + (step & 3) + ((step & 1) << 1)) / step;
        nibble = 0x8 | (mag > 7 ? 7 : mag);
    }
    else {
        int mag = ((delta << 2) + (step & 3) + ((step & 1) << 1)) / step;
        nibble = mag > 7 ? 7 : mag;
    }

    if (nibble & 1) trial_delta += (step >> 2);
    if (nibble & 2) trial_delta += (step >> 1);
    if (nibble & 4) trial_delta += step;

    if (nibble & 8)
        chan.pcmdata -= trial_delta;
    else
        chan.pcmdata += trial_delta;

    CLIP(chan.pcmdata, -32768, 32767);
    if (best_nibble) *best_nibble = nibble;
    min_error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);

    // if we're at a leaf, or we're not at a leaf but have already exceeded the error limit, return
    if (!(flags & LOOKAHEAD_DEPTH) || min_error >= max_error)
        return min_error;

    // otherwise we execute that naively closest nibble and search deeper for improvement

    chan.index += index_table[nibble & 0x07];
    CLIP(chan.index, 0, 88);

    if (flags & NOISE_SHAPING_ENABLED) {
        chan.error += chan.pcmdata;
        csample2 = noise_shape (&chan, psample [nch]);
    }
    else
        csample2 = psample [nch];

    min_error += min_error_4bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, max_error - min_error);

    // min_error is the error (from here to the leaf) for the naively closest nibble.
    // Unless we've been told not to try, we may be able to improve on that by choosing
    // an alternative (not closest) nibble.

    if (flags & LOOKAHEAD_NO_BRANCHING)
        return min_error;

    for (testnbl = 0; testnbl <= 0xF; ++testnbl) {
        rms_error_t error, threshold;

        if (testnbl == nibble)  // don't do the same value again
            continue;

        // we execute this branch if:
        // 1. we're doing an exhaustive search, or
        // 2. the test value is one of the maximum values (i.e., 0x7 or 0xf), or
        // 3. the test value's delta is within three of the initial estimate's delta

        if (flags & LOOKAHEAD_EXHAUSTIVE || !(~testnbl & 0x7) || abs (NIBBLE_TO_DELTA (4,nibble) - NIBBLE_TO_DELTA (4,testnbl)) <= 3) {
            trial_delta = (step >> 3);
            chan = *pchan;

            if (testnbl & 1) trial_delta += (step >> 2);
            if (testnbl & 2) trial_delta += (step >> 1);
            if (testnbl & 4) trial_delta += step;

            if (testnbl & 8)
                chan.pcmdata -= trial_delta;
            else
                chan.pcmdata += trial_delta;

            CLIP(chan.pcmdata, -32768, 32767);

            error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);
            threshold = max_error < min_error ? max_error : min_error;

            if (error < threshold) {
                chan.index += index_table[testnbl & 0x07];
                CLIP(chan.index, 0, 88);

                if (flags & NOISE_SHAPING_ENABLED) {
                    chan.error += chan.pcmdata;
                    csample2 = noise_shape (&chan, psample [nch]);
                }
                else
                    csample2 = psample [nch];

                error += min_error_4bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, threshold - error);

                if (error < min_error) {
                    if (best_nibble) *best_nibble = testnbl;
                    min_error = error;
                }
            }
        }
    }

    return min_error;
}

static rms_error_t min_error_2bit (const struct adpcm_channel *pchan, int nch, int32_t csample, const int16_t *psample, int flags, int *best_nibble, rms_error_t max_error)
{
    int32_t delta = csample - pchan->pcmdata, csample2;
    struct adpcm_channel chan = *pchan;
    uint16_t step = step_table[chan.index];
    int nibble, testnbl;
    rms_error_t min_error;

    if (delta < 0) {
        if (-delta >= step) {
            chan.pcmdata -= step + (step >> 1);
            nibble = 3;
        }
        else {
            chan.pcmdata -= step >> 1;
            nibble = 2;
        }
    }
    else
        chan.pcmdata += step * ((nibble = delta >= step)) + (step >> 1);

    CLIP(chan.pcmdata, -32768, 32767);
    if (best_nibble) *best_nibble = nibble;
    min_error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);

    // if we're at a leaf, or we're not at a leaf but have already exceeded the error limit, return
    if (!(flags & LOOKAHEAD_DEPTH) || min_error >= max_error)
        return min_error;

    // otherwise we execute that naively closest nibble and search deeper for improvement

    chan.index += (nibble & 1) * 3 - 1;
    CLIP(chan.index, 0, 88);

    if (flags & NOISE_SHAPING_ENABLED) {
        chan.error += chan.pcmdata;
        csample2 = noise_shape (&chan, psample [nch]);
    }
    else
        csample2 = psample [nch];

    min_error += min_error_2bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, max_error - min_error);

    // min_error is the error (from here to the leaf) for the naively closest nibble.
    // Unless we've been told not to try, we may be able to improve on that by choosing
    // an alternative (not closest) nibble.

    if (flags & LOOKAHEAD_NO_BRANCHING)
        return min_error;

    for (testnbl = 0; testnbl <= 0x3; ++testnbl) {
        rms_error_t error, threshold;

        if (testnbl == nibble)  // don't do the same value again
            continue;

        chan = *pchan;

        if (testnbl & 2)
            chan.pcmdata -= step * (testnbl & 1) + (step >> 1);
        else
            chan.pcmdata += step * (testnbl & 1) + (step >> 1);

        CLIP(chan.pcmdata, -32768, 32767);

        error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);
        threshold = max_error < min_error ? max_error : min_error;

        if (error < threshold) {
            chan.index += (testnbl & 1) * 3 - 1;
            CLIP(chan.index, 0, 88);

            if (flags & NOISE_SHAPING_ENABLED) {
                chan.error += chan.pcmdata;
                csample2 = noise_shape (&chan, psample [nch]);
            }
            else
                csample2 = psample [nch];

            error += min_error_2bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, threshold - error);

            if (error < min_error) {
                if (best_nibble) *best_nibble = testnbl;
                min_error = error;
            }
        }
    }

    return min_error;
}

static rms_error_t min_error_3bit (const struct adpcm_channel *pchan, int nch, int32_t csample, const int16_t *psample, int flags, int *best_nibble, rms_error_t max_error)
{
    int32_t delta = csample - pchan->pcmdata, csample2;
    struct adpcm_channel chan = *pchan;
    uint16_t step = step_table[chan.index];
    uint16_t trial_delta = (step >> 2);
    int nibble, testnbl;
    rms_error_t min_error;

    if (delta < 0) {
        int mag = ((-delta << 1) + (step & 1)) / step;
        nibble = 0x4 | (mag > 3 ? 3 : mag);
    }
    else {
        int mag = ((delta << 1) + (step & 1)) / step;
        nibble = mag > 3 ? 3 : mag;
    }

    if (nibble & 1) trial_delta += (step >> 1);
    if (nibble & 2) trial_delta += step;

    if (nibble & 4)
        chan.pcmdata -= trial_delta;
    else
        chan.pcmdata += trial_delta;

    CLIP(chan.pcmdata, -32768, 32767);
    if (best_nibble) *best_nibble = nibble;
    min_error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);

    // if we're at a leaf, or we're not at a leaf but have already exceeded the error limit, return
    if (!(flags & LOOKAHEAD_DEPTH) || min_error >= max_error)
        return min_error;

    // otherwise we execute that naively closest nibble and search deeper for improvement

    chan.index += index_table_3bit[nibble & 0x03];
    CLIP(chan.index, 0, 88);

    if (flags & NOISE_SHAPING_ENABLED) {
        chan.error += chan.pcmdata;
        csample2 = noise_shape (&chan, psample [nch]);
    }
    else
        csample2 = psample [nch];

    min_error += min_error_3bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, max_error - min_error);

    // min_error is the error (from here to the leaf) for the naively closest nibble.
    // Unless we've been told not to try, we may be able to improve on that by choosing
    // an alternative (not closest) nibble.

    if (flags & LOOKAHEAD_NO_BRANCHING)
        return min_error;

    for (testnbl = 0; testnbl <= 0x7; ++testnbl) {
        rms_error_t error, threshold;

        if (testnbl == nibble)  // don't do the same value again
            continue;

        // we execute this branch if:
        // 1. we're doing an exhaustive search, or
        // 2. the test value is one of the maximum values (i.e., 0x3 or 0x7), or
        // 3. the test value's delta is within two of the initial estimate's delta

        if (flags & LOOKAHEAD_EXHAUSTIVE || !(~testnbl & 0x3) || abs (NIBBLE_TO_DELTA (3,nibble) - NIBBLE_TO_DELTA (3,testnbl)) <= 2) {
            trial_delta = (step >> 2);
            chan = *pchan;

            if (testnbl & 1) trial_delta += (step >> 1);
            if (testnbl & 2) trial_delta += step;

            if (testnbl & 4)
                chan.pcmdata -= trial_delta;
            else
                chan.pcmdata += trial_delta;

            CLIP(chan.pcmdata, -32768, 32767);
            error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);
            threshold = max_error < min_error ? max_error : min_error;

            if (error < threshold) {
                chan.index += index_table_3bit[testnbl & 0x03];
                CLIP(chan.index, 0, 88);

                if (flags & NOISE_SHAPING_ENABLED) {
                    chan.error += chan.pcmdata;
                    csample2 = noise_shape (&chan, psample [nch]);
                }
                else
                    csample2 = psample [nch];

                error += min_error_3bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, threshold - error);

                if (error < min_error) {
                    if (best_nibble) *best_nibble = testnbl;
                    min_error = error;
                }
            }
        }
    }

    return min_error;
}

static rms_error_t min_error_5bit (const struct adpcm_channel *pchan, int nch, int32_t csample, const int16_t *psample, int flags, int *best_nibble, rms_error_t max_error)
{
    static char comp_table [16] = { 0, 0, 0, 5, 0, 6, 4, 10, 0, 7, 6, 10, 4, 11, 11, 13 };
    int32_t delta = csample - pchan->pcmdata, csample2;
    struct adpcm_channel chan = *pchan;
    uint16_t step = step_table[chan.index];
    uint16_t trial_delta = (step >> 4);
    int nibble, testnbl;
    rms_error_t min_error;

    if (delta < 0) {
        int mag = ((-delta << 3) + comp_table [step & 0xf]) / step;
        nibble = 0x10 | (mag > 0xf ? 0xf : mag);
    }
    else {
        int mag = ((delta << 3) + comp_table [step & 0xf]) / step;
        nibble = mag > 0xf ? 0xf : mag;
    }

    if (nibble & 1) trial_delta += (step >> 3);
    if (nibble & 2) trial_delta += (step >> 2);
    if (nibble & 4) trial_delta += (step >> 1);
    if (nibble & 8) trial_delta += step;

    if (nibble & 0x10)
        chan.pcmdata -= trial_delta;
    else
        chan.pcmdata += trial_delta;

    CLIP(chan.pcmdata, -32768, 32767);
    if (best_nibble) *best_nibble = nibble;
    min_error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);

    // if we're at a leaf, or we're not at a leaf but have already exceeded the error limit, return
    if (!(flags & LOOKAHEAD_DEPTH) || min_error >= max_error)
        return min_error;

    // otherwise we execute that naively closest nibble and search deeper for improvement

    chan.index += index_table_5bit[nibble & 0x0f];
    CLIP(chan.index, 0, 88);

    if (flags & NOISE_SHAPING_ENABLED) {
        chan.error += chan.pcmdata;
        csample2 = noise_shape (&chan, psample [nch]);
    }
    else
        csample2 = psample [nch];

    min_error += min_error_5bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, max_error - min_error);

    // min_error is the error (from here to the leaf) for the naively closest nibble.
    // Unless we've been told not to try, we may be able to improve on that by choosing
    // an alternative (not closest) nibble.

    if (flags & LOOKAHEAD_NO_BRANCHING)
        return min_error;

    for (testnbl = 0; testnbl <= 0x1F; ++testnbl) {
        rms_error_t error, threshold;

        if (testnbl == nibble)  // don't do the same value again
            continue;

        // we execute this trial if:
        // 1. we're doing an exhaustive search, or
        // 2. the trial value is one of the four maximum values for the sign, or
        // 3. the test value's delta is within three of the initial estimate's delta

        if (flags & LOOKAHEAD_EXHAUSTIVE || (testnbl | 3) == (nibble | 0xf) || abs (NIBBLE_TO_DELTA (5,nibble) - NIBBLE_TO_DELTA (5,testnbl)) <= 3) {
            trial_delta = (step >> 4);
            chan = *pchan;

            if (testnbl & 1) trial_delta += (step >> 3);
            if (testnbl & 2) trial_delta += (step >> 2);
            if (testnbl & 4) trial_delta += (step >> 1);
            if (testnbl & 8) trial_delta += step;

            if (testnbl & 0x10)
                chan.pcmdata -= trial_delta;
            else
                chan.pcmdata += trial_delta;

            CLIP(chan.pcmdata, -32768, 32767);

            error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);
            threshold = max_error < min_error ? max_error : min_error;

            if (error < threshold) {
                chan.index += index_table_5bit [testnbl & 0x0f];
                CLIP(chan.index, 0, 88);

                if (flags & NOISE_SHAPING_ENABLED) {
                    chan.error += chan.pcmdata;
                    csample2 = noise_shape (&chan, psample [nch]);
                }
                else
                    csample2 = psample [nch];

                error += min_error_5bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, threshold - error);

                if (error < min_error) {
                    if (best_nibble) *best_nibble = testnbl;
                    min_error = error;
                }
            }
        }
    }

    return min_error;
}

static uint8_t encode_sample (struct adpcm_context *pcnxt, int ch, int bps, const int16_t *psample, int num_samples)
{
    struct adpcm_channel *pchan = pcnxt->channels + ch;
    uint16_t step = step_table[pchan->index];
    int flags = pcnxt->config_flags, nibble;
    int32_t csample = *psample;
    uint16_t trial_delta;

    if (flags & NOISE_SHAPING_ENABLED)
        csample = noise_shape (pchan, csample); 

    if ((flags & LOOKAHEAD_DEPTH) > num_samples - 1)
        flags = (flags & ~LOOKAHEAD_DEPTH) + num_samples - 1;

    if (bps == 2) {
        min_error_2bit (pchan, pcnxt->num_channels, csample, psample, flags, &nibble, MAX_RMS_ERROR);

        if (nibble & 2)
            pchan->pcmdata -= step * (nibble & 1) + (step >> 1);
        else
            pchan->pcmdata += step * (nibble & 1) + (step >> 1);

        pchan->index += (nibble & 1) * 3 - 1;
    }
    else if (bps == 3) {
        min_error_3bit (pchan, pcnxt->num_channels, csample, psample, flags, &nibble, MAX_RMS_ERROR);
        trial_delta = (step >> 2);
        if (nibble & 1) trial_delta += (step >> 1);
        if (nibble & 2) trial_delta += step;

        if (nibble & 4)
            pchan->pcmdata -= trial_delta;
        else
            pchan->pcmdata += trial_delta;

        pchan->index += index_table_3bit[nibble & 0x03];
    }
    else if (bps == 4) {
        min_error_4bit (pchan, pcnxt->num_channels, csample, psample, flags, &nibble, MAX_RMS_ERROR);
        trial_delta = (step >> 3);
        if (nibble & 1) trial_delta += (step >> 2);
        if (nibble & 2) trial_delta += (step >> 1);
        if (nibble & 4) trial_delta += step;

        if (nibble & 8)
            pchan->pcmdata -= trial_delta;
        else
            pchan->pcmdata += trial_delta;

        pchan->index += index_table[nibble & 0x07];
    }
    else {  // bps == 5
        min_error_5bit (pchan, pcnxt->num_channels, csample, psample, flags, &nibble, MAX_RMS_ERROR);
        trial_delta = (step >> 4);
        if (nibble & 1) trial_delta += (step >> 3);
        if (nibble & 2) trial_delta += (step >> 2);
        if (nibble & 4) trial_delta += (step >> 1);
        if (nibble & 8) trial_delta += step;

        if (nibble & 0x10)
            pchan->pcmdata -= trial_delta;
        else
            pchan->pcmdata += trial_delta;

        pchan->index += index_table_5bit[nibble & 0x0f];
    }

    CLIP(pchan->index, 0, 88);
    CLIP(pchan->pcmdata, -32768, 32767);

    if (flags & NOISE_SHAPING_ENABLED)
        pchan->error += pchan->pcmdata;

    return nibble;
}

static void encode_chunks (struct adpcm_context *pcnxt, uint8_t *outbuf, int *outbufsize, const int16_t *inbuf, int inbufcount, int bps)
{
    const int16_t *pcmbuf;
    int ch;

    for (ch = 0; ch < pcnxt->num_channels; ++ch) {
        int shiftbits = 0, numbits = 0, i, j;

        if (pcnxt->config_flags & NOISE_SHAPING_STATIC)
            pcnxt->channels [ch].shaping_weight = pcnxt->static_shaping_weight;

        pcmbuf = inbuf + ch;

        for (j = i = 0; i < inbufcount; ++i) {
            if (pcnxt->config_flags & NOISE_SHAPING_DYNAMIC)
                pcnxt->channels [ch].shaping_weight = pcnxt->dynamic_shaping_array [i];

            shiftbits |= encode_sample (pcnxt, ch, bps, pcmbuf, inbufcount - i) << numbits;
            pcmbuf += pcnxt->num_channels;

            if ((numbits += bps) >= 8) {
                outbuf [(j & ~3) * pcnxt->num_channels + (ch * 4) + (j & 3)] = shiftbits;
                shiftbits >>= 8;
                numbits -= 8;
                j++;
            }
        }

        if (numbits)
            outbuf [(j & ~3) * pcnxt->num_channels + (ch * 4) + (j & 3)] = shiftbits;
    }

    *outbufsize += (inbufcount * bps + 31) / 32 * pcnxt->num_channels * 4;
}

int adpcm_encode_block(struct adpcm_context *adpcm_cnxt, uint8_t *outbuf, int *outbufsize, const int16_t *inbuf, int inbufcount, int bps)
{
    struct adpcm_context *pcnxt = (struct adpcm_context *) adpcm_cnxt;  
    *outbufsize = 0; 
    int ch = 0;

    if(bps < 2 || bps > 5)
        return 0;

    if(!inbufcount)
        return 0;

    for (ch = 0; ch < pcnxt->num_channels; ch++)
        pcnxt->channels[ch].pcmdata = *inbuf++;

    inbufcount--;  

    if (inbufcount && (pcnxt->channels [0].index < 0 || (pcnxt->config_flags & LOOKAHEAD_DEPTH) >= 3)) {
        int flags = 16 | LOOKAHEAD_NO_BRANCHING;

        if ((flags & LOOKAHEAD_DEPTH) > inbufcount - 1)
            flags = (flags & ~LOOKAHEAD_DEPTH) + inbufcount - 1;

        for (ch = 0; ch < pcnxt->num_channels; ch++) {
            rms_error_t min_error = MAX_RMS_ERROR;
            rms_error_t error_per_index [89];
            int best_index = 0, tindex;

            for (tindex = 0; tindex <= 88; tindex++) {
                struct adpcm_channel chan = pcnxt->channels [ch];

                chan.index = tindex;
                chan.shaping_weight = 0;

                if (bps == 2)
                    error_per_index [tindex] = min_error_2bit (&chan, pcnxt->num_channels, inbuf [ch], inbuf + ch, flags, NULL, MAX_RMS_ERROR);
                else if (bps == 3)
                    error_per_index [tindex] = min_error_3bit (&chan, pcnxt->num_channels, inbuf [ch], inbuf + ch, flags, NULL, MAX_RMS_ERROR);
                else if (bps == 5)
                    error_per_index [tindex] = min_error_5bit (&chan, pcnxt->num_channels, inbuf [ch], inbuf + ch, flags, NULL, MAX_RMS_ERROR);
                else
                    error_per_index [tindex] = min_error_4bit (&chan, pcnxt->num_channels, inbuf [ch], inbuf + ch, flags, NULL, MAX_RMS_ERROR);
            }

            // we use a 3-wide average window because the min_error_nbit() results can be noisy

            for (tindex = 0; tindex <= 87; tindex++) {
                rms_error_t terror = error_per_index [tindex];

                if (tindex)
                    terror = (error_per_index [tindex - 1] + terror + error_per_index [tindex + 1]) / 3;

                if (terror < min_error) {
                    best_index = tindex;
                    min_error = terror;
                }
            }
            pcnxt->channels [ch].index = best_index;
        }
    }

    // write the block header, which includes the first PCM sample verbatim

    for (ch = 0; ch < pcnxt->num_channels; ch++) {
        outbuf[0] = pcnxt->channels[ch].pcmdata;
        outbuf[1] = pcnxt->channels[ch].pcmdata >> 8;
        outbuf[2] = pcnxt->channels[ch].index;
        outbuf[3] = 0;

        outbuf += 4;
        *outbufsize += 4;
    } 

    void generate_dns_values (const int16_t *samples, int sample_count, int num_chans, int sample_rate,
        int16_t *values, int16_t min_value, int16_t last_value);

    if (inbufcount && (pcnxt->config_flags & NOISE_SHAPING_DYNAMIC)) {
        pcnxt->dynamic_shaping_array = malloc (inbufcount * sizeof (int16_t));
        generate_dns_values (inbuf, inbufcount, pcnxt->num_channels, pcnxt->sample_rate, pcnxt->dynamic_shaping_array, -512, pcnxt->last_shaping_weight);
        pcnxt->last_shaping_weight = pcnxt->dynamic_shaping_array [inbufcount - 1];
    }

    // encode the rest of the PCM samples, if any, into 32-bit, possibly interleaved, chunks

    if (inbufcount)
        encode_chunks(pcnxt, outbuf, outbufsize, inbuf, inbufcount, bps);

    if (pcnxt->dynamic_shaping_array && (pcnxt->config_flags & NOISE_SHAPING_DYNAMIC)) {
        free (pcnxt->dynamic_shaping_array);
        pcnxt->dynamic_shaping_array = NULL;
    }
    return 1;
}

int adpcm_encode(AdpcmEncoder *adpcm_enc, uint8_t *outbuf, int *outbufsize, int16_t *inbuf, int inbufcount, int bps)
{
    static uint8_t rddancy_encode_flag = 0;
    if(adpcm_enc->rddancy_num>0) { 

        TYPE_RINGBUF *ringbuf_original = adpcm_enc->ringbuf_original;
        TYPE_RINGBUF *ringbuf_encode = adpcm_enc->ringbuf_encode;
        int original_encode_len = (inbufcount*bps/8+4);
        int reddancy_encode_len = (inbufcount*4/8+4);
        int reddancy_total_len = 0;
        int rddancy_num = adpcm_enc->rddancy_num;

        uint8_t encode_buf[inbufcount*4/8+4];
        int16_t original_buf[inbufcount];

        push_ringbuf(ringbuf_original, (uint8_t*)inbuf, inbufcount*2);
        if(rddancy_encode_flag) {
            reddancy_encode_len = 0;
            if(adpcm_encode_block(adpcm_enc->rddancy_cnxt,encode_buf,&reddancy_encode_len,inbuf,inbufcount,4) != 1)
                return 0;
            if(reddancy_encode_len > 0) {
                push_ringbuf(ringbuf_encode,encode_buf,reddancy_encode_len);
            }
        }
        if(!rddancy_encode_flag)
            rddancy_encode_flag = 1;
        reddancy_total_len = reddancy_encode_len*rddancy_num;
        if(ringbuf_pop_available(ringbuf_original) >= (rddancy_num+1)*inbufcount*2) {
            pop_ringbuf(ringbuf_original,(uint8_t*)original_buf,inbufcount*2);
            original_encode_len = 0;
            if(adpcm_encode_block(adpcm_enc->normal_cnxt,outbuf+4,&original_encode_len,original_buf,inbufcount,bps) != 1) 
                return 0; 
            if(original_encode_len > 0) {    
                pop_ringbuf(ringbuf_encode,outbuf+4+original_encode_len,reddancy_encode_len);
                if(rddancy_num>1)
                    pop_ringbuf_notmove(ringbuf_encode,outbuf+4+original_encode_len+reddancy_encode_len,
                                                                reddancy_total_len-reddancy_encode_len);
            }  
            *outbufsize = original_encode_len+reddancy_total_len+4; 
            *((uint32_t*)outbuf) =  0x80008000;
            *((uint16_t*)outbuf) |= original_encode_len;  
            *((uint16_t*)outbuf+1) |= reddancy_encode_len;
        }
        else {    
            *outbufsize = original_encode_len+reddancy_total_len+4;   
            memset(outbuf,0,*outbufsize);
            *((uint16_t*)outbuf) |= original_encode_len;
            *((uint16_t*)outbuf+1) |= reddancy_encode_len;
        }
    }
    else {
        if(adpcm_encode_block(adpcm_enc->normal_cnxt,outbuf+4,outbufsize,inbuf,inbufcount,bps) != 1) 
            return 0;
        *((uint32_t*)outbuf) = 0x8000;   
        *((uint16_t*)outbuf) |= *outbufsize;  
        *outbufsize += 4;             
    }
    return 1;
}

int adpcm_decode_4bps (int16_t *outbuf, const uint8_t *inbuf, uint32_t inbufsize, int channels)
{
    int ch, samples = 1, chunks;
    int32_t pcmdata[2];
    int8_t index[2];

    if (inbufsize < (uint32_t) channels * 4)
        return 0;

    for (ch = 0; ch < channels; ch++) {
        *outbuf++ = pcmdata[ch] = (int16_t) (inbuf [0] | (inbuf [1] << 8));
        index[ch] = inbuf [2];

        if (index [ch] < 0 || index [ch] > 88 || inbuf [3])     // sanitize the input a little...
            return 0;

        inbufsize -= 4;
        inbuf += 4;
    }

    chunks = inbufsize / (channels * 4);
    samples += chunks * 8;

    while (chunks--) {
        int ch, i;

        for (ch = 0; ch < channels; ++ch) {

            for (i = 0; i < 4; ++i) {
                uint16_t step = step_table [index [ch]], delta = step >> 3;

                if (*inbuf & 1) delta += (step >> 2);
                if (*inbuf & 2) delta += (step >> 1);
                if (*inbuf & 4) delta += step;

                if (*inbuf & 8)
                    pcmdata[ch] -= delta;
                else
                    pcmdata[ch] += delta;

                index[ch] += index_table [*inbuf & 0x7];
                CLIP(index[ch], 0, 88);
                CLIP(pcmdata[ch], -32768, 32767);
                outbuf [i * 2 * channels] = pcmdata[ch];

                step = step_table [index [ch]]; delta = step >> 3;

                if (*inbuf & 0x10) delta += (step >> 2);
                if (*inbuf & 0x20) delta += (step >> 1);
                if (*inbuf & 0x40) delta += step;

                if (*inbuf & 0x80)
                    pcmdata[ch] -= delta;
                else
                    pcmdata[ch] += delta;
                
                index[ch] += index_table [(*inbuf >> 4) & 0x7];
                CLIP(index[ch], 0, 88);
                CLIP(pcmdata[ch], -32768, 32767);
                outbuf [(i * 2 + 1) * channels] = pcmdata[ch];

                inbuf++;
            }

            outbuf++;
        }

        outbuf += channels * 7;
    }

    return samples;
}

int adpcm_decode_block(int16_t *outbuf, const uint8_t *inbuf, uint32_t inbufsize, int channels, int bps)
{
    int samples = 1, ch;
    int32_t pcmdata[2];
    int8_t index[2];

    if (bps == 4)
        return adpcm_decode_4bps (outbuf, inbuf, inbufsize, channels);

    if (bps < 2 || bps > 5 || inbufsize < (uint32_t) channels * 4)
        return 0;

    for (ch = 0; ch < channels; ch++) {
        *outbuf++ = pcmdata[ch] = (int16_t) (inbuf [0] | (inbuf [1] << 8));
        index[ch] = inbuf [2];

        if (index [ch] < 0 || index [ch] > 88 || inbuf [3])     // sanitize the input a little...
            return 0;

        inbufsize -= 4;
        inbuf += 4;
    }

    if (!inbufsize || (inbufsize % (channels * 4)))             // extra clean
        return samples;

    samples += inbufsize / channels * 8 / bps;

    switch (bps) {
        case 2:
            for (ch = 0; ch < channels; ++ch) {
                int shiftbits = 0, numbits = 0, i, j;

                for (j = i = 0; i < samples - 1; ++i) {
                    uint16_t step = step_table [index [ch]];

                    if (numbits < bps) {
                        shiftbits |= inbuf [(j & ~3) * channels + (ch * 4) + (j & 3)] << numbits;
                        numbits += 8;
                        j++;
                    }

                    if (shiftbits & 2)
                        pcmdata[ch] -= step * (shiftbits & 1) + (step >> 1);
                    else
                        pcmdata[ch] += step * (shiftbits & 1) + (step >> 1);

                    index[ch] += (shiftbits & 1) * 3 - 1;
                    shiftbits >>= bps;
                    numbits -= bps;

                    CLIP(index[ch], 0, 88);
                    CLIP(pcmdata[ch], -32768, 32767);
                    outbuf [i * channels + ch] = pcmdata[ch];
                }
            }

            break;

        case 3:
            for (ch = 0; ch < channels; ++ch) {
                int shiftbits = 0, numbits = 0, i, j;

                for (j = i = 0; i < samples - 1; ++i) {
                    uint16_t step = step_table [index [ch]], delta = step >> 2;

                    if (numbits < bps) {
                        shiftbits |= inbuf [(j & ~3) * channels + (ch * 4) + (j & 3)] << numbits;
                        numbits += 8;
                        j++;
                    }

                    if (shiftbits & 1) delta += (step >> 1);
                    if (shiftbits & 2) delta += step;

                    if (shiftbits & 4)
                        pcmdata[ch] -= delta;
                    else
                        pcmdata[ch] += delta;

                    index[ch] += index_table_3bit [shiftbits & 0x3];
                    shiftbits >>= bps;
                    numbits -= bps;

                    CLIP(index[ch], 0, 88);
                    CLIP(pcmdata[ch], -32768, 32767);
                    outbuf [i * channels + ch] = pcmdata[ch];
                }
            }

            break;

        case 5:
            for (ch = 0; ch < channels; ++ch) {
                int shiftbits = 0, numbits = 0, i, j;

                for (j = i = 0; i < samples - 1; ++i) {
                    uint16_t step = step_table [index [ch]], delta = step >> 4;

                    if (numbits < bps) {
                        shiftbits |= inbuf [(j & ~3) * channels + (ch * 4) + (j & 3)] << numbits;
                        numbits += 8;
                        j++;
                    }

                    if (shiftbits & 1) delta += (step >> 3);
                    if (shiftbits & 2) delta += (step >> 2);
                    if (shiftbits & 4) delta += (step >> 1);
                    if (shiftbits & 8) delta += step;

                    if (shiftbits & 0x10)
                        pcmdata[ch] -= delta;
                    else
                        pcmdata[ch] += delta;

                    index[ch] += index_table_5bit [shiftbits & 0xf];
                    shiftbits >>= bps;
                    numbits -= bps;

                    CLIP(index[ch], 0, 88);
                    CLIP(pcmdata[ch], -32768, 32767);
                    outbuf [i * channels + ch] = pcmdata[ch];
                }
            }

            break;

        default:
            return 0;
    }

    return samples;
}

int adpcm_decode(AdpcmDecoder *adpcm_dec, int16_t *outbuf, const uint8_t *inbuf, uint32_t inbufsize, int bps)
{
    int samples = 0;
    int original_encode_len = 0;
    int reddancy_encode_len = 0;
    uint16_t inbuf_info[2];
    memcpy(inbuf_info,inbuf,4);
    original_encode_len = inbuf_info[0] & 0x7FFF;
    reddancy_encode_len = inbuf_info[1] & 0x7FFF;
    if((adpcm_dec->rddancy_num > 0)&&(inbuf_info[1]&0x8000)) {
        adpcm_dec->rddancy_encode_len = reddancy_encode_len;
        adpcm_dec->rddancy_encbuf_pos = 0;
        adpcm_dec->rddancy_encbuf_num = (inbufsize-(4+original_encode_len))/reddancy_encode_len;
        memcpy(adpcm_dec->rddancy_encbuf,inbuf+4+original_encode_len,inbufsize-(4+original_encode_len));
    }
    samples = adpcm_decode_block(adpcm_dec->decode_buf,inbuf+4,original_encode_len,1,bps);
    samples -= 1;
    adpcm_dec->plc_sta = 0;
    for(uint32_t i=0; i<samples/FRAMESZ; i++)
        g711plc_addtohistory(adpcm_dec->lowcfe, adpcm_dec->decode_buf+FRAMESZ*i);
    memcpy(outbuf,adpcm_dec->decode_buf,samples*2);
    return samples;
}

int adpcm_decode_plc(AdpcmDecoder *adpcm_dec, int16_t *outbuf)
{
    int samples = 0;
    if(adpcm_dec->rddancy_encbuf_num) {
        adpcm_dec->rddancy_encbuf_num--;
        samples = adpcm_decode_block(adpcm_dec->decode_buf,adpcm_dec->rddancy_encbuf+adpcm_dec->rddancy_encbuf_pos,
                                                                    adpcm_dec->rddancy_encode_len,1,4);
        samples -= 1;  
        adpcm_dec->plc_sta = 1;                                                  
        for(uint32_t i=0; i<samples/FRAMESZ; i++)
            g711plc_addtohistory(adpcm_dec->lowcfe, adpcm_dec->decode_buf+FRAMESZ*i);
        memcpy(outbuf,adpcm_dec->decode_buf,samples*2);
        adpcm_dec->rddancy_encbuf_pos += adpcm_dec->rddancy_encode_len;
        return samples;
    }
    else {
        adpcm_dec->plc_sta = 2;
        for(uint32_t i=0; i<adpcm_dec->block_size/FRAMESZ; i++)
            g711plc_dofe(adpcm_dec->lowcfe, adpcm_dec->decode_buf+FRAMESZ*i);
        memcpy(outbuf,adpcm_dec->decode_buf,adpcm_dec->block_size*2);
        return adpcm_dec->block_size;
    }
}

struct adpcm_context *adpcm_create_context (int num_channels, int sample_rate, int lookahead, int noise_shaping)
{
    struct adpcm_context *pcnxt = (struct adpcm_context *)adpcm_malloc(sizeof (struct adpcm_context));
    int ch;
    if(!pcnxt)
        return 0;
    memset (pcnxt, 0, sizeof (struct adpcm_context));
    pcnxt->config_flags = noise_shaping | lookahead;
    pcnxt->static_shaping_weight = 1024;
    pcnxt->num_channels = num_channels;
    pcnxt->sample_rate = sample_rate;

    // we set the indicies to invalid values so that we always recalculate them
    // on at least the first frame (and every frame if the depth is sufficient)

    for (ch = 0; ch < num_channels; ++ch)
        pcnxt->channels [ch].index = -1;

    return pcnxt;
}

void adpcm_free_context(struct adpcm_context *p)
{
    if(p) {
        adpcm_free(p);
        p = NULL;
    }
}

TYPE_RINGBUF *adpcm_create_ringbuf(int num_channels, int bufsize, int redundancy_num)
{
    TYPE_RINGBUF *ringbuf = NULL;
    ringbuf = (TYPE_RINGBUF *)adpcm_malloc(sizeof(TYPE_RINGBUF));
	if(ringbuf_Init(ringbuf,(bufsize*(redundancy_num+2)*num_channels)) == -1)
		return 0;
    ringbuf->data = (uint8_t*)adpcm_malloc(ringbuf->size);
    if(!ringbuf->data) {
        ringbuf_del(ringbuf);
        return 0;
    }
    return ringbuf;
}

void adpcm_free_ringbuf(TYPE_RINGBUF *p)
{
    if(p) {
        if(p->data) {
            adpcm_free(p->data);
            p->data = NULL;
        }
        ringbuf_del(p);
    }
}

AdpcmEncoder *adpcm_encoder_create(int sample_rate, int lookahead, int noise_shaping, int block_size, int redundancy_num)
{
    struct adpcm_context *adpcm_normal_cnxt = NULL;
    struct adpcm_context *adpcm_rddancy_cnxt = NULL; 
    TYPE_RINGBUF *ringbuf_original = NULL;
    TYPE_RINGBUF *ringbuf_encode = NULL;
    AdpcmEncoder *adpcm_enc = NULL;

    adpcm_normal_cnxt = adpcm_create_context(1,sample_rate,lookahead,noise_shaping);
    if(!adpcm_normal_cnxt)
        goto adpcm_encoder_create_err;

    if(redundancy_num) {
        adpcm_rddancy_cnxt = adpcm_create_context(1,sample_rate,lookahead,noise_shaping);
        if(!adpcm_rddancy_cnxt) 
            goto adpcm_encoder_create_err;

        ringbuf_original = adpcm_create_ringbuf(1,block_size*2,redundancy_num);
        if(!ringbuf_original) 
            goto adpcm_encoder_create_err;

        ringbuf_encode = adpcm_create_ringbuf(1,block_size*4/8+4,redundancy_num); //bps2:block_size*2/8+4
        if(!ringbuf_encode)
            goto adpcm_encoder_create_err;
    }
  
    adpcm_enc = (AdpcmEncoder *)adpcm_malloc(sizeof(AdpcmEncoder));
    if(!adpcm_enc) 
        goto adpcm_encoder_create_err;

    adpcm_enc->normal_cnxt = adpcm_normal_cnxt;
    adpcm_enc->rddancy_cnxt = adpcm_rddancy_cnxt;
    adpcm_enc->ringbuf_original = ringbuf_original;
    adpcm_enc->ringbuf_encode = ringbuf_encode;
    adpcm_enc->rddancy_num = redundancy_num;
    return adpcm_enc;

adpcm_encoder_create_err:
    adpcm_free_context(adpcm_normal_cnxt);
    adpcm_free_context(adpcm_rddancy_cnxt);
    adpcm_free_ringbuf(ringbuf_original);
    adpcm_free_ringbuf(ringbuf_encode);
    return 0;
}

void adpcm_free_encoder(AdpcmEncoder *adpcm_enc)
{
    if(adpcm_enc) {
        adpcm_free_context(adpcm_enc->normal_cnxt);
        adpcm_free_context(adpcm_enc->rddancy_cnxt);
        adpcm_free_ringbuf(adpcm_enc->ringbuf_original); 
        adpcm_free_ringbuf(adpcm_enc->ringbuf_encode); 
        adpcm_free(adpcm_enc);
        adpcm_enc = NULL;
    }
}

AdpcmDecoder *adpcm_decoder_create(int block_size, int redundancy_num)
{
    uint8_t *rddancy_encbuf = NULL;
    AdpcmDecoder *adpcm_dec = NULL;   
    LowcFE_c *lc = NULL;
    int16_t *decode_buf = NULL;

    if(redundancy_num) {
        rddancy_encbuf = (uint8_t*)adpcm_malloc(1*(block_size*4/8+4)*redundancy_num);
        if(!rddancy_encbuf) 
            goto adpcm_decoder_create_err;           
    } 

    lc = (LowcFE_c *)adpcm_malloc(sizeof(LowcFE_c));
    if(!lc)
        goto adpcm_decoder_create_err;

    adpcm_dec = (AdpcmDecoder *)adpcm_malloc(sizeof(AdpcmDecoder));
    if(!adpcm_dec) 
        goto adpcm_decoder_create_err;

    decode_buf = (int16_t *)adpcm_malloc(sizeof(short)*(block_size+1));
    if(!decode_buf)
        goto adpcm_decoder_create_err;

    adpcm_dec->rddancy_encbuf = rddancy_encbuf;
    adpcm_dec->lowcfe = lc;
    adpcm_dec->rddancy_num = redundancy_num;
    adpcm_dec->rddancy_encbuf_num = 0;
    adpcm_dec->block_size = block_size;
    adpcm_dec->decode_buf = decode_buf;
    g711plc_construct(adpcm_dec->lowcfe);
    return adpcm_dec; 
adpcm_decoder_create_err:
    if(rddancy_encbuf) {
        adpcm_free(rddancy_encbuf);
        rddancy_encbuf = NULL;
    }
    if(lc) {
        adpcm_free(lc);
        lc = NULL;
    }
    if(decode_buf) {
        adpcm_free(decode_buf);
        decode_buf = NULL;        
    }
    return 0;
}

void adpcm_free_decoder(AdpcmDecoder *adpcm_dec)
{
    if(adpcm_dec) {
        if(adpcm_dec->rddancy_encbuf) {
            adpcm_free(adpcm_dec->rddancy_encbuf);
            adpcm_dec->rddancy_encbuf = NULL;
        }
        if(adpcm_dec->lowcfe) {
            adpcm_free(adpcm_dec->lowcfe);
            adpcm_dec->lowcfe = NULL;            
        }
        if(adpcm_dec->decode_buf) {
            adpcm_free(adpcm_dec->decode_buf);
            adpcm_dec->decode_buf = NULL;        
        }
        adpcm_free(adpcm_dec);
        adpcm_dec = NULL;
    }
}