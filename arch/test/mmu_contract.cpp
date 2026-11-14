/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if ARCH_HAS_MMU

// Checks the contract in arch/include/arch/mmu.h against what the page tables
// hold afterwards, as arch_mmu_query() reports it and as the page table counts
// show, in a user aspace where the arch has them and in a window of the kernel
// aspace everywhere. Nothing here reads or writes through a mapping, so that
// an unmap or a protect also reached the TLB is not checked here: the unmap
// tests in mmu.cpp and the user mode tests in uspace.cpp cover part of that.

#include <arch/mmu.h>

#include <kernel/vm.h>
#include <lib/unittest.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lktl/auto_call.h>

#include "mmu_test_util.h"

namespace {

using namespace mmu_test;
// the window overloads below go next to these rather than hide them
using mmu_test::map;
using mmu_test::protect;
using mmu_test::query;
using mmu_test::unmap;

// Per arch geometry: the span one leaf table covers, the span one page table
// allocation serves, and every size of large page map may create, smallest
// first (kNumLargePages is 0 where it never makes one).
#if ARCH_ARM64
constexpr size_t kLeafSpan = 1UL << (2 * PAGE_SIZE_SHIFT - 3);
constexpr size_t kTableSpan = kLeafSpan;
#if PAGE_SIZE_SHIFT == 12
constexpr size_t kLargePages[] = { 2UL << 20, 1UL << 30 };
#else
constexpr size_t kLargePages[] = { kLeafSpan }; // 32MB or 512MB, one level only
#endif
constexpr size_t kNumLargePages = countof(kLargePages);
#elif ARCH_RISCV
constexpr size_t kLeafSpan = PAGE_SIZE * (PAGE_SIZE / sizeof(ulong));
constexpr size_t kTableSpan = kLeafSpan;
#if RISCV_MMU == 48
constexpr size_t kLargePages[] = { 2UL << 20, 1UL << 30 };
constexpr size_t kNumLargePages = countof(kLargePages);
#elif RISCV_MMU == 39
constexpr size_t kLargePages[] = { kLeafSpan }; // the top level never holds a leaf
constexpr size_t kNumLargePages = countof(kLargePages);
#else
constexpr size_t kLargePages[] = { 0 }; // sv32's only large page would be a top level leaf
constexpr size_t kNumLargePages = 0;
#endif
#elif ARCH_ARM
constexpr size_t kLeafSpan = 1UL << 20;
constexpr size_t kTableSpan = 4UL << 20; // one page holds the tables of four sections
constexpr size_t kLargePages[] = { kLeafSpan };
constexpr size_t kNumLargePages = countof(kLargePages);
#elif ARCH_X86_64
constexpr size_t kLeafSpan = 2UL << 20;
constexpr size_t kTableSpan = kLeafSpan;
constexpr size_t kLargePages[] = { 0 };
constexpr size_t kNumLargePages = 0;
#elif ARCH_X86_32
constexpr size_t kLeafSpan = 4UL << 20;
constexpr size_t kTableSpan = kLeafSpan;
constexpr size_t kLargePages[] = { 0 };
constexpr size_t kNumLargePages = 0;
#elif ARCH_M68K
constexpr size_t kLeafSpan = 4UL << 20;
constexpr size_t kTableSpan = kLeafSpan;
constexpr size_t kLargePages[] = { 0 };
constexpr size_t kNumLargePages = 0;
#else
#error "add the arch's table geometry"
#endif
constexpr size_t kLargestPage = kLargePages[countof(kLargePages) - 1];

// Room for three leaf tables in a row, for a range across the boundary between
// two table allocations, and for the largest large page with a page in front
// of it and two behind.
#if IS_64BIT
constexpr size_t kWindowSize = 4UL << 30;
constexpr uint8_t kWindowAlign = 30;
#else
constexpr size_t kWindowSize = 16UL << 20;
constexpr uint8_t kWindowAlign = 22;
#endif
static_assert(kWindowSize >= 3 * kLeafSpan && kWindowSize >= 2 * kTableSpan &&
              kWindowSize >= 3 * kLargestPage, "");

// The x86-32 kernel aspace keeps every page table it makes: each user page
// directory holds a copy of the kernel's entries. sv32's kernel tables below
// the root are the static ones every user root shares, all linked at boot.
#if ARCH_X86_32 || (ARCH_RISCV && RISCV_MMU == 32)
constexpr bool kKernelTablesPermanent = true;
#else
constexpr bool kKernelTablesPermanent = false;
#endif

constexpr uint log2_of(size_t v) { return v <= 1 ? 0 : 1 + log2_of(v / 2); }

// A place to map into: a range of address space nothing else uses, in a
// fresh user aspace or in the kernel's.
struct window {
    const char *name;
    vmm_aspace_t *as;
    vaddr_t base;
    size_t size;
    uint user;             // ARCH_MMU_FLAG_PERM_USER in a user aspace
    bool tables_permanent; // tables linked here are never freed
    table_tracker *tables; // counts the page tables the test allocates and frees
    vaddr_t outside;       // an address outside the aspace
};

// whether a page mapped with one cache policy may read back as another: the
// entry may not tell device memory from uncached, and riscv (no Svpbmt)
// maps everything cached
bool cache_readback_ok(uint requested, uint got) {
#if ARCH_RISCV
    return got == ARCH_MMU_FLAG_CACHED;
#else
    if (requested == ARCH_MMU_FLAG_UNCACHED_DEVICE) {
        return got == ARCH_MMU_FLAG_UNCACHED_DEVICE || got == ARCH_MMU_FLAG_UNCACHED;
    }
    return got == requested;
#endif
}

// The arch calls on the window's aspace, under its lock. map adds the
// window's USER bit.
status_t map(const window &w, vaddr_t va, paddr_t pa, uint count, uint flags = kRW) {
    return map(w.as, va, pa, count, flags | w.user);
}

status_t unmap(const window &w, vaddr_t va, uint count) {
    return unmap(w.as, va, count);
}

status_t protect(const window &w, vaddr_t va, uint count, uint flags) {
    return protect(w.as, va, count, flags);
}

status_t query(const window &w, vaddr_t va, paddr_t *pa, uint *flags) {
    return query(w.as, va, pa, flags);
}

bool is_mapped(const window &w, vaddr_t va) {
    return query(w, va, nullptr, nullptr) == NO_ERROR;
}

// va maps pa with flags (the window's USER bit added)
bool check_page(const window &w, vaddr_t va, paddr_t pa, uint flags = kRW) {
    BEGIN_TEST;
    paddr_t got_pa = 0;
    uint got_flags = 0;
    ASSERT_EQ(NO_ERROR, query(w, va, &got_pa, &got_flags), "query");
    EXPECT_EQ(pa, got_pa, "paddr");
    const uint cache = flags & ARCH_MMU_FLAG_CACHE_MASK;
    EXPECT_TRUE(cache_readback_ok(cache, got_flags & ARCH_MMU_FLAG_CACHE_MASK), "cache policy");
    EXPECT_EQ(query_flags((flags & ~ARCH_MMU_FLAG_CACHE_MASK) | w.user),
              got_flags & ~ARCH_MMU_FLAG_CACHE_MASK, "flags");
    END_TEST;
}

bool check_unmapped(const window &w, vaddr_t va, uint count) {
    BEGIN_TEST;
    for (uint i = 0; i < count; i++) {
        paddr_t pa;
        EXPECT_EQ(ERR_NOT_FOUND, query(w, va + i * PAGE_SIZE, &pa, nullptr), "unmapped");
    }
    END_TEST;
}

// What goes behind a large page of size bytes and extra pages after it: real
// pages where the pmm has that many contiguous and aligned, else physical
// address space the test does not own. The suite only queries what it maps,
// so the second serves as well; it is mapped as device memory, which nothing
// reads ahead of an access either.
struct large_backing {
    backing real;
    paddr_t pa = 0;
    uint flags = kRW; // what to map it with

