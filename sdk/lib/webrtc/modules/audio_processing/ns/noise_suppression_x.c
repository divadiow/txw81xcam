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
#include "nsx_defines.h"

//NoiseSuppressionFixedC noise_suppression_ctl;

NsxHandle* WebRtcNsx_Create() 
{
	NoiseSuppressionFixedC *noise_suppression_ctl = (NoiseSuppressionFixedC*)webrtc_zalloc(sizeof(NoiseSuppressionFixedC));
    return (NsxHandle*)noise_suppression_ctl;
}

int WebRtcNsx_Init(NsxHandle* nsxInst, u32 fs)
{
    return WebRtcNsx_InitCore((NoiseSuppressionFixedC*) nsxInst);
}

//int WebRtcNsx_set_policy(NsxHandle* nsxInst, int mode)
//{
//    return WebRtcNsx_set_policy_core((NoiseSuppressionFixedC*) nsxInst, mode);
//}

void WebRtcNsx_Process(NsxHandle* nsxInst,
                      short* speechFrame,
                      int num_bands,
                      short* outFrame)
{
    WebRtcNsx_ProcessCore((NoiseSuppressionFixedC*) nsxInst, speechFrame,
                          num_bands, outFrame);
}

