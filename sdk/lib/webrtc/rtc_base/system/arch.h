/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains platform-specific typedefs and defines.
// Much of it is derived from Chromium's build/build_config.h.

#ifndef RTC_BASE_SYSTEM_ARCH_H_
#define RTC_BASE_SYSTEM_ARCH_H_

#ifndef WEBRTC_ARCH_BIG_ENDIAN
#define WEBRTC_ARCH_BIG_ENDIAN 
#endif
// Processor architecture detection.  For more info on what's defined, see:
//   https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
//   https://www.agner.org/optimize/calling_conventions.pdf
//   https://sourceforge.net/p/predef/wiki/Architectures/
//   or with gcc, run: "echo | gcc -E -dM -"

#if !(defined(WEBRTC_ARCH_LITTLE_ENDIAN) ^ defined(WEBRTC_ARCH_BIG_ENDIAN))
#error Define either WEBRTC_ARCH_LITTLE_ENDIAN or WEBRTC_ARCH_BIG_ENDIAN
#endif

#endif  // RTC_BASE_SYSTEM_ARCH_H_
