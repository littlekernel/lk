// Copyright (c) 2026 Kelvin Zhang
// Use of this source code is governed by a MIT-style license in LICENSE.
#include <arch/mmu.h>
#include <kernel/vm.h>
#include <lib/unittest.h>
#include <lk/err.h>
#include <string.h>

#include "../defer.h"
#include "../image_permissions.h"
#include "../memory_protocols.h"
#include "../uefi_platform.h"

namespace {
constexpr uint kR = ARCH_MMU_FLAG_PERM_READ;
constexpr uint kRW = kR | ARCH_MMU_FLAG_PERM_WRITE;
constexpr uint kRX = kR | ARCH_MMU_FLAG_PERM_EXECUTE;

bool permissions() {
  BEGIN_TEST;
  DEFER { reset_heap(); };
  // Larger than an ARM64 4 KiB-granule L2 block, with differing permissions
  // inside its first block: this requires base-page image mappings.
  const size_t size = 2 * 1024 * 1024 + 6 * PAGE_SIZE;
  auto *image = static_cast<char *>(alloc_uefi_image(size, 0));
  ASSERT_NONNULL(image, "image allocation");
  DEFER { free_pages(image, size / PAGE_SIZE); };
  auto *aspace = set_boot_aspace();
  auto query = [&](size_t offset, uint expected) {
    uint flags = 0;
    paddr_t pa = 0;
    vmm_aspace_lock(aspace);
    const auto result = arch_mmu_query(&aspace->arch_aspace,
        reinterpret_cast<vaddr_t>(image + offset), &pa, &flags);
    vmm_aspace_unlock(aspace);
    return result == NO_ERROR && pa == reinterpret_cast<paddr_t>(image + offset) &&
           (flags & ARCH_MMU_FLAG_PERM_RWX_MASK) == expected;
  };
  ASSERT_TRUE(query(PAGE_SIZE, kRW), "image starts writable and NX");
  IMAGE_SECTION_HEADER sections[3]{};
  sections[0].VirtualAddress = PAGE_SIZE;
  sections[0].SizeOfRawData = PAGE_SIZE;
  sections[0].Characteristics = 0x60000000; // RX
  sections[1].VirtualAddress = 3 * PAGE_SIZE;
  sections[1].Misc.VirtualSize = 2 * PAGE_SIZE; // includes BSS
  sections[1].SizeOfRawData = 1;
  sections[1].Characteristics = 0xc0000000; // RW
  sections[2].VirtualAddress = 5 * PAGE_SIZE;
  sections[2].SizeOfRawData = 1;
  sections[2].Characteristics = 0x40000000; // R
  ASSERT_EQ(NO_ERROR, protect_uefi_image(image, size, sections, 3), "protect image");
  for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
    const uint expected = offset == PAGE_SIZE ? kRX :
        (offset == 3 * PAGE_SIZE || offset == 4 * PAGE_SIZE) ? kRW : kR;
    ASSERT_TRUE(query(offset, expected), "section/header/gap/padding permissions and PA");
  }
  memset(image + 3 * PAGE_SIZE, 0x5a, 2 * PAGE_SIZE);
  ASSERT_EQ(0x5a, static_cast<unsigned char>(image[5 * PAGE_SIZE - 1]), "RW data and BSS");
  auto *stack = alloc_page(PAGE_SIZE);
  ASSERT_NONNULL(stack, "generic allocation");
  DEFER { free_pages(stack, 1); };
  uint flags = 0;
  vmm_aspace_lock(aspace);
  const auto result = arch_mmu_query(&aspace->arch_aspace,
      reinterpret_cast<vaddr_t>(stack), nullptr, &flags);
  vmm_aspace_unlock(aspace);
  ASSERT_EQ(NO_ERROR, result, "query general allocation");
  ASSERT_EQ(kRW, flags & ARCH_MMU_FLAG_PERM_RWX_MASK, "heap/stack allocations are NX");
  END_TEST;
}

bool rejects_wx() {
  BEGIN_TEST;
  DEFER { reset_heap(); };
  auto *image = alloc_uefi_image(2 * PAGE_SIZE, 0);
  ASSERT_NONNULL(image, "image allocation");
  DEFER { free_pages(image, 2); };
  IMAGE_SECTION_HEADER sections[2]{};
  sections[0].SizeOfRawData = PAGE_SIZE / 2;
  sections[0].Characteristics = 0x60000000;
  sections[1].VirtualAddress = PAGE_SIZE / 2;
  sections[1].Misc.VirtualSize = PAGE_SIZE / 2;
  sections[1].Characteristics = 0xc0000000;
  ASSERT_EQ(ERR_NOT_SUPPORTED, protect_uefi_image(image, 2 * PAGE_SIZE, sections, 2),
            "disjoint RX and RW sections share a native page");
  sections[0].Characteristics = 0xe0000000;
  ASSERT_EQ(ERR_NOT_SUPPORTED, protect_uefi_image(image, 2 * PAGE_SIZE, sections, 1),
            "explicit writable executable section");
  sections[0].Characteristics = 0x60000000;
  sections[0].SizeOfRawData = PAGE_SIZE;
  sections[1].VirtualAddress = PAGE_SIZE;
  sections[1].Misc.VirtualSize = PAGE_SIZE;
  sections[1].Characteristics = 0xe0000000;
  ASSERT_EQ(ERR_NOT_SUPPORTED, protect_uefi_image(image, 2 * PAGE_SIZE, sections, 2),
            "later writable executable section rejected before any protection changes");
  auto *aspace = set_boot_aspace();
  uint flags = 0;
  vmm_aspace_lock(aspace);
  const auto result = arch_mmu_query(&aspace->arch_aspace,
      reinterpret_cast<vaddr_t>(image), nullptr, &flags);
  vmm_aspace_unlock(aspace);
  ASSERT_EQ(NO_ERROR, result, "query rejected image");
  ASSERT_EQ(kRW, flags & ARCH_MMU_FLAG_PERM_RWX_MASK, "rejection did not grant execute");
  END_TEST;
}

