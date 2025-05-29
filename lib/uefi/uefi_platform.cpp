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

#include "uefi_platform.h"

#include <libfdt.h>
#include <stdio.h>
#include <stdlib.h>
#include <uefi/protocols/gbl_efi_os_configuration_protocol.h>
#include <uefi/types.h>

#include "defer.h"

__WEAK EFI_STATUS efi_dt_fixup(struct EfiDtFixupProtocol *self, void *fdt,
                               size_t *buffer_size, uint32_t flags) {
  auto offset = fdt_subnode_offset(fdt, 0, "chosen");
  if (offset < 0) {
    printf("Failed to find chosen node %d\n", offset);
    return SUCCESS;
  }
  int length = 0;
  auto prop = fdt_get_property(fdt, offset, "bootargs", &length);

  if (prop == nullptr) {
    printf("Failed to find chosen/bootargs prop\n");
    return SUCCESS;
  }
  char *new_prop_data = reinterpret_cast<char *>(malloc(length));
  DEFER {
    free(new_prop_data);
    new_prop_data = nullptr;
  };
  auto prop_length = strnlen(prop->data, length);
  static constexpr auto &&to_add =
      "console=ttyAMA0 earlycon=pl011,mmio32,0x9000000 ";
  memset(new_prop_data, 0, length);
  memcpy(new_prop_data, to_add, sizeof(to_add) - 1);
  memcpy(new_prop_data + sizeof(to_add) - 1, prop->data, prop_length);
  auto ret = fdt_setprop(fdt, offset, "bootargs", new_prop_data, length);

  printf("chosen/bootargs: %d %d \"%s\"\n", ret, length, new_prop_data);

  return SUCCESS;
}

// Generates fixups for the kernel command line built by GBL.
__WEAK EfiStatus fixup_kernel_commandline(
    struct GblEfiOsConfigurationProtocol *self, const char *command_line,
    char *fixup, size_t *fixup_buffer_size) {
  printf("%s(%p, \"%s\")\n", __FUNCTION__, self, command_line);
  *fixup_buffer_size = 0;
  return SUCCESS;
}

// Generates fixups for the bootconfig built by GBL.
__WEAK EfiStatus fixup_bootconfig(struct GblEfiOsConfigurationProtocol *self,
                                  const char *bootconfig, size_t size,
                                  char *fixup, size_t *fixup_buffer_size) {
  printf("%s(%p, %s, %lu, %lu)\n", __FUNCTION__, self, bootconfig, size,
         *fixup_buffer_size);
  constexpr auto &&to_add =
      "\nandroidboot.fstab_suffix=cf.f2fs."
      "hctr2\nandroidboot.boot_devices=4010000000.pcie";
  const auto final_len = sizeof(to_add);
  if (final_len > *fixup_buffer_size) {
    *fixup_buffer_size = final_len;
    return OUT_OF_RESOURCES;
  }
  *fixup_buffer_size = final_len;
  memcpy(fixup, to_add, final_len);

  return SUCCESS;
}

// Selects which device trees and overlays to use from those loaded by GBL.
__WEAK EfiStatus select_device_trees(struct GblEfiOsConfigurationProtocol *self,
                                     GblEfiVerifiedDeviceTree *device_trees,
                                     size_t num_device_trees) {
  printf("%s(%p, %p %lu)\n", __FUNCTION__, self, device_trees,
         num_device_trees);
  return UNSUPPORTED;
}

__WEAK EfiStatus exit_boot_services(EfiHandle image_handle, size_t map_key) {
  printf("%s is called\n", __FUNCTION__);
  return SUCCESS;
}
