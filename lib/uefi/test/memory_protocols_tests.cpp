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
        ASSERT_EQ(ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE,
                  flags & ARCH_MMU_FLAG_PERM_RWX_MASK, "fixed range is RW/NX");
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

bool allocate_pages_permissions() {
    BEGIN_TEST;
    DEFER { reset_heap(); };
    ASSERT_NONNULL(set_boot_aspace(), "UEFI address space");
    const EfiMemoryType types[] = {
            EFI_MEMORY_TYPE_LOADER_CODE, EFI_MEMORY_TYPE_BOOT_SERVICES_CODE,
            EFI_MEMORY_TYPE_RUNTIME_SERVICES_CODE, EFI_MEMORY_TYPE_LOADER_DATA,
            EFI_MEMORY_TYPE_BOOT_SERVICES_DATA, EFI_MEMORY_TYPE_RUNTIME_SERVICES_DATA,
            EFI_MEMORY_TYPE_CONVENTIONAL_MEMORY};
    for (const auto type : types) {
        EfiPhysicalAddr address = 0;
        DEFER {
            if (address) free_pages(reinterpret_cast<void *>(address), 1);
        };
        ASSERT_EQ(EFI_STATUS_SUCCESS,
                  allocate_pages(EFI_ALLOCATOR_TYPE_ALLOCATE_ANY_PAGES, type, 1, &address),
                  "allocate EFI page");
        auto *memory = reinterpret_cast<void *>(address);
        memset(memory, 0xa5, PAGE_SIZE);
        uint flags = 0;
        auto *aspace = set_boot_aspace();
        vmm_aspace_lock(aspace);
        const status_t result = arch_mmu_query(&aspace->arch_aspace, address, nullptr, &flags);
        vmm_aspace_unlock(aspace);
        ASSERT_EQ(NO_ERROR, result, "query EFI allocation");
        const uint expected = ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE |
                ((type == EFI_MEMORY_TYPE_LOADER_CODE ||
                  type == EFI_MEMORY_TYPE_BOOT_SERVICES_CODE ||
                  type == EFI_MEMORY_TYPE_RUNTIME_SERVICES_CODE)
                         ? ARCH_MMU_FLAG_PERM_EXECUTE : 0);
        ASSERT_EQ(expected, flags & ARCH_MMU_FLAG_PERM_RWX_MASK,
                  "only EFI code allocations are RWX");
        ASSERT_EQ(EFI_STATUS_SUCCESS, free_pages(memory, 1), "free EFI allocation");
        address = 0;
    }
    constexpr size_t kLargePages = 4 * 1024 * 1024 / PAGE_SIZE;
    EfiPhysicalAddr large_code = 0;
    DEFER {
        if (large_code) free_pages(reinterpret_cast<void *>(large_code), kLargePages);
    };
    ASSERT_EQ(EFI_STATUS_SUCCESS,
              allocate_pages(EFI_ALLOCATOR_TYPE_ALLOCATE_ANY_PAGES,
                             EFI_MEMORY_TYPE_LOADER_CODE, kLargePages, &large_code),
              "allocate large EFI code range");
    const size_t offsets[] = {0, 2 * 1024 * 1024, 4 * 1024 * 1024 - PAGE_SIZE};
    for (const size_t offset : offsets) {
        uint flags = 0;
        auto *aspace = set_boot_aspace();
        vmm_aspace_lock(aspace);
        const status_t result = arch_mmu_query(&aspace->arch_aspace, large_code + offset,
                                               nullptr, &flags);
        vmm_aspace_unlock(aspace);
        ASSERT_EQ(NO_ERROR, result, "query large EFI code range");
        ASSERT_EQ(ARCH_MMU_FLAG_PERM_RWX_MASK, flags & ARCH_MMU_FLAG_PERM_RWX_MASK,
                  "large EFI code allocation is RWX throughout");
    }
    ASSERT_EQ(EFI_STATUS_SUCCESS, free_pages(reinterpret_cast<void *>(large_code), kLargePages),
              "free large EFI code range");
    large_code = 0;
    EfiPhysicalAddr address = 0;
    ASSERT_EQ(EFI_STATUS_INVALID_PARAMETER,
              allocate_pages(EFI_ALLOCATOR_TYPE_ALLOCATE_ANY_PAGES,
                             EFI_MEMORY_TYPE_LOADER_CODE, 0, &address),
              "zero pages rejected");
    ASSERT_EQ(EFI_STATUS_INVALID_PARAMETER,
              allocate_pages(EFI_ALLOCATOR_TYPE_ALLOCATE_ANY_PAGES,
                             EFI_MEMORY_TYPE_LOADER_CODE, SIZE_MAX / PAGE_SIZE + 1, &address),
              "overflowing page count rejected");
    END_TEST;
}

}  // namespace

BEGIN_TEST_CASE(uefi_memory_protocols)
RUN_TEST(fixed_address_allocation)
RUN_TEST(allocate_pages_permissions)
END_TEST_CASE(uefi_memory_protocols)
