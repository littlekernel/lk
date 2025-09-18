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

#ifndef __LIB_UEFI_MEMORY_PROTOCOLS_H
#define __LIB_UEFI_MEMORY_PROTOCOLS_H

#include <kernel/vm.h>
#include <stddef.h>
#include <uefi/boot_service.h>
#include <uefi/protocols/gbl_efi_boot_memory_protocol.h>
#include <uefi/types.h>

vmm_aspace_t *set_boot_aspace();
void setup_heap();
void reset_heap();

EfiStatus allocate_pages(EfiAllocatorType type, EfiMemoryType memory_type,
                         size_t pages, EfiPhysicalAddr *memory);

// Declaration moved to uefi_platform.h to keep all weak functions declared in
// one place.
// void *uefi_malloc(size_t size);
// EfiStatus allocate_pool(EfiMemoryType pool_type, size_t size, void **buf);
// EfiStatus free_pool(void *mem);
// EfiStatus free_pages(void *memory, size_t pages);

EfiStatus get_physical_memory_map(size_t *memory_map_size,
                                  EfiMemoryDescriptor *memory_map,
                                  size_t *map_key, size_t *desc_size,
                                  uint32_t *desc_version);

#endif
