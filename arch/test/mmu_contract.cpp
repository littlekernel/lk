/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if ARCH_HAS_MMU

// Checks every clause of the contract in arch/include/arch/mmu.h, in a user
// aspace where the arch has them and in a window of the kernel aspace
// everywhere.

#include <arch/mmu.h>

#include <kernel/vm.h>
#include <lib/unittest.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lktl/auto_call.h>

namespace {

constexpr uint kR = ARCH_MMU_FLAG_PERM_READ;
constexpr uint kW = ARCH_MMU_FLAG_PERM_WRITE;
constexpr uint kX = ARCH_MMU_FLAG_PERM_EXECUTE;
constexpr uint kRW = kR | kW;

// Per arch geometry: the span one leaf table covers, and the smallest large
// page map may create (0 where map never makes one).
#if ARCH_ARM64
constexpr size_t kLeafSpan = 1UL << (2 * PAGE_SIZE_SHIFT - 3);
constexpr size_t kLargePage = kLeafSpan;
#elif ARCH_RISCV
constexpr size_t kLeafSpan = PAGE_SIZE * (PAGE_SIZE / sizeof(ulong));
constexpr size_t kLargePage = kLeafSpan;
#elif ARCH_ARM
constexpr size_t kLeafSpan = 1UL << 20;
constexpr size_t kLargePage = kLeafSpan;
#elif ARCH_X86_64
constexpr size_t kLeafSpan = 2UL << 20;
constexpr size_t kLargePage = 0;
#elif ARCH_X86_32
constexpr size_t kLeafSpan = 4UL << 20;
constexpr size_t kLargePage = 0;
#elif ARCH_M68K
constexpr size_t kLeafSpan = 4UL << 20;
constexpr size_t kLargePage = 0;
#else
#error "add the arch's table geometry"
#endif

// Room for a large page with a page in front of it, and for a range across a
// leaf table boundary with fresh tables on both sides.
#if IS_64BIT
constexpr size_t kWindowSize = 1UL << 30;
constexpr uint8_t kWindowAlign = 30;
#else
constexpr size_t kWindowSize = 8UL << 20;
constexpr uint8_t kWindowAlign = 22;
#endif
static_assert(kWindowSize >= 2 * kLeafSpan && kWindowSize >= 2 * kLargePage, "");

constexpr uint log2_of(size_t v) { return v <= 1 ? 0 : 1 + log2_of(v / 2); }

// A place to map into: a range of address space nothing else uses, in a
// fresh user aspace or in the kernel's.
struct window {
    const char *name;
    vmm_aspace_t *vmm;
    arch_aspace_t *aspace;
    vaddr_t base;
    size_t size;
    uint user;             // ARCH_MMU_FLAG_PERM_USER in a user aspace
    bool tables_permanent; // tables linked here are never freed
    vaddr_t outside;       // an address outside the aspace
};

// what a query returns for a page mapped with these flags: without NX every
// page can be executed
uint expected_flags(uint flags) {
    if (!arch_mmu_supports_nx_mappings()) {
        flags |= kX;
    }
    return flags;
}

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

// arch_mmu_map with the window's USER bit added
status_t map(const window &w, vaddr_t va, paddr_t pa, uint count, uint flags = kRW) {
    return arch_mmu_map(w.aspace, va, pa, count, flags | w.user);
}

status_t unmap(const window &w, vaddr_t va, uint count) {
    return arch_mmu_unmap(w.aspace, va, count);
}

status_t protect(const window &w, vaddr_t va, uint count, uint flags) {
    return arch_mmu_protect(w.aspace, va, count, flags);
}

bool is_mapped(const window &w, vaddr_t va) {
    return arch_mmu_query(w.aspace, va, nullptr, nullptr) == NO_ERROR;
}

// va maps pa with flags (the window's USER bit added)
bool check_page(const window &w, vaddr_t va, paddr_t pa, uint flags = kRW) {
    BEGIN_TEST;
    paddr_t got_pa = 0;
    uint got_flags = 0;
    ASSERT_EQ(NO_ERROR, arch_mmu_query(w.aspace, va, &got_pa, &got_flags), "query");
    EXPECT_EQ(pa, got_pa, "paddr");
    const uint cache = flags & ARCH_MMU_FLAG_CACHE_MASK;
    EXPECT_TRUE(cache_readback_ok(cache, got_flags & ARCH_MMU_FLAG_CACHE_MASK), "cache policy");
    EXPECT_EQ(expected_flags((flags & ~ARCH_MMU_FLAG_CACHE_MASK) | w.user),
              got_flags & ~ARCH_MMU_FLAG_CACHE_MASK, "flags");
    END_TEST;
}

bool check_unmapped(const window &w, vaddr_t va, uint count) {
    BEGIN_TEST;
    for (uint i = 0; i < count; i++) {
        paddr_t pa;
        EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(w.aspace, va + i * PAGE_SIZE, &pa, nullptr), "unmapped");
    }
    END_TEST;
}