bool preferred_mapping_failure_falls_back() {
  BEGIN_TEST;
  DEFER { reset_heap(); };
  auto *aspace = set_boot_aspace();
  ASSERT_NONNULL(aspace, "UEFI address space");
  struct list_node blocker;
  list_initialize(&blocker);
  DEFER { pmm_free(&blocker); };
  paddr_t blocker_pa = 0;
  ASSERT_EQ(1u, pmm_alloc_contiguous(1, PAGE_SIZE_SHIFT, &blocker_pa, &blocker),
            "reserve collision backing page");

  struct list_node probe;
  list_initialize(&probe);
  DEFER { pmm_free(&probe); };
  paddr_t probe_pa = 0;
  ASSERT_EQ(4u, pmm_alloc_contiguous(4, PAGE_SIZE_SHIFT, &probe_pa, &probe),
            "find preferred physical range");
  const paddr_t preferred = IS_ALIGNED(probe_pa, 2 * 1024 * 1024)
                                ? probe_pa + PAGE_SIZE : probe_pa;

  const vaddr_t collision = preferred + PAGE_SIZE;
  vmm_aspace_lock(aspace);
  const status_t map_status = arch_mmu_map(&aspace->arch_aspace, collision, blocker_pa,
                                           1, kRW);
  vmm_aspace_unlock(aspace);
  ASSERT_EQ(NO_ERROR, map_status, "map collision at preferred image address");
  DEFER {
    vmm_aspace_lock(aspace);
    arch_mmu_unmap(&aspace->arch_aspace, collision, 1);
    vmm_aspace_unlock(aspace);
  };
  ASSERT_EQ(4u, pmm_free(&probe), "release preferred range after page-table allocation");
  struct list_node check;
  list_initialize(&check);
  DEFER { pmm_free(&check); };
  ASSERT_EQ(2u, pmm_alloc_range(preferred, 2, &check),
            "preferred physical pages remain available despite virtual collision");
  ASSERT_EQ(2u, pmm_free(&check), "release preferred pages for image allocation");
  // This test assumes no competing allocator takes the preferred pages before
  // alloc_uefi_image(); otherwise fallback could occur before the mapping fails.

  const size_t before = [&]() {
    size_t count = 0;
    pmm_arena_t *arena;
    list_for_every_entry(get_arena_list(), arena, pmm_arena_t, node) {
      count += arena->free_count;
    }
    return count;
  }();
  auto *image = alloc_uefi_image(2 * PAGE_SIZE, preferred);
  ASSERT_NONNULL(image, "mapping collision falls back to contiguous image");
  DEFER {
    if (image) {
      free_pages(image, 2);
    }
  };
  ASSERT_NE(reinterpret_cast<void *>(preferred), image, "fallback uses another address");
  ASSERT_EQ(EFI_STATUS_SUCCESS, free_pages(image, 2), "release fallback image");
  image = nullptr;
  size_t after = 0;
  pmm_arena_t *arena;
  list_for_every_entry(get_arena_list(), arena, pmm_arena_t, node) {
    after += arena->free_count;
  }
  ASSERT_EQ(before, after, "failed preferred mapping leaked no pages or page tables");
  END_TEST;
}

// A 64 KiB leaf table covers 512 MiB, the whole QEMU test RAM. Earlier
// identity mappings can leave that table in place, so an image allocation may
// need no new page table and cannot exercise the injected failure.
#if LK_DEBUGLEVEL > 0 && PAGE_SIZE < 65536
bool mapping_failure_cleanup() {
  BEGIN_TEST;
  DEFER { reset_heap(); };
  ASSERT_NONNULL(set_boot_aspace(), "boot aspace");
  vm_ptable_track(true);
  DEFER { vm_ptable_track(false); };
  // Permit the first table, then fail a later allocation. Repeating the
  // attempt checks cleanup of the partially constructed table hierarchy.
  auto free_count = []() {
    size_t count = 0;
    pmm_arena_t *arena;
    list_for_every_entry(get_arena_list(), arena, pmm_arena_t, node) {
      count += arena->free_count;
    }
    return count;
  };
  const size_t before = free_count();
  for (int run = 0; run < 2; ++run) {
    vm_ptable_budget_set(1);
    auto *image = alloc_uefi_image(4 * PAGE_SIZE, 0);
    vm_ptable_budget_set(-1);
    DEFER {
      if (image) {
        free_pages(image, 4);
      }
    };
    ASSERT_NULL(image, "injected page-table allocation failure");
    size_t allocated, freed;
    vm_ptable_counts(&allocated, &freed);
    ASSERT_EQ(allocated, freed, "failed map releases page tables");
    ASSERT_EQ(before, free_count(), "failed map releases physical image storage");
  }
  auto *image = alloc_uefi_image(4 * PAGE_SIZE, 0);
  ASSERT_NONNULL(image, "allocation works after failed attempts");
  DEFER { free_pages(image, 4); };
  END_TEST;
}
#endif
} // namespace

BEGIN_TEST_CASE(uefi_image_permissions)
RUN_TEST(permissions)
RUN_TEST(rejects_wx)
RUN_TEST(preferred_mapping_failure_falls_back)
#if LK_DEBUGLEVEL > 0 && PAGE_SIZE < 65536
RUN_TEST(mapping_failure_cleanup)
#endif
END_TEST_CASE(uefi_image_permissions)
