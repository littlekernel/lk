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

#ifndef __HASH2_PROTOCOL_H__
#define __HASH2_PROTOCOL_H__

#include <uefi/types.h>

typedef struct EfiHash2Protocol EfiHash2Protocol;

typedef uint8_t EfiMd5Hash2[16];
typedef uint8_t EfiSha1Hash2[20];
typedef uint8_t EfiSha224Hash2[28];
typedef uint8_t EfiSha256Hash2[32];
typedef uint8_t EfiSha384Hash2[48];
typedef uint8_t EfiSha512Hash2[64];

typedef union {
  EfiMd5Hash2 md5_hash;
  EfiSha1Hash2 sha1_hash;
  EfiSha224Hash2 sha224_hash;
  EfiSha256Hash2 sha256_hash;
  EfiSha384Hash2 sha384_hash;
  EfiSha512Hash2 sha512_hash;
} EfiHash2Output;

struct EfiHash2Protocol {
  EfiStatus (*get_hash_size)(const EfiHash2Protocol* self,
                             EfiGuid* hash_algorithm, size_t* hash_size);
  EfiStatus (*hash)(const EfiHash2Protocol* self, const EfiGuid* hash_algorithm,
                    const uint8_t* message, size_t message_size,
                    EfiHash2Output* out);
  EfiStatus (*hash_init)(const EfiHash2Protocol* self,
                         const EfiGuid* hash_algorithm);
  EfiStatus (*hash_update)(const EfiHash2Protocol* self, const uint8_t* message,
                           size_t message_size);
  EfiStatus (*hash_final)(const EfiHash2Protocol* self, EfiHash2Output* hash);
};

#endif  // __HASH2_PROTOCOL_H__
