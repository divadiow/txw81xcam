/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "noise_suppression_x.h"
#include "common_audio/signal_processing/include/real_fft.h"
#include "nsx_core.h"

//NR下复用的buf
//FFT用到的buf，需要4byte对齐
s16 nr_temp_buf_1[256];                  //为了方便fft计算，直接用256个s16数据
s16 nr_temp_buf_2[ANAL_BLOCKL_MAX+2];    //130个s16数据

//WebRtcNsx_ProcessCore函数用到的buf，找不到能复用的地方
u32 WebRtcNsx_noiseU32[HALF_ANAL_BLOCKL];
u32 WebRtcNsx_postLocSnr[HALF_ANAL_BLOCKL];
u32 WebRtcNsx_priorLocSnr[HALF_ANAL_BLOCKL];

static const s16 WebRtcNsx_kLogTable[9] = 
{
    0, 177, 355, 532, 710, 887, 1065, 1242, 1420
};

static const s16 WebRtcNsx_kCounterDiv[201] = 
{
    32767, 16384, 10923, 8192, 6554, 5461, 4681, 4096, 3641, 3277, 2979, 2731,
    2521, 2341, 2185, 2048, 1928, 1820, 1725, 1638, 1560, 1489, 1425, 1365, 1311,
    1260, 1214, 1170, 1130, 1092, 1057, 1024, 993, 964, 936, 910, 886, 862, 840,
    819, 799, 780, 762, 745, 728, 712, 697, 683, 669, 655, 643, 630, 618, 607,
    596, 585, 575, 565, 555, 546, 537, 529, 520, 512, 504, 496, 489, 482, 475,
    468, 462, 455, 449, 443, 437, 431, 426, 420, 415, 410, 405, 400, 395, 390,
    386, 381, 377, 372, 368, 364, 360, 356, 352, 349, 345, 341, 338, 334, 331,
    328, 324, 321, 318, 315, 312, 309, 306, 303, 301, 298, 295, 293, 290, 287,
    285, 282, 280, 278, 275, 273, 271, 269, 266, 264, 262, 260, 258, 256, 254,
    252, 250, 248, 246, 245, 243, 241, 239, 237, 236, 234, 232, 231, 229, 228,
    226, 224, 223, 221, 220, 218, 217, 216, 214, 213, 211, 210, 209, 207, 206,
    205, 204, 202, 201, 200, 199, 197, 196, 195, 194, 193, 192, 191, 189, 188,
    187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173,
    172, 172, 171, 170, 169, 168, 167, 166, 165, 165, 164, 163
};

static const s16 WebRtcNsx_kLogTableFrac[256] =
{
    0, 1, 3, 4, 6, 7, 9, 10, 11, 13, 14, 16, 17, 18, 20, 21,
    22, 24, 25, 26, 28, 29, 30, 32, 33, 34, 36, 37, 38, 40, 41, 42,
    44, 45, 46, 47, 49, 50, 51, 52, 54, 55, 56, 57, 59, 60, 61, 62,
    63, 65, 66, 67, 68, 69, 71, 72, 73, 74, 75, 77, 78, 79, 80, 81,
    82, 84, 85, 86, 87, 88, 89, 90, 92, 93, 94, 95, 96, 97, 98, 99,
    100, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 116,
    117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
    132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146,
    147, 148, 149, 150, 151, 152, 153, 154, 155, 155, 156, 157, 158, 159, 160,
    161, 162, 163, 164, 165, 166, 167, 168, 169, 169, 170, 171, 172, 173, 174,
    175, 176, 177, 178, 178, 179, 180, 181, 182, 183, 184, 185, 185, 186, 187,
    188, 189, 190, 191, 192, 192, 193, 194, 195, 196, 197, 198, 198, 199, 200,
    201, 202, 203, 203, 204, 205, 206, 207, 208, 208, 209, 210, 211, 212, 212,
    213, 214, 215, 216, 216, 217, 218, 219, 220, 220, 221, 222, 223, 224, 224,
    225, 226, 227, 228, 228, 229, 230, 231, 231, 232, 233, 234, 234, 235, 236,
    237, 238, 238, 239, 240, 241, 241, 242, 243, 244, 244, 245, 246, 247, 247,
    248, 249, 249, 250, 251, 252, 252, 253, 254, 255, 255
};

//// Skip first frequency bins during estimation. (0 <= value < 64)
//static const uint kStartBand = 5;

// hybrib Hanning & flat window
static const s16 kBlocks80w128x[128] = 
{
    0, 536, 1072, 1606, 2139, 2669, 3196, 3720, 4240, 4756, 5266,
    5771, 6270, 6762, 7246, 7723, 8192, 8652, 9102, 9543, 9974, 10394,
    10803, 11200, 11585, 11958, 12318, 12665, 12998, 13318, 13623, 13913, 14189,
    14449, 14694, 14924, 15137, 15334, 15515, 15679, 15826, 15956, 16069, 16165,
    16244, 16305, 16349, 16375, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
    16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
    16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
    16384, 16384, 16384, 16384, 16375, 16349, 16305, 16244, 16165, 16069, 15956,
    15826, 15679, 15515, 15334, 15137, 14924, 14694, 14449, 14189, 13913, 13623,
    13318, 12998, 12665, 12318, 11958, 11585, 11200, 10803, 10394, 9974, 9543,
    9102, 8652, 8192, 7723, 7246, 6762, 6270, 5771, 5266, 4756, 4240,
    3720, 3196, 2669, 2139, 1606, 1072, 536
};

// Gain factor1 table: Input value in Q8 and output value in Q13
// original floating point code
//  if (gain > blim) {
//    factor1 = 1.0 + 1.3 * (gain - blim);
//    if (gain * factor1 > 1.0) {
//      factor1 = 1.0 / gain;
//    }
//  } else {
//    factor1 = 1.0;
//  }
static const s16 kFactor1Table[257] =
{
    8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8233, 8274, 8315, 8355, 8396, 8436, 8475, 8515, 8554, 8592, 8631, 8669,
    8707, 8745, 8783, 8820, 8857, 8894, 8931, 8967, 9003, 9039, 9075, 9111, 9146, 9181,
    9216, 9251, 9286, 9320, 9354, 9388, 9422, 9456, 9489, 9523, 9556, 9589, 9622, 9655,
    9687, 9719, 9752, 9784, 9816, 9848, 9879, 9911, 9942, 9973, 10004, 10035, 10066,
    10097, 10128, 10158, 10188, 10218, 10249, 10279, 10308, 10338, 10368, 10397, 10426,
    10456, 10485, 10514, 10543, 10572, 10600, 10629, 10657, 10686, 10714, 10742, 10770,
    10798, 10826, 10854, 10882, 10847, 10810, 10774, 10737, 10701, 10666, 10631, 10596,
    10562, 10527, 10494, 10460, 10427, 10394, 10362, 10329, 10297, 10266, 10235, 10203,
    10173, 10142, 10112, 10082, 10052, 10023, 9994, 9965, 9936, 9908, 9879, 9851, 9824,
    9796, 9769, 9742, 9715, 9689, 9662, 9636, 9610, 9584, 9559, 9534, 9508, 9484, 9459,
    9434, 9410, 9386, 9362, 9338, 9314, 9291, 9268, 9245, 9222, 9199, 9176, 9154, 9132,
    9110, 9088, 9066, 9044, 9023, 9002, 8980, 8959, 8939, 8918, 8897, 8877, 8857, 8836,
    8816, 8796, 8777, 8757, 8738, 8718, 8699, 8680, 8661, 8642, 8623, 8605, 8586, 8568,
    8550, 8532, 8514, 8496, 8478, 8460, 8443, 8425, 8408, 8391, 8373, 8356, 8339, 8323,
    8306, 8289, 8273, 8256, 8240, 8224, 8208, 8192
};

// For Factor2 tables
// original floating point code
// if (gain > blim) {
//   factor2 = 1.0;
// } else {
//   factor2 = 1.0 - 0.3 * (blim - gain);
//   if (gain <= inst->denoiseBound) {
//     factor2 = 1.0 - 0.3 * (blim - inst->denoiseBound);
//   }
// }
//
// Gain factor table: Input value in Q8 and output value in Q13
static const s16 kFactor2Aggressiveness1[257] =
{
    7577, 7577, 7577, 7577, 7577, 7577,
    7577, 7577, 7577, 7577, 7577, 7577, 7577, 7577, 7577, 7577, 7577, 7596, 7614, 7632,
    7650, 7667, 7683, 7699, 7715, 7731, 7746, 7761, 7775, 7790, 7804, 7818, 7832, 7845,
    7858, 7871, 7884, 7897, 7910, 7922, 7934, 7946, 7958, 7970, 7982, 7993, 8004, 8016,
    8027, 8038, 8049, 8060, 8070, 8081, 8091, 8102, 8112, 8122, 8132, 8143, 8152, 8162,
    8172, 8182, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};

// Gain factor table: Input value in Q8 and output value in Q13
static const s16 kFactor2Aggressiveness2[257] =
{
    7270, 7270, 7270, 7270, 7270, 7306,
    7339, 7369, 7397, 7424, 7448, 7472, 7495, 7517, 7537, 7558, 7577, 7596, 7614, 7632,
    7650, 7667, 7683, 7699, 7715, 7731, 7746, 7761, 7775, 7790, 7804, 7818, 7832, 7845,
    7858, 7871, 7884, 7897, 7910, 7922, 7934, 7946, 7958, 7970, 7982, 7993, 8004, 8016,
    8027, 8038, 8049, 8060, 8070, 8081, 8091, 8102, 8112, 8122, 8132, 8143, 8152, 8162,
    8172, 8182, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};

// Gain factor table: Input value in Q8 and output value in Q13
static const s16 kFactor2Aggressiveness3[257] =
{
    7184, 7184, 7184, 7229, 7270, 7306,
    7339, 7369, 7397, 7424, 7448, 7472, 7495, 7517, 7537, 7558, 7577, 7596, 7614, 7632,
    7650, 7667, 7683, 7699, 7715, 7731, 7746, 7761, 7775, 7790, 7804, 7818, 7832, 7845,
    7858, 7871, 7884, 7897, 7910, 7922, 7934, 7946, 7958, 7970, 7982, 7993, 8004, 8016,
    8027, 8038, 8049, 8060, 8070, 8081, 8091, 8102, 8112, 8122, 8132, 8143, 8152, 8162,
    8172, 8182, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};

// sum of log2(i) from table index to inst->anaLen2 in Q5
// Note that the first table value is invalid, since log2(0) = -infinity
static const s16 kSumLogIndex[66] =
{
    0, 22917, 22917, 22885, 22834, 22770, 22696, 22613,
    22524, 22428, 22326, 22220, 22109, 21994, 21876, 21754,
    21629, 21501, 21370, 21237, 21101, 20963, 20822, 20679,
    20535, 20388, 20239, 20089, 19937, 19783, 19628, 19470,
    19312, 19152, 18991, 18828, 18664, 18498, 18331, 18164,
    17994, 17824, 17653, 17480, 17306, 17132, 16956, 16779,
    16602, 16423, 16243, 16063, 15881, 15699, 15515, 15331,
    15146, 14960, 14774, 14586, 14398, 14209, 14019, 13829,
    13637, 13445
};

// sum of log2(i)^2 from table index to inst->anaLen2 in Q2
// Note that the first table value is invalid, since log2(0) = -infinity
static const s16 kSumSquareLogIndex[66] =
{
    0, 16959, 16959, 16955, 16945, 16929, 16908, 16881,
    16850, 16814, 16773, 16729, 16681, 16630, 16575, 16517,
    16456, 16392, 16325, 16256, 16184, 16109, 16032, 15952,
    15870, 15786, 15700, 15612, 15521, 15429, 15334, 15238,
    15140, 15040, 14938, 14834, 14729, 14622, 14514, 14404,
    14292, 14179, 14064, 13947, 13830, 13710, 13590, 13468,
    13344, 13220, 13094, 12966, 12837, 12707, 12576, 12444,
    12310, 12175, 12039, 11902, 11763, 11624, 11483, 11341,
    11198, 11054
};

