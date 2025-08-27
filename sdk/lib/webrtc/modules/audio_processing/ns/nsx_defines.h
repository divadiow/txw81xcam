/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_DEFINES_H_
#define MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_DEFINES_H_

#define ANAL_BLOCKL_MAX         128 /* Max analysis block length，8K采样率只需要128*/
#define HALF_ANAL_BLOCKL        (ANAL_BLOCKL_MAX/2+1) /* Half max analysis block length + 1 */
//#define NUM_HIGH_BANDS_MAX      2   /* Max number of high bands */
#define SIMULT                  3
#define END_STARTUP_LONG        200
#define END_STARTUP_SHORT       50
#define FACTOR_Q16              2621440 /* 40 in Q16 */
#define FACTOR_Q7               5120 /* 40 in Q7 */
#define FACTOR_Q7_STARTUP       1024 /* 8 in Q7 */
#define WIDTH_Q8                3 /* 0.01 in Q8 (or 25 ) */

/* PARAMETERS FOR NEW METHOD */
#define DD_PR_SNR_Q11           2007 /* ~= Q11(0.98) DD update of prior SNR */
#define ONE_MINUS_DD_PR_SNR_Q11 41 /* DD update of prior SNR */
#define SPECT_FLAT_TAVG_Q14     4915 /* (0.30) tavg parameter for spectral flatness measure */
#define SPECT_DIFF_TAVG_Q8      77 /* (0.30) tavg parameter for spectral flatness measure */
#define PRIOR_UPDATE_Q14        1638 /* Q14(0.1) Update parameter of prior model */
#define NOISE_UPDATE_Q8         26 /* 26 ~= Q8(0.1) Update parameter for noise */

/* Probability threshold for noise state in speech/noise likelihood. */
#define ONE_MINUS_PROB_RANGE_Q8 205 /* 205 ~= Q8(0.8) */
#define HIST_PAR_EST            1000 /* Histogram size for estimation of parameters */

/* FEATURE EXTRACTION CONFIG  */
/* Bin size of histogram */
#define BIN_SIZE_LRT            10
/* Scale parameters: multiply dominant peaks of the histograms by scale factor to obtain. */
/* Thresholds for prior model */
#define FACTOR_1_LRT_DIFF       6 /* For LRT and spectral difference (5 times bigger) */
/* For spectral_flatness: used when noise is flatter than speech (10 times bigger). */
#define FACTOR_2_FLAT_Q10       922
/* Peak limit for spectral flatness (varies between 0 and 1) */
#define THRES_PEAK_FLAT         24 /* * 2 * BIN_SIZE_FLAT_FX */
/* Limit on spacing of two highest peaks in histogram: spacing determined by bin size. */
#define LIM_PEAK_SPACE_FLAT_DIFF    4 /* * 2 * BIN_SIZE_DIFF_FX */
/* Limit on relevance of second peak */
#define LIM_PEAK_WEIGHT_FLAT_DIFF   2
#define THRES_FLUCT_LRT         10240 /* = 20 * inst->modelUpdate; fluctuation limit of LRT feat. */
/* Limit on the max and min values for the feature thresholds */
#define MAX_FLAT_Q10            38912 /*  * 2 * BIN_SIZE_FLAT_FX */
#define MIN_FLAT_Q10            4096 /*  * 2 * BIN_SIZE_FLAT_FX */
#define MAX_DIFF                100 /* * 2 * BIN_SIZE_DIFF_FX */
#define MIN_DIFF                16 /* * 2 * BIN_SIZE_DIFF_FX */
/* Criteria of weight of histogram peak  to accept/reject feature */
#define THRES_WEIGHT_FLAT_DIFF  154 /*(int)(0.3*(inst->modelUpdate)) for flatness and difference */

#define STAT_UPDATES            9 /* Update every 512 = 1 << 9 block */
#define ONE_MINUS_GAMMA_PAUSE_Q8    13 /* ~= Q8(0.05) Update for conservative noise estimate */
#define GAMMA_NOISE_TRANS_AND_SPEECH_Q8 3 /* ~= Q8(0.01) Update for transition and noise region */

//使用宏代替一些不会改的变量。只针对8k采样率有用
#define NR_BLOCKLEN10MS         80
#define NR_ANALEN               128
#define NR_ANALEN2              (NR_ANALEN / 2)
#define NR_MAGNLEN              (NR_ANALEN2 + 1)
#define NR_STAGES               7
#define NR_MAXLRT               0x0040000
#define NR_MINLRT               52429
#define NR_MODELUPDATE          (1 << STAT_UPDATES)
#define NR_HANNING_WINDOW       kBlocks80w128x

//凤噪抑制等级参数
#define NSLEVEL                 3 //0-3
#if NSLEVEL == 0
#define NR_GAINMAP              0
#define NR_OVERDRIVE            256
#define NR_DENOISEBOUND         8192
#elif NSLEVEL == 1
#define NR_GAINMAP              1
#define NR_OVERDRIVE            256
#define NR_DENOISEBOUND         4096
#define NR_FACTOR2TABLE         kFactor2Aggressiveness1
#elif NSLEVEL == 2
#define NR_GAINMAP              1
#define NR_OVERDRIVE            282
#define NR_DENOISEBOUND         2048
#define NR_FACTOR2TABLE         kFactor2Aggressiveness2
#elif NSLEVEL == 3
#define NR_GAINMAP              1
#define NR_OVERDRIVE            320
#define NR_DENOISEBOUND         1475
#define NR_FACTOR2TABLE         kFactor2Aggressiveness3
#endif

// Skip first frequency bins during estimation. (0 <= value < 64)
#define NR_KSTARTBAND           5

#endif /* MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_DEFINES_H_ */
