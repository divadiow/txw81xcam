// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This header file defines macros for declaring attributes for functions,
// types, and variables.
//
// These macros are used within Abseil and allow the compiler to optimize, where
// applicable, certain function calls.
//
// This file is used for both C and C++!
//
// Most macros here are exposing GCC or Clang features, and are stubbed out for
// other compilers.
//
// GCC attributes documentation:
//   https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Function-Attributes.html
//   https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Variable-Attributes.html
//   https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Type-Attributes.html
//
// Most attributes in this file are already supported by GCC 4.7. However, some
// of them are not supported in older version of Clang. Thus, we check
// `__has_attribute()` first. If the check fails, we check if we are on GCC and
// assume the attribute exists on GCC (which is verified on GCC 4.7).
//
// -----------------------------------------------------------------------------
// Sanitizer Attributes
// -----------------------------------------------------------------------------
//
// Sanitizer-related attributes are not "defined" in this file (and indeed
// are not defined as such in any file). To utilize the following
// sanitizer-related attributes within your builds, define the following macros
// within your build using a `-D` flag, along with the given value for
// `-fsanitize`:
//
//   * `ADDRESS_SANITIZER` + `-fsanitize=address` (Clang, GCC 4.8)
//   * `MEMORY_SANITIZER` + `-fsanitize=memory` (Clang-only)
//   * `THREAD_SANITIZER + `-fsanitize=thread` (Clang, GCC 4.8+)
//   * `UNDEFINED_BEHAVIOR_SANITIZER` + `-fsanitize=undefined` (Clang, GCC 4.9+)
//   * `CONTROL_FLOW_INTEGRITY` + -fsanitize=cfi (Clang-only)
//
// Example:
//
//   // Enable branches in the Abseil code that are tagged for ASan:
//   $ bazel -D ADDRESS_SANITIZER -fsanitize=address *target*
//
// Since these macro names are only supported by GCC and Clang, we only check
// for `__GNUC__` (GCC or Clang) and the above macros.
#ifndef ABSL_BASE_ATTRIBUTES_H_
#define ABSL_BASE_ATTRIBUTES_H_

#define ABSL_HAVE_ATTRIBUTE(x) 0

#define ABSL_HAVE_CPP_ATTRIBUTE(x) 0

#define ABSL_PRINTF_ATTRIBUTE(string_index, first_to_check)
#define ABSL_SCANF_ATTRIBUTE(string_index, first_to_check)

#define ABSL_ATTRIBUTE_ALWAYS_INLINE

#define ABSL_ATTRIBUTE_NOINLINE

#define ABSL_ATTRIBUTE_NO_TAIL_CALL
#define ABSL_HAVE_ATTRIBUTE_NO_TAIL_CALL 0

#define ABSL_ATTRIBUTE_WEAK
#define ABSL_HAVE_ATTRIBUTE_WEAK 0

#define ABSL_ATTRIBUTE_NONNULL(...)

#define ABSL_ATTRIBUTE_NORETURN

#define ABSL_ATTRIBUTE_NO_SANITIZE_ADDRESS

#define ABSL_ATTRIBUTE_NO_SANITIZE_MEMORY

#define ABSL_ATTRIBUTE_NO_SANITIZE_THREAD

#define ABSL_ATTRIBUTE_NO_SANITIZE_UNDEFINED

#define ABSL_ATTRIBUTE_NO_SANITIZE_CFI

#define ABSL_ATTRIBUTE_RETURNS_NONNULL

#define ABSL_HAVE_ATTRIBUTE_SECTION 0

// provide dummy definitions
#define ABSL_ATTRIBUTE_SECTION(name)
#define ABSL_ATTRIBUTE_SECTION_VARIABLE(name)
#define ABSL_INIT_ATTRIBUTE_SECTION_VARS(name)
#define ABSL_DEFINE_ATTRIBUTE_SECTION_VARS(name)
#define ABSL_DECLARE_ATTRIBUTE_SECTION_VARS(name)
#define ABSL_ATTRIBUTE_SECTION_START(name) (reinterpret_cast<void *>(0))
#define ABSL_ATTRIBUTE_SECTION_STOP(name) (reinterpret_cast<void *>(0))

#define ABSL_ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC
#define ABSL_REQUIRE_STACK_ALIGN_TRAMPOLINE (0)

#define ABSL_MUST_USE_RESULT

#define ABSL_ATTRIBUTE_HOT

#define ABSL_ATTRIBUTE_COLD

#define ABSL_XRAY_ALWAYS_INSTRUMENT
#define ABSL_XRAY_NEVER_INSTRUMENT
#define ABSL_XRAY_LOG_ARGS(N)

#define ABSL_ATTRIBUTE_UNUSED

#define ABSL_ATTRIBUTE_INITIAL_EXEC

#define ABSL_ATTRIBUTE_PACKED

#define ABSL_CONST_INIT

#endif  // ABSL_BASE_ATTRIBUTES_H_
