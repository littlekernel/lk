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

#include "charset.h"

size_t utf16_strlen(const char16_t *str) {
  size_t len = 0;

  for (; *str; str++) {
    len++;
  }

  return len;
}

int utf16_strcmp(const char16_t *s1, const char16_t *s2) {
  int ret = 0;

  for (; ; s1++, s2++) {
    ret = *s1 - *s2;
    if (ret || !*s1)
      break;
  }

  return ret;
}


/**
 * @brief convert utf-8 string to utf-16 string.
 *
 * This function converts utf-8 string to utf-16 string. However
 * it only supports us-ascii input right now.
 */
int utf8_to_utf16(char16_t *dest, const char *src, size_t size) {
  size_t i = 0;
  for (; i < size - 1 && *src; i++, src++) {
    dest[i] = *src;
  }

  dest[i] = 0;
  return 0;
}

/**
 * @brief convert utf-16 string to utf-8 string.
 *
 * This function converts utf-16 string to utf-8 string. However
 * it only supports us-ascii output right now.
 */
int utf16_to_utf8(char *dest, const char16_t *src, size_t size) {
  size_t i = 0;
  for (; i < size - 1 && *src; i++, src++) {
    dest[i] = *src;
  }

  dest[i] = 0;
  return 0;
}
