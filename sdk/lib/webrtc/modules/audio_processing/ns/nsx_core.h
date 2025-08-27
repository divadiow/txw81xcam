/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_CORE_H_
#define MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_CORE_H_

#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "nsx_defines.h"
#include "typedefs.h"  // NOLINT(build/include)

typedef struct NoiseSuppressionFixedC_ {
  //u32                     fs;

//  const s16*              window;
  s16                     analysisBuffer[ANAL_BLOCKL_MAX];
  s16                     synthesisBuffer[ANAL_BLOCKL_MAX];
  u16                     noiseSupFilter[HALF_ANAL_BLOCKL];
  
//  u16                     overdrive; /* Q8 */
//  u16                     denoiseBound; /* Q14 */
//  const s16*              factor2Table;
  s16                     noiseEstLogQuantile[SIMULT* HALF_ANAL_BLOCKL];
  s16                     noiseEstDensity[SIMULT* HALF_ANAL_BLOCKL];
  s16                     noiseEstCounter[SIMULT];
  s16                     noiseEstQuantile[HALF_ANAL_BLOCKL];

//  uint                    anaLen;
//  uint                    anaLen2;
//  uint                    magnLen;
//  int                     aggrMode;
//  int                     stages;
//  int                     initFlag;
//  int                     gainMap;

//  s32                     maxLrt;
//  s32                     minLrt;
  // Log LRT factor with time-smoothing in Q8.
  s32                     logLrtTimeAvgW32[HALF_ANAL_BLOCKL];
  s32                     featureLogLrt;
  s32                     thresholdLogLrt;
  s16                     weightLogLrt;

  u32                     featureSpecDiff;
  u32                     thresholdSpecDiff;
  s16                     weightSpecDiff;

  u32                     featureSpecFlat;
  u32                     thresholdSpecFlat;
  s16                     weightSpecFlat;

  // Conservative estimate of noise spectrum.
  s32                     avgMagnPause[HALF_ANAL_BLOCKL];
  u32                     magnEnergy;
  u32                     sumMagn;
  u32                     curAvgMagnEnergy;
  u32                     timeAvgMagnEnergy;
  u32                     timeAvgMagnEnergyTmp;

  u32                     whiteNoiseLevel;  // Initial noise estimate.
  // Initial magnitude spectrum estimate.
  u32                     initMagnEst[HALF_ANAL_BLOCKL];
  // Pink noise parameters:
  s32                     pinkNoiseNumerator;  // Numerator.
  s32                     pinkNoiseExp;  // Power of freq.
  int                     minNorm;  // Smallest normalization factor.
  int                     zeroInputSignal;  // Zero input signal flag.

  // Noise spectrum from previous frame.
  u32                     prevNoiseU32[HALF_ANAL_BLOCKL];
  // Magnitude spectrum from previous frame.
  u16                     prevMagnU16[HALF_ANAL_BLOCKL];
  // Prior speech/noise probability in Q14.
  s16                     priorNonSpeechProb;

  int                     blockIndex;  // Frame index counter.
  // Parameter for updating or estimating thresholds/weights for prior model.
//  int                     modelUpdate;
  int                     cntThresUpdate;

  // Histograms for parameter estimation.
  s16                     histLrt[HIST_PAR_EST];
  s16                     histSpecFlat[HIST_PAR_EST];
  s16                     histSpecDiff[HIST_PAR_EST];

  // Quantities for high band estimate.
//  s16                     dataBufHBFX[NUM_HIGH_BANDS_MAX][ANAL_BLOCKL_MAX];

  int                     qNoise;
  int                     prevQNoise;
  int                     prevQMagn;
//  uint                    blockLen10ms;

  s16                     real[ANAL_BLOCKL_MAX];
  s16                     imag[ANAL_BLOCKL_MAX];
  s32                     energyIn;
  int                     scaleEnergyIn;
  int                     normData;

  struct RealFFT* real_fft;
} NoiseSuppressionFixedC;

/****************************************************************************
 * WebRtcNsx_InitCore(...)
 *
 * This function initializes a noise suppression instance
 *
 * Input:
 *      - inst          : Instance that should be initialized
 *      - fs            : Sampling frequency
 *
 * Output:
 *      - inst          : Initialized instance
 *
 * Return value         :  0 - Ok
 *                        -1 - Error
 */
s32 WebRtcNsx_InitCore(NoiseSuppressionFixedC* inst);

/****************************************************************************
 * WebRtcNsx_set_policy_core(...)
 *
 * This changes the aggressiveness of the noise suppression method.
 *
 * Input:
 *      - inst       : Instance that should be initialized
 *      - mode       : 0: Mild (6 dB), 1: Medium (10 dB), 2: Aggressive (15 dB)
 *
 * Output:
 *      - inst       : Initialized instance
 *
 * Return value      :  0 - Ok
 *                     -1 - Error
 */
//int WebRtcNsx_set_policy_core(NoiseSuppressionFixedC* inst, int mode);

/****************************************************************************
 * WebRtcNsx_ProcessCore
 *
 * Do noise suppression.
 *
 * Input:
 *      - inst          : Instance that should be initialized
 *      - inFrame       : Input speech frame for each band
 *      - num_bands     : Number of bands
 *
 * Output:
 *      - inst          : Updated instance
 *      - outFrame      : Output speech frame for each band
 */
void WebRtcNsx_ProcessCore(NoiseSuppressionFixedC* inst,
                           short* inFrame,
                           int num_bands,
                           short* outFrame);

/****************************************************************************
 * Some function pointers, for internal functions shared by ARM NEON and
 * generic C code.
 */
// Noise Estimation.
//typedef void (*NoiseEstimation)(NoiseSuppressionFixedC* inst,
//                                u16* magn,
//                                u32* noise,
//                                s16* q_noise);
//extern NoiseEstimation WebRtcNsx_NoiseEstimation;

// Filter the data in the frequency domain, and create spectrum.
//typedef void (*PrepareSpectrum)(NoiseSuppressionFixedC* inst,
//                                s16    * freq_buff);
//extern PrepareSpectrum WebRtcNsx_PrepareSpectrum;

// For the noise supression process, synthesis, read out fully processed
// segment, and update synthesis buffer.
//typedef void (*SynthesisUpdate)(NoiseSuppressionFixedC* inst,
//                                s16    * out_frame,
//                                s16     gain_factor);
//extern SynthesisUpdate WebRtcNsx_SynthesisUpdate;

// Update analysis buffer for lower band, and window data before FFT.
//typedef void (*AnalysisUpdate)(NoiseSuppressionFixedC* inst,
//                               s16    * out,
//                               s16    * new_speech);
//extern AnalysisUpdate WebRtcNsx_AnalysisUpdate;

// Denormalize the real-valued signal |in|, the output from inverse FFT.
//typedef void (*Denormalize)(NoiseSuppressionFixedC* inst,
//                            s16    * in,
//                            int factor);
//extern Denormalize WebRtcNsx_Denormalize;

// Normalize the real-valued signal |in|, the input to forward FFT.
//typedef void (*NormalizeRealBuffer)(NoiseSuppressionFixedC* inst,
//                                    const s16    * in,
//                                    s16    * out);
//extern NormalizeRealBuffer WebRtcNsx_NormalizeRealBuffer;

// Compute speech/noise probability.
// Intended to be private.
extern void WebRtcNsx_SpeechNoiseProb(NoiseSuppressionFixedC* inst,
                               u16* nonSpeechProbFinal,
                               u32* priorLocSnr,
                               u32* postLocSnr);

#endif  // MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_CORE_H_
