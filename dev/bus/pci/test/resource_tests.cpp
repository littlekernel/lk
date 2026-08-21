/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Unit tests for the PCI resource allocator: the free range pool and the typed allocator
// built on top of it.

#include <lib/unittest.h>
#include <lk/err.h>

#include "../bus_mgr/resource.h"

using pci::range_pool;
using pci::resource_allocator;

namespace {

bool pool_add_and_merge() {
    BEGIN_TEST;

    range_pool pool;
    EXPECT_TRUE(pool.empty());
    EXPECT_EQ(0U, pool.free_bytes());

    // rejects nonsense
    EXPECT_EQ(ERR_INVALID_ARGS, pool.add(0x1000, 0));
    EXPECT_EQ(ERR_INVALID_ARGS, pool.add(~0ULL - 0x10, 0x100));

    // two disjoint ranges, added out of order
    EXPECT_EQ(NO_ERROR, pool.add(0x3000, 0x1000));
    EXPECT_EQ(NO_ERROR, pool.add(0x1000, 0x1000));
    EXPECT_FALSE(pool.empty());
    EXPECT_EQ(0x2000U, pool.free_bytes());

    // overlaps are rejected
    EXPECT_EQ(ERR_ALREADY_EXISTS, pool.add(0x1800, 0x100));
    EXPECT_EQ(ERR_ALREADY_EXISTS, pool.add(0x0, 0x1001));
    EXPECT_EQ(ERR_ALREADY_EXISTS, pool.add(0x3fff, 0x10));
    EXPECT_EQ(0x2000U, pool.free_bytes());

    // filling the hole merges everything into one range
    EXPECT_EQ(NO_ERROR, pool.add(0x2000, 0x1000));
    EXPECT_EQ(0x3000U, pool.free_bytes());

    // and a single 0x3000 aligned allocation now fits, proving it's one range
    uint64_t addr;
    EXPECT_EQ(NO_ERROR, pool.alloc(0x3000, 12, 0, UINT64_MAX, &addr));
    EXPECT_EQ(0x1000U, addr);
    EXPECT_TRUE(pool.empty());

    END_TEST;
}

bool pool_reserve() {
    BEGIN_TEST;

    range_pool pool;
    EXPECT_EQ(NO_ERROR, pool.add(0x10000, 0x10000));

    // reserving something outside the pool is a no-op
    EXPECT_EQ(NO_ERROR, pool.reserve(0x0, 0x1000));
    EXPECT_EQ(NO_ERROR, pool.reserve(0x20000, 0x1000));
    EXPECT_EQ(0x10000U, pool.free_bytes());

    // carve out the middle: splits into two
    EXPECT_EQ(NO_ERROR, pool.reserve(0x14000, 0x2000));
    EXPECT_EQ(0xe000U, pool.free_bytes());

    // trim the head and the tail
    EXPECT_EQ(NO_ERROR, pool.reserve(0xf000, 0x2000)); // covers 0x10000..0x10fff
    EXPECT_EQ(NO_ERROR, pool.reserve(0x1f000, 0x2000)); // covers 0x1f000..0x1ffff
    EXPECT_EQ(0xc000U, pool.free_bytes());

    // remaining: [0x11000, 0x13fff] and [0x16000, 0x1efff]
    uint64_t addr;
    EXPECT_EQ(NO_ERROR, pool.alloc(0x3000, 12, 0, UINT64_MAX, &addr));
    EXPECT_EQ(0x11000U, addr);
    EXPECT_EQ(NO_ERROR, pool.alloc(0x9000, 12, 0, UINT64_MAX, &addr));
    EXPECT_EQ(0x16000U, addr);
    EXPECT_TRUE(pool.empty());

    // a reservation that swallows several ranges whole
    EXPECT_EQ(NO_ERROR, pool.add(0x1000, 0x1000));
    EXPECT_EQ(NO_ERROR, pool.add(0x3000, 0x1000));
    EXPECT_EQ(NO_ERROR, pool.add(0x5000, 0x1000));
    EXPECT_EQ(NO_ERROR, pool.reserve(0x0, 0x10000));
    EXPECT_TRUE(pool.empty());

    END_TEST;
}

bool pool_aligned_alloc() {
    BEGIN_TEST;

    range_pool pool;
    // a range that starts unaligned
    EXPECT_EQ(NO_ERROR, pool.add(0x1234, 0x10000));

    uint64_t addr;
    // 4K aligned allocation skips the unaligned head
    EXPECT_EQ(NO_ERROR, pool.alloc(0x1000, 12, 0, UINT64_MAX, &addr));
    EXPECT_EQ(0x2000U, addr);

    // the head is still there for a small unaligned request
    EXPECT_EQ(NO_ERROR, pool.alloc(0x10, 4, 0, UINT64_MAX, &addr));
    EXPECT_EQ(0x1240U, addr);

    // min address is honored
    EXPECT_EQ(NO_ERROR, pool.alloc(0x100, 8, 0x8000, UINT64_MAX, &addr));
    EXPECT_EQ(0x8000U, addr);

    // max address is honored: nothing large fits below 0x4000 anymore except the hole
    EXPECT_EQ(NO_ERROR, pool.alloc(0x1000, 12, 0, 0x3fff, &addr));
    EXPECT_EQ(0x3000U, addr);
    EXPECT_EQ(ERR_NO_RESOURCES, pool.alloc(0x1000, 12, 0, 0x3fff, &addr));

    // exhaustion
    EXPECT_EQ(ERR_NO_RESOURCES, pool.alloc(0x100000, 12, 0, UINT64_MAX, &addr));

    // huge alignment beyond the range
    EXPECT_EQ(ERR_NO_RESOURCES, pool.alloc(0x1000, 32, 0, UINT64_MAX, &addr));

    // bad args
    EXPECT_EQ(ERR_INVALID_ARGS, pool.alloc(0, 12, 0, UINT64_MAX, &addr));

    END_TEST;
}

bool allocator_io() {
    BEGIN_TEST;

    resource_allocator alloc;
    EXPECT_FALSE(alloc.has_range(PCI_RESOURCE_IO_RANGE, false));
    EXPECT_EQ(NO_ERROR, alloc.add_range(PCI_RESOURCE_IO_RANGE, false, 0, 0x10000));
    EXPECT_TRUE(alloc.has_range(PCI_RESOURCE_IO_RANGE, false));

    // io ports are never handed out at 0 and stay above the legacy range if possible
    uint32_t io;
    EXPECT_EQ(NO_ERROR, alloc.allocate_io(0x20, 5, &io));
    EXPECT_EQ(0x1000U, io);
    EXPECT_EQ(NO_ERROR, alloc.allocate_io(0x100, 8, &io));
    EXPECT_EQ(0x1100U, io);
    EXPECT_EQ(NO_ERROR, alloc.allocate_io(0x20, 5, &io));
    EXPECT_EQ(0x1020U, io);

    // when the window is tiny, we fall back to anything non zero
    resource_allocator small;
    EXPECT_EQ(NO_ERROR, small.add_range(PCI_RESOURCE_IO_RANGE, false, 0, 0x100));
    EXPECT_EQ(NO_ERROR, small.allocate_io(0x10, 4, &io));
    EXPECT_EQ(0x10U, io);

    // no io window at all
    resource_allocator none;
    EXPECT_EQ(ERR_NO_RESOURCES, none.allocate_io(0x10, 4, &io));

    // prefetchable io makes no sense
    EXPECT_EQ(ERR_INVALID_ARGS, none.add_range(PCI_RESOURCE_IO_RANGE, true, 0, 0x100));

    END_TEST;
}

bool allocator_mmio_fallback() {
    BEGIN_TEST;

    // a bridge style allocator: a 32bit window and a 64bit prefetchable window
    resource_allocator alloc;
    EXPECT_EQ(NO_ERROR, alloc.add_range(PCI_RESOURCE_MMIO_RANGE, false, 0x10000000, 0x100000));
    EXPECT_EQ(NO_ERROR, alloc.add_range(PCI_RESOURCE_MMIO64_RANGE, true, 0x8000000000ULL, 0x100000));

    uint64_t addr;
    if (sizeof(paddr_t) >= 8) {
        // 64bit prefetchable request lands in the 64bit prefetchable window
        EXPECT_EQ(NO_ERROR, alloc.allocate_mmio(true, true, 0x4000, 14, &addr));
        EXPECT_EQ(0x8000000000ULL, addr);
    } else {
        // a 32bit kernel can't go above 4GB, so it falls back to the plain 32bit window
        EXPECT_EQ(NO_ERROR, alloc.allocate_mmio(true, true, 0x4000, 14, &addr));
        EXPECT_EQ(0x10000000ULL, addr);
    }

    // 32bit prefetchable request has no 32bit prefetchable window: falls back to plain mmio
    EXPECT_EQ(NO_ERROR, alloc.allocate_mmio(false, true, 0x1000, 12, &addr));
    EXPECT_EQ((sizeof(paddr_t) >= 8) ? 0x10000000ULL : 0x10004000ULL, addr);

    // 64bit non prefetchable request: no 64bit non prefetchable window, uses the 32bit one
    EXPECT_EQ(NO_ERROR, alloc.allocate_mmio(true, false, 0x1000, 12, &addr));
    EXPECT_EQ((sizeof(paddr_t) >= 8) ? 0x10001000ULL : 0x10005000ULL, addr);

    // non prefetchable requests do NOT spill into the prefetchable window below a bridge
    EXPECT_EQ(ERR_NO_RESOURCES, alloc.allocate_mmio(true, false, 0x200000, 21, &addr));

    // but on a root they do
    resource_allocator root(true);
    EXPECT_EQ(NO_ERROR, root.add_range(PCI_RESOURCE_MMIO64_RANGE, true, 0x8000000000ULL, 0x100000));
    if (sizeof(paddr_t) >= 8) {
        EXPECT_EQ(NO_ERROR, root.allocate_mmio(true, false, 0x1000, 12, &addr));
        EXPECT_EQ(0x8000000000ULL, addr);
    } else {
        // unreachable above 4GB on a 32bit kernel
        EXPECT_EQ(ERR_NO_RESOURCES, root.allocate_mmio(true, false, 0x1000, 12, &addr));
    }

    // a 32bit request can't go above 4GB even on a root
    EXPECT_EQ(ERR_NO_RESOURCES, root.allocate_mmio(false, false, 0x1000, 12, &addr));

    END_TEST;
}

bool allocator_window_classification() {
    BEGIN_TEST;

    resource_allocator alloc(true);

    // a "64bit" window that sits entirely below 4GB is usable by 32bit requests
    EXPECT_EQ(NO_ERROR, alloc.add_range(PCI_RESOURCE_MMIO64_RANGE, false, 0xc0000000, 0x10000000));
    EXPECT_TRUE(alloc.has_range(PCI_RESOURCE_MMIO_RANGE, false));
    EXPECT_FALSE(alloc.has_range(PCI_RESOURCE_MMIO64_RANGE, false));

    uint64_t addr;
    EXPECT_EQ(NO_ERROR, alloc.allocate_mmio(false, false, 0x1000, 12, &addr));
    EXPECT_EQ(0xc0000000ULL, addr);

    // and a "32bit" window that crosses 4GB is really 64bit
    resource_allocator alloc2(true);
    EXPECT_EQ(NO_ERROR, alloc2.add_range(PCI_RESOURCE_MMIO_RANGE, false, 0xf0000000, 0x20000000));
    EXPECT_FALSE(alloc2.has_range(PCI_RESOURCE_MMIO_RANGE, false));
    EXPECT_TRUE(alloc2.has_range(PCI_RESOURCE_MMIO64_RANGE, false));

    // 32bit requests can still use the part of it below 4GB
    EXPECT_EQ(NO_ERROR, alloc2.allocate_mmio(false, false, 0x1000, 12, &addr));
    EXPECT_EQ(0xf0000000ULL, addr);

    END_TEST;
}

bool allocator_reserve_firmware() {
    BEGIN_TEST;

    // simulate keeping firmware assignments: reserve some bars, then allocate into the holes
    resource_allocator alloc(true);
    EXPECT_EQ(NO_ERROR, alloc.add_range(PCI_RESOURCE_MMIO_RANGE, false, 0xfe000000, 0x1000000));
    EXPECT_EQ(NO_ERROR, alloc.add_range(PCI_RESOURCE_IO_RANGE, false, 0x1000, 0xf000));

    // firmware placed a 64K bar at 0xfe000000 and a 4K bar at 0xfe010000, and io at 0xc000
    EXPECT_EQ(NO_ERROR, alloc.reserve(PCI_RESOURCE_MMIO_RANGE, 0xfe000000, 0x10000));
    EXPECT_EQ(NO_ERROR, alloc.reserve(PCI_RESOURCE_MMIO64_RANGE, 0xfe010000, 0x1000));
    EXPECT_EQ(NO_ERROR, alloc.reserve(PCI_RESOURCE_IO_RANGE, 0xc000, 0x40));

    uint64_t addr;
    // an unassigned 64K bar can't fit at 0xfe000000 anymore, next 64K aligned hole is 0xfe020000
    EXPECT_EQ(NO_ERROR, alloc.allocate_mmio(false, false, 0x10000, 16, &addr));
    EXPECT_EQ(0xfe020000ULL, addr);
    // but a 4K one fits right after the second firmware bar
    EXPECT_EQ(NO_ERROR, alloc.allocate_mmio(false, false, 0x1000, 12, &addr));
    EXPECT_EQ(0xfe011000ULL, addr);

    uint32_t io;
    EXPECT_EQ(NO_ERROR, alloc.allocate_io(0x40, 6, &io));
    EXPECT_EQ(0x1000U, io);

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(pci_resource)
RUN_TEST(pool_add_and_merge)
RUN_TEST(pool_reserve)
RUN_TEST(pool_aligned_alloc)
RUN_TEST(allocator_io)
RUN_TEST(allocator_mmio_fallback)
RUN_TEST(allocator_window_classification)
RUN_TEST(allocator_reserve_firmware)
END_TEST_CASE(pci_resource)
