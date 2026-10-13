// Copyright (c) 2026 Kelvin Zhang
// Use of this source code is governed by a MIT-style license in LICENSE.
#include "image_permissions.h"

#include <arch/mmu.h>
#include <arch/ops.h>
#include <kernel/vm.h>
#include <lk/err.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#include "defer.h"
#include "memory_protocols.h"

namespace {
constexpr uint32_t kSectionExecute = 0x20000000;
constexpr uint32_t kSectionWrite = 0x80000000;
constexpr uint kRead = ARCH_MMU_FLAG_PERM_READ;
constexpr uint kWrite = ARCH_MMU_FLAG_PERM_WRITE;
constexpr uint kExecute = ARCH_MMU_FLAG_PERM_EXECUTE;

bool map_image(vmm_aspace_t *aspace, paddr_t pa, uint count, size_t size) {
  uint mapped = 0;
  vmm_aspace_lock(aspace);
  for (; mapped < count; ++mapped) {
    const auto address = pa + mapped * PAGE_SIZE;
    if (arch_mmu_map(&aspace->arch_aspace, address, address, 1, kRead | kWrite) != NO_ERROR) {
      break;
    }
  }
  if (mapped != count) {
    if (mapped) {
      arch_mmu_unmap(&aspace->arch_aspace, pa, mapped);
    }
    vmm_aspace_unlock(aspace);
    return false;
  }
  vmm_aspace_unlock(aspace);
  if (vmm_reserve_space(aspace, "uefi_image", size, pa) == NO_ERROR) {
    return true;
  }
  vmm_aspace_lock(aspace);
  arch_mmu_unmap(&aspace->arch_aspace, pa, count);
  vmm_aspace_unlock(aspace);
  return false;
}
} // namespace

void *alloc_uefi_image(size_t size, paddr_t preferred) {
  if (!size || !IS_PAGE_ALIGNED(size) || size / PAGE_SIZE > UINT_MAX) {
    return nullptr;
  }
  auto *aspace = set_boot_aspace();
  if (!aspace) {
    return nullptr;
  }
  const uint count = size / PAGE_SIZE;
  struct list_node pages;
  list_initialize(&pages);
  DEFER { pmm_free(&pages); };
  // Keep the preferred-base attempt local to image loading; do not use the
  // legacy fixed-address alloc_page() path. Mapping failure also falls back.
  if (preferred && IS_PAGE_ALIGNED(preferred)) {
    if (pmm_alloc_range(preferred, count, &pages) == count &&
        map_image(aspace, preferred, count, size)) {
      while (list_remove_head(&pages)) {}
      return reinterpret_cast<void *>(preferred);
    }
    pmm_free(&pages);
  }
  paddr_t pa = 0;
  if (pmm_alloc_contiguous(count, 21, &pa, &pages) != count ||
      !map_image(aspace, pa, count, size)) {
    return nullptr;
  }
  // free_pages() takes ownership on success; no PMM node may retain this
  // stack list as its owner.
  while (list_remove_head(&pages)) {}
  return reinterpret_cast<void *>(pa);
}

status_t protect_uefi_image(void *image, size_t size,
                            const IMAGE_SECTION_HEADER *sections, size_t count) {
  if (!image || !size || (count && !sections) || !IS_PAGE_ALIGNED(image) ||
      !IS_PAGE_ALIGNED(size) || size / PAGE_SIZE > UINT_MAX ||
      size - 1 > UINTPTR_MAX - reinterpret_cast<uintptr_t>(image)) {
    return ERR_INVALID_ARGS;
  }
  if (!arch_mmu_supports_nx_mappings()) {
    printf("WARNING: UEFI image W^X protection unavailable on this architecture\n");
    return NO_ERROR;
  }
  // The loader already checks section order, non-overlap and native-page
  // alignment. Preflight them here too so no permission changes precede a
  // rejected W+X or shared-page pair.
  size_t previous_page_end = 0;
  for (size_t i = 0; i < count; ++i) {
    const auto &section = sections[i];
    const size_t start = section.VirtualAddress;
    const size_t span = MAX(section.Misc.VirtualSize, section.SizeOfRawData);
    if (!span) {
      continue;
    }
    if (start >= size || span > size - start) {
      return ERR_INVALID_ARGS;
    }
    if (!IS_PAGE_ALIGNED(start) || start < previous_page_end ||
        (section.Characteristics & (kSectionWrite | kSectionExecute)) ==
            (kSectionWrite | kSectionExecute)) {
      return ERR_NOT_SUPPORTED;
    }
    previous_page_end = ROUNDUP(start + span, PAGE_SIZE);
  }
  auto *aspace = set_boot_aspace();
  if (!aspace) {
    return ERR_NO_MEMORY;
  }
  vmm_aspace_lock(aspace);
  DEFER { vmm_aspace_unlock(aspace); };
  const vaddr_t image_va = reinterpret_cast<vaddr_t>(image);
  // Headers, gaps and padding default to R/NX. The loader frees the image if
  // any protection change fails before the entry point is called.
  status_t status = arch_mmu_protect(&aspace->arch_aspace, image_va, size / PAGE_SIZE, kRead);
  if (status != NO_ERROR) {
    return status;
  }
  // The UEFI identity VA is the execution VA. Synchronize only executable
  // sections through that alias after relocation and before granting X.
  for (size_t i = 0; i < count; ++i) {
    const auto &section = sections[i];
    if ((section.Characteristics & kSectionExecute) == 0) {
      continue;
    }
    const size_t span = MAX(section.Misc.VirtualSize, section.SizeOfRawData);
    if (span) {
      arch_sync_cache_range(image_va + section.VirtualAddress, span);
    }
  }
  for (size_t i = 0; i < count; ++i) {
    const auto &section = sections[i];
    const size_t span = MAX(section.Misc.VirtualSize, section.SizeOfRawData);
    if (!span) {
      continue;
    }
    uint flags = kRead;
    if (section.Characteristics & kSectionWrite) {
      flags |= kWrite;
    }
    if (section.Characteristics & kSectionExecute) {
      flags |= kExecute;
    }
    if (flags == kRead) {
      continue;
    }
    status = arch_mmu_protect(&aspace->arch_aspace, image_va + section.VirtualAddress,
                              ROUNDUP(span, PAGE_SIZE) / PAGE_SIZE, flags);
    if (status != NO_ERROR) {
      return status;
    }
  }
  return NO_ERROR;
}
