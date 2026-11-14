/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdint.h>

// rofs: a read-only file system image, made ahead of time (scripts/mkrofs.py) and mounted over
// any lib/bio device. An image is a header, an index and the files' bytes:
//
//   0              struct rofs_header
//   index_offset   struct rofs_entry[entry_count], then the names
//   ...            the files, each starting at a multiple of the header's align
//
// The index is a tree of directories, each a run of consecutive entries sorted by name,
// bytewise as strcmp compares them. The root is the run [0, root_count); a directory entry's
// run is [offset, offset + size) in entry numbers. The runs tile the index in order: each
// directory's starts where the run of the directory entry before it ends, the first where the
// root's does, and the last ends at entry_count, so every entry outside the root is in exactly
// one run, always after its directory's entry, and the tree has no cycles and no shared parts.
// A file entry's offset and size are its bytes in the image. A name is name_len bytes at
// name_offset in the index followed by a NUL, 1 to ROFS_NAME_MAX bytes, never "." or "..",
// and holds no '/' or NUL. Every integer is little endian.
//
// The driver checks all of this when it mounts an image and refuses one that breaks any of it
// with ERR_NOT_VALID, so an image is untrusted input. On a device that is memory mapped, a
// file's bytes are reachable in place: FS_IOCTL_GET_FILE_ADDR returns their address and
// FS_IOCTL_IS_LINEAR says whether the device is mapped.

#define ROFS_MAGIC 0x53464f52u // "ROFS" as the image's first four bytes
#define ROFS_VERSION 1
#define ROFS_NAME_MAX 63 // a struct dirent name with its NUL

enum rofs_type {
    ROFS_TYPE_FILE = 1,
    ROFS_TYPE_DIR = 2,
};

struct rofs_header {
    uint32_t magic;        // ROFS_MAGIC
    uint32_t version;      // ROFS_VERSION
    uint32_t header_size;  // sizeof(struct rofs_header)
    uint32_t align;        // every file starts at a multiple of this, a power of two
    uint32_t entry_count;  // entries in the index
    uint32_t root_count;   // the root directory: entries [0, root_count)
    uint64_t index_offset; // the entries, then the names; a multiple of 8
    uint64_t index_size;   // bytes of entries and names
    uint64_t image_size;   // the whole image, at most the device's size
};

struct rofs_entry {
    uint32_t name_offset; // the name's first byte, from index_offset
    uint16_t name_len;    // bytes in the name, not counting its NUL
    uint8_t type;         // enum rofs_type
    uint8_t reserved;     // 0
    uint64_t offset;      // a file: its first byte in the image; a directory: its first entry
    uint64_t size;        // a file: its bytes; a directory: its entries
};

STATIC_ASSERT(sizeof(struct rofs_header) == 48);
STATIC_ASSERT(sizeof(struct rofs_entry) == 24);