    void get(size_t size, uint extra = 0) {
        if (real.alloc(size / PAGE_SIZE + extra, log2_of(size))) {
            pa = real.pa;
        } else {
            pa = size; // any address aligned to the size will do
            flags = kRW | ARCH_MMU_FLAG_UNCACHED_DEVICE;
        }
    }

    // its mapping flags with other permissions
    uint with(uint perms) const { return (flags & ARCH_MMU_FLAG_CACHE_MASK) | perms; }
};

// runs fn for every size of large page, with the window empty before each
template <typename F>
bool for_each_large_page(const window &w, F fn) {
    BEGIN_TEST;
    for (size_t i = 0; i < kNumLargePages; i++) {
        if (!fn(w, kLargePages[i])) {
            unittest_printf(" (large page of %#zx)", kLargePages[i]);
            all_ok = false;
        }
        unmap(w, w.base, w.size / PAGE_SIZE);
    }
    END_TEST;
}

// Runs fn in a user aspace window and in a kernel aspace window. Whatever fn
// left mapped goes afterwards, and by then every page table the test
// allocated must have been freed again.
template <typename F>
bool in_each_window(F fn) {
    BEGIN_TEST;

    if (arch_mmu_supports_user_aspaces()) {
        window w = {};
        w.name = "user";
        scoped_user_aspace as;
        ASSERT_EQ(NO_ERROR, as.create("mmu_contract"), "create aspace");
        w.as = as;
        ASSERT_EQ(NO_ERROR, vmm_reserve_range(w.as, "window", kWindowSize, kWindowAlign, &w.base), "reserve");
        w.size = kWindowSize;
        w.user = ARCH_MMU_FLAG_PERM_USER;
        w.outside = KERNEL_ASPACE_BASE;
        table_tracker tables;
        w.tables = &tables;
        if (!fn(w)) {
            unittest_printf(" (in the user window)");
            all_ok = false;
        }
        unmap(w, w.base, w.size / PAGE_SIZE);
        EXPECT_EQ(0L, tables.live(), "every table freed, user window");
    }

    {
        window w = {};
        w.name = "kernel";
        w.as = vmm_get_kernel_aspace();
        ASSERT_EQ(NO_ERROR, vmm_reserve_range(w.as, "window", kWindowSize, kWindowAlign, &w.base), "reserve");
        auto cleanup = lk::make_auto_call([&]() { vmm_free_region(w.as, w.base); });
        w.size = kWindowSize;
        w.user = 0;
        w.tables_permanent = kKernelTablesPermanent;
        w.outside = KERNEL_ASPACE_BASE - PAGE_SIZE;
        table_tracker tables;
        w.tables = &tables;
        if (!fn(w)) {
            unittest_printf(" (in the kernel window)");
            all_ok = false;
        }
        unmap(w, w.base, w.size / PAGE_SIZE);
        if (!w.tables_permanent) {
            EXPECT_EQ(0L, tables.live(), "every table freed, kernel window");
        }
    }

    END_TEST;
}

// Map and unmap return exactly NO_ERROR, and an unmap of nothing succeeds.
bool return_values(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(3), "backing");
    EXPECT_EQ(NO_ERROR, map(w.as, w.base, b.pa, 3, kRW | w.user), "map");
    for (uint i = 0; i < 3; i++) {
        EXPECT_TRUE(check_page(w, w.base + i * PAGE_SIZE, b.pa + i * PAGE_SIZE), "mapped");
    }
    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 3), "unmap");
    EXPECT_TRUE(check_unmapped(w, w.base, 3), "gone");
    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 3), "unmap nothing");
    END_TEST;
}

// Either query out-parameter may be NULL, and paddr carries the offset.
bool query_out_params(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(1), "backing");
    ASSERT_EQ(NO_ERROR, map(w, w.base, b.pa, 1), "map");

    paddr_t pa = 0;
    uint flags = 0;
    EXPECT_EQ(NO_ERROR, query(w, w.base, nullptr, &flags), "no paddr");
    EXPECT_EQ(query_flags(kRW | w.user), flags, "flags");
    EXPECT_EQ(NO_ERROR, query(w, w.base + 0x123, &pa, nullptr), "no flags");
    EXPECT_EQ(b.pa + 0x123, pa, "paddr with offset");
    EXPECT_EQ(NO_ERROR, query(w, w.base, nullptr, nullptr), "neither");
    EXPECT_EQ(ERR_NOT_FOUND, query(w, w.base + PAGE_SIZE, nullptr, nullptr), "unmapped, neither");
    EXPECT_EQ(ERR_OUT_OF_RANGE, query(w, w.outside, nullptr, nullptr), "outside, neither");

    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 1), "unmap");
    END_TEST;
}

