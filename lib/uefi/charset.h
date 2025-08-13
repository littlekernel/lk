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
 */

#ifndef __CHARSET_
#define __CHARSET_

#include <uefi/types.h>

size_t utf16_strlen(const char16_t *str);
int utf16_strcmp(const char16_t *s1, const char16_t *s2);
int utf8_to_utf16(char16_t *dest, const char *src, size_t size);
int utf16_to_utf8(char *dest, const char16_t *src, size_t size);

#endif
