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
#include "blockio_protocols.h"

#include <kernel/vm.h>
#include <lib/bio.h>
#include <string.h>
#include <uefi/protocols/block_io_protocol.h>
#include <uefi/types.h>

#include "io_stack.h"
#include "switch_stack.h"
#include "uefi_platform.h"

namespace {

struct EfiBlockIoInterface {
  EfiBlockIoProtocol protocol;
  void *dev;
  EfiBlockIoMedia media;
  void *io_stack;
};

EfiStatus read_blocks(EfiBlockIoProtocol *self, uint32_t media_id, uint64_t lba,
                      size_t buffer_size, void *buffer) {
  auto interface = reinterpret_cast<EfiBlockIoInterface *>(self);
  auto dev = reinterpret_cast<bdev_t *>(interface->dev);
  if (lba >= dev->block_count) {
    printf("OOB read %s %llu %u\n", dev->name, lba, dev->block_count);
    return EFI_STATUS_END_OF_MEDIA;
  }
  if (interface->io_stack == nullptr) {
    printf("No IO stack allocted.\n");
    return EFI_STATUS_OUT_OF_RESOURCES;
  }

  const size_t bytes_read =
      call_with_stack(interface->io_stack, bio_read_block, dev, buffer, lba,
                      buffer_size / dev->block_size);
  if (bytes_read != buffer_size) {
    printf("Failed to read %ld bytes from %s\n", buffer_size, dev->name);
    return EFI_STATUS_DEVICE_ERROR;
  }
  return EFI_STATUS_SUCCESS;
}

EfiStatus write_blocks(EfiBlockIoProtocol *self, uint32_t media_id,
                       uint64_t lba, size_t buffer_size, void *buffer) {
  printf("%s is called\n", __FUNCTION__);
  return EFI_STATUS_SUCCESS;
}

EfiStatus flush_blocks(EfiBlockIoProtocol *self) {
  printf("%s is called\n", __FUNCTION__);
  return EFI_STATUS_SUCCESS;
}

EfiStatus reset(EfiBlockIoProtocol *self, bool extended_verification) {
  printf("%s is called\n", __FUNCTION__);
  return EFI_STATUS_UNSUPPORTED;
}
}  // namespace

__WEAK EfiStatus open_block_device(EfiHandle handle, void** intf) {
  printf("%s(%p)\n", __FUNCTION__, handle);
  auto io_stack = get_io_stack();
  if (io_stack == nullptr) {
    return EFI_STATUS_OUT_OF_RESOURCES;
  }
  const auto interface = reinterpret_cast<EfiBlockIoInterface *>(
      uefi_malloc(sizeof(EfiBlockIoInterface)));
  if (interface == nullptr) {
    return EFI_STATUS_OUT_OF_RESOURCES;
  }
  memset(interface, 0, sizeof(EfiBlockIoInterface));
  auto dev = bio_open(reinterpret_cast<const char *>(handle));
  interface->dev = dev;
  interface->protocol.revision = EFI_BLOCK_IO_PROTOCOL_REVISION;
  interface->protocol.reset = reset;
  interface->protocol.read_blocks = read_blocks;
  interface->protocol.write_blocks = write_blocks;
  interface->protocol.flush_blocks = flush_blocks;
  interface->protocol.media = &interface->media;
  interface->media.block_size = dev->block_size;
  interface->media.io_align = interface->media.block_size;
  interface->media.last_block = dev->block_count - 1;
  interface->io_stack = reinterpret_cast<char *>(io_stack) + kIoStackSize;
  *intf = interface;
  return EFI_STATUS_SUCCESS;
}

EfiStatus list_block_devices(size_t *num_handles, EfiHandle **buf) {
  size_t device_count = 0;
  bio_iter_devices([&device_count](bdev_t *dev) {
    device_count++;
    return true;
  });
  auto devices =
      reinterpret_cast<char **>(uefi_malloc(sizeof(char *) * device_count));
  size_t i = 0;
  bio_iter_devices([&i, devices, device_count](bdev_t *dev) {
    devices[i] = dev->name;
    i++;
    return i < device_count;
  });
  *num_handles = i;
  *buf = reinterpret_cast<EfiHandle *>(devices);
  return EFI_STATUS_SUCCESS;
}