// count == 0 succeeds without touching anything, but only once the other
// arguments have passed.
bool count_zero(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(1), "backing");
    EXPECT_EQ(NO_ERROR, map(w, w.base, b.pa, 0), "map nothing");
    EXPECT_FALSE(is_mapped(w, w.base), "nothing mapped");
    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 0), "unmap nothing");
    EXPECT_EQ(ERR_OUT_OF_RANGE, map(w, w.outside, b.pa, 0), "map nothing outside");
    EXPECT_EQ(ERR_OUT_OF_RANGE, unmap(w, w.outside, 0), "unmap nothing outside");
    END_TEST;
}

// A map that runs into a mapped page fails with ERR_ALREADY_EXISTS, leaves
// that page as it was and takes back the pages it mapped before it, also
// across a leaf table boundary.
bool collision(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(5), "backing");
    const paddr_t other = b.pa + 4 * PAGE_SIZE;

    ASSERT_EQ(NO_ERROR, map(w, w.base + 2 * PAGE_SIZE, other, 1), "map page 2");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, w.base, b.pa, 4), "map over page 2");
    EXPECT_TRUE(check_page(w, w.base + 2 * PAGE_SIZE, other), "page 2 intact");
    EXPECT_TRUE(check_unmapped(w, w.base, 2), "pages 0-1 rolled back");
    EXPECT_TRUE(check_unmapped(w, w.base + 3 * PAGE_SIZE, 1), "page 3 never mapped");

    // the same page again, and a range starting at it
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, w.base + 2 * PAGE_SIZE, b.pa, 1), "map the same page");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, w.base + 2 * PAGE_SIZE, b.pa, 2), "map starting at it");
    EXPECT_TRUE(check_page(w, w.base + 2 * PAGE_SIZE, other), "page 2 still intact");
    EXPECT_TRUE(check_unmapped(w, w.base + 3 * PAGE_SIZE, 1), "page 3 still unmapped");
    EXPECT_EQ(NO_ERROR, unmap(w, w.base + 2 * PAGE_SIZE, 1), "unmap page 2");

    // across a leaf table boundary: the pages in the first table go again
    const vaddr_t edge = w.base + kLeafSpan;
    ASSERT_EQ(NO_ERROR, map(w, edge + PAGE_SIZE, other, 1), "map past the boundary");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, edge - 2 * PAGE_SIZE, b.pa, 4), "map across the boundary");
    EXPECT_TRUE(check_unmapped(w, edge - 2 * PAGE_SIZE, 3), "pages before the collision rolled back");
    EXPECT_TRUE(check_page(w, edge + PAGE_SIZE, other), "colliding page intact");
    EXPECT_EQ(NO_ERROR, unmap(w, edge + PAGE_SIZE, 1), "unmap");
    END_TEST;
}

#if LK_DEBUGLEVEL > 0
// A map that runs out of page tables fails with ERR_NO_MEMORY and leaves the
// aspace as it was: nothing mapped, and every table it linked freed again.
// The range crosses the boundary between two table allocations, so one of the
// budgets runs out only after pages went in.
bool out_of_tables(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(4), "backing");
    table_tracker &tables = *w.tables;
    auto budget_cleanup = lk::make_auto_call([&]() { tables.set_budget(-1); });

    const vaddr_t va = w.base + kTableSpan - 2 * PAGE_SIZE;

    if (w.tables_permanent) {
        // A table linked here stays, so each attempt gets one table further
        // than the last, and only the first run of a boot starts with none.
        constexpr int kMaxAttempts = 16;
        int failures = 0;
        status_t err = ERR_NO_MEMORY;
        for (int attempt = 0; attempt < kMaxAttempts; attempt++) {
            tables.set_budget(attempt == 0 ? 0 : 1);
            err = map(w, va, b.pa, 4);
            tables.set_budget(-1);
            if (err == NO_ERROR) {
                break;
            }
            failures++;
            EXPECT_EQ(ERR_NO_MEMORY, err, "map short of tables");
            EXPECT_TRUE(check_unmapped(w, va, 4), "nothing mapped");
            if (!all_ok) {
                break;
            }
        }
        EXPECT_EQ(NO_ERROR, err, "maps once the tables are there");
        if (failures == 0) {
            unittest_printf(" (every table already there)");
        }
        EXPECT_EQ(NO_ERROR, unmap(w, va, 4), "unmap");
        END_TEST;
    }

    // how many tables the range needs
    const long live = tables.live();
    size_t before = tables.allocs();
    ASSERT_EQ(NO_ERROR, map(w, va, b.pa, 4), "map");
    const int needed = (int)(tables.allocs() - before);
    EXPECT_EQ(NO_ERROR, unmap(w, va, 4), "unmap");
    EXPECT_EQ(live, tables.live(), "the unmap freed them");
    EXPECT_LE(2, needed, "a table either side of the boundary");

    // every budget short of that
    for (int budget = 0; budget < needed; budget++) {
        tables.set_budget(budget);
        const status_t err = map(w, va, b.pa, 4);
        tables.set_budget(-1);
        EXPECT_EQ(ERR_NO_MEMORY, err, "map short of tables");
        EXPECT_TRUE(check_unmapped(w, va, 4), "nothing mapped");
        EXPECT_EQ(live, tables.live(), "every table it linked freed again");
        if (err == NO_ERROR) {
            unmap(w, va, 4);
        }
    }

    // and the aspace still maps normally, needing as many as before
    before = tables.allocs();
    tables.set_budget(needed);
    const status_t err = map(w, va, b.pa, 4);
    tables.set_budget(-1);
    EXPECT_EQ(NO_ERROR, err, "map with just enough");
    EXPECT_EQ(needed, (int)(tables.allocs() - before), "as many tables as before");
    for (uint i = 0; i < 4; i++) {
        EXPECT_TRUE(check_page(w, va + i * PAGE_SIZE, b.pa + i * PAGE_SIZE), "mapped");
    }
    EXPECT_EQ(NO_ERROR, unmap(w, va, 4), "unmap");
    EXPECT_EQ(live, tables.live(), "tables freed");
    END_TEST;
}
#endif

