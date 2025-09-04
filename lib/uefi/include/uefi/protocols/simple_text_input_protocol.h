/*
 * Copyright (C) 2024 The Android Open Source Project
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

#ifndef __SIMPLE_TEXT_INPUT_PROTOCOL_H__
#define __SIMPLE_TEXT_INPUT_PROTOCOL_H__

#include "types.h"

typedef struct EfiInputKey {
  uint16_t scan_code;
  uint16_t unicode_char;
} EfiInputKey;

typedef struct EfiSimpleTextInputProtocol {
  EfiStatus (*reset)(struct EfiSimpleTextInputProtocol* self,
                     bool extendend_verification);

  EfiStatus (*read_key_stroke)(struct EfiSimpleTextInputProtocol* self,
                               EfiInputKey* key);

  EfiEvent wait_for_key;
} EfiSimpleTextInputProtocol;

#endif  // __SIMPLE_TEXT_INPUT_PROTOCOL_H__
