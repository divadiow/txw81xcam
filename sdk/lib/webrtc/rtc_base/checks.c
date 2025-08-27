/*
 *  Copyright 2006 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Most of this was borrowed (with minor modifications) from V8's and Chromium's
// src/base/logging.cc.
#include "checks.h"
#include "webrtc_typedef.h"

#define LAST_SYSTEM_ERROR (0)

#if !defined(WEBRTC_CHROMIUM_BUILD)
void WriteFatalLog(char *output) {
  printf("%s",output);
}

#endif  // !defined(WEBRTC_CHROMIUM_BUILD)

void FatalLog(const char* file, int line) {
  char *s = "FatalLog err\n";
  WriteFatalLog(s);
}

// Function to call from the C version of the RTC_CHECK and RTC_DCHECK macros.
void rtc_FatalMessage(const char* file,
                                   int line,
                                   const char* msg) {
  FatalLog(file, line);
}
