/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_SANITIZER_H_
#define RTC_BASE_SANITIZER_H_

#include "webrtc_typedef.h"  // For size_t.

#ifndef RTC_HAS_ASAN
#define RTC_HAS_ASAN 0
#endif
#ifndef RTC_HAS_MSAN
#define RTC_HAS_MSAN 0
#endif

#ifndef RTC_NO_SANITIZE
#define RTC_NO_SANITIZE(what)
#endif

// Ask ASan to mark the memory range [ptr, ptr + element_size * num_elements)
// as being unaddressable, so that reads and writes are not allowed. ASan may
// narrow the range to the nearest alignment boundaries.
static inline void rtc_AsanPoison(const volatile void* ptr,
                                  size_t element_size,
                                  size_t num_elements) {
}

// Ask ASan to mark the memory range [ptr, ptr + element_size * num_elements)
// as being addressable, so that reads and writes are allowed. ASan may widen
// the range to the nearest alignment boundaries.
static inline void rtc_AsanUnpoison(const volatile void* ptr,
                                    size_t element_size,
                                    size_t num_elements) {
}

// Ask MSan to mark the memory range [ptr, ptr + element_size * num_elements)
// as being uninitialized.
static inline void rtc_MsanMarkUninitialized(const volatile void* ptr,
                                             size_t element_size,
                                             size_t num_elements) {
}

// Force an MSan check (if any bits in the memory range [ptr, ptr +
// element_size * num_elements) are uninitialized the call will crash with an
// MSan report).
static inline void rtc_MsanCheckInitialized(const volatile void* ptr,
                                            size_t element_size,
                                            size_t num_elements) {
}

#endif  // RTC_BASE_SANITIZER_H_
