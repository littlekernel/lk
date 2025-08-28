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
 */

#include "text_protocol.h"
#include <stdio.h>

EfiStatus output_string(struct EfiSimpleTextOutputProtocol *self,
                        uint16_t *string) {
  char buffer[512];
  size_t i = 0;
  while (string[i]) {
    size_t j = 0;
    for (j = 0; j < sizeof(buffer) - 1 && string[i + j]; j++) {
      buffer[j] = string[i + j];
    }
    i += j;
    buffer[j] = 0;

    printf("%s", reinterpret_cast<const char *>(buffer));
  }
  return EFI_STATUS_SUCCESS;
}

EfiSimpleTextOutputProtocol get_text_output_protocol() {
  EfiSimpleTextOutputProtocol console_out;
  console_out.output_string = output_string;
  return console_out;
}