// physical pages for a test, contiguous and aligned as asked
struct backing {
    struct list_node pages = LIST_INITIAL_VALUE(pages);
    paddr_t pa = 0;
    size_t count = 0;

    bool alloc(size_t n, uint8_t align_log2 = PAGE_SIZE_SHIFT) {
        count = pmm_alloc_contiguous(n, align_log2, &pa, &pages);
        return count == n;
    }
    ~backing() { pmm_free(&pages); }
};

// runs fn in a user aspace window and in a kernel aspace window
template <typename F>
bool in_each_window(F fn) {
    BEGIN_TEST;

    if (arch_mmu_supports_user_aspaces()) {
        window w = {};
        w.name = "user";
        ASSERT_EQ(NO_ERROR, vmm_create_aspace(&w.vmm, "mmu_contract", 0), "create aspace");
        auto cleanup = lk::make_auto_call([&]() { vmm_free_aspace(w.vmm); });
        ASSERT_EQ(NO_ERROR, vmm_reserve_range(w.vmm, "window", kWindowSize, kWindowAlign, &w.base), "reserve");
        w.aspace = &w.vmm->arch_aspace;
        w.size = kWindowSize;
        w.user = ARCH_MMU_FLAG_PERM_USER;
        w.outside = KERNEL_ASPACE_BASE;
        if (!fn(w)) {
            unittest_printf(" (in the user window)");
            all_ok = false;
        }
        unmap(w, w.base, w.size / PAGE_SIZE);
    }

    {
        window w = {};
        w.name = "kernel";
        w.vmm = vmm_get_kernel_aspace();
        ASSERT_EQ(NO_ERROR, vmm_reserve_range(w.vmm, "window", kWindowSize, kWindowAlign, &w.base), "reserve");
        auto cleanup = lk::make_auto_call([&]() { vmm_free_region(w.vmm, w.base); });
        w.aspace = &w.vmm->arch_aspace;
        w.size = kWindowSize;
        w.user = 0;
#if ARCH_X86_32
        w.tables_permanent = true; // user page directories copy the kernel's
#endif
        w.outside = KERNEL_ASPACE_BASE - PAGE_SIZE;
        if (!fn(w)) {
            unittest_printf(" (in the kernel window)");
            all_ok = false;
        }
        unmap(w, w.base, w.size / PAGE_SIZE);
    }

    END_TEST;
}