// log2(table index) in Q12
// Note that the first table value is invalid, since log2(0) = -infinity
static const s16 kLogIndex[129] =
{
    0, 0, 4096, 6492, 8192, 9511, 10588, 11499,
    12288, 12984, 13607, 14170, 14684, 15157, 15595, 16003,
    16384, 16742, 17080, 17400, 17703, 17991, 18266, 18529,
    18780, 19021, 19253, 19476, 19691, 19898, 20099, 20292,
    20480, 20662, 20838, 21010, 21176, 21338, 21496, 21649,
    21799, 21945, 22087, 22226, 22362, 22495, 22625, 22752,
    22876, 22998, 23117, 23234, 23349, 23462, 23572, 23680,
    23787, 23892, 23994, 24095, 24195, 24292, 24388, 24483,
    24576, 24668, 24758, 24847, 24934, 25021, 25106, 25189,
    25272, 25354, 25434, 25513, 25592, 25669, 25745, 25820,
    25895, 25968, 26041, 26112, 26183, 26253, 26322, 26390,
    26458, 26525, 26591, 26656, 26721, 26784, 26848, 26910,
    26972, 27033, 27094, 27154, 27213, 27272, 27330, 27388,
    27445, 27502, 27558, 27613, 27668, 27722, 27776, 27830,
    27883, 27935, 27988, 28039, 28090, 28141, 28191, 28241,
    28291, 28340, 28388, 28437, 28484, 28532, 28579, 28626,
    28672
};

// determinant of estimation matrix in Q0 corresponding to the log2 tables above
// Note that the first table value is invalid, since log2(0) = -infinity
static const s16 kDeterminantEstMatrix[66] =
{
    0, 29814, 25574, 22640, 20351, 18469, 16873, 15491,
    14277, 13199, 12233, 11362, 10571, 9851, 9192, 8587,
    8030, 7515, 7038, 6596, 6186, 5804, 5448, 5115,
    4805, 4514, 4242, 3988, 3749, 3524, 3314, 3116,
    2930, 2755, 2590, 2435, 2289, 2152, 2022, 1900,
    1785, 1677, 1575, 1478, 1388, 1302, 1221, 1145,
    1073, 1005, 942, 881, 825, 771, 721, 674,
    629, 587, 547, 510, 475, 442, 411, 382,
    355, 330
};

// Update the noise estimation information.

static void UpdateNoiseEstimate(NoiseSuppressionFixedC* inst, int offset)
{
    s32 tmp32no1 = 0;
    s32 tmp32no2 = 0;
    s16 tmp16 = 0;
    const s16 kExp2Const = 11819; // Q13

    uint i = 0;

    tmp16 = WebRtcSpl_MaxValueW16C(inst->noiseEstLogQuantile + offset, NR_MAGNLEN);
    // Guarantee a Q-domain as high as possible and still fit in int16
    inst->qNoise = 14 - (int) WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                   kExp2Const, tmp16, 21);
    for (i = 0; i < NR_MAGNLEN; i++) {
        // inst->quantile[i]=exp(inst->lquantile[offset+i]);
        // in Q21
        tmp32no2 = kExp2Const * inst->noiseEstLogQuantile[offset + i];
        tmp32no1 = (0x00200000 | (tmp32no2 & 0x001FFFFF)); // 2^21 + frac
        tmp16 = (s16) (tmp32no2 >> 21);
        tmp16 -= 21; // shift 21 to get result in Q0
        tmp16 += (s16) inst->qNoise; //shift to get result in Q(qNoise)
        if (tmp16 < 0) {
            tmp32no1 >>= -tmp16;
        } else {
            tmp32no1 <<= tmp16;
        }
        inst->noiseEstQuantile[i] = WebRtcSpl_SatW32ToW16(tmp32no1);
    }
}

// Noise Estimation
static void NoiseEstimationC(NoiseSuppressionFixedC* inst,
                               u16* magn,
                               u32* noise,
                               s16* q_noise)
{
    //s16 lmagn[NR_MAGNLEN];
    s16 *lmagn = nr_temp_buf_1; //只需要用65个s16
    s16 counter, countDiv;
    s16 countProd, delta, zeros, frac;
    s16 log2, tabind, logval, tmp16, tmp16no1, tmp16no2;
    const s16 log2_const = 22713; // Q15
    const s16 width_factor = 21845;

    uint i, s, offset;

    tabind = NR_STAGES - inst->normData;

    if (tabind < 0) {
        logval = -WebRtcNsx_kLogTable[-tabind];
    } else {
        logval = WebRtcNsx_kLogTable[tabind];
    }

    // lmagn(i)=log(magn(i))=log(2)*log2(magn(i))
    // magn is in Q(-stages), and the real lmagn values are:
    // real_lmagn(i)=log(magn(i)*2^stages)=log(magn(i))+log(2^stages)
    // lmagn in Q8
    for (i = 0; i < NR_MAGNLEN; i++) {
        if (magn[i]) {
            zeros = WebRtcSpl_NormU32((u32) magn[i]);
            frac = (s16) ((((u32) magn[i] << zeros)
                    & 0x7FFFFFFF) >> 23);
            // log2(magn(i))
            log2 = (s16) (((31 - zeros) << 8)
                    + WebRtcNsx_kLogTableFrac[frac]);
            // log2(magn(i))*log(2)
            lmagn[i] = (s16) ((log2 * log2_const) >> 15);
            // + log(2^stages)
            lmagn[i] += logval;
        } else {
            lmagn[i] = logval; //0;
        }
    }

    // loop over simultaneous estimates
    for (s = 0; s < SIMULT; s++) {
        offset = s * NR_MAGNLEN;

        // Get counter values from state
        counter = inst->noiseEstCounter[s];
        countDiv = WebRtcNsx_kCounterDiv[counter];
        countProd = (s16) (counter * countDiv);

        // quant_est(...)
        for (i = 0; i < NR_MAGNLEN; i++) {
            // compute delta
            if (inst->noiseEstDensity[offset + i] > 512) {
                // Get the value for delta by shifting intead of dividing.
                int factor = WebRtcSpl_NormW16(inst->noiseEstDensity[offset + i]);
                delta = (s16) (FACTOR_Q16 >> (14 - factor));
            } else {
                delta = FACTOR_Q7;
                if (inst->blockIndex < END_STARTUP_LONG) {
                    // Smaller step size during startup. This prevents from using
                    // unrealistic values causing overflow.
                    delta = FACTOR_Q7_STARTUP;
                }
            }

            // update log quantile estimate
            tmp16 = (s16) ((delta * countDiv) >> 14);
            if (lmagn[i] > inst->noiseEstLogQuantile[offset + i]) {
                // +=QUANTILE*delta/(inst->counter[s]+1) QUANTILE=0.25, =1 in Q2
                // CounterDiv=1/(inst->counter[s]+1) in Q15
                tmp16 += 2;
                inst->noiseEstLogQuantile[offset + i] += tmp16 / 4;
            } else {
                tmp16 += 1;
                // *(1-QUANTILE), in Q2 QUANTILE=0.25, 1-0.25=0.75=3 in Q2
                // TODO(bjornv): investigate why we need to truncate twice.
                tmp16no2 = (s16) ((tmp16 / 2) * 3 / 2);
                inst->noiseEstLogQuantile[offset + i] -= tmp16no2;
                if (inst->noiseEstLogQuantile[offset + i] < logval) {
                    // This is the smallest fixed point representation we can
                    // have, hence we limit the output.
                    inst->noiseEstLogQuantile[offset + i] = logval;
                }
            }

            // update density estimate
            if (WEBRTC_SPL_ABS_W16(lmagn[i] - inst->noiseEstLogQuantile[offset + i])
                    < WIDTH_Q8) {
                tmp16no1 = (s16) WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                        inst->noiseEstDensity[offset + i], countProd, 15);
                tmp16no2 = (s16) WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                        width_factor, countDiv, 15);
                inst->noiseEstDensity[offset + i] = tmp16no1 + tmp16no2;
            }
        } // end loop over magnitude spectrum

        if (counter >= END_STARTUP_LONG) {
            inst->noiseEstCounter[s] = 0;
            if (inst->blockIndex >= END_STARTUP_LONG) {
                UpdateNoiseEstimate(inst, offset);
            }
        }
        inst->noiseEstCounter[s]++;

    } // end loop over simultaneous estimates

    // Sequentially update the noise during startup
    if (inst->blockIndex < END_STARTUP_LONG) {
        UpdateNoiseEstimate(inst, offset);
    }

    for (i = 0; i < NR_MAGNLEN; i++) {
        noise[i] = (u32) (inst->noiseEstQuantile[i]); // Q(qNoise)
    }
    (*q_noise) = (s16) inst->qNoise;
}

// Filter the data in the frequency domain, and create spectrum.
static void PrepareSpectrumC(NoiseSuppressionFixedC* inst, s16* freq_buf)
{
    uint i = 0, j = 0;

    for (i = 0; i < NR_MAGNLEN; i++) {
        inst->real[i] = (s16) ((inst->real[i] *
                (s16) (inst->noiseSupFilter[i])) >> 14); // Q(normData-stages)
        inst->imag[i] = (s16) ((inst->imag[i] *
                (s16) (inst->noiseSupFilter[i])) >> 14); // Q(normData-stages)
    }

    freq_buf[0] = inst->real[0];
    freq_buf[1] = -inst->imag[0];
    for (i = 1, j = 2; i < NR_ANALEN2; i += 1, j += 2) {
        freq_buf[j] = inst->real[i];
        freq_buf[j + 1] = -inst->imag[i];
    }
    freq_buf[NR_ANALEN] = inst->real[NR_ANALEN2];
    freq_buf[NR_ANALEN + 1] = -inst->imag[NR_ANALEN2];
}

// Denormalize the real-valued signal |in|, the output from inverse FFT.
static void DenormalizeC(NoiseSuppressionFixedC* inst,
                          s16* in,
                          int factor)
{
    uint i = 0;
    s32 tmp32 = 0;
    for (i = 0; i < NR_ANALEN; i += 1) {
        tmp32 = WEBRTC_SPL_SHIFT_W32((s32) in[i],
                factor - inst->normData);
        inst->real[i] = WebRtcSpl_SatW32ToW16(tmp32); // Q0
    }
}

// For the noise supression process, synthesis, read out fully processed
// segment, and update synthesis buffer.
static void SynthesisUpdateC(NoiseSuppressionFixedC* inst,
                               s16* out_frame,
                               s16 gain_factor)
{
    uint i = 0;
    s16 tmp16a = 0;
    s16 tmp16b = 0;
    s32 tmp32 = 0;

    // synthesis
    for (i = 0; i < NR_ANALEN; i++) {
        tmp16a = (s16) WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                 NR_HANNING_WINDOW[i], inst->real[i], 14); // Q0, window in Q14
        tmp32 = WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(tmp16a, gain_factor, 13); // Q0
        // Down shift with rounding
        tmp16b = WebRtcSpl_SatW32ToW16(tmp32); // Q0
        inst->synthesisBuffer[i] = WebRtcSpl_AddSatW16(inst->synthesisBuffer[i],
                                   tmp16b); // Q0
    }

    // read out fully processed segment
    for (i = 0; i < NR_BLOCKLEN10MS; i++) {
        out_frame[i] = inst->synthesisBuffer[i]; // Q0
    }

    // update synthesis buffer
    memcpy(inst->synthesisBuffer, inst->synthesisBuffer + NR_BLOCKLEN10MS,
            (NR_ANALEN - NR_BLOCKLEN10MS) * sizeof (*inst->synthesisBuffer));
    WebRtcSpl_ZerosArrayW16(inst->synthesisBuffer
            + NR_ANALEN - NR_BLOCKLEN10MS, NR_BLOCKLEN10MS);
}

