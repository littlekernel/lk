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

#ifndef __RANDOM_NUMBER_GENERATOR_PROTOCOL__
#define __RANDOM_NUMBER_GENERATOR_PROTOCOL__

#include <uefi/gbl_protocol_utils.h>
#include <uefi/types.h>

EFI_GUID(EFI_RNG_PROTOCOL_GUID, 0x3152bca5, 0xeade, 0x433d, 0x86, 0x2e, 0xc0,
         0x1c, 0xdc, 0x29, 0x1f, 0x44);

EFI_GUID(EFI_RNG_ALGORITHM_RAW_GUID, 0xe43176d7, 0xb6e8, 0x4827, 0xb7, 0x84,
         0x7f, 0xfd, 0xc4, 0xb6, 0x85, 0x61);

typedef EfiGuid EfiRngAlgorithm;
typedef struct EfiRngProtocol EfiRngProtocol;

struct EfiRngProtocol {
  EfiStatus (*get_info)(EfiRngProtocol *self, size_t *rng_algorithm_list_size,
                        EfiRngAlgorithm *rng_algorithm_list);
  EfiStatus (*get_rng)(EfiRngProtocol *self,
                       const EfiRngAlgorithm *rng_algorithm,
                       size_t rng_value_length, uint8_t *rng_value);
};

#endif  // __RANDOM_NUMBER_GENERATOR_PROTOCOL__
