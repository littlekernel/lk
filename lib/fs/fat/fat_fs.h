/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <kernel/mutex.h>
#include <lib/bcache.h>
#include <lib/bio.h>
#include <lib/fs.h>

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

    // FAT32 FSInfo metadata
    uint32_t fsinfo_sector = 0;
    bool fsinfo_valid = false;
    uint32_t fsinfo_free_clusters = UINT32_MAX;
    uint32_t fsinfo_next_free = UINT32_MAX;
};

class fat_file;
struct dir_entry_location;

const int DIR_ENTRY_LENGTH = 32;
const size_t MAX_FILE_NAME_LEN = 256;

// main fs object representing a mount
class fat_fs {
  public:
    // mount hook, creates a new fs instance and passes it back in fscookie
    static status_t mount(bdev_t *dev, fscookie **cookie, enum fs_mount_options options);
    static status_t unmount(fscookie *cookie);
    static status_t fs_stat(fscookie *cookie, struct fs_stat *stat);
    static status_t format(bdev_t *dev, const void *args);

    bdev_t *dev() { return dev_; }
    bcache_t bcache() { return bcache_; }
    const fat_info &info() const { return info_; }
    bool is_read_only() const { return read_only_; }

    // FAT32 FSInfo helpers (no-op on FAT12/16 or invalid FSInfo)
    status_t adjust_fsinfo_free_clusters(int32_t delta);
    status_t set_fsinfo_next_free(uint32_t next_free);
    status_t write_fsinfo_locked();

    // FAT16/32 volume dirty/clean bit in FAT entry 1 (no-op on FAT12)
    status_t mark_volume_dirty_locked();
    status_t mark_volume_clean_locked();

    // count the free (zero) entries in the active FAT by walking the whole table
    status_t count_free_clusters_locked(uint32_t *out_free);

    // file list apis
    // must be called with lock held
    void add_to_file_list(fat_file *file);
    fat_file *lookup_file(const dir_entry_location &loc);

    // Scratch buffers for the directory walk machinery, guarded by lock. Kept off the
    // stack because these routines nest several frames deep on threads with small
    // kernel stacks. name_scratch() holds the candidate name read out of each directory
    // entry by fat_find_next_entry; element_scratch() holds the path element currently
    // being matched, which stays live across a nested find that fills name_scratch().
    char *name_scratch() {
        DEBUG_ASSERT(lock.is_held());
        return name_scratch_;
    }
    char *element_scratch() {
        DEBUG_ASSERT(lock.is_held());
        return element_scratch_;
    }

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
    fat_info info_{};

    // shared implementation for mark_volume_dirty/clean_locked
    status_t set_volume_clean_bit_locked(bool clean);

    char name_scratch_[MAX_FILE_NAME_LEN];
    char element_scratch_[MAX_FILE_NAME_LEN];

    bool read_only_ = false;
};

enum class fat_attribute : uint8_t {
    file = 0x0, // lack of attribute is a file
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

inline void fat_write32(void *_buffer, size_t offset, uint32_t val) {
    auto *buffer = (uint8_t *)_buffer;

    buffer[offset] = val;
    buffer[offset + 1] = val >> 8;
    buffer[offset + 2] = val >> 16;
    buffer[offset + 3] = val >> 24;
}

inline uint16_t fat_read16(const void *_buffer, size_t offset) {
    auto *buffer = (const uint8_t *)_buffer;

    return buffer[offset] +
           (buffer[offset + 1] << 8);
}

inline void fat_write16(void *_buffer, size_t offset, uint16_t val) {
    auto *buffer = (uint8_t *)_buffer;

    buffer[offset] = val;
    buffer[offset + 1] = val >> 8;
}

// In fat32, clusters between 0x0fff.fff8 and 0x0fff.ffff are interpreted as
// end of file.
const uint32_t EOF_CLUSTER_BASE = 0x0ffffff8;
const uint32_t EOF_CLUSTER = 0x0fffffff;

inline bool is_eof_cluster(uint32_t cluster) {
    return cluster >= EOF_CLUSTER_BASE && cluster <= EOF_CLUSTER;
}
