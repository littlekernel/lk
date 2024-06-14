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

#ifndef __EFI_H__
#define __EFI_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "boot_service.h"
#include "protocols/android_boot_protocol.h"
#include "protocols/block_io_protocol.h"
#include "protocols/device_path_protocol.h"
#include "protocols/loaded_image_protocol.h"
#include "protocols/riscv_efi_boot_protocol.h"
#include "protocols/simple_network_protocol.h"
#include "protocols/simple_text_input_protocol.h"
#include "protocols/simple_text_output_protocol.h"
#include "system_table.h"
#include "types.h"

#endif  // __EFI_H__
