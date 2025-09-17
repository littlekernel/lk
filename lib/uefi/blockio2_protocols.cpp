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

#include "blockio2_protocols.h"

#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lib/bio.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stddef.h>
#include <stdio.h>
#include <uefi/protocols/block_io2_protocol.h>
#include <uefi/types.h>

#include "events.h"
#include "io_stack.h"
#include "memory_protocols.h"
#include "switch_stack.h"
#include "thread_utils.h"
#include "uefi_platform.h"

#define LOCAL_TRACE 0

namespace {

struct EfiBlockIo2Interface {
  EfiBlockIo2Protocol protocol;
  EfiBlockIoMedia media;
  void* dev;
};

EfiStatus reset(EfiBlockIo2Protocol* self, bool extended_verification) {
  return EFI_STATUS_UNSUPPORTED;
}

void async_read_callback(void* cookie, struct bdev* dev, ssize_t bytes_read) {
  // |cookie| might be identity mapped memory, which is in UEFI address space.
  // We need to switch to the UEFI address space to access it.
  auto aspace = set_boot_aspace();
  auto old_aspace = vmm_set_active_aspace(aspace);
  auto token = reinterpret_cast<EfiBlockIo2Token*>(cookie);
  if (bytes_read < 0) {
    token->transaction_status = EFI_STATUS_DEVICE_ERROR;
  } else {
    token->transaction_status = EFI_STATUS_SUCCESS;
  }
  signal_event(token->event);
  vmm_set_active_aspace(old_aspace);
}

// Read from dev, after I/O completes, signal token->event and set
// token->transaction_status
EfiStatus read_blocks_async(bdev_t* dev, uint64_t lba, EfiBlockIo2Token* token,
                            size_t buffer_size, void* buffer) {
  if (lba >= dev->block_count) {
    printf("OOB async read %s %llu %u\n", dev->name, lba, dev->block_count);
    return EFI_STATUS_END_OF_MEDIA;
  }
  if (token == nullptr) {
    printf("Invalid token %p\n", token);
    return EFI_STATUS_INVALID_PARAMETER;
  }
  if (dev->read_async != nullptr) {
    bio_read_async(dev, buffer, lba * dev->block_size, buffer_size,
                   async_read_callback, token);
    return EFI_STATUS_SUCCESS;
  }
  // First draft of this API will just use a background thread.
  // More efficient version can be implemented once LK's bio layer
  // supports async IO
  auto thread = thread_create_functor(
      "async_bio",
      [dev, lba, buffer_size, buffer, token]() {
        auto aspace = set_boot_aspace();
        vmm_set_active_aspace(aspace);
        auto bytes_read =
            bio_read_block(dev, buffer, lba, buffer_size / dev->block_size);
        async_read_callback(token, dev, bytes_read);
        return 0;
      },
      get_current_thread()->priority, kIoStackSize);
  if (thread == nullptr) {
    printf("Failed to create thread for IO read\n");
    return EFI_STATUS_DEVICE_ERROR;
  }

  auto err = thread_detach_and_resume(thread);
  if (err != NO_ERROR) {
    printf("Failed to resume thread for IO read %d\n", err);
    return EFI_STATUS_DEVICE_ERROR;
  }
  return EFI_STATUS_SUCCESS;
}

EfiStatus read_blocks_trampoline(EfiBlockIo2Protocol* self, uint32_t media_id,
                                 uint64_t lba, EfiBlockIo2Token* token,
                                 size_t buffer_size, void* buffer) {
  auto interface = reinterpret_cast<EfiBlockIo2Interface*>(self);
  auto dev = reinterpret_cast<bdev_t*>(interface->dev);
  void* io_stack = reinterpret_cast<char*>(get_io_stack()) + kIoStackSize;
  auto ret = call_with_stack(io_stack, read_blocks_async, dev, lba, token,
                             buffer_size, buffer);
  return static_cast<EfiStatus>(ret);
}

EfiStatus write_blocks_ex(EfiBlockIo2Protocol* self, uint32_t media_id,
                          uint64_t lba, EfiBlockIo2Token* token,
                          size_t buffer_size, const void* buffer) {
  printf(
      "Writing blocks from UEFI app is currently not supported to protect the "
      "device.\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus flush_blocks_ex(EfiBlockIo2Protocol* self, EfiBlockIo2Token* token) {
  return EFI_STATUS_SUCCESS;
}

}  // namespace

__WEAK EfiStatus open_async_block_device(EfiHandle handle, void** intf) {
  auto dev = bio_open(reinterpret_cast<const char*>(handle));
  printf("%s(%s)\n", __FUNCTION__, dev->name);
  auto interface = reinterpret_cast<EfiBlockIo2Interface*>(
      uefi_malloc(sizeof(EfiBlockIo2Interface)));
  auto protocol = &interface->protocol;
  auto media = &interface->media;
  protocol->media = media;
  protocol->reset = reset;
  protocol->read_blocks_ex = read_blocks_trampoline;
  protocol->write_blocks_ex = write_blocks_ex;
  protocol->flush_blocks_ex = flush_blocks_ex;
  media->block_size = dev->block_size;
  media->io_align = media->block_size;
  media->last_block = dev->block_count - 1;
  interface->dev = dev;
  *intf = interface;

  return EFI_STATUS_SUCCESS;
}