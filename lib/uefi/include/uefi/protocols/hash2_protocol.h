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

#include <uefi/gbl_protocol_utils.h>
#include <uefi/types.h>

EFI_GUID(EFI_HASH2_PROTOCOL_GUID, 0x55b1d734, 0xc5e1, 0x49db, 0x96, 0x47, 0xb1,
         0x6a, 0xfb, 0x0e, 0x30, 0x5b);

EFI_GUID(EFI_HASH2_SERVICE_BINDING_PROTOCOL_GUID, 0xda836f8d, 0x217f, 0x4ca0,
         0x99, 0xc2, 0x1c, 0xa4, 0xe1, 0x60, 0x77, 0xea);

EFI_GUID(EFI_HASH_ALGORITHM_SHA1_GUID, 0x2ae9d80f, 0x3fb2, 0x4095, 0xb7, 0xb1,
         0xe9, 0x31, 0x57, 0xb9, 0x46, 0xb6);

EFI_GUID(EFI_HASH_ALGORITHM_SHA256_GUID, 0x51aa59de, 0xfdf2, 0x4ea3, 0xbc, 0x63,
         0x87, 0x5f, 0xb7, 0x84, 0x2e, 0xe9);

EFI_GUID(EFI_HASH_ALGORITHM_SHA512_GUID, 0xcaa4381e, 0x750c, 0x4770, 0xb8, 0x70,
         0x7a, 0x23, 0xb4, 0xe4, 0x21, 0x30);

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
