/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0 OR BSD-2-Clause-Patent
 *
 * You may choose to use or redistribute this file under
 *  (a) the Apache License, Version 2.0, or
 *  (b) the BSD 2-Clause Patent license.
 *
 * Unless you expressly elect the BSD-2-Clause-Patent terms, the Apache-2.0
 * terms apply by default.
 */

#ifndef __GBL_PROTOCOL_UTILS_H__
#define __GBL_PROTOCOL_UTILS_H__

#define GBL_PROTOCOL_MAJOR_REV(x) (((x) >> 16) & 0xFFFF)
#define GBL_PROTOCOL_MINOR_REV(x) ((x) & 0xFFFF)

#define GBL_PROTOCOL_REVISION(major, minor) \
  ((((major) & 0xFFFF) << 16) | ((minor) & 0xFFFF))

// Macro for defining enums with explicit width.
//
// It is an ergonomics and safety benefit to explicitly define
// the width of enums in the EFI interfaces defined and used by GBL.
//
// The following conventions are used for enums:
// * The enum is named using CamelCase.
// * Enum variants are defined in ALL_CAPS and are prefixed
//   with the enum name in ALL_CAPS.
// * By default enum variants start at `0` and increment.
// * If the value for the first enum variant is `0` it is omitted.
//
// e.g.
//
// EFI_ENUM(EfiMollusc, uintptr_t,
//          EFI_MOLLUSC_UNKNOWN,
//          EFI_MOLLUSC_SQUID = 1 << 0,
//          EFI_MOLLUSC_CLAM = 1 << 1,
//          EFI_MOLLUSC_WHELK = 1 << 2);
//
// If you are using C++ and your compiler does not support C++11,
// you can explicitly disable the strongly typed enum by
// defining `GBL_EFI_DISABLE_CPP_ENUMS`.
#if defined(__cplusplus) && !defined(GBL_EFI_DISABLE_CPP_ENUMS)
#define EFI_ENUM(camelname, width, ...) \
  enum class camelname : width { __VA_ARGS__ }
#else
#define EFI_ENUM(camelname, width, ...) \
  enum { __VA_ARGS__ };                 \
  typedef width camelname
#endif

#endif  // __GBL_PROTOCOL_UTILS_H__
