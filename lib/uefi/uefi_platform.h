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

#ifndef __GBL_OS_CONFIGURATION_
#define __GBL_OS_CONFIGURATION_

#include <arch/defines.h>
#include <uefi/protocols/gbl_efi_boot_memory_protocol.h>
#include <uefi/protocols/gbl_efi_image_loading_protocol.h>
#include <uefi/protocols/gbl_efi_os_configuration_protocol.h>
#include <uefi/protocols/timestamp.h>
#include <uefi/system_table.h>
#include <uefi/types.h>

// Functions which should be implemented by individual platforms.
// The UEFI library provides a default no-op implementation that
// is weakly linked.

EFI_STATUS efi_dt_fixup(struct EfiDtFixupProtocol *self, void *fdt,
                        size_t *buffer_size, uint32_t flags);

EfiStatus fixup_bootconfig(struct GblEfiOsConfigurationProtocol* self,
                           const uint8_t* bootconfig, size_t size,
                           uint8_t* fixup, size_t* fixup_buffer_size);

EfiStatus select_device_trees(struct GblEfiOsConfigurationProtocol *self,
                              GblEfiVerifiedDeviceTree *device_trees,
                              size_t num_device_trees);
EfiStatus exit_boot_services(EfiHandle image_handle, size_t map_key);

EfiStatus platform_setup_system_table(EfiSystemTable *table);

uint64_t get_timestamp();

EfiStatus get_timestamp_properties(EfiTimestampProperties *properties);

// timeout unit is in seconds.
EfiStatus set_watchdog_timer(size_t timeout, uint64_t watchdog_code,
                             size_t data_size, uint16_t* watchdog_data);

// alloc_page/free_pages is implemented in memory_protocols.cpp
void *alloc_page(void *addr, size_t size, size_t align_log2 = PAGE_SIZE_SHIFT);

void *alloc_page(size_t size, size_t align_log2 = PAGE_SIZE_SHIFT);

EfiStatus free_pages(void *memory, size_t pages);

EfiStatus get_buffer(struct GblEfiImageLoadingProtocol *self,
                     const GblEfiImageInfo *ImageInfo,
                     GblEfiImageBuffer *Buffer);

// uefi_malloc is used by LK to allocate memory that would be used by UEFI
// applications
void *uefi_malloc(size_t size);

// Used by UEFI application to allocate heap memory.
EfiStatus allocate_pool(EfiMemoryType pool_type, size_t size, void **buf);
EfiStatus free_pool(void *mem);

// Called by LK once before executing UEFI application to setup the heap
void setup_heap();
// Caled by LK once after executing UEFI application to tear down the heap
void reset_heap();

EfiStatus open_efi_erase_block_protocol(EfiHandle handle, void** intf);

GblEfiBootMemoryProtocol* open_boot_memory_protocol();

#endif
