// Copyright (c) 2009 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

// Block I/O (BIO) public API
//
// This header declares the LK block device abstraction. A block device (bdev_t)
// presents a linear byte-addressable storage space with a natural block size.
//
// Key points:
// - Offsets/lengths are in bytes unless explicitly stated as "block".
// - block_size/block_shift define the natural access granularity.
// - Drivers implement the function pointers in bdev_t; users access devices via
//   the bio_* helpers (bio_open/close, read/write/erase, block I/O, async I/O).
// - Synchronous calls return when I/O completes; async calls return immediately
//   and notify via a callback.
// - Many functions return ssize_t/status_t; on error they return a negative LK
//   error code (e.g. ERR_NOT_SUPPORTED, ERR_INVALID_ARGS).
// - Erase semantics (for flash-like devices) are described by geometry.
// - Device enumeration and subdevice publishing allow partition-style devices.
//
// Threading:
// - The BIO layer is thread-safe; individual drivers may have additional
//   constraints (documented by those drivers). Avoid long blocking operations
//   inside bio_iter_devices() callbacks (internal lock held).

#include <assert.h>
#include <lk/list.h>
#include <sys/types.h>

__BEGIN_CDECLS

// NOLINTBEGIN(modernize-use-using)

// Device flags for cache alignment behavior.
// BIO_FLAG_CACHE_ALIGNED_READS: reads are naturally cacheline aligned.
// BIO_FLAG_CACHE_ALIGNED_WRITES: writes are naturally cacheline aligned.
// Drivers may set these to allow callers to optimize buffer alignment.
#define BIO_FLAGS_NONE                (0 << 0)
#define BIO_FLAG_CACHE_ALIGNED_READS  (1 << 0)
#define BIO_FLAG_CACHE_ALIGNED_WRITES (1 << 1)

// Block number type.
// NOTE: 32-bit for historical reasons; may be extended to 64-bit on 64-bit
// architectures in the future.
typedef uint32_t bnum_t;

// Erase geometry describes regions and erase-unit constraints typical of
// flash-like devices.
// - start/size: region in bytes
// - erase_size: natural erase granularity in bytes (power-of-two recommended)
// - erase_shift: log2(erase_size) for fast math when available
typedef struct bio_erase_geometry_info {
    off_t start; // start of the region in bytes.
    off_t size;
    size_t erase_size;
    size_t erase_shift;
} bio_erase_geometry_info_t;

// Block device descriptor.
// Filled by drivers and registered via bio_register_device(). Most fields are
// read-only to users; access the device via the provided API functions.
typedef struct bdev {
    struct list_node node;
    volatile int ref; // internal reference count, managed by BIO layer

    // Informational properties
    char *name;       // device name (stable, unique in the namespace)
    off_t total_size; // total size in bytes

    size_t block_size;  // natural block size in bytes (>= 1)
    size_t block_shift; // log2(block_size) if power-of-two, else undefined
    bnum_t block_count; // total_size / block_size (truncated)

    size_t geometry_count;                     // number of geometry entries
    const bio_erase_geometry_info_t *geometry; // erase geometry array (optional)

    uint8_t erase_byte; // byte pattern used after erase (e.g., 0xFF for NOR)
    uint32_t flags;     // BIO_FLAG_* values indicating device properties

    // Driver-provided operations.
    // Synchronous ops return number of bytes/blocks processed (>= 0) or
    // negative LK error code on failure.
    // Async ops return status immediately and invoke callback on completion.
    ssize_t (*read)(struct bdev *, void *buf, off_t offset, size_t len);
    status_t (*read_async)(struct bdev *, void *buf, off_t offset, size_t len,
                           void (*callback)(void *cookie, struct bdev *, ssize_t),
                           void *callback_context);
    ssize_t (*read_block)(struct bdev *, void *buf, bnum_t block, uint count);

    ssize_t (*write)(struct bdev *, const void *buf, off_t offset, size_t len);
    status_t (*write_async)(struct bdev *, const void *buf, off_t offset, size_t len,
                            void (*callback)(void *cookie, struct bdev *, ssize_t),
                            void *callback_context);
    ssize_t (*write_block)(struct bdev *, const void *buf, bnum_t block, uint count);

    // Erase len bytes starting at offset, adhering to erase geometry if present.
    // Optional; return ERR_NOT_SUPPORTED if not implemented.
    ssize_t (*erase)(struct bdev *, off_t offset, size_t len);

    // Device-specific control. See enum bio_ioctl_num for common requests.
    int (*ioctl)(struct bdev *, int request, void *argp);

    // Close hook for drivers (optional). The BIO layer handles refcounts and
    // will call this when the last handle is closed.
    void (*close)(struct bdev *);
} bdev_t;