// Only the ends of a range can cut a large page, and unmap refuses that
// before it changes anything.
bool large_page_partial_unmap(const window &w, size_t size) {
    BEGIN_TEST;
    large_backing big;
    big.get(size);
    backing small;
    ASSERT_TRUE(small.alloc(1), "backing");

    const vaddr_t large = w.base + size;
    const uint large_count = size / PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, large - PAGE_SIZE, small.pa, 1), "map page before");
    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count, big.flags), "map large page");

    EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(w, large - PAGE_SIZE, 2), "unmap into the large page");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa), "page before intact");
    EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(w, large + PAGE_SIZE, 1), "unmap inside");
    EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(w, large + size - PAGE_SIZE, 2), "unmap out of the end");
    EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(w, large - PAGE_SIZE, large_count), "unmap all but the last page");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa), "page before still intact");
    for (size_t off = 0; off < size; off += size / 4) {
        EXPECT_TRUE(check_page(w, large + off, big.pa + off, big.flags), "large page intact");
    }

    EXPECT_EQ(NO_ERROR, unmap(w, large - PAGE_SIZE, large_count + 1), "unmap both");
    EXPECT_TRUE(check_unmapped(w, large - PAGE_SIZE, 2), "gone");
    EXPECT_TRUE(check_unmapped(w, large + size - PAGE_SIZE, 1), "gone at the end");
    END_TEST;
}

// A page inside a large page collides like any other, and so does a large
// map over a table holding a page. Once the page is gone the large page fits.
bool large_page_collision(const window &w, size_t size) {
    BEGIN_TEST;
    large_backing big;
    big.get(size);
    backing small;
    ASSERT_TRUE(small.alloc(2), "backing");

    const vaddr_t large = w.base + size;
    const uint large_count = size / PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count, big.flags), "map large page");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, large + PAGE_SIZE, small.pa, 1), "map inside");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, large - PAGE_SIZE, small.pa, 2), "map into it");
    EXPECT_TRUE(check_unmapped(w, large - PAGE_SIZE, 1), "page before rolled back");
    EXPECT_TRUE(check_page(w, large + PAGE_SIZE, big.pa + PAGE_SIZE, big.flags), "large page intact");
    EXPECT_EQ(NO_ERROR, unmap(w, large, large_count), "unmap large page");

    ASSERT_EQ(NO_ERROR, map(w, large + PAGE_SIZE, small.pa, 1), "map a page");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, large, big.pa, large_count, big.flags), "map large over it");
    EXPECT_TRUE(check_page(w, large + PAGE_SIZE, small.pa), "page intact");
    EXPECT_TRUE(check_unmapped(w, large, 1), "large map rolled back");
    EXPECT_EQ(NO_ERROR, unmap(w, large + PAGE_SIZE, 1), "unmap the page");

    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count, big.flags), "map large where the table was");
    for (size_t off = 0; off < size; off += size / 4) {
        EXPECT_TRUE(check_page(w, large + off + 0x10, big.pa + off + 0x10, big.flags), "mapped");
    }
    EXPECT_EQ(NO_ERROR, unmap(w, large, large_count), "unmap");
    END_TEST;
}

// A map that fails after it made a large page takes the large page back
// with everything else: on a collision past it, and when the tables for the
// pages after it cannot be had.
bool large_page_rollback(const window &w, size_t size) {
    BEGIN_TEST;
    large_backing big;
    big.get(size, 2);
    backing small;
    ASSERT_TRUE(small.alloc(1), "backing");
    table_tracker &tables = *w.tables;

    const vaddr_t large = w.base + size;
    const uint large_count = size / PAGE_SIZE;
    const vaddr_t after = large + size;

    ASSERT_EQ(NO_ERROR, map(w, after + PAGE_SIZE, small.pa, 1), "map the second page after");
    const long live = tables.live();
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, large, big.pa, large_count + 2, big.flags), "map into that page");
    EXPECT_TRUE(check_unmapped(w, large, 1), "large page rolled back");
    EXPECT_TRUE(check_unmapped(w, after - PAGE_SIZE, 2), "to its end, and the page after it");
    EXPECT_TRUE(check_page(w, after + PAGE_SIZE, small.pa), "existing page intact");
    EXPECT_EQ(live, tables.live(), "no table left behind");
    EXPECT_EQ(NO_ERROR, unmap(w, after + PAGE_SIZE, 1), "unmap the page");

#if LK_DEBUGLEVEL > 0
    // as many tables as the large page takes, and none for the page after it
    size_t before = tables.allocs();
    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count, big.flags), "map large page");
    const int for_large = (int)(tables.allocs() - before);
    EXPECT_EQ(NO_ERROR, unmap(w, large, large_count), "unmap");
    before = tables.allocs();
    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count + 1, big.flags), "map a page more");
    const int for_both = (int)(tables.allocs() - before);
    EXPECT_EQ(NO_ERROR, unmap(w, large, large_count + 1), "unmap");

    if (for_both > for_large) {
        const long empty = tables.live();
        tables.set_budget(for_large);
        const status_t err = map(w, large, big.pa, large_count + 1, big.flags);
        tables.set_budget(-1);
        EXPECT_EQ(ERR_NO_MEMORY, err, "map short of the page's tables");
        EXPECT_TRUE(check_unmapped(w, large, 1), "large page rolled back");
        EXPECT_TRUE(check_unmapped(w, after - PAGE_SIZE, 2), "to its end, and the page after it");
        EXPECT_EQ(empty, tables.live(), "every table it linked freed again");
    } else {
        unittest_printf(" (the page after needs no table)");
    }
#endif
    END_TEST;
}

