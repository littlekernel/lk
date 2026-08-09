/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/bio.h>
#include <lib/fs.h>
#include <lib/fs/fat.h>
#include <lk/compiler.h>
#include <lk/cpp.h>
#include <sys/types.h>

namespace fat_test {

// A FAT volume living in a heap buffer: allocate, register as a bio device,
// format, mount. The destructor unmounts, unregisters and frees, so a test can
// declare one on the stack and not worry about cleanup on an early return.
//
// create() returns ERR_NO_MEMORY when the buffer will not fit, which lets a test
// skip a geometry that is too large for the target rather than fail on it. Small
// targets are the reason this matters: a spec-legal FAT32 needs at least 65525
// clusters, which does not fit everywhere.
class ram_volume {
  public:
    ram_volume() = default;
    ~ram_volume();

    DISALLOW_COPY_ASSIGN_AND_MOVE(ram_volume);

    // `name` must be unique among live volumes; it becomes the bio device name.
    status_t create(const char *name, const char *mount_path, size_t size,
                    const fat_format_args_t &args);

    // Unmount and remount, to check that everything reached the device.
    status_t remount();

    const char *path() const { return mount_path_; }
    const char *device_name() const { return device_name_; }
    size_t size() const { return size_; }

  private:
    void destroy();

    void *mem_ = nullptr;
    size_t size_ = 0;
    bdev_t *dev_ = nullptr;
    bool mounted_ = false;
    char device_name_[32] = {};
    char mount_path_[64] = {};
};

// One row of the geometry matrix the correctness tests sweep.
struct geometry {
    const char *name;
    uint32_t fat_bits;
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    // Number of data clusters to aim for. The volume size is derived from this
    // plus a generous allowance for the FATs and the root directory.
    uint32_t target_clusters;
};

// The sweep itself. Covers both FAT type boundaries from either side and the
// non-512 byte sector sizes the driver claims to support.
extern const geometry kGeometries[];
extern const size_t kGeometryCount;

// Size a volume for a geometry, including metadata overhead.
size_t volume_size_for(const geometry &g);

// Fill args from a geometry row.
fat_format_args_t format_args_for(const geometry &g);

} // namespace fat_test
