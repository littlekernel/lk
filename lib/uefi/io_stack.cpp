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

#include "io_stack.h"

#include <kernel/vm.h>
#include <stdio.h>

void *get_io_stack() {
  static void *io_stack = nullptr;
  if (io_stack == nullptr) {
    auto status = vmm_alloc(vmm_get_kernel_aspace(), "uefi_io_stack",
                            kIoStackSize, &io_stack, PAGE_SIZE_SHIFT, 0, 0);
    if (io_stack == nullptr) {
      printf("Failed to allocated IO stack of size %zu error %d\n",
             kIoStackSize, status);
      return nullptr;
    }
  }
  return io_stack;
}