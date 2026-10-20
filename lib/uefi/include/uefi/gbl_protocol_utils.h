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

// Defines a UEFI GUID constant in a way that is usable for both C/C++ and Rust.
//
// Bindgen doesn't know what to do with C struct initializers e.g. {0xABCD,
// 0xEF}, so if we define GUIDs like that then we have to manually re-define
// them in Rust which can easily get out of sync. Instead we have this macro
// which creates definitions usable for both C/C++ and Rust (though not quite
// optimal for either).
//
// Given a declaration like:
//
// ```
// EFI_GUID(FOO, 0x01234567, 0x89AB, 0xCDEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
//          0xCD, 0xEF);
// ```
//
// A C/C++ user has two options:
//  1. Use the `static const EfiGuid FOO` which this macro will declare
//  2. Copy-paste the raw values from the macro usage site - they should already
//     be in a usable format, most likely will just need surrounding braces
//
// For our Rust code, bindgen generates two u64 values, `FOO_U64_0` and
// `FOO_U64_1`. These values can be const-converted into a Rust GUID via
// `EfiGuid::from_u64s(FOO_U64_0, FOO_U64_1)`.
#ifdef RUST_BINDGEN
#define EFI_GUID(name, d1, d2, d3, d4_0, d4_1, d4_2, d4_3, d4_4, d4_5, d4_6,   \
                 d4_7)                                                         \
  /* Note: we use enums here as a bindgen workaround; if we use `static const` \
   * and the MSB happens to be set (which it can be for GUIDs) then bindgen    \
   * defines it as a link-time `pub static` rather than compile-time           \
   * `pub const` which means we couldn't use it in const definitions.          \
   * Bindgen enums do not have this behavior, and we can tag these as          \
   * `--constified-enum` so they end up being standalone `pub const` in Rust   \
   * anyway but without the MSB weirdness. */                                  \
  EFI_ENUM(name##_U64_ENUM, uint64_t,                                          \
           name##_U64_0 =                                                      \
               ((uint64_t)(d1) << 32 | (uint64_t)(d2) << 16 | (uint64_t)(d3)), \
           name##_U64_1 = ((uint64_t)(d4_0) << 56 | (uint64_t)(d4_1) << 48 |   \
                           (uint64_t)(d4_2) << 40 | (uint64_t)(d4_3) << 32 |   \
                           (uint64_t)(d4_4) << 24 | (uint64_t)(d4_5) << 16 |   \
                           (uint64_t)(d4_6) << 8 | (uint64_t)(d4_7)));
#else
#define EFI_GUID(name, d1, d2, d3, d4_0, d4_1, d4_2, d4_3, d4_4, d4_5, d4_6, \
                 d4_7)                                                       \
  static const EfiGuid name = {                                              \
      d1, d2, d3, {d4_0, d4_1, d4_2, d4_3, d4_4, d4_5, d4_6, d4_7}}
#endif  // RUST_BINDGEN

#endif  // __GBL_PROTOCOL_UTILS_H__
