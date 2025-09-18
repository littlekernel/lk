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
#include <lk/trace.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <uefi/boot_service.h>
#include <uefi/protocols/gbl_efi_boot_memory_protocol.h>
#include <uefi/types.h>

#include "uefi_platform.h"

#define LOCAL_TRACE 0

// MACRO list_for_every_entry cast between int/ptr
// NOLINTBEGIN(performance-no-int-to-ptr)

namespace {
vmm_aspace_t *old_aspace = nullptr;
constexpr size_t kHeapSize = 300ul * 1024 * 1024;

void *heap = nullptr;
void *get_heap() {
  if (heap == nullptr) {
    heap = alloc_page(kHeapSize);
  }
  return heap;
}

mspace create_mspace_with_base_limit(void *base, size_t capacity, int locked) {
  auto space = create_mspace_with_base(get_heap(), kHeapSize, 1);
  mspace_set_footprint_limit(space, capacity);
  return space;
}

mspace space = nullptr;

mspace get_mspace() {
  if (space == nullptr) {
    space = create_mspace_with_base_limit(get_heap(), kHeapSize, 1);
  }
  return space;
}

vmm_aspace_t *uefi_aspace = nullptr;

void restore_aspace() {
  if (uefi_aspace != nullptr) {
    vmm_set_active_aspace(old_aspace);
    vmm_free_aspace(uefi_aspace);
    uefi_aspace = nullptr;
  }
}

}  // namespace

__WEAK void setup_heap() {
  set_boot_aspace();
  get_mspace();
}

__WEAK void reset_heap() {
  if (space != nullptr) {
    destroy_mspace(space);
    space = nullptr;
  }
  if (heap != nullptr) {
    free_pages(heap, kHeapSize / PAGE_SIZE);
    heap = nullptr;
  }
  restore_aspace();
}

vmm_aspace_t *set_boot_aspace() {
  if (uefi_aspace == nullptr) {
    auto err = vmm_create_aspace(&uefi_aspace, "linux_kernel", 0);
    if (err) {
      printf("Failed to create address space for linux kernel %d\n", err);
      return nullptr;
    }
    old_aspace = vmm_set_active_aspace(uefi_aspace);
  }
  return uefi_aspace;
}

