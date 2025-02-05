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

#include <kernel/vm.h>
#include <lib/dlmalloc.h>
#include <stdio.h>
#include <string.h>

#include "boot_service.h"
#include "types.h"

static vmm_aspace_t *old_aspace = nullptr;

namespace {
constexpr size_t kHeapSize = 256ul * 1024 * 1024;

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

void restore_aspace() { vmm_set_active_aspace(old_aspace); }

void *identity_map(void *addr, size_t size) {
  size = ROUNDUP(size, PAGE_SIZE);
  auto vaddr = reinterpret_cast<vaddr_t>(addr);
  paddr_t pa{};
  uint flags{};
  auto aspace = set_boot_aspace();
  auto err = arch_mmu_query(&aspace->arch_aspace, vaddr, &pa, &flags);
  if (err) {
    printf("Failed to query physical address for memory 0x%p\n", addr);
    return nullptr;
  }

  err = arch_mmu_unmap(&aspace->arch_aspace, vaddr, size / PAGE_SIZE);
  if (err) {
    printf("Failed to unmap virtual address 0x%lx\n", vaddr);
    return nullptr;
  }
  arch_mmu_map(&aspace->arch_aspace, pa, pa, size / PAGE_SIZE, flags);
  if (err) {
    printf("Failed to identity map physical address 0x%lx\n", pa);
    return nullptr;
  }
  printf("Identity mapped physical address 0x%lx size %zu flags 0x%x\n", pa,
         size, flags);

  return reinterpret_cast<void *>(pa);
}

void *alloc_page(size_t size, size_t align_log2) {
  auto aspace = set_boot_aspace();
  void *vptr{};
  status_t err = vmm_alloc_contiguous(aspace, "uefi_program", size, &vptr,
                                      align_log2, 0, 0);
  if (err) {
    printf("Failed to allocate memory for uefi program %d\n", err);
    return nullptr;
  }
  return identity_map(vptr, size);
}

void *alloc_page(void *addr, size_t size, size_t align_log2) {
  if (addr == nullptr) {
    return alloc_page(size, align_log2);
  }
  auto err =
      vmm_alloc_contiguous(set_boot_aspace(), "uefi_program", size, &addr,
                           align_log2, VMM_FLAG_VALLOC_SPECIFIC, 0);
  if (err) {
    printf(
        "Failed to allocate memory for uefi program @ fixed address 0x%p %d , "
        "falling back to non-fixed allocation\n",
        addr, err);
    return alloc_page(size, align_log2);
  }
  return identity_map(addr, size);
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
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
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