// A range that starts inside the aspace and runs past its end is refused,
// whatever is mapped at its last page, and nothing changes.
bool past_the_end(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(2), "backing");
    const vaddr_t last = w.as->base + (w.as->size - PAGE_SIZE);

    paddr_t pa = 0;
    uint flags = 0;
    const status_t there = query(w, last, &pa, &flags);
    EXPECT_TRUE(there == NO_ERROR || there == ERR_NOT_FOUND, "the last page is in range");

    EXPECT_EQ(ERR_OUT_OF_RANGE, map(w, last, b.pa, 2), "map past the end");
    EXPECT_EQ(ERR_OUT_OF_RANGE, unmap(w, last, 2), "unmap past the end");
    EXPECT_EQ(ERR_OUT_OF_RANGE, protect(w, last, 2, kR), "protect past the end");
    // one page more than there is from the window to the end, where a count can say that
    const size_t to_end = (last - w.base) / PAGE_SIZE + 1;
    if (to_end < UINT_MAX) {
        EXPECT_EQ(ERR_OUT_OF_RANGE, unmap(w, w.base, (uint)to_end + 1), "unmap one page past the end");
        EXPECT_EQ(ERR_OUT_OF_RANGE, protect(w, w.base, (uint)to_end + 1, kR), "protect one page past the end");
    }

    EXPECT_EQ(NO_ERROR, map(w, last, b.pa, 0), "map nothing at the last page");
    EXPECT_EQ(NO_ERROR, unmap(w, last, 0), "unmap nothing at the last page");
    EXPECT_EQ(NO_ERROR, protect(w, last, 0, kR), "protect nothing at the last page");

    paddr_t pa2 = 0;
    uint flags2 = 0;
    EXPECT_EQ(there, query(w, last, &pa2, &flags2), "the last page is as it was");
    if (there == NO_ERROR) {
        EXPECT_EQ(pa, pa2, "paddr");
        EXPECT_EQ(flags, flags2, "flags");
    }
    END_TEST;
}

// Bad arguments are refused before anything else, count == 0 included.
bool bad_args(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(1), "backing");
    const vaddr_t va = w.base;

    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va + 1, b.pa, 1), "unaligned vaddr");
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa + 1, 1), "unaligned paddr");
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va + 1, b.pa, 0), "unaligned vaddr, no pages");
    EXPECT_EQ(ERR_INVALID_ARGS, unmap(w, va + 1, 1), "unaligned unmap");
    EXPECT_EQ(ERR_INVALID_ARGS, unmap(w, va + 1, 0), "unaligned unmap, no pages");

    const paddr_t top = ROUNDDOWN((paddr_t)-1, PAGE_SIZE);
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, top, 2), "paddr wraps");
#if ARCH_ARM64
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, 1ULL << 48, 1), "paddr past the output address size");
#elif ARCH_RISCV && IS_64BIT
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, 1ULL << 56, 1), "paddr past the ppn");
#elif ARCH_X86_64
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, 1ULL << 52, 1), "paddr past MAXPHYADDR");
#endif

    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa, 1, kRW | 3), "cache field 3");
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa, 1, kRW | ARCH_MMU_FLAG_INVALID), "INVALID");
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa, 1, kRW | (1U << 30)), "undefined bit");
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa, 0, kRW | ARCH_MMU_FLAG_INVALID), "INVALID, no pages");
    if (!arch_mmu_supports_ns_mappings()) {
        EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa, 1, kRW | ARCH_MMU_FLAG_NS), "NS");
    }
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa, 1, 0), "no permissions");
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa, 1, ARCH_MMU_FLAG_PERM_WRITE), "write only");
    EXPECT_EQ(ERR_INVALID_ARGS, map(w, va, b.pa, 1, ARCH_MMU_FLAG_PERM_EXECUTE), "execute only");
    EXPECT_EQ(ERR_INVALID_ARGS,
              map(w, va, b.pa, 1, ARCH_MMU_FLAG_PERM_WRITE | ARCH_MMU_FLAG_PERM_EXECUTE), "WX");
    EXPECT_EQ(ERR_INVALID_ARGS,
              map(w, va, b.pa, 1, ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_EXECUTE | ARCH_MMU_FLAG_UNCACHED),
              "execute uncached");
    EXPECT_EQ(ERR_INVALID_ARGS,
              map(w, va, b.pa, 1, ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_EXECUTE | ARCH_MMU_FLAG_UNCACHED_DEVICE),
              "execute device");
    EXPECT_TRUE(check_unmapped(w, va, 1), "nothing mapped");
    END_TEST;
}


// Each cache policy maps and reads back as what the entry can encode.
bool cache_policies(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(3), "backing");
    const uint policies[] = { ARCH_MMU_FLAG_CACHED, ARCH_MMU_FLAG_UNCACHED, ARCH_MMU_FLAG_UNCACHED_DEVICE };
    for (uint i = 0; i < countof(policies); i++) {
        const vaddr_t va = w.base + i * PAGE_SIZE;
        ASSERT_EQ(NO_ERROR, map(w, va, b.pa + i * PAGE_SIZE, 1, kRW | policies[i]), "map");
        EXPECT_TRUE(check_page(w, va, b.pa + i * PAGE_SIZE, kRW | policies[i]), "read back");
    }
    EXPECT_EQ(NO_ERROR, unmap(w, w.base, countof(policies)), "unmap");
    END_TEST;
}

// protect replaces the permissions of every page and keeps the rest.
bool protect_permissions(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(4), "backing");
    // across a leaf table boundary
    const vaddr_t va = w.base + kLeafSpan - 2 * PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, va, b.pa, 4), "map");
    // and a non-secure page where the arch has them, under a table of its own
    const vaddr_t ns_va = w.base + 2 * kLeafSpan;
    const bool ns = arch_mmu_supports_ns_mappings();
    if (ns) {
        ASSERT_EQ(NO_ERROR, map(w, ns_va, b.pa, 1, kRW | ARCH_MMU_FLAG_NS), "map non-secure");
        EXPECT_TRUE(check_page(w, ns_va, b.pa, kRW | ARCH_MMU_FLAG_NS), "non-secure");
    }

    const uint sets[] = { kR, kR | kX, kR | kW | kX, kR | kW, kR };
    for (const uint perms : sets) {
        EXPECT_EQ(NO_ERROR, protect(w, va, 4, perms), "protect");
        for (uint i = 0; i < 4; i++) {
            EXPECT_TRUE(check_page(w, va + i * PAGE_SIZE, b.pa + i * PAGE_SIZE, perms), "new permissions");
        }
        if (ns) {
            EXPECT_EQ(NO_ERROR, protect(w, ns_va, 1, perms), "protect non-secure");
            EXPECT_TRUE(check_page(w, ns_va, b.pa, perms | ARCH_MMU_FLAG_NS), "still non-secure");
        }
    }
    // the same set again changes nothing
    EXPECT_EQ(NO_ERROR, protect(w, va, 4, kR), "protect to the same");
    EXPECT_TRUE(check_page(w, va, b.pa, kR), "unchanged");

    EXPECT_EQ(NO_ERROR, unmap(w, va, 4), "unmap");
    if (ns) {
        EXPECT_EQ(NO_ERROR, unmap(w, ns_va, 1), "unmap non-secure");
    }
    END_TEST;
}