// Map and unmap return exactly NO_ERROR, and an unmap of nothing succeeds.
bool return_values(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(3), "backing");
    EXPECT_EQ(NO_ERROR, arch_mmu_map(w.aspace, w.base, b.pa, 3, kRW | w.user), "map");
    for (uint i = 0; i < 3; i++) {
        EXPECT_TRUE(check_page(w, w.base + i * PAGE_SIZE, b.pa + i * PAGE_SIZE), "mapped");
    }
    EXPECT_EQ(NO_ERROR, arch_mmu_unmap(w.aspace, w.base, 3), "unmap");
    EXPECT_TRUE(check_unmapped(w, w.base, 3), "gone");
    EXPECT_EQ(NO_ERROR, arch_mmu_unmap(w.aspace, w.base, 3), "unmap nothing");
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
    EXPECT_EQ(NO_ERROR, arch_mmu_query(w.aspace, w.base, nullptr, &flags), "no paddr");
    EXPECT_EQ(expected_flags(kRW | w.user), flags, "flags");
    EXPECT_EQ(NO_ERROR, arch_mmu_query(w.aspace, w.base + 0x123, &pa, nullptr), "no flags");
    EXPECT_EQ(b.pa + 0x123, pa, "paddr with offset");
    EXPECT_EQ(NO_ERROR, arch_mmu_query(w.aspace, w.base, nullptr, nullptr), "neither");
    EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(w.aspace, w.base + PAGE_SIZE, nullptr, nullptr), "unmapped, neither");
    EXPECT_EQ(ERR_OUT_OF_RANGE, arch_mmu_query(w.aspace, w.outside, nullptr, nullptr), "outside, neither");

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
// aspace as it was. The range crosses a leaf table boundary, so the cut can
// fall before any table, between tables of one page's chain, or after the
// first page went in. A table left linked by a failed attempt would make one
// table fewer than needed enough, which the attempt at needed - 1 checks.
bool out_of_tables(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(4), "backing");
    vm_ptable_track(true);
    auto budget_cleanup = lk::make_auto_call([]() { vm_ptable_track(false); });

    const vaddr_t va = w.base + kLeafSpan - 2 * PAGE_SIZE;
    constexpr int kMaxTables = 16;

    // how many tables the range needs, from a clean window
    int needed = -1;
    for (int budget = 0; budget <= kMaxTables; budget++) {
        vm_ptable_budget_set(budget);
        status_t err = map(w, va, b.pa, 4);
        vm_ptable_budget_set(-1);
        if (err == NO_ERROR) {
            needed = budget;
            break;
        }
        EXPECT_EQ(ERR_NO_MEMORY, err, "map short of tables");
        EXPECT_TRUE(check_unmapped(w, va, 4), "nothing mapped");
        if (!all_ok) {
            break;
        }
    }
    ASSERT_LE(0, needed, "some budget is enough");
    EXPECT_EQ(NO_ERROR, unmap(w, va, 4), "unmap");
    if (needed == 0) {
        unittest_printf(" (every table already there)");
        END_TEST;
    }
    if (w.tables_permanent) {
        // the first pass left the tables in place for good
        END_TEST;
    }

    // every smaller budget again, each followed by the one just short
    for (int budget = 0; budget < needed; budget++) {
        vm_ptable_budget_set(budget);
        EXPECT_EQ(ERR_NO_MEMORY, map(w, va, b.pa, 4), "map short of tables");
        vm_ptable_budget_set(needed - 1);
        status_t err = map(w, va, b.pa, 4);
        vm_ptable_budget_set(-1);
        EXPECT_EQ(ERR_NO_MEMORY, err, "one short after a failed map");
        EXPECT_TRUE(check_unmapped(w, va, 4), "nothing mapped");
        if (err == NO_ERROR) {
            unittest_printf(" (budget %d left tables linked)", budget);
            unmap(w, va, 4);
            break;
        }
    }

    // and the aspace still maps normally, needing as many as before
    vm_ptable_budget_set(needed);
    status_t err = map(w, va, b.pa, 4);
    vm_ptable_budget_set(-1);
    EXPECT_EQ(NO_ERROR, err, "map with the same budget");
    for (uint i = 0; i < 4; i++) {
        EXPECT_TRUE(check_page(w, va + i * PAGE_SIZE, b.pa + i * PAGE_SIZE), "mapped");
    }
    EXPECT_EQ(NO_ERROR, unmap(w, va, 4), "unmap");
    END_TEST;
}
#endif