// Update analysis buffer for lower band, and window data before FFT.
static void AnalysisUpdateC(NoiseSuppressionFixedC* inst,
                             s16* out,
                             s16* new_speech)
{
    uint i = 0;

    // For lower band update analysis buffer.
    memcpy(inst->analysisBuffer, inst->analysisBuffer + NR_BLOCKLEN10MS,
            (NR_ANALEN - NR_BLOCKLEN10MS) * sizeof (*inst->analysisBuffer));
    memcpy(inst->analysisBuffer + NR_ANALEN - NR_BLOCKLEN10MS, new_speech,
            NR_BLOCKLEN10MS * sizeof (*inst->analysisBuffer));

    // Window data before FFT.
    for (i = 0; i < NR_ANALEN; i++) {
        out[i] = (s16) WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                NR_HANNING_WINDOW[i], inst->analysisBuffer[i], 14); // Q0
    }
}

// Normalize the real-valued signal |in|, the input to forward FFT.
static void NormalizeRealBufferC(NoiseSuppressionFixedC* inst,
                                   const s16* in,
                                   s16* out)
{
    uint i = 0;
    for (i = 0; i < NR_ANALEN; ++i) {
        out[i] = in[i] << inst->normData; // Q(normData)
    }
}

void WebRtcNsx_CalcParametricNoiseEstimate(NoiseSuppressionFixedC* inst,
                                               s16 pink_noise_exp_avg,
                                               s32 pink_noise_num_avg,
                                               int freq_index,
                                               u32* noise_estimate,
                                               u32* noise_estimate_avg)
{
    s32 tmp32no1 = 0;
    s32 tmp32no2 = 0;

    s16 int_part = 0;
    s16 frac_part = 0;

    // Use pink noise estimate
    // noise_estimate = 2^(pinkNoiseNumerator + pinkNoiseExp * log2(j))
    tmp32no2 = (pink_noise_exp_avg * kLogIndex[freq_index]) >> 15; // Q11
    tmp32no1 = pink_noise_num_avg - tmp32no2; // Q11

    // Calculate output: 2^tmp32no1
    // Output in Q(minNorm-stages)
    tmp32no1 += (inst->minNorm - NR_STAGES) << 11;
    if (tmp32no1 > 0) {
        int_part = (s16) (tmp32no1 >> 11);
        frac_part = (s16) (tmp32no1 & 0x000007ff); // Q11
        // Piecewise linear approximation of 'b' in
        // 2^(int_part+frac_part) = 2^int_part * (1 + b)
        // 'b' is given in Q11 and below stored in frac_part.
        if (frac_part >> 10) {
            // Upper fractional part
            tmp32no2 = (2048 - frac_part) * 1244; // Q21
            tmp32no2 = 2048 - (tmp32no2 >> 10);
        } else {
            // Lower fractional part
            tmp32no2 = (frac_part * 804) >> 10;
        }
        // Shift fractional part to Q(minNorm-stages)
        tmp32no2 = WEBRTC_SPL_SHIFT_W32(tmp32no2, int_part - 11);
        *noise_estimate_avg = (1 << int_part) + (u32) tmp32no2;
        // Scale up to initMagnEst, which is not block averaged
        *noise_estimate = (*noise_estimate_avg) * (u32) (inst->blockIndex + 1);
    }
}

// Initialize state
s32 WebRtcNsx_InitCore(NoiseSuppressionFixedC* inst)
{
    int i;
    //check for valid pointer
    if (inst == NULL) {
        return -1;
    }

//    inst->window = kBlocks80w128x;
    inst->thresholdLogLrt = 131072; //default threshold for LRT feature

    inst->real_fft = WebRtcSpl_CreateRealFFT(NR_STAGES);
    if (inst->real_fft == NULL) {
        return -1;
    }

    //  WebRtcSpl_ZerosArrayW16(inst->analysisBuffer, ANAL_BLOCKL_MAX);
    memset(inst->analysisBuffer, 0, sizeof (*inst->analysisBuffer) * ANAL_BLOCKL_MAX);
    //  WebRtcSpl_ZerosArrayW16(inst->synthesisBuffer, ANAL_BLOCKL_MAX);
    memset(inst->synthesisBuffer, 0, sizeof (*inst->synthesisBuffer) * ANAL_BLOCKL_MAX);

    // for quantile noise estimation
    //  WebRtcSpl_ZerosArrayW16(inst->noiseEstQuantile, HALF_ANAL_BLOCKL);
    memset(inst->noiseEstQuantile, 0, sizeof (*inst->noiseEstQuantile) * HALF_ANAL_BLOCKL);
    for (i = 0; i < SIMULT * HALF_ANAL_BLOCKL; i++) {
        inst->noiseEstLogQuantile[i] = 2048; // Q8
        inst->noiseEstDensity[i] = 153; // Q9
    }
    for (i = 0; i < SIMULT; i++) {
        inst->noiseEstCounter[i] = (s16) (END_STARTUP_LONG * (i + 1)) / SIMULT;
    }

    // Initialize suppression filter with ones
    WebRtcSpl_MemSetW16((s16*) inst->noiseSupFilter, 16384, HALF_ANAL_BLOCKL);

    // Set the aggressiveness: default
    //  inst->aggrMode = 0;

    //initialize variables for new method
    inst->priorNonSpeechProb = 8192; // Q14(0.5) prior probability for speech/noise
    for (i = 0; i < HALF_ANAL_BLOCKL; i++) {
        inst->prevMagnU16[i] = 0;
        inst->prevNoiseU32[i] = 0; //previous noise-spectrum
        inst->logLrtTimeAvgW32[i] = 0; //smooth LR ratio
        inst->avgMagnPause[i] = 0; //conservative noise spectrum estimate
        inst->initMagnEst[i] = 0; //initial average magnitude spectrum
    }

    //feature quantities
    inst->thresholdSpecDiff = 50; //threshold for difference feature: determined on-line
    inst->thresholdSpecFlat = 20480; //threshold for flatness: determined on-line
    inst->featureLogLrt = inst->thresholdLogLrt; //average LRT factor (= threshold)
    inst->featureSpecFlat = inst->thresholdSpecFlat; //spectral flatness (= threshold)
    inst->featureSpecDiff = inst->thresholdSpecDiff; //spectral difference (= threshold)
    inst->weightLogLrt = 6; //default weighting par for LRT feature
    inst->weightSpecFlat = 0; //default weighting par for spectral flatness feature
    inst->weightSpecDiff = 0; //default weighting par for spectral difference feature

    inst->curAvgMagnEnergy = 0; //window time-average of input magnitude spectrum
    inst->timeAvgMagnEnergy = 0; //normalization for spectral difference
    inst->timeAvgMagnEnergyTmp = 0; //normalization for spectral difference

    //histogram quantities: used to estimate/update thresholds for features
    //  WebRtcSpl_ZerosArrayW16(inst->histLrt, HIST_PAR_EST);
    memset(inst->histLrt, 0, sizeof (*inst->histLrt) * HIST_PAR_EST);
    //  WebRtcSpl_ZerosArrayW16(inst->histSpecDiff, HIST_PAR_EST);
    memset(inst->histSpecDiff, 0, sizeof (*inst->histSpecDiff) * HIST_PAR_EST);
    //  WebRtcSpl_ZerosArrayW16(inst->histSpecFlat, HIST_PAR_EST);
    memset(inst->histSpecDiff, 0, sizeof (*inst->histSpecFlat) * HIST_PAR_EST);

    inst->blockIndex = -1; //frame counter

    //inst->modelUpdate    = 500;   //window for update
    //  inst->modelUpdate = (1 << STAT_UPDATES); //window for update
    inst->cntThresUpdate = 0; //counter feature thresholds updates

    inst->sumMagn = 0;
    inst->magnEnergy = 0;
    inst->prevQMagn = 0;
    inst->qNoise = 0;
    inst->prevQNoise = 0;

    inst->energyIn = 0;
    inst->scaleEnergyIn = 0;

    inst->whiteNoiseLevel = 0;
    inst->pinkNoiseNumerator = 0;
    inst->pinkNoiseExp = 0;
    inst->minNorm = 15; // Start with full scale
    inst->zeroInputSignal = 0;

    //default mode
    //  WebRtcNsx_set_policy_core(inst, 0);
    return 0;
}

//int WebRtcNsx_set_policy_core(NoiseSuppressionFixedC* inst, int mode) {
//    // allow for modes:0,1,2,3
//    if (mode < 0 || mode > 3) {
//        return -1;
//    }
//
//    //  inst->aggrMode = mode;
//    if (mode == 0) {
//        //    inst->overdrive = 256; // Q8(1.0)
//        //    inst->denoiseBound = 8192; // Q14(0.5)
//        //    inst->gainMap = 0; // No gain compensation
//    } else if (mode == 1) {
//        //    inst->overdrive = 256; // Q8(1.0)
//        //    inst->denoiseBound = 4096; // Q14(0.25)
////        inst->factor2Table = kFactor2Aggressiveness1;
//        //    inst->gainMap = 1;
//    } else if (mode == 2) {
//        //    inst->overdrive = 282; // ~= Q8(1.1)
//        //    inst->denoiseBound = 2048; // Q14(0.125)
////        inst->factor2Table = kFactor2Aggressiveness2;
//        //    inst->gainMap = 1;
//    } else if (mode == 3) {
//        //    inst->overdrive = 320; // Q8(1.25)
//        //    inst->denoiseBound = 1475; // ~= Q14(0.09)
////        inst->factor2Table = kFactor2Aggressiveness3;
//        //    inst->gainMap = 1;
//    }
//    return 0;
//}

