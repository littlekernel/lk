/*
 * Copyright (C) 2023 The Android Open Source Project
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
 */

#ifndef __SIMPLE_TEXT_OUTPUT_PROTOCOL_H__
#define __SIMPLE_TEXT_OUTPUT_PROTOCOL_H__

#include "types.h"

typedef struct {
  int32_t max_mode;
  int32_t mode;
  int32_t attribute;
  int32_t cursor_column;
  int32_t cursor_row;
  bool cursor_visible;
} SimpleTextOutputMode;

typedef struct EfiSimpleTextOutputProtocol {
  EfiStatus (*reset)(struct EfiSimpleTextOutputProtocol* self, bool extended_verification);
  EfiStatus (*output_string)(struct EfiSimpleTextOutputProtocol* self, char16_t* string);
  EfiStatus (*test_string)(struct EfiSimpleTextOutputProtocol* self, char16_t* string);
  EfiStatus (*query_mode)(struct EfiSimpleTextOutputProtocol* self, size_t mode_num, size_t* cols,
                          size_t* rows);
  EfiStatus (*set_mode)(struct EfiSimpleTextOutputProtocol* self, size_t mode_num);
  EfiStatus (*set_attribute)(struct EfiSimpleTextOutputProtocol* self, size_t attribute);
  EfiStatus (*clear_screen)(struct EfiSimpleTextOutputProtocol* self);
  EfiStatus (*set_cursor_position)(struct EfiSimpleTextOutputProtocol* self, size_t col,
                                   size_t row);
  EfiStatus (*enable_cursor)(struct EfiSimpleTextOutputProtocol* self, bool visible);
  SimpleTextOutputMode* mode;
} EfiSimpleTextOutputProtocol;

#endif  // __SIMPLE_TEXT_OUTPUT_PROTOCOL_H__
