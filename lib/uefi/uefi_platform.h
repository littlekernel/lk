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

#include <uefi/protocols/gbl_efi_os_configuration_protocol.h>
#include <uefi/system_table.h>
#include <uefi/types.h>

// Functions which should be implemented by individual platforms.
// The UEFI library provides a default no-op implementation that
// is weakly linked.

EFI_STATUS efi_dt_fixup(struct EfiDtFixupProtocol *self, void *fdt,
                        size_t *buffer_size, uint32_t flags);

EfiStatus fixup_bootconfig(struct GblEfiOsConfigurationProtocol *self,
                           const char *bootconfig, size_t size, char *fixup,
                           size_t *fixup_buffer_size);

EfiStatus select_device_trees(struct GblEfiOsConfigurationProtocol *self,
                              GblEfiVerifiedDeviceTree *device_trees,
                              size_t num_device_trees);
EfiStatus exit_boot_services(EfiHandle image_handle, size_t map_key);

EfiStatus platform_setup_system_table(EfiSystemTable *table);

#endif