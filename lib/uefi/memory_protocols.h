#ifndef __LIB_UEFI_MEMORY_PROTOCOLS_H
#define __LIB_UEFI_MEMORY_PROTOCOLS_H

#include "types.h"
#include <kernel/vm.h>
#include <stddef.h>

#include "boot_service.h"

vmm_aspace_t *set_boot_aspace();

void *alloc_page(void *addr, size_t size, size_t align_log2 = PAGE_SIZE_SHIFT);
void *alloc_page(size_t size, size_t align_log2 = PAGE_SIZE_SHIFT);
EfiStatus free_pages(EfiPhysicalAddr memory, size_t pages);
void *identity_map(void *addr, size_t size);
void *uefi_malloc(size_t size);
EfiStatus allocate_pool(EfiMemoryType pool_type, size_t size, void **buf);
EfiStatus free_pool(void *mem);

EfiStatus get_physical_memory_map(size_t *memory_map_size,
                                  EfiMemoryDescriptor *memory_map,
                                  size_t *map_key, size_t *desc_size,
                                  uint32_t *desc_version);

#endif