// Extract thresholds for feature parameters
// histograms are computed over some window_size (given by window_pars)
// thresholds and weights are extracted every window
// flag 0 means update histogram only, flag 1 means compute the thresholds/weights
// threshold and weights are returned in: inst->priorModelPars
void WebRtcNsx_FeatureParameterExtraction(NoiseSuppressionFixedC* inst,
                                              int flag)
{
    u32 tmpU32;
    u32 histIndex;
    u32 posPeak1SpecFlatFX, posPeak2SpecFlatFX;
    u32 posPeak1SpecDiffFX, posPeak2SpecDiffFX;

    s32 tmp32;
    s32 fluctLrtFX, thresFluctLrtFX;
    s32 avgHistLrtFX, avgSquareHistLrtFX, avgHistLrtComplFX;

    s16 j;
    s16 numHistLrt;

    int i;
    int useFeatureSpecFlat, useFeatureSpecDiff, featureSum;
    int maxPeak1, maxPeak2;
    int weightPeak1SpecFlat, weightPeak2SpecFlat;
    int weightPeak1SpecDiff, weightPeak2SpecDiff;

    //update histograms
    if (!flag) {
        // LRT
        // Type casting to UWord32 is safe since negative values will not be wrapped to larger
        // values than HIST_PAR_EST
        histIndex = (u32) (inst->featureLogLrt);
        if (histIndex < HIST_PAR_EST) {
            inst->histLrt[histIndex]++;
        }
        // Spectral flatness
        // (inst->featureSpecFlat*20)>>10 = (inst->featureSpecFlat*5)>>8
        histIndex = (inst->featureSpecFlat * 5) >> 8;
        if (histIndex < HIST_PAR_EST) {
            inst->histSpecFlat[histIndex]++;
        }
        // Spectral difference
        histIndex = HIST_PAR_EST;
        if (inst->timeAvgMagnEnergy > 0) {
            // Guard against division by zero
            // If timeAvgMagnEnergy == 0 we have no normalizing statistics and
            // therefore can't update the histogram
            histIndex = ((inst->featureSpecDiff * 5) >> NR_STAGES) /
                    inst->timeAvgMagnEnergy;
        }
        if (histIndex < HIST_PAR_EST) {
            inst->histSpecDiff[histIndex]++;
        }
    }

    // extract parameters for speech/noise probability
    if (flag) {
        useFeatureSpecDiff = 1;
        //for LRT feature:
        // compute the average over inst->featureExtractionParams.rangeAvgHistLrt
        avgHistLrtFX = 0;
        avgSquareHistLrtFX = 0;
        numHistLrt = 0;
        for (i = 0; i < BIN_SIZE_LRT; i++) {
            j = (2 * i + 1);
            tmp32 = inst->histLrt[i] * j;
            avgHistLrtFX += tmp32;
            numHistLrt += inst->histLrt[i];
            avgSquareHistLrtFX += tmp32 * j;
        }
        avgHistLrtComplFX = avgHistLrtFX;
        for (; i < HIST_PAR_EST; i++) {
            j = (2 * i + 1);
            tmp32 = inst->histLrt[i] * j;
            avgHistLrtComplFX += tmp32;
            avgSquareHistLrtFX += tmp32 * j;
        }
        fluctLrtFX = avgSquareHistLrtFX * numHistLrt -
                     avgHistLrtFX * avgHistLrtComplFX;
        thresFluctLrtFX = THRES_FLUCT_LRT * numHistLrt;
        // get threshold for LRT feature:
        tmpU32 = (FACTOR_1_LRT_DIFF * (u32) avgHistLrtFX);
        if ((fluctLrtFX < thresFluctLrtFX) || (numHistLrt == 0) ||
                (tmpU32 > (u32) (100 * numHistLrt))) {
            //very low fluctuation, so likely noise
            inst->thresholdLogLrt = NR_MAXLRT;
        } else {
            tmp32 = (s32) ((tmpU32 << (9 + NR_STAGES)) / numHistLrt / 25);
            // check if value is within min/max range
            inst->thresholdLogLrt = WEBRTC_SPL_SAT(NR_MAXLRT, tmp32, NR_MINLRT);
        }
        if (fluctLrtFX < thresFluctLrtFX) {
            // Do not use difference feature if fluctuation of LRT feature is very low:
            // most likely just noise state
            useFeatureSpecDiff = 0;
        }

        // for spectral flatness and spectral difference: compute the main peaks of histogram
        maxPeak1 = 0;
        maxPeak2 = 0;
        posPeak1SpecFlatFX = 0;
        posPeak2SpecFlatFX = 0;
        weightPeak1SpecFlat = 0;
        weightPeak2SpecFlat = 0;

        // peaks for flatness
        for (i = 0; i < HIST_PAR_EST; i++) {
            if (inst->histSpecFlat[i] > maxPeak1) {
                // Found new "first" peak
                maxPeak2 = maxPeak1;
                weightPeak2SpecFlat = weightPeak1SpecFlat;
                posPeak2SpecFlatFX = posPeak1SpecFlatFX;

                maxPeak1 = inst->histSpecFlat[i];
                weightPeak1SpecFlat = inst->histSpecFlat[i];
                posPeak1SpecFlatFX = (u32) (2 * i + 1);
            } else if (inst->histSpecFlat[i] > maxPeak2) {
                // Found new "second" peak
                maxPeak2 = inst->histSpecFlat[i];
                weightPeak2SpecFlat = inst->histSpecFlat[i];
                posPeak2SpecFlatFX = (u32) (2 * i + 1);
            }
        }

        // for spectral flatness feature
        useFeatureSpecFlat = 1;
        // merge the two peaks if they are close
        if ((posPeak1SpecFlatFX - posPeak2SpecFlatFX < LIM_PEAK_SPACE_FLAT_DIFF)
                && (weightPeak2SpecFlat * LIM_PEAK_WEIGHT_FLAT_DIFF > weightPeak1SpecFlat)) {
            weightPeak1SpecFlat += weightPeak2SpecFlat;
            posPeak1SpecFlatFX = (posPeak1SpecFlatFX + posPeak2SpecFlatFX) >> 1;
        }
        //reject if weight of peaks is not large enough, or peak value too small
        if (weightPeak1SpecFlat < THRES_WEIGHT_FLAT_DIFF || posPeak1SpecFlatFX
                < THRES_PEAK_FLAT) {
            useFeatureSpecFlat = 0;
        } else { // if selected, get the threshold
            // compute the threshold and check if value is within min/max range
            inst->thresholdSpecFlat = WEBRTC_SPL_SAT(MAX_FLAT_Q10, FACTOR_2_FLAT_Q10
                    * posPeak1SpecFlatFX, MIN_FLAT_Q10); //Q10
        }
        // done with flatness feature

        if (useFeatureSpecDiff) {
            //compute two peaks for spectral difference
            maxPeak1 = 0;
            maxPeak2 = 0;
            posPeak1SpecDiffFX = 0;
            posPeak2SpecDiffFX = 0;
            weightPeak1SpecDiff = 0;
            weightPeak2SpecDiff = 0;
            // peaks for spectral difference
            for (i = 0; i < HIST_PAR_EST; i++) {
                if (inst->histSpecDiff[i] > maxPeak1) {
                    // Found new "first" peak
                    maxPeak2 = maxPeak1;
                    weightPeak2SpecDiff = weightPeak1SpecDiff;
                    posPeak2SpecDiffFX = posPeak1SpecDiffFX;

                    maxPeak1 = inst->histSpecDiff[i];
                    weightPeak1SpecDiff = inst->histSpecDiff[i];
                    posPeak1SpecDiffFX = (u32) (2 * i + 1);
                } else if (inst->histSpecDiff[i] > maxPeak2) {
                    // Found new "second" peak
                    maxPeak2 = inst->histSpecDiff[i];
                    weightPeak2SpecDiff = inst->histSpecDiff[i];
                    posPeak2SpecDiffFX = (u32) (2 * i + 1);
                }
            }

            // merge the two peaks if they are close
            if ((posPeak1SpecDiffFX - posPeak2SpecDiffFX < LIM_PEAK_SPACE_FLAT_DIFF)
                    && (weightPeak2SpecDiff * LIM_PEAK_WEIGHT_FLAT_DIFF > weightPeak1SpecDiff)) {
                weightPeak1SpecDiff += weightPeak2SpecDiff;
                posPeak1SpecDiffFX = (posPeak1SpecDiffFX + posPeak2SpecDiffFX) >> 1;
            }
            // get the threshold value and check if value is within min/max range
            inst->thresholdSpecDiff = WEBRTC_SPL_SAT(MAX_DIFF, FACTOR_1_LRT_DIFF
                    * posPeak1SpecDiffFX, MIN_DIFF); //5x bigger
            //reject if weight of peaks is not large enough
            if (weightPeak1SpecDiff < THRES_WEIGHT_FLAT_DIFF) {
                useFeatureSpecDiff = 0;
            }
            // done with spectral difference feature
        }

        // select the weights between the features
        // inst->priorModelPars[4] is weight for LRT: always selected
        featureSum = 6 / (1 + useFeatureSpecFlat + useFeatureSpecDiff);
        inst->weightLogLrt = featureSum;
        inst->weightSpecFlat = useFeatureSpecFlat * featureSum;
        inst->weightSpecDiff = useFeatureSpecDiff * featureSum;

        // set histograms to zero for next update
        WebRtcSpl_ZerosArrayW16(inst->histLrt, HIST_PAR_EST);
        WebRtcSpl_ZerosArrayW16(inst->histSpecDiff, HIST_PAR_EST);
        WebRtcSpl_ZerosArrayW16(inst->histSpecFlat, HIST_PAR_EST);
    } // end of flag == 1
}


// Compute spectral flatness on input spectrum
// magn is the magnitude spectrum
// spectral flatness is returned in inst->featureSpecFlat
void WebRtcNsx_ComputeSpectralFlatness(NoiseSuppressionFixedC* inst,
                                           u16* magn)
{
    u32 tmpU32;
    u32 avgSpectralFlatnessNum, avgSpectralFlatnessDen;

    s32 tmp32;
    s32 currentSpectralFlatness, logCurSpectralFlatness;

    s16 zeros, frac, intPart;

    uint i;

    // for flatness
    avgSpectralFlatnessNum = 0;
    avgSpectralFlatnessDen = inst->sumMagn - (u32) magn[0]; // Q(normData-stages)

    // compute log of ratio of the geometric to arithmetic mean: check for log(0) case
    // flatness = exp( sum(log(magn[i]))/N - log(sum(magn[i])/N) )
    //          = exp( sum(log(magn[i]))/N ) * N / sum(magn[i])
    //          = 2^( sum(log2(magn[i]))/N - (log2(sum(magn[i])) - log2(N)) ) [This is used]
    for (i = 1; i < NR_MAGNLEN; i++) {
        // First bin is excluded from spectrum measures. Number of bins is now a power of 2
        if (magn[i]) {
            zeros = WebRtcSpl_NormU32((u32) magn[i]);
            frac = (s16) (((u32) ((u32) (magn[i]) << zeros) & 0x7FFFFFFF) >> 23);
            // log2(magn(i))
            tmpU32 = (u32) (((31 - zeros) << 8) + WebRtcNsx_kLogTableFrac[frac]); // Q8
            avgSpectralFlatnessNum += tmpU32; // Q8
        } else {
            //if at least one frequency component is zero, treat separately
            tmpU32 = WEBRTC_SPL_UMUL_32_16(inst->featureSpecFlat, SPECT_FLAT_TAVG_Q14); // Q24
            inst->featureSpecFlat -= tmpU32 >> 14; // Q10
            return;
        }
    }
    //ratio and inverse log: check for case of log(0)
    zeros = WebRtcSpl_NormU32(avgSpectralFlatnessDen);
    frac = (s16) (((avgSpectralFlatnessDen << zeros) & 0x7FFFFFFF) >> 23);
    // log2(avgSpectralFlatnessDen)
    tmp32 = (s32) (((31 - zeros) << 8) + WebRtcNsx_kLogTableFrac[frac]); // Q8
    logCurSpectralFlatness = (s32) avgSpectralFlatnessNum;
    logCurSpectralFlatness += ((s32) (NR_STAGES - 1) << (NR_STAGES + 7)); // Q(8+stages-1)
    logCurSpectralFlatness -= (tmp32 << (NR_STAGES - 1));
    logCurSpectralFlatness <<= (10 - NR_STAGES); // Q17
    tmp32 = (s32) (0x00020000 | (WEBRTC_SPL_ABS_W32(logCurSpectralFlatness)
            & 0x0001FFFF)); //Q17
    intPart = 7 - (logCurSpectralFlatness >> 17); // Add 7 for output in Q10.
    if (intPart > 0) {
        currentSpectralFlatness = tmp32 >> intPart;
    } else {
        currentSpectralFlatness = tmp32 << -intPart;
    }

    //time average update of spectral flatness feature
    tmp32 = currentSpectralFlatness - (s32) inst->featureSpecFlat; // Q10
    tmp32 *= SPECT_FLAT_TAVG_Q14; // Q24
    inst->featureSpecFlat += tmp32 >> 14; // Q10
    // done with flatness feature
}


