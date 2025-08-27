/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains platform-specific typedefs and defines.
// Much of it is derived from Chromium's build/build_config.h.

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <stdint.h>

typedef int16_t         s16, S16;
typedef uint16_t        u16, U16;
typedef int32_t         s32, S32;
typedef uint32_t        uint, u32, U32;

#define ALIGNED(n)      __aligned(n)

#endif  // TYPEDEFS_H_