// Only the ends of a range can cut a large page, and unmap refuses that
// before it changes anything.
bool large_page_partial_unmap(const window &w) {
    BEGIN_TEST;
    if (kLargePage == 0) {
        END_TEST;
    }
    backing big;
    if (!big.alloc(kLargePage / PAGE_SIZE, log2_of(kLargePage))) {
        unittest_printf(" (no aligned backing for a large page)");
        END_TEST;
    }
    backing small;
    ASSERT_TRUE(small.alloc(1), "backing");

    const vaddr_t large = w.base + kLargePage;
    const uint large_count = kLargePage / PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, large - PAGE_SIZE, small.pa, 1), "map page before");
    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count), "map large page");

    EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(w, large - PAGE_SIZE, 2), "unmap into the large page");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa), "page before intact");
    EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(w, large + PAGE_SIZE, 1), "unmap inside");
    EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(w, large + kLargePage - PAGE_SIZE, 2), "unmap out of the end");
    EXPECT_EQ(ERR_NOT_SUPPORTED, unmap(w, large - PAGE_SIZE, large_count), "unmap all but the last page");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa), "page before still intact");
    for (size_t off = 0; off < kLargePage; off += kLargePage / 4) {
        EXPECT_TRUE(check_page(w, large + off, big.pa + off), "large page intact");
    }

    EXPECT_EQ(NO_ERROR, unmap(w, large - PAGE_SIZE, large_count + 1), "unmap both");
    EXPECT_TRUE(check_unmapped(w, large - PAGE_SIZE, 2), "gone");
    EXPECT_TRUE(check_unmapped(w, large + kLargePage - PAGE_SIZE, 1), "gone at the end");
    END_TEST;
}

// A page inside a large page collides like any other, and so does a large
// map over a table holding a page. An empty table is no collision.
bool large_page_collision(const window &w) {
    BEGIN_TEST;
    if (kLargePage == 0) {
        END_TEST;
    }
    backing big;
    if (!big.alloc(kLargePage / PAGE_SIZE, log2_of(kLargePage))) {
        unittest_printf(" (no aligned backing for a large page)");
        END_TEST;
    }
    backing small;
    ASSERT_TRUE(small.alloc(2), "backing");

    const vaddr_t large = w.base + kLargePage;
    const uint large_count = kLargePage / PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count), "map large page");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, large + PAGE_SIZE, small.pa, 1), "map inside");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, large - PAGE_SIZE, small.pa, 2), "map into it");
    EXPECT_TRUE(check_unmapped(w, large - PAGE_SIZE, 1), "page before rolled back");
    EXPECT_TRUE(check_page(w, large + PAGE_SIZE, big.pa + PAGE_SIZE), "large page intact");
    EXPECT_EQ(NO_ERROR, unmap(w, large, large_count), "unmap large page");

    ASSERT_EQ(NO_ERROR, map(w, large + PAGE_SIZE, small.pa, 1), "map a page");
    EXPECT_EQ(ERR_ALREADY_EXISTS, map(w, large, big.pa, large_count), "map large over it");
    EXPECT_TRUE(check_page(w, large + PAGE_SIZE, small.pa), "page intact");
    EXPECT_TRUE(check_unmapped(w, large, 1), "large map rolled back");
    EXPECT_EQ(NO_ERROR, unmap(w, large + PAGE_SIZE, 1), "unmap the page");

    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count), "map large where the table was");
    for (size_t off = 0; off < kLargePage; off += kLargePage / 4) {
        EXPECT_TRUE(check_page(w, large + off + 0x10, big.pa + off + 0x10), "mapped");
    }
    EXPECT_EQ(NO_ERROR, unmap(w, large, large_count), "unmap");
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

    const uint sets[] = { kR, kR | kX, kR | kW | kX, kR | kW, kR };
    for (const uint perms : sets) {
        EXPECT_EQ(NO_ERROR, protect(w, va, 4, perms), "protect");
        for (uint i = 0; i < 4; i++) {
            EXPECT_TRUE(check_page(w, va + i * PAGE_SIZE, b.pa + i * PAGE_SIZE, perms), "new permissions");
        }
    }
    // the same set again changes nothing
    EXPECT_EQ(NO_ERROR, protect(w, va, 4, kR), "protect to the same");
    EXPECT_TRUE(check_page(w, va, b.pa, kR), "unchanged");

    EXPECT_EQ(NO_ERROR, unmap(w, va, 4), "unmap");
    END_TEST;
}