// Compute the difference measure between input spectrum and a template/learned noise spectrum
// magn_tmp is the input spectrum
// the reference/template spectrum is  inst->magn_avg_pause[i]
// returns (normalized) spectral difference in inst->featureSpecDiff
void WebRtcNsx_ComputeSpectralDifference(NoiseSuppressionFixedC* inst,
                                             u16* magnIn)
{
    // This is to be calculated:
    // avgDiffNormMagn = var(magnIn) - cov(magnIn, magnAvgPause)^2 / var(magnAvgPause)

    u32 tmpU32no1, tmpU32no2;
    u32 varMagnUFX, varPauseUFX, avgDiffNormMagnUFX;

    s32 tmp32no1, tmp32no2;
    s32 avgPauseFX, avgMagnFX, covMagnPauseFX;
    s32 maxPause, minPause;

    s16 tmp16no1;

    uint i;
    int norm32, nShifts;

    avgPauseFX = 0;
    maxPause = 0;
    minPause = inst->avgMagnPause[0]; // Q(prevQMagn)
    // compute average quantities
    for (i = 0; i < NR_MAGNLEN; i++) {
        // Compute mean of magn_pause
        avgPauseFX += inst->avgMagnPause[i]; // in Q(prevQMagn)
        maxPause = WEBRTC_SPL_MAX(maxPause, inst->avgMagnPause[i]);
        minPause = WEBRTC_SPL_MIN(minPause, inst->avgMagnPause[i]);
    }
    // normalize by replacing div of "inst->magnLen" with "inst->stages-1" shifts
    avgPauseFX >>= NR_STAGES - 1;
    avgMagnFX = inst->sumMagn >> (NR_STAGES - 1);
    // Largest possible deviation in magnPause for (co)var calculations
    tmp32no1 = WEBRTC_SPL_MAX(maxPause - avgPauseFX, avgPauseFX - minPause);
    // Get number of shifts to make sure we don't get wrap around in varPause
    nShifts = WEBRTC_SPL_MAX(0, 10 + NR_STAGES - WebRtcSpl_NormW32(tmp32no1));

    varMagnUFX = 0;
    varPauseUFX = 0;
    covMagnPauseFX = 0;
    for (i = 0; i < NR_MAGNLEN; i++) {
        // Compute var and cov of magn and magn_pause
        tmp16no1 = (s16) ((s32) magnIn[i] - avgMagnFX);
        tmp32no2 = inst->avgMagnPause[i] - avgPauseFX;
        varMagnUFX += (u32) (tmp16no1 * tmp16no1); // Q(2*qMagn)
        tmp32no1 = tmp32no2 * tmp16no1; // Q(prevQMagn+qMagn)
        covMagnPauseFX += tmp32no1; // Q(prevQMagn+qMagn)
        tmp32no1 = tmp32no2 >> nShifts; // Q(prevQMagn-minPause).
        varPauseUFX += tmp32no1 * tmp32no1; // Q(2*(prevQMagn-minPause))
    }
    //update of average magnitude spectrum: Q(-2*stages) and averaging replaced by shifts
    inst->curAvgMagnEnergy += inst->magnEnergy >> (2 * inst->normData + NR_STAGES - 1);

    avgDiffNormMagnUFX = varMagnUFX; // Q(2*qMagn)
    if ((varPauseUFX) && (covMagnPauseFX)) {
        tmpU32no1 = (u32) WEBRTC_SPL_ABS_W32(covMagnPauseFX); // Q(prevQMagn+qMagn)
        norm32 = WebRtcSpl_NormU32(tmpU32no1) - 16;
        if (norm32 > 0) {
            tmpU32no1 <<= norm32; // Q(prevQMagn+qMagn+norm32)
        } else {
            tmpU32no1 >>= -norm32; // Q(prevQMagn+qMagn+norm32)
        }
        tmpU32no2 = WEBRTC_SPL_UMUL(tmpU32no1, tmpU32no1); // Q(2*(prevQMagn+qMagn-norm32))

        nShifts += norm32;
        nShifts <<= 1;
        if (nShifts < 0) {
            varPauseUFX >>= (-nShifts); // Q(2*(qMagn+norm32+minPause))
            nShifts = 0;
        }
        if (varPauseUFX > 0) {
            // Q(2*(qMagn+norm32-16+minPause))
            tmpU32no1 = tmpU32no2 / varPauseUFX;
            tmpU32no1 >>= nShifts;

            // Q(2*qMagn)
            avgDiffNormMagnUFX -= WEBRTC_SPL_MIN(avgDiffNormMagnUFX, tmpU32no1);
        } else {
            avgDiffNormMagnUFX = 0;
        }
    }
    //normalize and compute time average update of difference feature
    tmpU32no1 = avgDiffNormMagnUFX >> (2 * inst->normData);
    if (inst->featureSpecDiff > tmpU32no1) {
        tmpU32no2 = WEBRTC_SPL_UMUL_32_16(inst->featureSpecDiff - tmpU32no1,
                    SPECT_DIFF_TAVG_Q8); // Q(8-2*stages)
        inst->featureSpecDiff -= tmpU32no2 >> 8; // Q(-2*stages)
    } else {
        tmpU32no2 = WEBRTC_SPL_UMUL_32_16(tmpU32no1 - inst->featureSpecDiff,
                    SPECT_DIFF_TAVG_Q8); // Q(8-2*stages)
        inst->featureSpecDiff += tmpU32no2 >> 8; // Q(-2*stages)
    }
}

// Transform input (speechFrame) to frequency domain magnitude (magnU16)
void WebRtcNsx_DataAnalysis(NoiseSuppressionFixedC* inst,
                              short* speechFrame,
                              u16* magnU16)
{
    u32 tmpU32no1;

    s32 tmp_1_w32 = 0;
    s32 tmp_2_w32 = 0;
    s32 sum_log_magn = 0;
    s32 sum_log_i_log_magn = 0;

    u16 sum_log_magn_u16 = 0;
    u16 tmp_u16 = 0;

    s16 sum_log_i = 0;
    s16 sum_log_i_square = 0;
    s16 frac = 0;
    s16 log2 = 0;
    s16 matrix_determinant = 0;
    s16 maxWinData;

    uint i, j;
    int zeros;
    int net_norm = 0;
    int right_shifts_in_magnU16 = 0;
    int right_shifts_in_initMagnEst = 0;

    // Align the structures to 32-byte boundary for the FFT function.
    s16* winData = nr_temp_buf_1;     //winData需要256个int16_t
    s16* realImag = nr_temp_buf_2;    //realImag需要128个int16_t

    // Update analysis buffer for lower band, and window data before FFT.
    AnalysisUpdateC(inst, winData, speechFrame);

    // Get input energy
    inst->energyIn = WebRtcSpl_Energy(winData, NR_ANALEN, &inst->scaleEnergyIn);

    // Reset zero input flag
    inst->zeroInputSignal = 0;
    // Acquire norm for winData
    maxWinData = WebRtcSpl_MaxAbsValueW16C(winData, NR_ANALEN);
    inst->normData = WebRtcSpl_NormW16(maxWinData);
    if (maxWinData == 0) {
        // Treat zero input separately.
        inst->zeroInputSignal = 1;
        return;
    }

    // Determine the net normalization in the frequency domain
    net_norm = NR_STAGES - inst->normData;
    // Track lowest normalization factor and use it to prevent wrap around in shifting
    right_shifts_in_magnU16 = inst->normData - inst->minNorm;
    right_shifts_in_initMagnEst = WEBRTC_SPL_MAX(-right_shifts_in_magnU16, 0);
    inst->minNorm -= right_shifts_in_initMagnEst;
    right_shifts_in_magnU16 = WEBRTC_SPL_MAX(right_shifts_in_magnU16, 0);

    // create realImag as winData interleaved with zeros (= imag. part), normalize it
    //归一化操作的时候，放大的倍数为128时，下面的fft结果才会和普通的fft一致
    //如果放大倍数不是128，使用普通fft代替的时候，需要注意重新放缩
    NormalizeRealBufferC(inst, winData, realImag);

    // FFT output will be in winData[].
    WebRtcSpl_RealForwardFFT(inst->real_fft, realImag, winData);

    inst->imag[0] = 0; // Q(normData-stages)
    inst->imag[NR_ANALEN2] = 0;
    inst->real[0] = winData[0]; // Q(normData-stages)
    inst->real[NR_ANALEN2] = winData[NR_ANALEN];
    // Q(2*(normData-stages))
    inst->magnEnergy = (u32) (inst->real[0] * inst->real[0]);
    inst->magnEnergy += (u32) (inst->real[NR_ANALEN2] * inst->real[NR_ANALEN2]);
    magnU16[0] = (u16) WEBRTC_SPL_ABS_W16(inst->real[0]); // Q(normData-stages)
    magnU16[NR_ANALEN2] = (u16) WEBRTC_SPL_ABS_W16(inst->real[NR_ANALEN2]);
    inst->sumMagn = (u32) magnU16[0]; // Q(normData-stages)
    inst->sumMagn += (u32) magnU16[NR_ANALEN2];

    if (inst->blockIndex >= END_STARTUP_SHORT) {
        for (i = 1, j = 2; i < NR_ANALEN2; i += 1, j += 2) {
            inst->real[i] = winData[j];
            inst->imag[i] = -winData[j + 1];
            // magnitude spectrum
            // energy in Q(2*(normData-stages))
            tmpU32no1 = (u32) (winData[j] * winData[j]);
            tmpU32no1 += (u32) (winData[j + 1] * winData[j + 1]);
            inst->magnEnergy += tmpU32no1; // Q(2*(normData-stages))

            magnU16[i] = (u16) WebRtcSpl_SqrtFloor(tmpU32no1); // Q(normData-stages)
            inst->sumMagn += (u32) magnU16[i]; // Q(normData-stages)
        }
    } else {
        //
        // Gather information during startup for noise parameter estimation
        //

        // Switch initMagnEst to Q(minNorm-stages)
        inst->initMagnEst[0] >>= right_shifts_in_initMagnEst;
        inst->initMagnEst[NR_ANALEN2] >>= right_shifts_in_initMagnEst;

        // Update initMagnEst with magnU16 in Q(minNorm-stages).
        inst->initMagnEst[0] += magnU16[0] >> right_shifts_in_magnU16;
        inst->initMagnEst[NR_ANALEN2] += magnU16[NR_ANALEN2] >> right_shifts_in_magnU16;

        log2 = 0;
        if (magnU16[NR_ANALEN2]) {
            // Calculate log2(magnU16[inst->anaLen2])
            zeros = WebRtcSpl_NormU32((u32) magnU16[NR_ANALEN2]);
            frac = (s16) ((((u32) magnU16[NR_ANALEN2] << zeros) & 0x7FFFFFFF) >> 23); // Q8
            // log2(magnU16(i)) in Q8
            log2 = (s16) (((31 - zeros) << 8) + WebRtcNsx_kLogTableFrac[frac]);
        }

        sum_log_magn = (s32) log2; // Q8
        // sum_log_i_log_magn in Q17
        sum_log_i_log_magn = (kLogIndex[NR_ANALEN2] * log2) >> 3;

        for (i = 1, j = 2; i < NR_ANALEN2; i += 1, j += 2) {
            inst->real[i] = winData[j];
            inst->imag[i] = -winData[j + 1];
            // magnitude spectrum
            // energy in Q(2*(normData-stages))
            tmpU32no1 = (u32) (winData[j] * winData[j]);
            tmpU32no1 += (u32) (winData[j + 1] * winData[j + 1]);
            inst->magnEnergy += tmpU32no1; // Q(2*(normData-stages))

            magnU16[i] = (u16) WebRtcSpl_SqrtFloor(tmpU32no1); // Q(normData-stages)
            inst->sumMagn += (u32) magnU16[i]; // Q(normData-stages)

            // Switch initMagnEst to Q(minNorm-stages)
            inst->initMagnEst[i] >>= right_shifts_in_initMagnEst;

            // Update initMagnEst with magnU16 in Q(minNorm-stages).
            inst->initMagnEst[i] += magnU16[i] >> right_shifts_in_magnU16;

            if (i >= NR_KSTARTBAND) {
                // For pink noise estimation. Collect data neglecting lower frequency band
                log2 = 0;
                if (magnU16[i]) {
                    zeros = WebRtcSpl_NormU32((u32) magnU16[i]);
                    frac = (s16) ((((u32) magnU16[i] << zeros) & 0x7FFFFFFF) >> 23);
                    // log2(magnU16(i)) in Q8
                    log2 = (s16) (((31 - zeros) << 8) + WebRtcNsx_kLogTableFrac[frac]);
                }
                sum_log_magn += (s32) log2; // Q8
                // sum_log_i_log_magn in Q17
                sum_log_i_log_magn += (kLogIndex[i] * log2) >> 3;
            }
        }

        //
        //compute simplified noise model during startup
        //

        // Estimate White noise

        // Switch whiteNoiseLevel to Q(minNorm-stages)
        inst->whiteNoiseLevel >>= right_shifts_in_initMagnEst;

        // Update the average magnitude spectrum, used as noise estimate.
        tmpU32no1 = WEBRTC_SPL_UMUL_32_16(inst->sumMagn, NR_OVERDRIVE);
        tmpU32no1 >>= NR_STAGES + 8;

        // Replacing division above with 'stages' shifts
        // Shift to same Q-domain as whiteNoiseLevel
        tmpU32no1 >>= right_shifts_in_magnU16;
        // This operation is safe from wrap around as long as END_STARTUP_SHORT < 128
        inst->whiteNoiseLevel += tmpU32no1; // Q(minNorm-stages)

        // Estimate Pink noise parameters
        // Denominator used in both parameter estimates.
        // The value is only dependent on the size of the frequency band (kStartBand)
        // and to reduce computational complexity stored in a table (kDeterminantEstMatrix[])
        matrix_determinant = kDeterminantEstMatrix[NR_KSTARTBAND]; // Q0
        sum_log_i = kSumLogIndex[NR_KSTARTBAND]; // Q5
        sum_log_i_square = kSumSquareLogIndex[NR_KSTARTBAND]; // Q2
        //if (inst->fs == 8000) {
        // Adjust values to shorter blocks in narrow band.
        tmp_1_w32 = (s32) matrix_determinant;
        tmp_1_w32 += (kSumLogIndex[65] * sum_log_i) >> 9;
        tmp_1_w32 -= (kSumLogIndex[65] * kSumLogIndex[65]) >> 10;
        tmp_1_w32 -= (s32) sum_log_i_square << 4;
        tmp_1_w32 -= ((NR_MAGNLEN - NR_KSTARTBAND) * kSumSquareLogIndex[65]) >> 2;
        matrix_determinant = (s16) tmp_1_w32;
        sum_log_i -= kSumLogIndex[65]; // Q5
        sum_log_i_square -= kSumSquareLogIndex[65]; // Q2
        //}

        // Necessary number of shifts to fit sum_log_magn in a word16
        zeros = 16 - WebRtcSpl_NormW32(sum_log_magn);
        if (zeros < 0) {
            zeros = 0;
        }
        tmp_1_w32 = sum_log_magn << 1; // Q9
        sum_log_magn_u16 = (u16) (tmp_1_w32 >> zeros); // Q(9-zeros).

        // Calculate and update pinkNoiseNumerator. Result in Q11.
        tmp_2_w32 = WEBRTC_SPL_MUL_16_U16(sum_log_i_square, sum_log_magn_u16); // Q(11-zeros)
        tmpU32no1 = sum_log_i_log_magn >> 12; // Q5

        // Shift the largest value of sum_log_i and tmp32no3 before multiplication
        tmp_u16 = ((u16) sum_log_i << 1); // Q6
        if ((u32) sum_log_i > tmpU32no1) {
            tmp_u16 >>= zeros;
        } else {
            tmpU32no1 >>= zeros;
        }
        tmp_2_w32 -= (s32) WEBRTC_SPL_UMUL_32_16(tmpU32no1, tmp_u16); // Q(11-zeros)
        matrix_determinant >>= zeros; // Q(-zeros)
        tmp_2_w32 = WebRtcSpl_DivW32W16(tmp_2_w32, matrix_determinant); // Q11
        tmp_2_w32 += (s32) net_norm << 11; // Q11
        if (tmp_2_w32 < 0) {
            tmp_2_w32 = 0;
        }
        inst->pinkNoiseNumerator += tmp_2_w32; // Q11

        // Calculate and update pinkNoiseExp. Result in Q14.
        tmp_2_w32 = WEBRTC_SPL_MUL_16_U16(sum_log_i, sum_log_magn_u16); // Q(14-zeros)
        tmp_1_w32 = sum_log_i_log_magn >> (3 + zeros);
        tmp_1_w32 *= NR_MAGNLEN - NR_KSTARTBAND;
        tmp_2_w32 -= tmp_1_w32; // Q(14-zeros)
        if (tmp_2_w32 > 0) {
            // If the exponential parameter is negative force it to zero, which means a
            // flat spectrum.
            tmp_1_w32 = WebRtcSpl_DivW32W16(tmp_2_w32, matrix_determinant); // Q14
            inst->pinkNoiseExp += WEBRTC_SPL_SAT(16384, tmp_1_w32, 0); // Q14
        }
    }
}

