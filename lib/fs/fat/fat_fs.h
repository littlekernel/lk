/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/bio.h>
#include <lib/bcache.h>
#include <lib/fs.h>
#include <kernel/mutex.h>

// computed constants about a particular mount
struct fat_info {
    uint32_t bytes_per_sector = 0;
    uint32_t sectors_per_cluster = 0;
    uint32_t bytes_per_cluster = 0;
    uint32_t reserved_sectors = 0;
    uint32_t fat_bits = 0;
    uint32_t fat_count = 0;
    uint32_t sectors_per_fat = 0;
    uint32_t total_sectors = 0;
    uint32_t active_fat = 0;
    uint32_t data_start_sector = 0;
    uint32_t total_clusters = 0;
    uint32_t root_cluster = 0;
    uint32_t root_entries = 0;
    uint32_t root_start_sector = 0;
    uint32_t root_dir_sectors = 0;
};

class fat_file;
struct dir_entry_location;

// main fs object representing a mount
class fat_fs {
public:
    // mount hook, creates a new fs instance and passes it back in fscookie
    static status_t mount(bdev_t *dev, fscookie **cookie);
    static status_t unmount(fscookie *cookie);

    bdev_t *dev() { return dev_; }
    bcache_t bcache() { return bcache_; }
    const fat_info &info() const { return info_; }

    // file list apis
    // must be called with lock held
    void add_to_file_list(fat_file *file);
    fat_file *lookup_file(const dir_entry_location &loc);

    // for now keep the lock public
    Mutex lock;

private:
    fat_fs();
    ~fat_fs();

    bdev_t *dev_ = nullptr;
    bcache_t bcache_ = nullptr;

    // list of all the open files and directories
    list_node file_list_ = LIST_INITIAL_VALUE(file_list_);

    // data computed from BPB
    fat_info info_ {};
};

enum class fat_attribute : uint8_t {
    read_only = 0x01,
    hidden = 0x02,
    system = 0x04,
    volume_id = 0x08,
    directory = 0x10,
    archive = 0x20,
    lfn = read_only | hidden | system | volume_id,
};

inline uint32_t fat_read32(const void *_buffer, size_t offset) {
    auto *buffer = (const uint8_t *)_buffer;

    return buffer[offset] +
          (buffer[offset + 1] << 8) +
          (buffer[offset + 2] << 16) +
          (buffer[offset + 3] << 24);
}

inline uint16_t fat_read16(const void *_buffer, size_t offset) {
    auto *buffer = (const uint8_t *)_buffer;

    return buffer[offset] +
          (buffer[offset + 1] << 8);
}

// In fat32, clusters between 0x0fff.fff8 and 0x0fff.ffff are interpreted as
// end of file.
const uint32_t EOF_CLUSTER_BASE = 0x0ffffff8;
const uint32_t EOF_CLUSTER = 0x0fffffff;

inline bool is_eof_cluster(uint32_t cluster) {
    return cluster >= EOF_CLUSTER_BASE && cluster <= EOF_CLUSTER;
}

const int DIR_ENTRY_LENGTH = 32;
const size_t MAX_FILE_NAME_LEN = 256;

