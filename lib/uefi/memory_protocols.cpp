/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "memory_protocols.h"

#include <arch/defines.h>
#include <arch/mmu.h>
#include <kernel/vm.h>
#include <lib/dlmalloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <uefi/boot_service.h>
#include <uefi/types.h>

// MACRO list_for_every_entry cast between int/ptr
// NOLINTBEGIN(performance-no-int-to-ptr)

namespace {
vmm_aspace_t *old_aspace = nullptr;
constexpr size_t kHeapSize = 300ul * 1024 * 1024;

void *get_heap() {
  static auto heap = alloc_page(kHeapSize);
  return heap;
}

mspace create_mspace_with_base_limit(void *base, size_t capacity, int locked) {
  auto space = create_mspace_with_base(get_heap(), kHeapSize, 1);
  mspace_set_footprint_limit(space, capacity);
  return space;
}

mspace get_mspace() {
  static auto space = create_mspace_with_base_limit(get_heap(), kHeapSize, 1);
  return space;
}

void restore_aspace() { vmm_set_active_aspace(old_aspace); }

} // namespace

vmm_aspace_t *set_boot_aspace() {
  static vmm_aspace_t *aspace = nullptr;
  if (aspace == nullptr) {
    auto err = vmm_create_aspace(&aspace, "linux_kernel", 0);
    if (err) {
      printf("Failed to create address space for linux kernel %d\n", err);
      return nullptr;
    }
    old_aspace = vmm_set_active_aspace(aspace);
  }
  return aspace;
}

void *alloc_page(size_t size, size_t align_log2) {
  size = ROUNDUP(size, PAGE_SIZE);
  auto aspace = set_boot_aspace();
  paddr_t pa{};
  size_t allocated =
      pmm_alloc_contiguous(size / PAGE_SIZE, align_log2, &pa, nullptr);
  if (allocated != size / PAGE_SIZE) {
    printf("Failed to allocate physical memory size %zu\n", size);
    return nullptr;
  }
  int ret = arch_mmu_map(&aspace->arch_aspace, pa, pa, size / PAGE_SIZE, 0);
  if (ret != 0) {
    printf("Failed to arch_mmu_map(0x%lx)\n", pa);
    return nullptr;
  }
  status_t err{};
  err = vmm_reserve_space(aspace, "uefi_program", size, pa);
  if (err) {
    printf("Failed to vmm_reserve_space for uefi program %d\n", err);
    return nullptr;
  }
  return reinterpret_cast<void *>(pa);
}

void *alloc_page(void *addr, size_t size, size_t align_log2) {
  if (addr == nullptr) {
    return alloc_page(size, align_log2);
  }
  auto aspace = set_boot_aspace();
  size = ROUNDUP(size, PAGE_SIZE);
  struct list_node list;
  size_t allocated =
      pmm_alloc_range(reinterpret_cast<paddr_t>(addr), size, &list);
  if (allocated != size / PAGE_SIZE) {
    printf(
        "Failed to allocate physical memory size %zu at specified address %p "
        "fallback to regular allocation\n",
        size, addr);
    return alloc_page(size, align_log2);
  }
  status_t err{};
  err = vmm_reserve_space(aspace, "uefi_program", size,
                          reinterpret_cast<vaddr_t>(addr));
  if (err) {
    printf("Failed to vmm_reserve_space for uefi program %d\n", err);
    return nullptr;
  }
  return addr;
}

void *uefi_malloc(size_t size) { return mspace_malloc(get_mspace(), size); }

EfiStatus allocate_pool(EfiMemoryType pool_type, size_t size, void **buf) {
  if (buf == nullptr) {
    return INVALID_PARAMETER;
  }
  if (size == 0) {
    *buf = nullptr;
    return SUCCESS;
  }
  *buf = mspace_malloc(get_mspace(), size);
  if (*buf != nullptr) {
    return SUCCESS;
  }
  return OUT_OF_RESOURCES;
}

EfiStatus free_pool(void *mem) {
  mspace_free(get_mspace(), mem);
  return SUCCESS;
}

EfiStatus free_pages(EfiPhysicalAddr memory, size_t pages) {
  auto pa = reinterpret_cast<void *>(
      vaddr_to_paddr(reinterpret_cast<void *>(memory)));
  if (pa != reinterpret_cast<void *>(memory)) {
    printf(
        "WARN: virtual address 0x%llx is not identity mapped, physical addr: "
        "%p\n",
        memory, pa);
  }
  status_t err =
      vmm_free_region(set_boot_aspace(), static_cast<vaddr_t>(memory));
  if (err) {
    printf("%s err:%d memory [0x%llx] pages:%zu\n", __FUNCTION__, err, memory,
           pages);
    return DEVICE_ERROR;
  }
  auto pages_freed = pmm_free_kpages(pa, pages);
  if (pages_freed != pages) {
    printf("Failed to free physical pages %p %zu, only freed %zu pages\n", pa,
           pages, pages_freed);
    return DEVICE_ERROR;
  }
  return SUCCESS;
}

