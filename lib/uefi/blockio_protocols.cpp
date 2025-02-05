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

#include "memory_protocols.h"
#include "switch_stack.h"
#include "types.h"
#include <kernel/vm.h>
#include <lib/bio.h>
#include <protocols/block_io_protocol.h>
#include <string.h>

EfiStatus read_blocks(EfiBlockIoProtocol *self, uint32_t media_id, uint64_t lba,
                      size_t buffer_size, void *buffer) {
  auto interface = reinterpret_cast<EfiBlockIoInterface *>(self);
  auto dev = reinterpret_cast<bdev_t *>(interface->dev);
  if (lba >= dev->block_count) {
    printf("OOB read %llu %u\n", lba, dev->block_count);
    return END_OF_MEDIA;
  }

  const size_t bytes_read =
      call_with_stack(interface->io_stack, bio_read_block, dev, buffer, lba,
                      buffer_size / dev->block_size);
  if (bytes_read != buffer_size) {
    printf("Failed to read %ld bytes from %s\n", buffer_size, dev->name);
    return DEVICE_ERROR;
  }
  return SUCCESS;
}

EfiStatus write_blocks(EfiBlockIoProtocol *self, uint32_t media_id,
                       uint64_t lba, size_t buffer_size, const void *buffer) {
  printf("%s is called\n", __FUNCTION__);
  return SUCCESS;
}

EfiStatus flush_blocks(EfiBlockIoProtocol *self) {
  printf("%s is called\n", __FUNCTION__);
  return SUCCESS;
}

EfiStatus reset(EfiBlockIoProtocol *self, bool extended_verification) {
  printf("%s is called\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus open_block_device(EfiHandle handle, void **intf) {
  static constexpr size_t kIoStackSize = 1024ul * 1024 * 64;
  static void *io_stack = nullptr;
  if (io_stack == nullptr) {
    vmm_alloc(vmm_get_kernel_aspace(), "uefi_io_stack", kIoStackSize, &io_stack,
              PAGE_SIZE_SHIFT, 0, 0);
  }
  printf("%s(%p)\n", __FUNCTION__, handle);
  const auto interface = reinterpret_cast<EfiBlockIoInterface *>(
      uefi_malloc(sizeof(EfiBlockIoInterface)));
  memset(interface, 0, sizeof(EfiBlockIoInterface));
  auto dev = bio_open(reinterpret_cast<const char *>(handle));
  interface->dev = dev;
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
  return SUCCESS;
}