typedef void (*bio_async_callback_t)(void *cookie, bdev_t *dev, ssize_t status);

// Open a block device by name.
// Returns a device handle with its reference count incremented, or NULL on
// failure (e.g., not found). Must be paired with bio_close().
bdev_t *bio_open(const char *name);

// Close a previously opened device handle. Decrements reference count and
// may trigger device teardown when it reaches zero.
void bio_close(bdev_t *dev);

// Read len bytes at byte offset into buf. Returns number of bytes read or a
// negative error. Short reads may occur at end-of-device.
ssize_t bio_read(bdev_t *dev, void *buf, off_t offset, size_t len);

// Asynchronous read. Returns status immediately; on completion the callback is
// invoked with the final byte count or negative error.
status_t bio_read_async(bdev_t *dev, void *buf, off_t offset, size_t len,
                        bio_async_callback_t callback, void *callback_context);

// Read count blocks starting at block index into buf. count is in blocks.
// Returns blocks read (in bytes via ssize_t, i.e. count*block_size) or error.
ssize_t bio_read_block(bdev_t *dev, void *buf, bnum_t block, uint count);

// Write len bytes at byte offset from buf. Returns number of bytes written or
// a negative error. Partial writes may occur at end-of-device.
ssize_t bio_write(bdev_t *dev, const void *buf, off_t offset, size_t len);

// Asynchronous write. Returns status immediately; callback receives final byte
// count or negative error.
status_t bio_write_async(bdev_t *dev, const void *buf, off_t offset, size_t len,
                         bio_async_callback_t callback, void *callback_context);

// Write count blocks starting at block index from buf. Returns bytes written
// (count*block_size) or negative error.
ssize_t bio_write_block(bdev_t *dev, const void *buf, bnum_t block, uint count);

// Erase len bytes starting at offset. The driver rounds/aligns as required by
// its erase geometry. Returns bytes erased or negative error.
ssize_t bio_erase(bdev_t *dev, off_t offset, size_t len);

// Device-specific control interface. See enum bio_ioctl_num and driver docs.
// Returns >= 0 on success, negative error on failure.
int bio_ioctl(bdev_t *dev, int request, void *argp);

// Register/unregister a device with the BIO layer. Typically called by drivers
// after initializing bdev_t and before exposing it to users.
void bio_register_device(bdev_t *dev);
void bio_unregister_device(bdev_t *dev);

// Initialize common bdev fields during device construction. Must be called
// before bio_register_device(). geometry may be NULL if not applicable.
void bio_initialize_bdev(bdev_t *dev,
                         const char *name,
                         size_t block_size,
                         bnum_t block_count,
                         size_t geometry_count,
                         const bio_erase_geometry_info_t *geometry,
                         uint32_t flags);

// Print a summary of registered devices to the debug console.
void bio_dump_devices(void);

// Publish a subdevice representing a contiguous block range of an existing
// device (e.g., a partition). The new device is named "<parent>.<subdev>".
// Returns status.
status_t bio_publish_subdevice(const char *parent_dev,
                               const char *subdev,
                               bnum_t startblock,
                               bnum_t block_count);