// Holes are stepped over and stay holes, a table's worth of them included.
bool protect_holes(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(3), "backing");
    ASSERT_EQ(NO_ERROR, map(w, w.base, b.pa, 1), "map page 0");
    ASSERT_EQ(NO_ERROR, map(w, w.base + 2 * PAGE_SIZE, b.pa + PAGE_SIZE, 1), "map page 2");
    // past a leaf table's worth of nothing
    const vaddr_t far = w.base + 2 * kLeafSpan - PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, far, b.pa + 2 * PAGE_SIZE, 1), "map a far page");

    EXPECT_EQ(NO_ERROR, protect(w, w.base, (far - w.base) / PAGE_SIZE + 1, kR), "protect around holes");
    EXPECT_TRUE(check_page(w, w.base, b.pa, kR), "page 0");
    EXPECT_TRUE(check_page(w, w.base + 2 * PAGE_SIZE, b.pa + PAGE_SIZE, kR), "page 2");
    EXPECT_TRUE(check_page(w, far, b.pa + 2 * PAGE_SIZE, kR), "far page");
    EXPECT_TRUE(check_unmapped(w, w.base + PAGE_SIZE, 1), "hole 1 stays");
    EXPECT_TRUE(check_unmapped(w, w.base + 3 * PAGE_SIZE, 1), "hole 3 stays");
    EXPECT_TRUE(check_unmapped(w, w.base + kLeafSpan, 1), "hole in the next table stays");
    EXPECT_EQ(NO_ERROR, protect(w, w.base + 4 * PAGE_SIZE, 4, kR), "protect nothing");

    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 3), "unmap");
    EXPECT_EQ(NO_ERROR, unmap(w, far, 1), "unmap far page");
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
// changes; where the entry cannot be uncached it is allowed.
bool protect_execute_uncached(const window &w) {
    BEGIN_TEST;
    backing b;
    ASSERT_TRUE(b.alloc(3), "backing");
    ASSERT_EQ(NO_ERROR, map(w, w.base, b.pa, 1, kRW), "map cached");
    ASSERT_EQ(NO_ERROR, map(w, w.base + PAGE_SIZE, b.pa + PAGE_SIZE, 1, kRW | ARCH_MMU_FLAG_UNCACHED), "map uncached");
    ASSERT_EQ(NO_ERROR, map(w, w.base + 2 * PAGE_SIZE, b.pa + 2 * PAGE_SIZE, 1, kRW | ARCH_MMU_FLAG_UNCACHED_DEVICE),
              "map device");

    uint flags = 0;
    ASSERT_EQ(NO_ERROR, arch_mmu_query(w.aspace, w.base + PAGE_SIZE, nullptr, &flags), "query");
    if ((flags & ARCH_MMU_FLAG_CACHE_MASK) != ARCH_MMU_FLAG_CACHED) {
        EXPECT_EQ(ERR_INVALID_ARGS, protect(w, w.base, 3, kR | kX), "execute over uncached");
        EXPECT_TRUE(check_page(w, w.base, b.pa, kRW), "cached page unchanged");
        EXPECT_EQ(ERR_INVALID_ARGS, protect(w, w.base + 2 * PAGE_SIZE, 1, kR | kX), "execute device");
        EXPECT_EQ(NO_ERROR, protect(w, w.base + PAGE_SIZE, 2, kR), "read only uncached");
        EXPECT_TRUE(check_page(w, w.base + PAGE_SIZE, b.pa + PAGE_SIZE, kR | ARCH_MMU_FLAG_UNCACHED), "uncached kept");
        EXPECT_TRUE(check_page(w, w.base + 2 * PAGE_SIZE, b.pa + 2 * PAGE_SIZE, kR | ARCH_MMU_FLAG_UNCACHED_DEVICE),
                    "device kept");
    } else {
        EXPECT_EQ(NO_ERROR, protect(w, w.base, 3, kR | kX), "every page is cached");
    }
    EXPECT_EQ(NO_ERROR, protect(w, w.base, 1, kR | kX), "execute cached");
    EXPECT_TRUE(check_page(w, w.base, b.pa, kR | kX), "executable");

    EXPECT_EQ(NO_ERROR, unmap(w, w.base, 3), "unmap");
    END_TEST;
}

