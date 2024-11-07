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

#ifndef __RISCV_EFI_BOOT_PROTOCOL_H__
#define __RISCV_EFI_BOOT_PROTOCOL_H__

#include <stddef.h>

#include "types.h"

// Source: https://github.com/riscv-non-isa/riscv-uefi
struct EfiRiscvBootProtocol {
  uint64_t revision;

  EfiStatus (*get_boot_hartid)(struct EfiRiscvBootProtocol* self, size_t* boot_hartid);
};

#endif  // __RISCV_EFI_BOOT_PROTOCOL_H__