// Create a memory-backed block device over a RAM buffer. Name must be unique.
// Returns 0 on success or negative error.
int create_membdev(const char *name, void *ptr, size_t len);

// Trim an (offset,len) range to device bounds. Returns the clamped length
// (possibly zero). Does not perform I/O.
size_t bio_trim_range(const bdev_t *dev, off_t offset, size_t len);

// Trim a block range to device bounds. Returns the clamped block count
// (possibly zero). Does not perform I/O.
uint bio_trim_block_range(const bdev_t *dev, bnum_t block, uint count);

// Utility: check if two [start, start+len) ranges overlap.
static inline bool bio_does_overlap(uint64_t start1, uint64_t len1,
                                    uint64_t start2, uint64_t len2) {
    uint64_t end1 = start1 + len1;
    uint64_t end2 = start2 + len2;

    DEBUG_ASSERT(end1 >= start1);
    DEBUG_ASSERT(end2 >= start2);

    return (((start1 >= start2) && (start1 < end2)) ||
            ((start2 >= start1) && (start2 < end1)));
}

// Utility: check if one range fully contains another.
static inline bool bio_contains_range(uint64_t container_start, uint64_t container_len,
                                      uint64_t contained_start, uint64_t contained_len) {
    uint64_t container_end = container_start + container_len;
    uint64_t contained_end = contained_start + contained_len;

    DEBUG_ASSERT(container_end >= container_start);
    DEBUG_ASSERT(contained_end >= contained_start);

    return ((container_start <= contained_start) &&
            (container_end >= contained_end));
}

// Generic BIO ioctls that drivers may support. See individual driver docs for
// argument types and semantics:
// - BIO_IOCTL_GET_MEM_MAP: argp -> void **out; maps device memory and returns
//   a linear pointer if supported.
// - BIO_IOCTL_PUT_MEM_MAP: argp ignored or driver-defined; releases mapping.
// - BIO_IOCTL_GET_MAP_ADDR: argp -> void **out; fetch map address without
//   forcing linear mode if supported by the driver.
// - BIO_IOCTL_IS_MAPPED: argp -> bool *out; returns whether device is mapped.
enum bio_ioctl_num {
    BIO_IOCTL_NULL = 0,
    BIO_IOCTL_GET_MEM_MAP,  // if supported, request a pointer to the memory map of the device
    BIO_IOCTL_PUT_MEM_MAP,  // if needed, return the pointer (to 'close' the map)
    BIO_IOCTL_GET_MAP_ADDR, // if supported, request a pointer to the memory map without putting the device into linear mode
    BIO_IOCTL_IS_MAPPED,    // if supported, returns whether or not the device is memory mapped.
};

// The callback will be called once for every block device, with the cookie and pointer
// to the bdev structure. Note callback would be called with internal mutex held, which
// prevents other process/threads from using APIs such as bio_open, so kindly ask callers
// not to do any long blocking operations in callback functions. If the callback function
// returns |false|, iteration stop immediately, and bio_iter_devices returns.
// Iterate over all registered devices. The callback receives (cookie, bdev*).
// The iteration lock is held during the callback; do not perform long or
// blocking operations, nor attempt to open/close devices from within.
// If callback returns false, iteration stops early.
void bio_iter_devices(bool (*callback)(void *, bdev_t *), void *cookie);

__END_CDECLS

#ifdef __cplusplus
// C++ convenience wrapper for bio_iter_devices: allows passing a callable.
template <typename Callable>
bool iter_device_callback(void *cookie, bdev_t *dev) {
    auto func = reinterpret_cast<Callable *>(cookie);
    return (*func)(dev);
}

template <typename Callable>
void bio_iter_devices(Callable &&func) {
    bio_iter_devices(&iter_device_callback<Callable>, &func);
}
#endif

// NOLINTEND(modernize-use-using)