// Part of a large page cannot be protected; all of it can.
bool protect_large_page(const window &w) {
    BEGIN_TEST;
    if (kLargePage == 0) {
        END_TEST;
    }
    backing big;
    if (!big.alloc(kLargePage / PAGE_SIZE, log2_of(kLargePage))) {
        unittest_printf(" (no aligned backing for a large page)");
        END_TEST;
    }
    backing small;
    ASSERT_TRUE(small.alloc(1), "backing");

    const vaddr_t large = w.base + kLargePage;
    const uint large_count = kLargePage / PAGE_SIZE;
    ASSERT_EQ(NO_ERROR, map(w, large - PAGE_SIZE, small.pa, 1), "map page before");
    ASSERT_EQ(NO_ERROR, map(w, large, big.pa, large_count), "map large page");

    EXPECT_EQ(ERR_NOT_SUPPORTED, protect(w, large - PAGE_SIZE, 2, kR), "protect into the large page");
    EXPECT_EQ(ERR_NOT_SUPPORTED, protect(w, large + PAGE_SIZE, 1, kR), "protect inside");
    EXPECT_EQ(ERR_NOT_SUPPORTED, protect(w, large + kLargePage - PAGE_SIZE, 2, kR), "protect out of the end");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa, kRW), "page before unchanged");
    EXPECT_TRUE(check_page(w, large, big.pa, kRW), "large page unchanged");

    EXPECT_EQ(NO_ERROR, protect(w, large - PAGE_SIZE, large_count + 1, kR), "protect both");
    EXPECT_TRUE(check_page(w, large - PAGE_SIZE, small.pa, kR), "page before");
    for (size_t off = 0; off < kLargePage; off += kLargePage / 4) {
        EXPECT_TRUE(check_page(w, large + off, big.pa + off, kR), "large page");
    }
    EXPECT_EQ(NO_ERROR, protect(w, large, large_count, kRW), "protect the large page alone");
    EXPECT_TRUE(check_page(w, large + kLargePage - PAGE_SIZE, big.pa + kLargePage - PAGE_SIZE, kRW), "large page");
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
bool contract_large_page_partial_unmap() { return in_each_window(large_page_partial_unmap); }
bool contract_large_page_collision() { return in_each_window(large_page_collision); }
bool contract_bad_args() { return in_each_window(bad_args); }
bool contract_cache_policies() { return in_each_window(cache_policies); }
bool contract_protect_permissions() { return in_each_window(protect_permissions); }
bool contract_protect_holes() { return in_each_window(protect_holes); }
bool contract_protect_bad_args() { return in_each_window(protect_bad_args); }
bool contract_protect_execute_uncached() { return in_each_window(protect_execute_uncached); }
bool contract_protect_large_page() { return in_each_window(protect_large_page); }

