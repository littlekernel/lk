/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "types.h"

#ifndef __BLOCK_IO_PROTOCOL_H__
#define __BLOCK_IO_PROTOCOL_H__

typedef struct EfiBlockIoMedia EfiBlockIoMedia;
typedef struct EfiBlockIoProtocol EfiBlockIoProtocol;

struct EfiBlockIoProtocol {
  uint64_t revision;
  EfiBlockIoMedia* media;
  EfiStatus (*reset)(EfiBlockIoProtocol* self, bool extended_verification);
  EfiStatus (*read_blocks)(EfiBlockIoProtocol* self, uint32_t media_id, uint64_t lba,
                           size_t buffer_size, void* buffer);
  EfiStatus (*write_blocks)(EfiBlockIoProtocol* self, uint32_t media_id, uint64_t lba,
                            size_t buffer_size, const void* buffer);
  EfiStatus (*flush_blocks)(EfiBlockIoProtocol* self);
};

struct EfiBlockIoMedia {
  // present in rev1
  uint32_t media_id;
  bool removable_media;
  bool media_present;
  bool logical_partition;
  bool read_only;
  bool write_caching;
  uint32_t block_size;
  uint32_t io_align;
  uint64_t last_block;

  // present in rev2
  uint64_t lowest_aligned_lba;
  uint32_t logical_blocks_per_physical_block;

  // present in rev3
  uint32_t optimal_transfer_length_granularity;
};

#endif  //__BLOCK_IO_PROTOCOL_H__