// Holes are stepped over and stay holes, whether a few entries or a leaf
// table that is not there, and protect allocates nothing to cross them.
bool protect_holes(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(3), "backing");
    ASSERT_EQ(NO_ERROR, map(w, w.base, b.pa, 1), "map page 0");
    ASSERT_EQ(NO_ERROR, map(w, w.base + 2 * PAGE_SIZE, b.pa + PAGE_SIZE, 1), "map page 2");
    // two leaf tables on, with no table for the span in between
    const vaddr_t far = w.base + 2 * kLeafSpan;
    ASSERT_EQ(NO_ERROR, map(w, far, b.pa + 2 * PAGE_SIZE, 1), "map a far page");

    const size_t allocs = w.tables->allocs();
    EXPECT_EQ(NO_ERROR, protect(w, w.base, (far - w.base) / PAGE_SIZE + 1, kR), "protect around holes");
    EXPECT_EQ(allocs, w.tables->allocs(), "no table allocated");
    EXPECT_TRUE(check_page(w, w.base, b.pa, kR), "page 0");
    EXPECT_TRUE(check_page(w, w.base + 2 * PAGE_SIZE, b.pa + PAGE_SIZE, kR), "page 2");
    EXPECT_TRUE(check_page(w, far, b.pa + 2 * PAGE_SIZE, kR), "far page");
    EXPECT_TRUE(check_unmapped(w, w.base + PAGE_SIZE, 1), "hole 1 stays");
    EXPECT_TRUE(check_unmapped(w, w.base + 3 * PAGE_SIZE, 1), "hole 3 stays");
    EXPECT_TRUE(check_unmapped(w, w.base + kLeafSpan, 1), "the span without a table stays");
    EXPECT_TRUE(check_unmapped(w, far - PAGE_SIZE, 1), "to its last page");
    EXPECT_EQ(NO_ERROR, protect(w, w.base + 4 * PAGE_SIZE, 4, kR), "protect nothing");

    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 3), "unmap");
    EXPECT_EQ(NO_ERROR, unmap(w, far, 1), "unmap far page");
    EXPECT_EQ(NO_ERROR, protect(w, w.base, w.size / PAGE_SIZE, kR), "protect an empty window");
    END_TEST;
}

// Anything but an R/W/X set with READ in it is refused, and so are bad
// ranges, before anything changes.
bool protect_bad_args(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(1), "backing");
    ASSERT_EQ(NO_ERROR, map(w, w.base, b.pa, 1), "map");

    const uint bad[] = {
        0, kW, kX, kW | kX,
        kR | ARCH_MMU_FLAG_PERM_USER, kR | ARCH_MMU_FLAG_UNCACHED, kR | ARCH_MMU_FLAG_UNCACHED_DEVICE,
        kR | ARCH_MMU_FLAG_NS, kR | ARCH_MMU_FLAG_INVALID, kR | (1U << 30),
    };
    for (const uint flags : bad) {
        EXPECT_EQ(ERR_INVALID_ARGS, protect(w, w.base, 1, flags), "bad set");
        EXPECT_EQ(ERR_INVALID_ARGS, protect(w, w.base, 0, flags), "bad set, no pages");
    }
    EXPECT_EQ(ERR_INVALID_ARGS, protect(w, w.base + 1, 1, kR), "unaligned");
    EXPECT_EQ(ERR_INVALID_ARGS, protect(w, w.base + 1, 0, kR), "unaligned, no pages");
    EXPECT_EQ(ERR_OUT_OF_RANGE, protect(w, w.outside, 1, kR), "outside");
    EXPECT_EQ(ERR_OUT_OF_RANGE, protect(w, w.outside, 0, kR), "outside, no pages");
    EXPECT_EQ(NO_ERROR, protect(w, w.base, 0, kR), "no pages");
    EXPECT_TRUE(check_page(w, w.base, b.pa, kRW), "unchanged");

    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 1), "unmap");
    END_TEST;
}

// EXECUTE is refused on an uncached page, before any page in the range
// changes; where the entry cannot be uncached it is allowed. The uncached
// page sits between two cached ones, so it is not at either end of the range.
bool protect_execute_uncached(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(4), "backing");
    const vaddr_t uncached = w.base + PAGE_SIZE;
    const vaddr_t device = w.base + 3 * PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, w.base, b.pa, 1, kRW), "map cached");
    ASSERT_EQ(NO_ERROR, map(w, uncached, b.pa + PAGE_SIZE, 1, kRW | ARCH_MMU_FLAG_UNCACHED), "map uncached");
    ASSERT_EQ(NO_ERROR, map(w, w.base + 2 * PAGE_SIZE, b.pa + 2 * PAGE_SIZE, 1, kRW), "map cached");
    ASSERT_EQ(NO_ERROR, map(w, device, b.pa + 3 * PAGE_SIZE, 1, kRW | ARCH_MMU_FLAG_UNCACHED_DEVICE), "map device");

    uint flags = 0;
    ASSERT_EQ(NO_ERROR, query(w, uncached, nullptr, &flags), "query");
    if ((flags & ARCH_MMU_FLAG_CACHE_MASK) != ARCH_MMU_FLAG_CACHED) {
        EXPECT_EQ(ERR_INVALID_ARGS, protect(w, w.base, 3, kR | kX), "execute over an uncached page");
        EXPECT_TRUE(check_page(w, w.base, b.pa, kRW), "page before it unchanged");
        EXPECT_TRUE(check_page(w, w.base + 2 * PAGE_SIZE, b.pa + 2 * PAGE_SIZE, kRW), "page after it unchanged");
        EXPECT_EQ(ERR_INVALID_ARGS, protect(w, device, 1, kR | kX), "execute device");
        EXPECT_EQ(NO_ERROR, protect(w, uncached, 1, kR), "read only uncached");
        EXPECT_TRUE(check_page(w, uncached, b.pa + PAGE_SIZE, kR | ARCH_MMU_FLAG_UNCACHED), "uncached kept");
        EXPECT_EQ(NO_ERROR, protect(w, device, 1, kR), "read only device");
        EXPECT_TRUE(check_page(w, device, b.pa + 3 * PAGE_SIZE, kR | ARCH_MMU_FLAG_UNCACHED_DEVICE), "device kept");
    } else {
        EXPECT_EQ(NO_ERROR, protect(w, w.base, 4, kR | kX), "every page is cached");
    }
    EXPECT_EQ(NO_ERROR, protect(w, w.base, 1, kR | kX), "execute cached");
    EXPECT_TRUE(check_page(w, w.base, b.pa, kR | kX), "executable");

    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 4), "unmap");
    END_TEST;
}

