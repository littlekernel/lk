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

#include <arch/arm64.h>
#include <lib/bio.h>
#include <lib/watchdog.h>
#include <libfdt.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <uefi/protocols/erase_block_protocol.h>
#include <uefi/protocols/gbl_efi_image_loading_protocol.h>
#include <uefi/protocols/gbl_efi_os_configuration_protocol.h>
#include <uefi/types.h>

#include "defer.h"

#define LOCAL_TRACE 0

__WEAK EFI_STATUS efi_dt_fixup(struct EfiDtFixupProtocol* self, void* fdt,
                               size_t* buffer_size, uint32_t flags) {
  auto offset = fdt_subnode_offset(fdt, 0, "chosen");
  if (offset < 0) {
    printf("Failed to find chosen node %d\n", offset);
    return EFI_STATUS_SUCCESS;
  }
  int length = 0;
  auto prop = fdt_get_property(fdt, offset, "bootargs", &length);

  if (prop == nullptr) {
    printf("Failed to find chosen/bootargs prop\n");
    return EFI_STATUS_SUCCESS;
  }
  char* new_prop_data = reinterpret_cast<char*>(malloc(length));
  DEFER {
    free(new_prop_data);
    new_prop_data = nullptr;
  };
  auto prop_length = strnlen(prop->data, length);
  static constexpr auto&& to_add =
      "console=ttyAMA0 earlycon=pl011,mmio32,0x9000000 ";
  memset(new_prop_data, 0, length);
  memcpy(new_prop_data, to_add, sizeof(to_add) - 1);
  memcpy(new_prop_data + sizeof(to_add) - 1, prop->data, prop_length);
  auto ret = fdt_setprop(fdt, offset, "bootargs", new_prop_data, length);

  printf("chosen/bootargs: %d %d \"%s\"\n", ret, length, new_prop_data);

  return EFI_STATUS_SUCCESS;
}

// Generates fixups for the bootconfig built by GBL.
__WEAK EfiStatus fixup_bootconfig(struct GblEfiOsConfigurationProtocol* self,
                                  const uint8_t* bootconfig, size_t size,
                                  uint8_t* fixup, size_t* fixup_buffer_size) {
  printf("%s(%p, %s, %lu, %lu)\n", __FUNCTION__, self,
         reinterpret_cast<const char*>(bootconfig), size, *fixup_buffer_size);
  constexpr auto&& to_add =
      "\nandroidboot.fstab_suffix=cf.f2fs."
      "hctr2\nandroidboot.boot_devices=4010000000.pcie";
  const auto final_len = sizeof(to_add);
  if (final_len > *fixup_buffer_size) {
    *fixup_buffer_size = final_len;
    return EFI_STATUS_OUT_OF_RESOURCES;
  }
  *fixup_buffer_size = final_len;
  memcpy(fixup, to_add, final_len);

  return EFI_STATUS_SUCCESS;
}

// Selects which device trees and overlays to use from those loaded by GBL.
__WEAK EfiStatus select_device_trees(struct GblEfiOsConfigurationProtocol* self,
                                     GblEfiVerifiedDeviceTree* device_trees,
                                     size_t num_device_trees) {
  printf("%s(%p, %p %lu)\n", __FUNCTION__, self, device_trees,
         num_device_trees);
  return EFI_STATUS_UNSUPPORTED;
}

__WEAK EfiStatus exit_boot_services(EfiHandle image_handle, size_t map_key) {
  printf("%s is called\n", __FUNCTION__);
  return EFI_STATUS_SUCCESS;
}

__WEAK EfiStatus platform_setup_system_table(EfiSystemTable* table) {
  printf("%s is called\n", __FUNCTION__);
  return EFI_STATUS_SUCCESS;
}

__WEAK uint64_t get_timestamp() { return ARM64_READ_SYSREG(cntpct_el0); }