size_t get_aspace_entry_count(vmm_aspace_t *aspace) {
  vmm_region_t *region = nullptr;
  size_t num_entries = 0;
  list_for_every_entry(&aspace->region_list, region, vmm_region_t, node) {
    num_entries++;
  }
  return num_entries;
}

void fill_memory_map_entry(vmm_aspace_t *aspace, EfiMemoryDescriptor *entry,
                           const vmm_region_t *region) {
  entry->virtual_start = region->base;
  entry->physical_start = entry->virtual_start;
  entry->number_of_pages = region->size / PAGE_SIZE;
  paddr_t pa{};
  uint flags{};
  status_t err =
      arch_mmu_query(&aspace->arch_aspace, region->base, &pa, &flags);
  if (err >= 0) {
    entry->physical_start = pa;
  }
  if ((flags & ARCH_MMU_FLAG_CACHE_MASK) == ARCH_MMU_FLAG_CACHED) {
    entry->attributes |= EFI_MEMORY_WB | EFI_MEMORY_WC | EFI_MEMORY_WT;
  }
}

EfiStatus get_physical_memory_map(size_t *memory_map_size,
                                  EfiMemoryDescriptor *memory_map,
                                  size_t *map_key, size_t *desc_size,
                                  uint32_t *desc_version) {
  if (memory_map_size == nullptr) {
    return INVALID_PARAMETER;
  }
  if (map_key) {
    *map_key = 0;
  }
  if (desc_size) {
    *desc_size = sizeof(EfiMemoryDescriptor);
  }
  if (desc_version) {
    *desc_version = 1;
  }
  pmm_arena_t *a{};
  size_t num_entries = 0;
  list_for_every_entry(get_arena_list(), a, pmm_arena_t, node) {
    num_entries++;
  }
  const size_t size_needed = num_entries * sizeof(EfiMemoryDescriptor);
  if (*memory_map_size < size_needed) {
    *memory_map_size = size_needed;
    return BUFFER_TOO_SMALL;
  }
  *memory_map_size = size_needed;
  size_t i = 0;
  memset(memory_map, 0, size_needed);
  list_for_every_entry(get_arena_list(), a, pmm_arena_t, node) {
    memory_map[i].physical_start = a->base;
    memory_map[i].number_of_pages = a->size / PAGE_SIZE;
    memory_map[i].attributes |= EFI_MEMORY_WB;
    memory_map[i].memory_type = LOADER_CODE;
    i++;
  }
  return SUCCESS;
}

EfiStatus get_memory_map(size_t *memory_map_size,
                         EfiMemoryDescriptor *memory_map, size_t *map_key,
                         size_t *desc_size, uint32_t *desc_version) {
  if (memory_map_size == nullptr) {
    return INVALID_PARAMETER;
  }
  if (map_key) {
    *map_key = 0;
  }
  if (desc_size) {
    *desc_size = sizeof(EfiMemoryDescriptor);
  }
  if (desc_version) {
    *desc_version = 1;
  }
  vmm_region_t *region = nullptr;
  auto aspace = vmm_get_kernel_aspace();
  size_t num_entries = 0;
  list_for_every_entry(&aspace->region_list, region, vmm_region_t, node) {
    num_entries++;
  }
  const size_t size_needed = num_entries * sizeof(EfiMemoryDescriptor);
  if (*memory_map_size < size_needed) {
    *memory_map_size = size_needed;
    return BUFFER_TOO_SMALL;
  }
  *memory_map_size = size_needed;
  size_t i = 0;
  memset(memory_map, 0, size_needed);
  list_for_every_entry(&aspace->region_list, region, vmm_region_t, node) {
    memory_map[i].virtual_start = region->base;
    memory_map[i].physical_start = memory_map[i].virtual_start;
    memory_map[i].number_of_pages = region->size / PAGE_SIZE;
    paddr_t pa{};
    uint flags{};
    status_t err =
        arch_mmu_query(&aspace->arch_aspace, region->base, &pa, &flags);
    if (err >= 0) {
      memory_map[i].physical_start = pa;
    }
    i++;
  }

  return SUCCESS;
}

// NOLINTEND(performance-no-int-to-ptr)