__WEAK void *alloc_page(size_t size, size_t align_log2) {
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

__WEAK void *alloc_page(void *addr, size_t size, size_t align_log2) {
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

EfiStatus allocate_pages(EfiAllocatorType type, EfiMemoryType memory_type,
                         size_t pages, EfiPhysicalAddr *memory) {
  if (memory == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  if (type == EFI_ALLOCATOR_TYPE_ALLOCATE_MAX_ADDRESS && *memory < 0xFFFFFFFF) {
    LTRACEF("%d, %d, %zu, 0x%llx unsupported\n", type, memory_type, pages,
            *memory);
    return EFI_STATUS_UNSUPPORTED;
  }
  *memory = reinterpret_cast<EfiPhysicalAddr>(alloc_page(pages * PAGE_SIZE));
  if (*memory == 0) {
    return EFI_STATUS_OUT_OF_RESOURCES;
  }
  return EFI_STATUS_SUCCESS;
}

__WEAK void *uefi_malloc(size_t size) {
  return mspace_malloc(get_mspace(), size);
}

__WEAK EfiStatus allocate_pool(EfiMemoryType pool_type, size_t size,
                               void **buf) {
  if (buf == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  if (size == 0) {
    *buf = nullptr;
    return EFI_STATUS_SUCCESS;
  }
  *buf = mspace_malloc(get_mspace(), size);
  if (*buf != nullptr) {
    return EFI_STATUS_SUCCESS;
  }
  return EFI_STATUS_OUT_OF_RESOURCES;
}

__WEAK EfiStatus free_pool(void *mem) {
  mspace_free(get_mspace(), mem);
  return EFI_STATUS_SUCCESS;
}

__WEAK EfiStatus free_pages(void *memory, size_t pages) {
  auto pa = reinterpret_cast<void *>(vaddr_to_paddr(memory));
  if (pa != memory) {
    printf("WARN: virtual address %p is not identity mapped, physical addr: "
           "%p\n",
           memory, pa);
  }
  status_t err =
      vmm_free_region(set_boot_aspace(), reinterpret_cast<vaddr_t>(memory));
  if (err) {
    printf("%s err:%d memory [%p] pages:%zu\n", __FUNCTION__, err, memory,
           pages);
    return EFI_STATUS_DEVICE_ERROR;
  }
  auto pages_freed = pmm_free_kpages(pa, pages);
  if (pages_freed != pages) {
    printf("Failed to free physical pages %p %zu, only freed %zu pages\n", pa,
           pages, pages_freed);
    return EFI_STATUS_DEVICE_ERROR;
  }
  return EFI_STATUS_SUCCESS;
}

EfiStatus get_physical_memory_map(size_t *memory_map_size,
                                  EfiMemoryDescriptor *memory_map,
                                  size_t *map_key, size_t *desc_size,
                                  uint32_t *desc_version) {
  if (memory_map_size == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
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
    return EFI_STATUS_BUFFER_TOO_SMALL;
  }
  *memory_map_size = size_needed;
  size_t i = 0;
  memset(memory_map, 0, size_needed);
  list_for_every_entry(get_arena_list(), a, pmm_arena_t, node) {
    memory_map[i].physical_start = a->base;
    memory_map[i].number_of_pages = a->size / PAGE_SIZE;
    memory_map[i].attributes = static_cast<EfiMemoryAttribute>(memory_map[i].attributes | EFI_MEMORY_ATTRIBUTE_EMA_WB);
    memory_map[i].memory_type = EFI_MEMORY_TYPE_LOADER_CODE;
    i++;
  }
  return EFI_STATUS_SUCCESS;
}

// NOLINTEND(performance-no-int-to-ptr)

namespace {
EfiStatus get_partition_buffer(struct GblEfiBootMemoryProtocol* self,
                               /* in */ const uint8_t* base_name,
                               /* out */ size_t* size,
                               /* out */ void** addr,
                               /* out */ GblEfiPartitionBufferFlag* flag) {
  return EFI_STATUS_NOT_FOUND;
}

EfiStatus sync_partition_buffer(struct GblEfiBootMemoryProtocol* self,
                                /* in */ bool sync_preloaded) {
  return EFI_STATUS_SUCCESS;
}

EfiStatus get_boot_buffer(struct GblEfiBootMemoryProtocol* self,
                          /* in */ GblEfiBootBufferType buf_type,
                          /* out */ size_t* size,
                          /* out */ void** addr) {
  if (buf_type == GBL_EFI_BOOT_BUFFER_TYPE_GENERAL_LOAD) {
    *size = 128ul * 1024 * 1024;
    // 2^21 = 2MB alignment, required by linux kernel
    *addr = alloc_page(*size, 21);
    if (*addr == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    return EFI_STATUS_SUCCESS;
  } else if (buf_type == GBL_EFI_BOOT_BUFFER_TYPE_PVMFW_DATA) {
    *size = 1024ul * 1024;
    *addr = alloc_page(*size, PAGE_SIZE_SHIFT);
    if (*addr == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    return EFI_STATUS_SUCCESS;
  }
  printf("get_boot_buffer(%d, %zu) unsupported\n", buf_type, *size);
  return EFI_STATUS_NOT_FOUND;
}

}  // namespace

__WEAK GblEfiBootMemoryProtocol* open_boot_memory_protocol() {
  static GblEfiBootMemoryProtocol protocol = {
      .revision = GBL_EFI_BOOT_MEMORY_PROTOCOL_REVISION,
      .get_partition_buffer = get_partition_buffer,
      .sync_partition_buffer = sync_partition_buffer,
      .get_boot_buffer = get_boot_buffer,
  };
  return &protocol;
}