void WebRtcNsx_DataSynthesis(NoiseSuppressionFixedC* inst, short* outFrame)
{
    s32 energyOut;

    // Align the structures to 32-byte boundary for the FFT function.
    s16* realImag = nr_temp_buf_1;    //realImag需要256个int16_t
    s16* rfft_out = nr_temp_buf_2;    //rfft_out需要128个int16_t

    s16 tmp16no1, tmp16no2;
    s16 energyRatio;
    s16 gainFactor, gainFactor1, gainFactor2;

    uint i;
    int outCIFFT;
    int scaleEnergyOut = 0;

    if (inst->zeroInputSignal) {
        // synthesize the special case of zero input
        // read out fully processed segment
        for (i = 0; i < NR_BLOCKLEN10MS; i++) {
            outFrame[i] = inst->synthesisBuffer[i]; // Q0
        }
        // update synthesis buffer
        memcpy(inst->synthesisBuffer, inst->synthesisBuffer + NR_BLOCKLEN10MS,
                (NR_ANALEN - NR_BLOCKLEN10MS) * sizeof (*inst->synthesisBuffer));
        WebRtcSpl_ZerosArrayW16(inst->synthesisBuffer + NR_ANALEN - NR_BLOCKLEN10MS,
                                NR_BLOCKLEN10MS);
        return;
    }

    // Filter the data in the frequency domain, and create spectrum.
    PrepareSpectrumC(inst, realImag);

    // Inverse FFT output will be in rfft_out[].
    //webrtc的ifft结果会动态放缩，暂时没有想出如何使用普通ifft替代
    outCIFFT = WebRtcSpl_RealInverseFFT(inst->real_fft, realImag, rfft_out);

    DenormalizeC(inst, rfft_out, outCIFFT);

    //scale factor: only do it after END_STARTUP_LONG time
    gainFactor = 8192; // 8192 = Q13(1.0)
#if NR_GAINMAP
    if (inst->blockIndex > END_STARTUP_LONG && inst->energyIn > 0) {
        // Q(-scaleEnergyOut)
        energyOut = WebRtcSpl_Energy(inst->real, NR_ANALEN, &scaleEnergyOut);
        if (scaleEnergyOut == 0 && !(energyOut & 0x7f800000)) {
            energyOut = WEBRTC_SPL_SHIFT_W32(energyOut, 8 + scaleEnergyOut
                        - inst->scaleEnergyIn);
        } else {
            // |energyIn| is currently in Q(|scaleEnergyIn|), but to later on end up
            // with an |energyRatio| in Q8 we need to change the Q-domain to
            // Q(-8-scaleEnergyOut).
            inst->energyIn >>= 8 + scaleEnergyOut - inst->scaleEnergyIn;
        }

        energyRatio = (energyOut + inst->energyIn / 2) / inst->energyIn; // Q8
        // Limit the ratio to [0, 1] in Q8, i.e., [0, 256]
        energyRatio = WEBRTC_SPL_SAT(256, energyRatio, 0);

        // all done in lookup tables now
        gainFactor1 = kFactor1Table[energyRatio]; // Q8
        gainFactor2 = NR_FACTOR2TABLE[energyRatio]; // Q8

        //combine both scales with speech/noise prob: note prior (priorSpeechProb) is not frequency dependent

        // factor = inst->priorSpeechProb*factor1 + (1.0-inst->priorSpeechProb)*factor2; // original code
        tmp16no1 = (s16) (((16384 - inst->priorNonSpeechProb) * gainFactor1) >>
                14); // in Q13, where 16384 = Q14(1.0)
        tmp16no2 = (s16) ((inst->priorNonSpeechProb * gainFactor2) >> 14);
        gainFactor = tmp16no1 + tmp16no2; // Q13
    } // out of flag_gain_map==1
#endif
    // Synthesis, read out fully processed segment, and update synthesis buffer.
    SynthesisUpdateC(inst, outFrame, gainFactor);
}

