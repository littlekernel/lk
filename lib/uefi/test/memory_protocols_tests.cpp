/*
 * Copyright (c) 2026 Kelvin Zhang
 *
 * Use of this source code is governed by a MIT-style license that can be
 * found in the LICENSE file or at https://opensource.org/licenses/MIT
 */

#include <arch/mmu.h>
#include <kernel/vm.h>
#include <lib/unittest.h>
#include <lk/err.h>
#include <string.h>

#include "../defer.h"
#include "../memory_protocols.h"
#include "../uefi_platform.h"

namespace {

size_t free_page_count() {
    size_t count = 0;
    pmm_arena_t *arena;
    list_for_every_entry(get_arena_list(), arena, pmm_arena_t, node) {
        count += arena->free_count;
    }
    return count;
}

bool fixed_address_allocation() {
    BEGIN_TEST;
    reset_heap();
    const size_t before = free_page_count();
    void *allocation = nullptr;
    struct list_node blocker;
    list_initialize(&blocker);
    DEFER {
        if (allocation) free_pages(allocation, 3);
        pmm_free(&blocker);
        reset_heap();
    };

    auto *aspace = set_boot_aspace();
    ASSERT_NONNULL(aspace, "UEFI address space");

    // Find four consecutive free pages, then choose a three-page subrange that
    // is deliberately not aligned for the fallback allocator. Returning this
    // exact address therefore proves that the fixed-address path succeeded.
    struct list_node probe;
    list_initialize(&probe);
    DEFER { pmm_free(&probe); };
    paddr_t probe_pa = 0;
    ASSERT_EQ(4u, pmm_alloc_contiguous(4, PAGE_SIZE_SHIFT, &probe_pa, &probe),
              "find free physical range");
    const paddr_t requested =
            IS_ALIGNED(probe_pa, 2 * PAGE_SIZE) ? probe_pa + PAGE_SIZE : probe_pa;
    ASSERT_FALSE(IS_ALIGNED(requested, 2 * PAGE_SIZE),
                 "requested range bypasses fallback alignment");
    ASSERT_EQ(4u, pmm_free(&probe), "release probe range");

    allocation = alloc_page(reinterpret_cast<void *>(requested), 3 * PAGE_SIZE,
                            PAGE_SIZE_SHIFT + 1);
    ASSERT_EQ(reinterpret_cast<void *>(requested), allocation, "fixed physical address");
    for (size_t i = 0; i < 3; ++i) {
        paddr_t mapped = 0;
        uint flags = 0;
        vmm_aspace_lock(aspace);
        const status_t status = arch_mmu_query(
                &aspace->arch_aspace, reinterpret_cast<vaddr_t>(allocation) + i * PAGE_SIZE,
                &mapped, &flags);
        vmm_aspace_unlock(aspace);
        ASSERT_EQ(NO_ERROR, status, "fixed range is mapped");
        ASSERT_EQ(requested + i * PAGE_SIZE, mapped, "identity mapping");
        ASSERT_EQ(ARCH_MMU_FLAG_PERM_RWX_MASK, flags & ARCH_MMU_FLAG_PERM_RWX_MASK,
                  "fixed range has read, write, and execute permissions");
    }
    memset(allocation, 0xa5, 3 * PAGE_SIZE);
    ASSERT_EQ(EFI_STATUS_SUCCESS, free_pages(allocation, 3), "free fixed range");
    allocation = nullptr;

    // Occupying the middle page forces pmm_alloc_range() to return a partial
    // prefix. The prefix must be returned before falling back elsewhere.
    ASSERT_EQ(1u, pmm_alloc_range(requested + PAGE_SIZE, 1, &blocker),
              "reserve collision page");
    allocation = alloc_page(reinterpret_cast<void *>(requested), 3 * PAGE_SIZE,
                            PAGE_SIZE_SHIFT + 1);
    ASSERT_NONNULL(allocation, "fallback allocation");
    ASSERT_NE(reinterpret_cast<void *>(requested), allocation,
              "partial fixed range must fall back");
    ASSERT_EQ(EFI_STATUS_SUCCESS, free_pages(allocation, 3), "free fallback range");
    allocation = nullptr;
    ASSERT_EQ(1u, pmm_free(&blocker), "release collision page");

    // The physical range is free, but its virtual address is already reserved.
    // Mapping failure must return those physical pages without falling back.
    {
        ASSERT_EQ(NO_ERROR,
                  vmm_reserve_space(aspace, "uefi_test_collision", 3 * PAGE_SIZE, requested),
                  "reserve fixed virtual range");
        DEFER { vmm_free_region(aspace, requested); };
        const size_t before_failed_mapping = free_page_count();
        allocation = alloc_page(reinterpret_cast<void *>(requested), 3 * PAGE_SIZE,
                                PAGE_SIZE_SHIFT + 1);
        ASSERT_NULL(allocation, "occupied virtual range rejects mapping");
        ASSERT_EQ(before_failed_mapping, free_page_count(),
                  "failed mapping returns physical pages");
    }

    reset_heap();
    ASSERT_EQ(before, free_page_count(), "allocation paths leak no PMM pages");
    END_TEST;
}

}  // namespace

BEGIN_TEST_CASE(uefi_memory_protocols)
RUN_TEST(fixed_address_allocation)
END_TEST_CASE(uefi_memory_protocols)
