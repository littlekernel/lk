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
#include <uefi/protocols/gbl_efi_avb_protocol.h>
#include <uefi/protocols/gbl_efi_boot_control_protocol.h>
#include <uefi/protocols/random_number_generator_protocol.h>
#include <uefi/types.h>

#include "defer.h"

#define LOCAL_TRACE 0

__WEAK EfiStatus efi_dt_fixup(struct EfiDtFixupProtocol* self, void* fdt,
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
                                  size_t bootconfig_size,
                                  const uint8_t* bootconfig,
                                  size_t* fixup_buffer_size, uint8_t* fixup) {
  printf("%s(%p, %s, %lu, %lu)\n", __FUNCTION__, self,
         reinterpret_cast<const char*>(bootconfig), bootconfig_size,
         *fixup_buffer_size);
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

namespace {

bool select_first_dtb(size_t num_device_trees,
                      GblEfiVerifiedDeviceTree* device_trees,
                      GblEfiDeviceTreeType type) {
  for (size_t i = 0; i < num_device_trees; i++) {
    if (device_trees[i].metadata.type == type) {
      device_trees[i].selected = true;
      return true;
    }
  }
  return false;
}

}  // namespace

// Selects which device trees and overlays to use from those loaded by GBL.
__WEAK EfiStatus select_device_trees(struct GblEfiOsConfigurationProtocol* self,
                                     size_t num_device_trees,
                                     GblEfiVerifiedDeviceTree* device_trees) {
  printf("%s(%p, %p %lu)\n", __FUNCTION__, self, device_trees,
         num_device_trees);
  if (num_device_trees > 0) {
    if (device_trees == nullptr) {
      return EFI_STATUS_INVALID_PARAMETER;
    }
    for (size_t i = 0; i < num_device_trees; i++) {
      device_trees[i].selected = false;
    }
    if (!select_first_dtb(num_device_trees, device_trees,
                          GBL_EFI_DEVICE_TREE_TYPE_DEVICE_TREE)) {
      return EFI_STATUS_UNSUPPORTED;
    }
    select_first_dtb(num_device_trees, device_trees,
                     GBL_EFI_DEVICE_TREE_TYPE_OVERLAY);
  }
  return EFI_STATUS_SUCCESS;
}

__WEAK EfiStatus select_fit_configuration(
    struct GblEfiOsConfigurationProtocol* self, size_t fit_size, const uint8_t* fit,
    size_t metadata_size, const uint8_t* metadata, size_t* selected_configuration_offset) {
  printf("%s(%p, %lu, %p, %lu, %p, %p)\n", __FUNCTION__, self, fit_size, fit, metadata_size,
         metadata, selected_configuration_offset);
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

EfiStatus avb_read_partition_attributes(
    struct GblEfiAvbProtocol* self,
    /* in-out */ size_t* num_partitions,
    /* in-out */ GblEfiAvbPartitionAttributes* partitions) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_read_device_status(
    struct GblEfiAvbProtocol* self,
    /* out */ GblEfiAvbDeviceStatus* status_flags) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_validate_vbmeta_public_key(
    struct GblEfiAvbProtocol* self,
    /* in */ size_t public_key_length,
    /* in */ const uint8_t* public_key_data,
    /* in */ size_t public_key_metadata_length,
    /* in */ const uint8_t* public_key_metadata,
    /* out */ GblEfiAvbKeyValidationStatus* validation_status) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_read_rollback_index(struct GblEfiAvbProtocol* self,
                                  /* in */ size_t index_location,
                                  /* out */ uint64_t* rollback_index) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_write_rollback_index(struct GblEfiAvbProtocol* self,
                                   /* in */ size_t index_location,
                                   /* in */ uint64_t rollback_index) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_read_persistent_value(struct GblEfiAvbProtocol* self,
                                    /* in */ const EfiChar8* name,
                                    /* in-out */ size_t* value_size,
                                    /* out */ uint8_t* value) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_write_persistent_value(struct GblEfiAvbProtocol* self,
                                     /* in */ const EfiChar8* name,
                                     /* in */ size_t value_size,
                                     /* in */ const uint8_t* value) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_handle_verification_result(
    struct GblEfiAvbProtocol* self,
    /* in */ const GblEfiAvbVerificationResult* result) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_write_lock_state(struct GblEfiAvbProtocol* self,
                               /* in */ GblEfiAvbLockType type,
                               /* in */ GblEfiAvbLockState state) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus avb_factory_data_reset(struct GblEfiAvbProtocol* self) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus boot_control_get_slot_count(struct GblEfiBootControlProtocol* self,
                                      uint8_t* slot_count) {
  if (slot_count == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  *slot_count = 1;
  return EFI_STATUS_SUCCESS;
}

EfiStatus boot_control_get_slot_info(struct GblEfiBootControlProtocol* self,
                                     uint8_t index, GblEfiSlotInfo* info) {
  if (info == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  if (index > 0) {
    return EFI_STATUS_NOT_FOUND;
  }
  info->suffix = 'a';
  info->unbootable_reason = GBL_EFI_UNBOOTABLE_REASON_UNKNOWN_REASON;
  info->priority = 15;
  info->remaining_tries = 7;
  info->successful = 1;
  return EFI_STATUS_SUCCESS;
}

EfiStatus boot_control_get_current_slot(struct GblEfiBootControlProtocol* self,
                                        GblEfiSlotInfo* info) {
  return boot_control_get_slot_info(self, 0, info);
}

EfiStatus boot_control_set_active_slot(struct GblEfiBootControlProtocol* self,
                                       uint8_t index) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus boot_control_get_one_shot_boot_mode(
    struct GblEfiBootControlProtocol* self, GblEfiOneShotBootMode* mode) {
  if (mode == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  *mode = GBL_EFI_ONE_SHOT_BOOT_MODE_NONE;
  return EFI_STATUS_SUCCESS;
}

EfiStatus boot_control_handle_loaded_os(struct GblEfiBootControlProtocol* self,
                                        const GblEfiLoadedOs* os) {
  return EFI_STATUS_SUCCESS;
}

EfiStatus rng_get_info(EfiRngProtocol* self, size_t* rng_algorithm_list_size,
                       EfiRngAlgorithm* rng_algorithm_list) {
  if (rng_algorithm_list_size == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  *rng_algorithm_list_size = 0;
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus rng_get_rng(EfiRngProtocol* self, const EfiRngAlgorithm* rng_algorithm,
                      size_t rng_value_length, uint8_t* rng_value) {
  if (rng_value == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  return EFI_STATUS_UNSUPPORTED;
}

}  // namespace

__WEAK EfiStatus open_efi_erase_block_protocol(EfiHandle handle, const void** intf) {
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

__WEAK GblEfiAvbProtocol* open_gbl_efi_avb_protocol() {
  static GblEfiAvbProtocol protocol = {
      .revision = GBL_EFI_AVB_PROTOCOL_REVISION,
      .read_partition_attributes = avb_read_partition_attributes,
      .read_device_status = avb_read_device_status,
      .validate_vbmeta_public_key = avb_validate_vbmeta_public_key,
      .read_rollback_index = avb_read_rollback_index,
      .write_rollback_index = avb_write_rollback_index,
      .read_persistent_value = avb_read_persistent_value,
      .write_persistent_value = avb_write_persistent_value,
      .handle_verification_result = avb_handle_verification_result,
      .write_lock_state = avb_write_lock_state,
      .factory_data_reset = avb_factory_data_reset,
  };
  return &protocol;
}

__WEAK GblEfiBootControlProtocol* open_gbl_efi_boot_control_protocol() {
  static GblEfiBootControlProtocol protocol = {
      .revision = GBL_EFI_BOOT_CONTROL_PROTOCOL_REVISION,
      .get_slot_count = boot_control_get_slot_count,
      .get_slot_info = boot_control_get_slot_info,
      .get_current_slot = boot_control_get_current_slot,
      .set_active_slot = boot_control_set_active_slot,
      .get_one_shot_boot_mode = boot_control_get_one_shot_boot_mode,
      .handle_loaded_os = boot_control_handle_loaded_os,
  };
  return &protocol;
}

__WEAK EfiRngProtocol* open_efi_rng_protocol() {
  static EfiRngProtocol protocol = {
      .get_info = rng_get_info,
      .get_rng = rng_get_rng,
  };
  return &protocol;
}