void WebRtcNsx_ProcessCore(NoiseSuppressionFixedC* inst,
                              short* speechFrame,
                              int num_bands,
                              short* outFrame)
{
    // main routine for noise suppression
    u32 tmpU32no1, tmpU32no2, tmpU32no3;
    const u32 satMax = (u32) 1048575; // Largest possible value without getting overflow despite shifting 12 steps
    u32 maxNoiseU32;
    u32 tmpMagnU32, tmpNoiseU32;
    u32 nearMagnEst;
    u32 noiseUpdateU32;
//    u32 WebRtcNsx_noiseU32[HALF_ANAL_BLOCKL];
//    u32 WebRtcNsx_postLocSnr[HALF_ANAL_BLOCKL];
//    u32 WebRtcNsx_priorLocSnr[HALF_ANAL_BLOCKL];
//    u32 prevNearSnr[HALF_ANAL_BLOCKL];
    u32 *prevNearSnr = (u32 *)nr_temp_buf_2;    //整片nr_temp_buf_2用作prevNearSnr
    u32 curNearSnr;
    u32 priorSnr;
    u32 noise_estimate = 0;
    u32 noise_estimate_avg = 0;
    u32 numerator = 0;

    s32 tmp32no1, tmp32no2;
    s32 pink_noise_num_avg = 0;

    u16 tmpU16no1;
    u16 magnU16[HALF_ANAL_BLOCKL];
    
//    u16 prevNoiseU16[HALF_ANAL_BLOCKL];
    //nr_temp_buf_1前65个s16用作prevNoiseU16
    u16 *prevNoiseU16 = (u16 *)nr_temp_buf_1;
//    u16 nonSpeechProbFinal[HALF_ANAL_BLOCKL];
    //nr_temp_buf_1接下来65个s16用作nonSpeechProbFinal
    u16 *nonSpeechProbFinal = (u16 *)nr_temp_buf_1+HALF_ANAL_BLOCKL;
    
    u16 gammaNoise, prevGammaNoise;
//    u16 noiseSupFilterTmp[HALF_ANAL_BLOCKL];
    //nr_temp_buf_1接下来65个s16用作noiseSupFilterTmp
    u16 *noiseSupFilterTmp = (u16 *)nr_temp_buf_1+HALF_ANAL_BLOCKL*2;

    s16 qMagn, qNoise;
    s16 pink_noise_exp_avg = 0;

    uint i;
    int nShifts, postShifts;
    int norm32no1, norm32no2;
    int flag, sign;
    int q_domain_to_use = 0;

    // Store speechFrame and transform to frequency domain
    WebRtcNsx_DataAnalysis(inst, speechFrame, magnU16);

    if (inst->zeroInputSignal) {
        WebRtcNsx_DataSynthesis(inst, outFrame);
        return;
    }

    // Update block index when we have something to process
    inst->blockIndex++;
    //

    // Norm of magn
    qMagn = inst->normData - NR_STAGES;

    // Compute spectral flatness on input spectrum
    WebRtcNsx_ComputeSpectralFlatness(inst, magnU16);

    // quantile noise estimate
    NoiseEstimationC(inst, magnU16, WebRtcNsx_noiseU32, &qNoise);

    //noise estimate from previous frame
    for (i = 0; i < NR_MAGNLEN; i++) {
        prevNoiseU16[i] = (u16) (inst->prevNoiseU32[i] >> 11); // Q(prevQNoise)
    }

    if (inst->blockIndex < END_STARTUP_SHORT) {
        // Noise Q-domain to be used later; see description at end of section.
        q_domain_to_use = WEBRTC_SPL_MIN((int) qNoise, inst->minNorm - NR_STAGES);

        // Calculate frequency independent parts in parametric noise estimate and calculate
        // the estimate for the lower frequency band (same values for all frequency bins)
        if (inst->pinkNoiseExp) {
            pink_noise_exp_avg = (s16) WebRtcSpl_DivW32W16(inst->pinkNoiseExp,
                                 (s16) (inst->blockIndex + 1)); // Q14
            pink_noise_num_avg = WebRtcSpl_DivW32W16(inst->pinkNoiseNumerator,
                                 (s16) (inst->blockIndex + 1)); // Q11
            WebRtcNsx_CalcParametricNoiseEstimate(inst,
                                                  pink_noise_exp_avg,
                                                  pink_noise_num_avg,
                                                  NR_KSTARTBAND,
                                                  &noise_estimate,
                                                  &noise_estimate_avg);
        } else {
            // Use white noise estimate if we have poor pink noise parameter estimates
            noise_estimate = inst->whiteNoiseLevel; // Q(minNorm-stages)
            noise_estimate_avg = noise_estimate / (inst->blockIndex + 1); // Q(minNorm-stages)
        }
        for (i = 0; i < NR_MAGNLEN; i++) {
            // Estimate the background noise using the pink noise parameters if permitted
            if ((inst->pinkNoiseExp) && (i >= NR_KSTARTBAND)) {
                // Reset noise_estimate
                noise_estimate = 0;
                noise_estimate_avg = 0;
                // Calculate the parametric noise estimate for current frequency bin
                WebRtcNsx_CalcParametricNoiseEstimate(inst,
                                                      pink_noise_exp_avg,
                                                      pink_noise_num_avg,
                                                      i,
                                                      &noise_estimate,
                                                      &noise_estimate_avg);
            }
            // Calculate parametric Wiener filter
            noiseSupFilterTmp[i] = NR_DENOISEBOUND;
            if (inst->initMagnEst[i]) {
                // numerator = (initMagnEst - noise_estimate * overdrive)
                // Result in Q(8+minNorm-stages)
                tmpU32no1 = WEBRTC_SPL_UMUL_32_16(noise_estimate, NR_OVERDRIVE);
                numerator = inst->initMagnEst[i] << 8;
                if (numerator > tmpU32no1) {
                    // Suppression filter coefficient larger than zero, so calculate.
                    numerator -= tmpU32no1;

                    // Determine number of left shifts in numerator for best accuracy after
                    // division
                    nShifts = WebRtcSpl_NormU32(numerator);
                    nShifts = WEBRTC_SPL_SAT(6, nShifts, 0);

                    // Shift numerator to Q(nShifts+8+minNorm-stages)
                    numerator <<= nShifts;

                    // Shift denominator to Q(nShifts-6+minNorm-stages)
                    tmpU32no1 = inst->initMagnEst[i] >> (6 - nShifts);
                    if (tmpU32no1 == 0) {
                        // This is only possible if numerator = 0, in which case
                        // we don't need any division.
                        tmpU32no1 = 1;
                    }
                    tmpU32no2 = numerator / tmpU32no1; // Q14
                    noiseSupFilterTmp[i] = (u16) WEBRTC_SPL_SAT(16384, tmpU32no2,
                            (u32) (NR_DENOISEBOUND)); // Q14
                }
            }
            // Weight quantile noise 'WebRtcNsx_noiseU32' with modeled noise 'noise_estimate_avg'
            // 'WebRtcNsx_noiseU32 is in Q(qNoise) and 'noise_estimate' in Q(minNorm-stages)
            // To guarantee that we do not get wrap around when shifting to the same domain
            // we use the lowest one. Furthermore, we need to save 6 bits for the weighting.
            // 'noise_estimate_avg' can handle this operation by construction, but 'WebRtcNsx_noiseU32'
            // may not.

            // Shift 'WebRtcNsx_noiseU32' to 'q_domain_to_use'
            tmpU32no1 = WebRtcNsx_noiseU32[i] >> (qNoise - q_domain_to_use);
            // Shift 'noise_estimate_avg' to 'q_domain_to_use'
            tmpU32no2 = noise_estimate_avg >> (inst->minNorm - NR_STAGES - q_domain_to_use);
            // Make a simple check to see if we have enough room for weighting 'tmpU32no1'
            // without wrap around
            nShifts = 0;
            if (tmpU32no1 & 0xfc000000) {
                tmpU32no1 >>= 6;
                tmpU32no2 >>= 6;
                nShifts = 6;
            }
            tmpU32no1 *= inst->blockIndex;
            tmpU32no2 *= (END_STARTUP_SHORT - inst->blockIndex);
            // Add them together and divide by startup length
            WebRtcNsx_noiseU32[i] = WebRtcSpl_DivU32U16(tmpU32no1 + tmpU32no2, END_STARTUP_SHORT);
            // Shift back if necessary
            WebRtcNsx_noiseU32[i] <<= nShifts;
        }
        // Update new Q-domain for 'WebRtcNsx_noiseU32'
        qNoise = q_domain_to_use;
    }
    // compute average signal during END_STARTUP_LONG time:
    // used to normalize spectral difference measure
    if (inst->blockIndex < END_STARTUP_LONG) {
        // substituting division with shift ending up in Q(-2*stages)
        inst->timeAvgMagnEnergyTmp +=
                inst->magnEnergy >> (2 * inst->normData + NR_STAGES - 1);
        inst->timeAvgMagnEnergy = WebRtcSpl_DivU32U16(inst->timeAvgMagnEnergyTmp,
                inst->blockIndex + 1);
    }

    //start processing at frames == converged+1
    // STEP 1: compute prior and post SNR based on quantile noise estimates

    // compute direct decision (DD) estimate of prior SNR: needed for new method
    postShifts = 6 + qMagn - qNoise;
    nShifts = 5 - inst->prevQMagn + inst->prevQNoise;
    for (i = 0; i < NR_MAGNLEN; i++) {
        // FLOAT:
        // post SNR
        // WebRtcNsx_postLocSnr[i] = 0.0;
        // if (magn[i] > noise[i])
        // {
        //   WebRtcNsx_postLocSnr[i] = magn[i] / (noise[i] + 0.0001);
        // }
        // // previous post SNR
        // // previous estimate: based on previous frame with gain filter (smooth is previous filter)
        //
        // prevNearSnr[i] = inst->prevMagnU16[i] / (inst->noisePrev[i] + 0.0001) * (inst->smooth[i]);
        //
        // // DD estimate is sum of two terms: current estimate and previous estimate
        // // directed decision update of priorSnr (or we actually store [2*priorSnr+1])
        //
        // WebRtcNsx_priorLocSnr[i] = DD_PR_SNR * prevNearSnr[i] + (1.0 - DD_PR_SNR) * (WebRtcNsx_postLocSnr[i] - 1.0);

        // calculate post SNR: output in Q11
        WebRtcNsx_postLocSnr[i] = 2048; // 1.0 in Q11
        tmpU32no1 = (u32) magnU16[i] << 6; // Q(6+qMagn)
        if (postShifts < 0) {
            tmpU32no2 = WebRtcNsx_noiseU32[i] >> -postShifts; // Q(6+qMagn)
        } else {
            tmpU32no2 = WebRtcNsx_noiseU32[i] << postShifts; // Q(6+qMagn)
        }
        if (tmpU32no1 > tmpU32no2) {
            // Current magnitude larger than noise
            tmpU32no1 <<= 11; // Q(17+qMagn)
            if (tmpU32no2 > 0) {
                tmpU32no1 /= tmpU32no2; // Q11
                WebRtcNsx_postLocSnr[i] = WEBRTC_SPL_MIN(satMax, tmpU32no1); // Q11
            } else {
                WebRtcNsx_postLocSnr[i] = satMax;
            }
        }

        // calculate prevNearSnr[i] and save for later instead of recalculating it later
        // |nearMagnEst| in Q(prevQMagn + 14)
        nearMagnEst = inst->prevMagnU16[i] * inst->noiseSupFilter[i];
        tmpU32no1 = nearMagnEst << 3; // Q(prevQMagn+17)
        tmpU32no2 = inst->prevNoiseU32[i] >> nShifts; // Q(prevQMagn+6)

        if (tmpU32no2 > 0) {
            tmpU32no1 /= tmpU32no2; // Q11
            tmpU32no1 = WEBRTC_SPL_MIN(satMax, tmpU32no1); // Q11
        } else {
            tmpU32no1 = satMax; // Q11
        }
        prevNearSnr[i] = tmpU32no1; // Q11

        //directed decision update of priorSnr
        tmpU32no1 = WEBRTC_SPL_UMUL_32_16(prevNearSnr[i], DD_PR_SNR_Q11); // Q22
        tmpU32no2 = WEBRTC_SPL_UMUL_32_16(WebRtcNsx_postLocSnr[i] - 2048, ONE_MINUS_DD_PR_SNR_Q11); // Q22
        priorSnr = tmpU32no1 + tmpU32no2 + 512; // Q22 (added 512 for rounding)
        // WebRtcNsx_priorLocSnr = 1 + 2*priorSnr
        WebRtcNsx_priorLocSnr[i] = 2048 + (priorSnr >> 10); // Q11
    } // end of loop over frequencies
    // done with step 1: DD computation of prior and post SNR

    // STEP 2: compute speech/noise likelihood

    //compute difference of input spectrum with learned/estimated noise spectrum
    WebRtcNsx_ComputeSpectralDifference(inst, magnU16);
    //compute histograms for determination of parameters (thresholds and weights for features)
    //parameters are extracted once every window time (=inst->modelUpdate)
    //counter update
    inst->cntThresUpdate++;
    flag = (int) (inst->cntThresUpdate == NR_MODELUPDATE);
    //update histogram
    WebRtcNsx_FeatureParameterExtraction(inst, flag);
    //compute model parameters
    if (flag) {
        inst->cntThresUpdate = 0; // Reset counter
        //update every window:
        // get normalization for spectral difference for next window estimate

        // Shift to Q(-2*stages)
        inst->curAvgMagnEnergy >>= STAT_UPDATES;

        tmpU32no1 = (inst->curAvgMagnEnergy + inst->timeAvgMagnEnergy + 1) >> 1; //Q(-2*stages)
        // Update featureSpecDiff
        if ((tmpU32no1 != inst->timeAvgMagnEnergy) && (inst->featureSpecDiff) &&
                (inst->timeAvgMagnEnergy > 0)) {
            norm32no1 = 0;
            tmpU32no3 = tmpU32no1;
            while (0xFFFF0000 & tmpU32no3) {
                tmpU32no3 >>= 1;
                norm32no1++;
            }
            tmpU32no2 = inst->featureSpecDiff;
            while (0xFFFF0000 & tmpU32no2) {
                tmpU32no2 >>= 1;
                norm32no1++;
            }
            tmpU32no3 = WEBRTC_SPL_UMUL(tmpU32no3, tmpU32no2);
            tmpU32no3 /= inst->timeAvgMagnEnergy;
            if (WebRtcSpl_NormU32(tmpU32no3) < norm32no1) {
                inst->featureSpecDiff = 0x007FFFFF;
            } else {
                inst->featureSpecDiff = WEBRTC_SPL_MIN(0x007FFFFF,
                        tmpU32no3 << norm32no1);
            }
        }

        inst->timeAvgMagnEnergy = tmpU32no1; // Q(-2*stages)
        inst->curAvgMagnEnergy = 0;
    }

    //compute speech/noise probability
    WebRtcNsx_SpeechNoiseProb(inst, nonSpeechProbFinal, WebRtcNsx_priorLocSnr, WebRtcNsx_postLocSnr);

    //time-avg parameter for noise update
    gammaNoise = NOISE_UPDATE_Q8; // Q8

    maxNoiseU32 = 0;
    postShifts = inst->prevQNoise - qMagn;
    nShifts = inst->prevQMagn - qMagn;
    for (i = 0; i < NR_MAGNLEN; i++) {
        // temporary noise update: use it for speech frames if update value is less than previous
        // the formula has been rewritten into:
        // noiseUpdate = noisePrev[i] + (1 - gammaNoise) * nonSpeechProb * (magn[i] - noisePrev[i])

        if (postShifts < 0) {
            tmpU32no2 = magnU16[i] >> -postShifts; // Q(prevQNoise)
        } else {
            tmpU32no2 = (u32) magnU16[i] << postShifts; // Q(prevQNoise)
        }
        if (prevNoiseU16[i] > tmpU32no2) {
            sign = -1;
            tmpU32no1 = prevNoiseU16[i] - tmpU32no2;
        } else {
            sign = 1;
            tmpU32no1 = tmpU32no2 - prevNoiseU16[i];
        }
        noiseUpdateU32 = inst->prevNoiseU32[i]; // Q(prevQNoise+11)
        tmpU32no3 = 0;
        if ((tmpU32no1) && (nonSpeechProbFinal[i])) {
            // This value will be used later, if gammaNoise changes
            tmpU32no3 = WEBRTC_SPL_UMUL_32_16(tmpU32no1, nonSpeechProbFinal[i]); // Q(prevQNoise+8)
            if (0x7c000000 & tmpU32no3) {
                // Shifting required before multiplication
                tmpU32no2 = (tmpU32no3 >> 5) * gammaNoise; // Q(prevQNoise+11)
            } else {
                // We can do shifting after multiplication
                tmpU32no2 = (tmpU32no3 * gammaNoise) >> 5; // Q(prevQNoise+11)
            }
            if (sign > 0) {
                noiseUpdateU32 += tmpU32no2; // Q(prevQNoise+11)
            } else {
                // This operation is safe. We can never get wrap around, since worst
                // case scenario means magnU16 = 0
                noiseUpdateU32 -= tmpU32no2; // Q(prevQNoise+11)
            }
        }

        //increase gamma (i.e., less noise update) for frame likely to be speech
        prevGammaNoise = gammaNoise;
        gammaNoise = NOISE_UPDATE_Q8;
        //time-constant based on speech/noise state
        //increase gamma (i.e., less noise update) for frames likely to be speech
        if (nonSpeechProbFinal[i] < ONE_MINUS_PROB_RANGE_Q8) {
            gammaNoise = GAMMA_NOISE_TRANS_AND_SPEECH_Q8;
        }

        if (prevGammaNoise != gammaNoise) {
            // new noise update
            // this line is the same as above, only that the result is stored in a different variable and the gammaNoise
            // has changed
            //
            // noiseUpdate = noisePrev[i] + (1 - gammaNoise) * nonSpeechProb * (magn[i] - noisePrev[i])

            if (0x7c000000 & tmpU32no3) {
                // Shifting required before multiplication
                tmpU32no2 = (tmpU32no3 >> 5) * gammaNoise; // Q(prevQNoise+11)
            } else {
                // We can do shifting after multiplication
                tmpU32no2 = (tmpU32no3 * gammaNoise) >> 5; // Q(prevQNoise+11)
            }
            if (sign > 0) {
                tmpU32no1 = inst->prevNoiseU32[i] + tmpU32no2; // Q(prevQNoise+11)
            } else {
                tmpU32no1 = inst->prevNoiseU32[i] - tmpU32no2; // Q(prevQNoise+11)
            }
            if (noiseUpdateU32 > tmpU32no1) {
                noiseUpdateU32 = tmpU32no1; // Q(prevQNoise+11)
            }
        }
        WebRtcNsx_noiseU32[i] = noiseUpdateU32; // Q(prevQNoise+11)
        if (noiseUpdateU32 > maxNoiseU32) {
            maxNoiseU32 = noiseUpdateU32;
        }

        // conservative noise update
        // // original FLOAT code
        // if (prob_speech < PROB_RANGE) {
        // inst->avgMagnPause[i] = inst->avgMagnPause[i] + (1.0 - gamma_pause)*(magn[i] - inst->avgMagnPause[i]);
        // }

        tmp32no2 = WEBRTC_SPL_SHIFT_W32(inst->avgMagnPause[i], -nShifts);
        if (nonSpeechProbFinal[i] > ONE_MINUS_PROB_RANGE_Q8) {
            if (nShifts < 0) {
                tmp32no1 = (s32) magnU16[i] - tmp32no2; // Q(qMagn)
                tmp32no1 *= ONE_MINUS_GAMMA_PAUSE_Q8; // Q(8+prevQMagn+nShifts)
                tmp32no1 = (tmp32no1 + 128) >> 8; // Q(qMagn).
            } else {
                // In Q(qMagn+nShifts)
                tmp32no1 = ((s32) magnU16[i] << nShifts) - inst->avgMagnPause[i];
                tmp32no1 *= ONE_MINUS_GAMMA_PAUSE_Q8; // Q(8+prevQMagn+nShifts)
                tmp32no1 = (tmp32no1 + (128 << nShifts)) >> (8 + nShifts); // Q(qMagn).
            }
            tmp32no2 += tmp32no1; // Q(qMagn)
        }
        inst->avgMagnPause[i] = tmp32no2;
    } // end of frequency loop

    norm32no1 = WebRtcSpl_NormU32(maxNoiseU32);
    qNoise = inst->prevQNoise + norm32no1 - 5;
    // done with step 2: noise update

    // STEP 3: compute dd update of prior snr and post snr based on new noise estimate
    nShifts = inst->prevQNoise + 11 - qMagn;
    for (i = 0; i < NR_MAGNLEN; i++) {
        // FLOAT code
        // // post and prior SNR
        // curNearSnr = 0.0;
        // if (magn[i] > noise[i])
        // {
        // curNearSnr = magn[i] / (noise[i] + 0.0001) - 1.0;
        // }
        // // DD estimate is sum of two terms: current estimate and previous estimate
        // // directed decision update of snrPrior
        // snrPrior = DD_PR_SNR * prevNearSnr[i] + (1.0 - DD_PR_SNR) * curNearSnr;
        // // gain filter
        // tmpFloat1 = inst->overdrive + snrPrior;
        // tmpFloat2 = snrPrior / tmpFloat1;
        // theFilter[i] = tmpFloat2;

        // calculate curNearSnr again, this is necessary because a new noise estimate has been made since then. for the original
        curNearSnr = 0; // Q11
        if (nShifts < 0) {
            // This case is equivalent with magn < noise which implies curNearSnr = 0;
            tmpMagnU32 = (u32) magnU16[i]; // Q(qMagn)
            tmpNoiseU32 = WebRtcNsx_noiseU32[i] << -nShifts; // Q(qMagn)
        } else if (nShifts > 17) {
            tmpMagnU32 = (u32) magnU16[i] << 17; // Q(qMagn+17)
            tmpNoiseU32 = WebRtcNsx_noiseU32[i] >> (nShifts - 17); // Q(qMagn+17)
        } else {
            tmpMagnU32 = (u32) magnU16[i] << nShifts; // Q(qNoise_prev+11)
            tmpNoiseU32 = WebRtcNsx_noiseU32[i]; // Q(qNoise_prev+11)
        }
        if (tmpMagnU32 > tmpNoiseU32) {
            tmpU32no1 = tmpMagnU32 - tmpNoiseU32; // Q(qCur)
            norm32no2 = WEBRTC_SPL_MIN(11, WebRtcSpl_NormU32(tmpU32no1));
            tmpU32no1 <<= norm32no2; // Q(qCur+norm32no2)
            tmpU32no2 = tmpNoiseU32 >> (11 - norm32no2); // Q(qCur+norm32no2-11)
            if (tmpU32no2 > 0) {
                tmpU32no1 /= tmpU32no2; // Q11
            }
            curNearSnr = WEBRTC_SPL_MIN(satMax, tmpU32no1); // Q11
        }

        //directed decision update of priorSnr
        // FLOAT
        // priorSnr = DD_PR_SNR * prevNearSnr + (1.0-DD_PR_SNR) * curNearSnr;

        tmpU32no1 = WEBRTC_SPL_UMUL_32_16(prevNearSnr[i], DD_PR_SNR_Q11); // Q22
        tmpU32no2 = WEBRTC_SPL_UMUL_32_16(curNearSnr, ONE_MINUS_DD_PR_SNR_Q11); // Q22
        priorSnr = tmpU32no1 + tmpU32no2; // Q22

        //gain filter
        tmpU32no1 = NR_OVERDRIVE + ((priorSnr + 8192) >> 14); // Q8
        tmpU16no1 = (priorSnr + tmpU32no1 / 2) / tmpU32no1; // Q14
        inst->noiseSupFilter[i] = WEBRTC_SPL_SAT(16384, tmpU16no1, NR_DENOISEBOUND); // 16384 = Q14(1.0) // Q14

        // Weight in the parametric Wiener filter during startup
        if (inst->blockIndex < END_STARTUP_SHORT) {
            // Weight the two suppression filters
            tmpU32no1 = inst->noiseSupFilter[i] * inst->blockIndex;
            tmpU32no2 = noiseSupFilterTmp[i] *
                    (END_STARTUP_SHORT - inst->blockIndex);
            tmpU32no1 += tmpU32no2;
            inst->noiseSupFilter[i] = (u16) WebRtcSpl_DivU32U16(tmpU32no1,
                    END_STARTUP_SHORT);
        }
    } // end of loop over frequencies
    //done with step3

    // save noise and magnitude spectrum for next frame
    inst->prevQNoise = qNoise;
    inst->prevQMagn = qMagn;
    if (norm32no1 > 5) {
        for (i = 0; i < NR_MAGNLEN; i++) {
            inst->prevNoiseU32[i] = WebRtcNsx_noiseU32[i] << (norm32no1 - 5); // Q(qNoise+11)
            inst->prevMagnU16[i] = magnU16[i]; // Q(qMagn)
        }
    } else {
        for (i = 0; i < NR_MAGNLEN; i++) {
            inst->prevNoiseU32[i] = WebRtcNsx_noiseU32[i] >> (5 - norm32no1); // Q(qNoise+11)
            inst->prevMagnU16[i] = magnU16[i]; // Q(qMagn)
        }
    }

    WebRtcNsx_DataSynthesis(inst, outFrame);
}