__WEAK EfiStatus get_timestamp_properties(EfiTimestampProperties* properties) {
  if (properties == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  properties->frequency = ARM64_READ_SYSREG(cntfrq_el0) & 0xFFFFFFFF;
  properties->end_value = UINT64_MAX;
  return EFI_STATUS_SUCCESS;
}

__BEGIN_CDECLS

// Redeclaring these as WEAK has two effects:
//  1. Since we are also including lib/watchdog.h, the compiler would ensure
//     that the declarations had matching prototypes.
//  2. If the platform did not provide these methods, then these symbols would
//     resolve to NULL, and can be checked at runtime.
extern __WEAK status_t platform_watchdog_init(
    lk_time_t target_timeout, lk_time_t* recommended_pet_period);
extern __WEAK void platform_watchdog_set_enabled(bool enabled);

__END_CDECLS

__WEAK EfiStatus set_watchdog_timer(size_t timeout, uint64_t watchdog_code,
                                    size_t data_size, uint16_t* watchdog_data) {
  if (platform_watchdog_init == nullptr ||
      platform_watchdog_set_enabled == nullptr) {
    TRACEF(
        "unimplemented: platform_watchdog_init = %p "
        "platform_watchdog_set_enabled = %p\n",
        platform_watchdog_init, platform_watchdog_set_enabled);
    return EFI_STATUS_UNSUPPORTED;
  }
  if (timeout != 0) {
    lk_time_t ignored = 0;
    status_t ret = platform_watchdog_init(timeout * 1000, &ignored);
    LTRACEF("platform_watchdog_init() ret=%d\n", ret);
    if (ret == ERR_INVALID_ARGS) {
      return EFI_STATUS_INVALID_PARAMETER;
    } else if (ret != NO_ERROR) {
      return EFI_STATUS_UNSUPPORTED;
    }
    platform_watchdog_set_enabled(true);
    LTRACEF("enabled hw watchdog\n");
  } else {
    platform_watchdog_set_enabled(false);
    LTRACEF("disabled hw watchdog\n");
  }
  return EFI_STATUS_SUCCESS;
}

namespace {

struct EfiEraseBlockInterface {
  EfiEraseBlockProtocol protocol;
  bdev_t* dev;
};

EfiStatus erase_blocks(EfiEraseBlockProtocol* self, uint32_t media_id,
                       uint64_t lba, EfiEraseBlockToken* token, size_t size) {
  LTRACEF("media_id=%d, lba=0x%llx, token=%p, size=0x%zx\n", media_id, lba,
          token, size);
  bdev_t* dev = reinterpret_cast<EfiEraseBlockInterface*>(self)->dev;
  if (dev == NULL) {
    LTRACEF("block dev is NULL\n");
    return EFI_STATUS_NO_MEDIA;
  }
  if (token->event != nullptr) {
    // TODO: Implement async IO
    LTRACEF("async erase not supported, token->event=%p\n", token->event);
    return EFI_STATUS_DEVICE_ERROR;
  }

  if (lba >= dev->block_count) {
    LTRACEF("OOB erase lba=%llu block_count=%u\n", lba, dev->block_count);
    return EFI_STATUS_INVALID_PARAMETER;
  }

  ssize_t ret = bio_erase(dev, lba * dev->block_size, size);
  if (ret < 0) {
    LTRACEF("bio_erase failed ret=%zd\n", ret);
    return EFI_STATUS_DEVICE_ERROR;
  }
  return EFI_STATUS_SUCCESS;
}

}  // namespace

__WEAK EfiStatus open_efi_erase_block_protocol(EfiHandle handle, void** intf) {
  auto* device_name = static_cast<const char*>(handle);
  LTRACEF("handle=%p (%s)\n", handle, device_name);
  auto* p = reinterpret_cast<EfiEraseBlockInterface*>(
      uefi_malloc(sizeof(EfiEraseBlockInterface)));
  if (p == nullptr) {
    return EFI_STATUS_OUT_OF_RESOURCES;
  }
  memset(p, 0, sizeof(*p));
  p->dev = bio_open(device_name);
  p->protocol = {
      .revision = EFI_ERASE_BLOCK_PROTOCOL_REVISION,
      .erase_length_granularity = 1,  // Erase block size == 1 filesystem block
      .erase_blocks = erase_blocks,
  };
  *intf = p;
  return EFI_STATUS_SUCCESS;
}