// The kernel's own mappings: a page of the physmap collides like any other,
// and where the physmap is made of large pages, part of one cannot be
// unmapped. Uses the physmap alias of a buffer this test owns.
bool contract_physmap() {
    BEGIN_TEST;
    arch_aspace_t *kas = &vmm_get_kernel_aspace()->arch_aspace;

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
    EXPECT_EQ(NO_ERROR, arch_mmu_query(kas, kva + PAGE_SIZE, &pa, nullptr), "query");
    EXPECT_EQ(b.pa + PAGE_SIZE, pa, "paddr");

    status_t err = arch_mmu_map(kas, kva + PAGE_SIZE, other.pa, 1, kRW);
    EXPECT_EQ(ERR_ALREADY_EXISTS, err, "map over the physmap");
    EXPECT_EQ(NO_ERROR, arch_mmu_query(kas, kva + PAGE_SIZE, &pa, nullptr), "query");
    EXPECT_EQ(b.pa + PAGE_SIZE, pa, "physmap intact");

    uint orig = 0;
    ASSERT_EQ(NO_ERROR, arch_mmu_query(kas, kva + PAGE_SIZE, nullptr, &orig), "query");
    uint flags = 0;
    err = arch_mmu_protect(kas, kva + PAGE_SIZE, 1, kR);
#if (ARCH_X86 && !X86_LEGACY) || ARCH_RISCV || ARCH_ARM
    // large or fixed pages there; arm64 uses both, depending on the granule
    // and where the kernel image ends, and a legacy x86 build small pages
    EXPECT_EQ(ERR_NOT_SUPPORTED, err, "protect part of the physmap");
#endif
    if (err == ERR_NOT_SUPPORTED) {
        EXPECT_EQ(ERR_NOT_SUPPORTED, arch_mmu_unmap(kas, kva + PAGE_SIZE, 1), "unmap part of the physmap");
        EXPECT_EQ(NO_ERROR, arch_mmu_query(kas, kva + PAGE_SIZE, &pa, &flags), "query");
        EXPECT_EQ(b.pa + PAGE_SIZE, pa, "physmap still intact");
        EXPECT_EQ(orig, flags, "physmap flags intact");
    } else {
        // a small page: it can be made read only and back
        EXPECT_EQ(NO_ERROR, err, "protect a physmap page");
        EXPECT_EQ(NO_ERROR, arch_mmu_query(kas, kva + PAGE_SIZE, &pa, &flags), "query");
        EXPECT_EQ(b.pa + PAGE_SIZE, pa, "physmap paddr");
        EXPECT_EQ(expected_flags((orig & ~ARCH_MMU_FLAG_PERM_RWX_MASK) | kR), flags, "read only");
        EXPECT_EQ(NO_ERROR, arch_mmu_protect(kas, kva + PAGE_SIZE, 1, orig & ARCH_MMU_FLAG_PERM_RWX_MASK), "restore");
        EXPECT_EQ(NO_ERROR, arch_mmu_query(kas, kva + PAGE_SIZE, nullptr, &flags), "query");
        EXPECT_EQ(orig, flags, "restored");
    }
#if ARCH_X86_64
    // the 2MB page of the physmap covering the buffer, as a whole
    EXPECT_EQ(NO_ERROR, arch_mmu_protect(kas, kva, 512, kR), "protect a 2MB physmap page");
    EXPECT_EQ(NO_ERROR, arch_mmu_query(kas, kva + 0x12345, &pa, &flags), "query");
    EXPECT_EQ(b.pa + 0x12345, pa, "paddr");
    EXPECT_EQ(expected_flags((orig & ~ARCH_MMU_FLAG_PERM_RWX_MASK) | kR), flags, "read only");
    EXPECT_EQ(NO_ERROR, arch_mmu_protect(kas, kva, 512, orig & ARCH_MMU_FLAG_PERM_RWX_MASK), "restore");
    EXPECT_EQ(NO_ERROR, arch_mmu_query(kas, kva + 0x12345, nullptr, &flags), "query");
    EXPECT_EQ(orig, flags, "restored");
#endif
    END_TEST;
}

BEGIN_TEST_CASE(arch_mmu_contract)
RUN_TEST(contract_return_values);
RUN_TEST(contract_query_out_params);
RUN_TEST(contract_count_zero);
RUN_TEST(contract_bad_args);
RUN_TEST(contract_collision);
#if LK_DEBUGLEVEL > 0
RUN_TEST(contract_out_of_tables);
#endif
RUN_TEST(contract_large_page_collision);
RUN_TEST(contract_large_page_partial_unmap);
RUN_TEST(contract_cache_policies);
RUN_TEST(contract_protect_permissions);
RUN_TEST(contract_protect_holes);
RUN_TEST(contract_protect_bad_args);
RUN_TEST(contract_protect_execute_uncached);
RUN_TEST(contract_protect_large_page);
RUN_TEST(contract_physmap);
END_TEST_CASE(arch_mmu_contract)

} // namespace

#endif // ARCH_HAS_MMU