// Part of a large page cannot be protected; all of it can.
bool protect_large_page(const window &w, size_t size) {
    BEGIN_TEST;
    large_backing big;
    big.get(size);
    backing small;
    ASSERT_TRUE(small.alloc(1), "backing");

    const vaddr_t large = w.base + size;
    const uint large_count = size / PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, large - PAGE_SIZE, small.pa, 1), "map page before");
    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count, big.flags), "map large page");

    EXPECT_EQ(ERR_NOT_SUPPORTED, protect(w, large - PAGE_SIZE, 2, kR), "protect into the large page");
    EXPECT_EQ(ERR_NOT_SUPPORTED, protect(w, large + PAGE_SIZE, 1, kR), "protect inside");
    EXPECT_EQ(ERR_NOT_SUPPORTED, protect(w, large + size - PAGE_SIZE, 2, kR), "protect out of the end");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa, kRW), "page before unchanged");
    EXPECT_TRUE(check_page(w, large, big.pa, big.flags), "large page unchanged");

    EXPECT_EQ(NO_ERROR, protect(w, large - PAGE_SIZE, large_count + 1, kR), "protect both");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa, kR), "page before");
    for (size_t off = 0; off < size; off += size / 4) {
        EXPECT_TRUE(check_page(w, large + off, big.pa + off, big.with(kR)), "large page");
    }
    EXPECT_EQ(NO_ERROR, protect(w, large, large_count, kRW), "protect the large page alone");
    EXPECT_TRUE(check_page(w, large + size - PAGE_SIZE, big.pa + size - PAGE_SIZE, big.flags), "large page");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa, kR), "page before untouched");

    EXPECT_EQ(NO_ERROR, unmap(w, large - PAGE_SIZE, large_count + 1), "unmap");
    END_TEST;
}

bool contract_return_values() { return in_each_window(return_values); }
bool contract_query_out_params() { return in_each_window(query_out_params); }
bool contract_count_zero() { return in_each_window(count_zero); }
bool contract_collision() { return in_each_window(collision); }
#if LK_DEBUGLEVEL > 0
bool contract_out_of_tables() { return in_each_window(out_of_tables); }
#endif
bool contract_large_page_partial_unmap() {
    return in_each_window([](const window &w) { return for_each_large_page(w, large_page_partial_unmap); });
}
bool contract_large_page_collision() {
    return in_each_window([](const window &w) { return for_each_large_page(w, large_page_collision); });
}
bool contract_large_page_rollback() {
    return in_each_window([](const window &w) { return for_each_large_page(w, large_page_rollback); });
}
bool contract_bad_args() { return in_each_window(bad_args); }
bool contract_past_the_end() { return in_each_window(past_the_end); }
bool contract_cache_policies() { return in_each_window(cache_policies); }
bool contract_protect_permissions() { return in_each_window(protect_permissions); }
bool contract_protect_holes() { return in_each_window(protect_holes); }
bool contract_protect_bad_args() { return in_each_window(protect_bad_args); }
bool contract_protect_execute_uncached() { return in_each_window(protect_execute_uncached); }
bool contract_protect_large_page() {
    return in_each_window([](const window &w) { return for_each_large_page(w, protect_large_page); });
}

// The kernel's own mappings: a page of the physmap collides like any other,
// and where the physmap is made of large pages, part of one cannot be
// unmapped. Uses the physmap alias of a buffer this test owns.
bool contract_physmap() {
    BEGIN_TEST;
    vmm_aspace_t *kas = vmm_get_kernel_aspace();

    backing b;
#if IS_64BIT
    // a 2MB aligned buffer, so a large page of the physmap covers only it
    if (!b.alloc(512, 21)) {
        unittest_printf(" (no 2MB aligned backing)");
        END_TEST;
    }
#else
    ASSERT_TRUE(b.alloc(2), "backing");
#endif
    backing other;
    ASSERT_TRUE(other.alloc(1), "backing");
    const vaddr_t kva = (vaddr_t)paddr_to_kvaddr(b.pa);
    ASSERT_NE(0UL, kva, "in the physmap");

    paddr_t pa = 0;
    EXPECT_EQ(NO_ERROR, query(kas, kva + PAGE_SIZE, &pa, nullptr), "query");
    EXPECT_EQ(b.pa + PAGE_SIZE, pa, "paddr");

    status_t err = map(kas, kva + PAGE_SIZE, other.pa, 1, kRW);
    EXPECT_EQ(ERR_ALREADY_EXISTS, err, "map over the physmap");
    EXPECT_EQ(NO_ERROR, query(kas, kva + PAGE_SIZE, &pa, nullptr), "query");
    EXPECT_EQ(b.pa + PAGE_SIZE, pa, "physmap intact");

    uint orig = 0;
    ASSERT_EQ(NO_ERROR, query(kas, kva + PAGE_SIZE, nullptr, &orig), "query");
    uint flags = 0;
    err = protect(kas, kva + PAGE_SIZE, 1, kR);
#if (ARCH_X86 && !X86_LEGACY) || ARCH_RISCV || ARCH_ARM
    // large or fixed pages there; arm64 uses both, depending on the granule
    // and where the kernel image ends, and a legacy x86 build small pages
    EXPECT_EQ(ERR_NOT_SUPPORTED, err, "protect part of the physmap");
#endif
    if (err == ERR_NOT_SUPPORTED) {
        EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(kas, kva + PAGE_SIZE, 1), "unmap part of the physmap");
        EXPECT_EQ(NO_ERROR, query(kas, kva + PAGE_SIZE, &pa, &flags), "query");
        EXPECT_EQ(b.pa + PAGE_SIZE, pa, "physmap still intact");
        EXPECT_EQ(orig, flags, "physmap flags intact");
    } else {
        // a small page: it can be made read only and back
        EXPECT_EQ(NO_ERROR, err, "protect a physmap page");
        EXPECT_EQ(NO_ERROR, query(kas, kva + PAGE_SIZE, &pa, &flags), "query");
        EXPECT_EQ(b.pa + PAGE_SIZE, pa, "physmap paddr");
        EXPECT_EQ(query_flags((orig & ~ARCH_MMU_FLAG_PERM_RWX_MASK) | kR), flags, "read only");
        EXPECT_EQ(NO_ERROR, protect(kas, kva + PAGE_SIZE, 1, orig & ARCH_MMU_FLAG_PERM_RWX_MASK), "restore");
        EXPECT_EQ(NO_ERROR, query(kas, kva + PAGE_SIZE, nullptr, &flags), "query");
        EXPECT_EQ(orig, flags, "restored");
    }
