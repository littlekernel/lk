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

#include <kernel/mutex.h>
#include <kernel/vm.h>
#include <lib/bio.h>
#include <lk/list.h>
#include <malloc.h>
#include <string.h>
#include <uefi/protocols/block_io_protocol.h>
#include <uefi/types.h>

#include "defer.h"
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
    printf("Failed to read %zu bytes from %s\n", buffer_size, dev->name);
    return EFI_STATUS_DEVICE_ERROR;
  }
  return EFI_STATUS_SUCCESS;
}

EfiStatus write_blocks(EfiBlockIoProtocol *self, uint32_t media_id,
                       uint64_t lba, size_t buffer_size, void *buffer) {
  printf("Writing blocks from UEFI app is currently not supported to protect "
         "the device.\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus flush_blocks(EfiBlockIoProtocol *self) {
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus reset(EfiBlockIoProtocol *self, bool extended_verification) {
  printf("%s is called\n", __FUNCTION__);
  return EFI_STATUS_UNSUPPORTED;
}
}  // namespace

namespace {
// Each distinct bdev opened during the current image run is tracked once, with
// a count of how many UEFI opens are still outstanding. Reusing an already-open
// device keeps the list bounded by the number of bdevs rather than the
// cumulative number of OpenProtocol calls. The underlying bio reference is
// taken on the first open and released by close_tracked_bdev() when the last
// open closes, or by close_tracked_bdevs() at teardown.
struct TrackedBdev {
  list_node node;
  bdev_t *dev;
  size_t open_count;
};
list_node tracked_bdevs = LIST_INITIAL_VALUE(tracked_bdevs);
Mutex tracked_bdevs_mutex;

TrackedBdev *find_tracked_bdev_locked(const char *name) {
  TrackedBdev *entry;
  list_for_every_entry(&tracked_bdevs, entry, TrackedBdev, node) {
    if (strcmp(entry->dev->name, name) == 0) {
      return entry;
    }
  }
  return nullptr;
}
}  // namespace

EfiStatus open_tracked_bdev(const char *name, bdev_t **out_dev) {
  *out_dev = nullptr;
  AutoLock lock{&tracked_bdevs_mutex};
  // Reuse a device that is already open so repeated OpenProtocol calls share a
  // single bio reference and tracking node instead of growing the list.
  if (TrackedBdev *existing = find_tracked_bdev_locked(name)) {
    existing->open_count++;
    *out_dev = existing->dev;
    return EFI_STATUS_SUCCESS;
  }
  bdev_t *dev = bio_open(name);
  if (dev == nullptr) {
    return EFI_STATUS_NOT_FOUND;
  }
  bool dev_is_tracked = false;
  DEFER {
    if (!dev_is_tracked) {
      bio_close(dev);
    }
  };
  auto *node = reinterpret_cast<TrackedBdev *>(malloc(sizeof(TrackedBdev)));
  if (node == nullptr) {
    // Can't remember the reference, so don't hand out an untracked one.
    printf("%s: out of memory tracking %s\n", __FUNCTION__, name);
    return EFI_STATUS_OUT_OF_RESOURCES;
  }
  node->node = LIST_INITIAL_CLEARED_VALUE;
  node->dev = dev;
  node->open_count = 1;
  list_add_tail(&tracked_bdevs, &node->node);
  dev_is_tracked = true;
  *out_dev = dev;
  return EFI_STATUS_SUCCESS;
}

bool close_tracked_bdev(const char *name) {
  AutoLock lock{&tracked_bdevs_mutex};
  TrackedBdev *entry = find_tracked_bdev_locked(name);
  if (entry == nullptr) {
    return false;
  }

  // Release the bio reference only once the last outstanding open closes.
  if (--entry->open_count == 0) {
    list_delete(&entry->node);
    bio_close(entry->dev);
    free(entry);
  }
  return true;
}

void close_tracked_bdevs() {
  AutoLock lock{&tracked_bdevs_mutex};
  TrackedBdev *entry;
  while ((entry = list_remove_head_type(&tracked_bdevs, TrackedBdev, node)) !=
         nullptr) {
    bio_close(entry->dev);
    free(entry);
  }
}

__WEAK EfiStatus open_block_device(EfiHandle handle, const void** intf) {
  printf("%s(%p)\n", __FUNCTION__, handle);
  auto io_stack = get_io_stack();
  if (io_stack == nullptr) {
    return EFI_STATUS_OUT_OF_RESOURCES;
  }
  bdev_t *dev = nullptr;
  EfiStatus status =
      open_tracked_bdev(reinterpret_cast<const char *>(handle), &dev);
  if (status != EFI_STATUS_SUCCESS) {
    if (status == EFI_STATUS_NOT_FOUND) {
      printf("%s: no such block device\n", __FUNCTION__);
    }
    return status;
  }
  bool close_dev_on_failure = true;
  DEFER {
    if (close_dev_on_failure) {
      close_tracked_bdev(reinterpret_cast<const char *>(handle));
    }
  };
  const auto interface = reinterpret_cast<EfiBlockIoInterface *>(
      uefi_malloc(sizeof(EfiBlockIoInterface)));
  if (interface == nullptr) {
    return EFI_STATUS_OUT_OF_RESOURCES;
  }
  memset(interface, 0, sizeof(EfiBlockIoInterface));
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
  close_dev_on_failure = false;
  return EFI_STATUS_SUCCESS;
}

EfiStatus list_block_devices(size_t *num_handles, EfiHandle *buf) {
  if (num_handles == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }

  size_t device_count = 0;
  bio_iter_devices([&device_count](bdev_t *dev) {
    device_count++;
    return true;
  });

  if (device_count == 0) {
    *num_handles = 0;
    return EFI_STATUS_NOT_FOUND;
  }

  if (*num_handles < device_count) {
    *num_handles = device_count;
    return EFI_STATUS_BUFFER_TOO_SMALL;
  }

  // Allow null buffers for size queries, but require storage before writing.
  if (buf == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }

  size_t i = 0;
  bio_iter_devices([&i, buf, device_count](bdev_t *dev) {
    buf[i] = dev->name;
    i++;
    return i < device_count;
  });
  *num_handles = i;
  return EFI_STATUS_SUCCESS;
}