#if ARCH_X86_64
    // the 2MB page of the physmap covering the buffer, as a whole
    EXPECT_EQ(NO_ERROR, protect(kas, kva, 512, kR), "protect a 2MB physmap page");
    EXPECT_EQ(NO_ERROR, query(kas, kva + 0x12345, &pa, &flags), "query");
    EXPECT_EQ(b.pa + 0x12345, pa, "paddr");
    EXPECT_EQ(query_flags((orig & ~ARCH_MMU_FLAG_PERM_RWX_MASK) | kR), flags, "read only");
    EXPECT_EQ(NO_ERROR, protect(kas, kva, 512, orig & ARCH_MMU_FLAG_PERM_RWX_MASK), "restore");
    EXPECT_EQ(NO_ERROR, query(kas, kva + 0x12345, nullptr, &flags), "query");
    EXPECT_EQ(orig, flags, "restored");
#endif
    END_TEST;
}

// The last pages of the kernel aspace, where on most arches the end of a
// range wraps to zero. The vmm never hands them out, so where the platform's
// own mappings do not reach them they are mapped, protected and unmapped in
// ranges that end exactly at the top; where they do, a map there collides
// and leaves what is mapped alone.
bool contract_kernel_top() {
    BEGIN_TEST;
    vmm_aspace_t *kas = vmm_get_kernel_aspace();
    backing b;
    ASSERT_TRUE(b.alloc(2), "backing");
    const vaddr_t last = KERNEL_ASPACE_BASE + (KERNEL_ASPACE_SIZE - PAGE_SIZE);
    const vaddr_t va = last - PAGE_SIZE;

    paddr_t pa = 0;
    uint flags = 0;
    if (query(kas, last, &pa, &flags) == NO_ERROR) {
        unittest_printf(" (top page already mapped)");
        EXPECT_EQ(ERR_ALREADY_EXISTS, map(kas, last, b.pa, 1, kRW), "map over the top page");
        paddr_t pa2 = 0;
        uint flags2 = 0;
        EXPECT_EQ(NO_ERROR, query(kas, last + (PAGE_SIZE - 1), &pa2, &flags2), "query the last byte");
        EXPECT_EQ(pa + (PAGE_SIZE - 1), pa2, "top page as it was");
        EXPECT_EQ(flags, flags2, "flags as they were");
        END_TEST;
    }
    if (query(kas, va, nullptr, nullptr) == NO_ERROR) {
        unittest_printf(" (page below the top already mapped)");
        END_TEST;
    }
    table_tracker tables;

    ASSERT_EQ(NO_ERROR, map(kas, va, b.pa, 2, kRW), "map the top two pages");
    EXPECT_EQ(NO_ERROR, query(kas, last + (PAGE_SIZE - 1), &pa, &flags), "query the last byte");
    EXPECT_EQ(b.pa + PAGE_SIZE + (PAGE_SIZE - 1), pa, "paddr");
    EXPECT_EQ(query_flags(kRW), flags, "flags");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(kas, last, b.pa, 1, kRW), "map over the top page");

    EXPECT_EQ(NO_ERROR, protect(kas, va, 2, kR), "protect to the top");
    EXPECT_EQ(NO_ERROR, query(kas, last, &pa, &flags), "query");
    EXPECT_EQ(query_flags(kR), flags, "read only");
    EXPECT_EQ(NO_ERROR, protect(kas, last, 1, kRW), "protect the top page");
    EXPECT_EQ(NO_ERROR, query(kas, va, nullptr, &flags), "query");
    EXPECT_EQ(query_flags(kR), flags, "page below untouched");

    EXPECT_EQ(NO_ERROR, unmap(kas, last, 1), "unmap the top page");
    EXPECT_EQ(ERR_NOT_FOUND, query(kas, last, nullptr, nullptr), "gone");
    EXPECT_EQ(NO_ERROR, query(kas, va, &pa, nullptr), "page below still there");
    EXPECT_EQ(b.pa, pa, "paddr");
    EXPECT_EQ(NO_ERROR, unmap(kas, va, 2), "unmap to the top, over the hole");
    EXPECT_EQ(ERR_NOT_FOUND, query(kas, va, nullptr, nullptr), "gone");
    EXPECT_EQ(NO_ERROR, unmap(kas, va, 2), "unmap nothing at the top");
    EXPECT_EQ(NO_ERROR, protect(kas, va, 2, kR), "protect nothing at the top");
    if (!kKernelTablesPermanent) {
        EXPECT_EQ(0L, tables.live(), "every table freed");
    }
    END_TEST;
}

// out_of_tables goes first: where the kernel's tables are never freed it
// needs a window no other test has mapped into yet.
BEGIN_TEST_CASE(arch_mmu_contract)
#if LK_DEBUGLEVEL > 0
RUN_TEST(contract_out_of_tables);
#endif
RUN_TEST(contract_return_values);
RUN_TEST(contract_query_out_params);
RUN_TEST(contract_count_zero);
RUN_TEST(contract_bad_args);
RUN_TEST(contract_past_the_end);
RUN_TEST(contract_collision);
RUN_TEST(contract_large_page_collision);
RUN_TEST(contract_large_page_partial_unmap);
RUN_TEST(contract_large_page_rollback);
RUN_TEST(contract_cache_policies);
RUN_TEST(contract_protect_permissions);
RUN_TEST(contract_protect_holes);
RUN_TEST(contract_protect_bad_args);
RUN_TEST(contract_protect_execute_uncached);
RUN_TEST(contract_protect_large_page);
RUN_TEST(contract_physmap);
RUN_TEST(contract_kernel_top);
END_TEST_CASE(arch_mmu_contract)

} // namespace

#endif // ARCH_HAS_MMU
