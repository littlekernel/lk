/*
 * Copyright (c) 2014 Google Inc. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <arch/arm64/mmu.h>
#include <assert.h>
#include <debug.h>
#include <err.h>
#include <kernel/vm.h>
#include <lib/heap.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <trace.h>

#define LOCAL_TRACE 0

STATIC_ASSERT(((long)KERNEL_BASE >> MMU_KERNEL_SIZE_SHIFT) == -1);
STATIC_ASSERT(((long)KERNEL_ASPACE_BASE >> MMU_KERNEL_SIZE_SHIFT) == -1);
STATIC_ASSERT(MMU_KERNEL_SIZE_SHIFT <= 48);
STATIC_ASSERT(MMU_KERNEL_SIZE_SHIFT >= 25);

/* the main translation table */
pte_t arm64_kernel_translation_table[MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP] __ALIGNED(MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP * 8) __SECTION(".bss.prebss.translation_table");

/* convert user level mmu flags to flags that go in L1 descriptors */
static pte_t mmu_flags_to_pte_attr(uint flags)
{
    pte_t attr = MMU_PTE_ATTR_AF;

    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_CACHED:
            attr |= MMU_PTE_ATTR_NORMAL_MEMORY | MMU_PTE_ATTR_SH_INNER_SHAREABLE;
            break;
        case ARCH_MMU_FLAG_UNCACHED:
            attr |= MMU_PTE_ATTR_STRONGLY_ORDERED;
            break;
        case ARCH_MMU_FLAG_UNCACHED_DEVICE:
            attr |= MMU_PTE_ATTR_DEVICE;
            break;
        default:
            /* invalid user-supplied flag */
            DEBUG_ASSERT(1);
            return ERR_INVALID_ARGS;
    }

    switch (flags & (ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO)) {
        case 0:
            attr |= MMU_PTE_ATTR_AP_P_RW_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_RO:
            attr |= MMU_PTE_ATTR_AP_P_RO_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_USER:
            attr |= MMU_PTE_ATTR_AP_P_RW_U_RW;
            break;
        case ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO:
            attr |= MMU_PTE_ATTR_AP_P_RO_U_RO;
            break;
    }

    if (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE) {
        attr |= MMU_PTE_ATTR_UXN | MMU_PTE_ATTR_PXN;
    }

    if (flags & ARCH_MMU_FLAG_NS) {
            attr |= MMU_PTE_ATTR_NON_SECURE;
    }

    return attr;
}

status_t arch_mmu_query(vaddr_t vaddr, paddr_t *paddr, uint *flags)
{
    uint index;
    uint index_shift;
    pte_t pte;
    pte_t pte_addr;
    uint descriptor_type;
    pte_t *page_table;
    vaddr_t kernel_base = ~0UL << MMU_KERNEL_SIZE_SHIFT;
    vaddr_t vaddr_rem;

    if (vaddr < kernel_base) {
        TRACEF("vaddr 0x%lx < base 0x%lx\n", vaddr, kernel_base);
        return ERR_INVALID_ARGS;
    }

    index_shift = MMU_KERNEL_TOP_SHIFT;
    page_table = arm64_kernel_translation_table;

    vaddr_rem = vaddr - kernel_base;
    index = vaddr_rem >> index_shift;
    ASSERT(index < MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP);

    while (true) {
        index = vaddr_rem >> index_shift;
        vaddr_rem -= (vaddr_t)index << index_shift;
        pte = page_table[index];
        descriptor_type = pte & MMU_PTE_DESCRIPTOR_MASK;
        pte_addr = pte & MMU_PTE_OUTPUT_ADDR_MASK;

        LTRACEF("va 0x%lx, index %d, index_shift %d, rem 0x%lx, pte 0x%llx\n",
                vaddr, index, index_shift, vaddr_rem, pte);

        if (descriptor_type == MMU_PTE_DESCRIPTOR_INVALID)
            return ERR_NOT_FOUND;

        if (descriptor_type == ((index_shift > MMU_KERNEL_PAGE_SIZE_SHIFT) ?
                                 MMU_PTE_L012_DESCRIPTOR_BLOCK :
                                 MMU_PTE_L3_DESCRIPTOR_PAGE)) {
            break;
        }

        if (index_shift <= MMU_KERNEL_PAGE_SIZE_SHIFT ||
            descriptor_type != MMU_PTE_L012_DESCRIPTOR_TABLE) {
            PANIC_UNIMPLEMENTED;
        }

        page_table = paddr_to_kvaddr(pte_addr);
        index_shift -= MMU_KERNEL_PAGE_SIZE_SHIFT - 3;
    }

    if (paddr)
        *paddr = pte_addr + vaddr_rem;
    if (flags) {
        *flags = 0;
        if (pte & MMU_PTE_ATTR_NON_SECURE)
            *flags |= ARCH_MMU_FLAG_NS;
        switch (pte & MMU_PTE_ATTR_ATTR_INDEX_MASK) {
            case MMU_PTE_ATTR_STRONGLY_ORDERED:
                *flags |= ARCH_MMU_FLAG_UNCACHED;
                break;
            case MMU_PTE_ATTR_DEVICE:
                *flags |= ARCH_MMU_FLAG_UNCACHED_DEVICE;
                break;
            case MMU_PTE_ATTR_NORMAL_MEMORY:
                break;
            default:
                PANIC_UNIMPLEMENTED;
        }
        switch (pte & MMU_PTE_ATTR_AP_MASK) {
            case MMU_PTE_ATTR_AP_P_RW_U_NA:
                break;
            case MMU_PTE_ATTR_AP_P_RW_U_RW:
                *flags |= ARCH_MMU_FLAG_PERM_USER;
                break;
            case MMU_PTE_ATTR_AP_P_RO_U_NA:
                *flags |= ARCH_MMU_FLAG_PERM_RO;
                break;
            case MMU_PTE_ATTR_AP_P_RO_U_RO:
                *flags |= ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO;
                break;
        }
        if ((pte & MMU_PTE_ATTR_UXN) && (pte & MMU_PTE_ATTR_PXN)) {
            *flags |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
        }
    }
    LTRACEF("va 0x%lx, paddr 0x%lx, flags 0x%x\n",
            vaddr, paddr ? *paddr : ~0UL, flags ? *flags : ~0U);
    return 0;
}

static int alloc_page_table(paddr_t *paddrp, uint page_size_shift)
{
    int ret;
    int count;
    size_t size = 1U << page_size_shift;
    void *vaddr;

    if (size >= PAGE_SIZE) {
        count = size / PAGE_SIZE;
        ret = pmm_alloc_contiguous(count, page_size_shift, paddrp, NULL);
        if (ret != count)
            return ERR_NO_MEMORY;
    } else {
        vaddr = heap_alloc(size, size);
        if (!vaddr)
            return ERR_NO_MEMORY;
        ret = arch_mmu_query((vaddr_t)vaddr, paddrp, NULL);
        if (ret) {
            heap_free(vaddr);
            return ret;
        }
    }
    return 0;
}

static void free_page_table(void *vaddr, paddr_t paddr, uint page_size_shift)
{
    vm_page_t *address_to_page(paddr_t addr); /* TODO: remove */

    size_t size = 1U << page_size_shift;
    vm_page_t *page;

    if (size >= PAGE_SIZE) {
        page = address_to_page(paddr);
        if (!page)
            panic("bad page table paddr 0x%lx\n", paddr);
        pmm_free_page(page);
    } else {
        heap_free(vaddr);
    }
}

static pte_t *arm64_mmu_get_page_table(vaddr_t index, uint page_size_shift, pte_t *page_table)
{
    pte_t pte;
    paddr_t paddr;
    void *vaddr;
    int ret;

    pte = page_table[index];
    switch (pte & MMU_PTE_DESCRIPTOR_MASK) {
    case MMU_PTE_DESCRIPTOR_INVALID:
        ret = alloc_page_table(&paddr, page_size_shift);
        if (ret) {
            TRACEF("failed to allocate page table\n");
            return NULL;
        }
        vaddr = paddr_to_kvaddr(paddr);
        LTRACEF("allocated page table, vaddr %p, paddr 0x%lx\n", vaddr, paddr);
        memset(vaddr, MMU_PTE_DESCRIPTOR_INVALID, 1U << page_size_shift);
        __asm__ volatile("dmb ishst" ::: "memory");
        pte = paddr | MMU_PTE_L012_DESCRIPTOR_TABLE;
        page_table[index] = pte;
        LTRACEF("pte %p[0x%lx] = 0x%llx\n", page_table, index, pte);
        return vaddr;

    case MMU_PTE_L012_DESCRIPTOR_TABLE:
        paddr = pte & MMU_PTE_OUTPUT_ADDR_MASK;
        LTRACEF("found page table 0x%lx\n", paddr);
        return paddr_to_kvaddr(paddr);

    case MMU_PTE_L012_DESCRIPTOR_BLOCK:
        return NULL;

    default:
        PANIC_UNIMPLEMENTED;
    }
}

static bool page_table_is_clear(pte_t *page_table, uint page_size_shift)
{
    int i;
    int count = 1U << (page_size_shift - 3);
    pte_t pte;

    for (i = 0; i < count; i++) {
        pte = page_table[i];
        if (pte != MMU_PTE_DESCRIPTOR_INVALID) {
            LTRACEF("page_table at %p still in use, index %d is 0x%llx\n",
                    page_table, i, pte);
            return false;
        }
    }

    LTRACEF("page table at %p is clear\n", page_table);
    return true;
}

static void arm64_mmu_unmap_pt(vaddr_t vaddr, vaddr_t vaddr_rel,
                               size_t size,
                               uint index_shift, uint page_size_shift,
                               pte_t *page_table, uint asid)
{
    pte_t *next_page_table;
    vaddr_t index;
    size_t chunk_size;
    vaddr_t vaddr_rem;
    vaddr_t block_size;
    vaddr_t block_mask;
    pte_t pte;
    paddr_t page_table_paddr;

    LTRACEF("vaddr 0x%lx, vaddr_rel 0x%lx, size 0x%lx, index shift %d, page_size_shift %d, page_table %p\n",
            vaddr, vaddr_rel, size, index_shift, page_size_shift, page_table);

    while (size) {
        block_size = 1UL << index_shift;
        block_mask = block_size - 1;
        vaddr_rem = vaddr_rel & block_mask;
        chunk_size = MIN(size, block_size - vaddr_rem);
        index = vaddr_rel >> index_shift;

        pte = page_table[index];

        if (index_shift > page_size_shift &&
            (pte & MMU_PTE_DESCRIPTOR_MASK) == MMU_PTE_L012_DESCRIPTOR_TABLE) {
            page_table_paddr = pte & MMU_PTE_OUTPUT_ADDR_MASK;
            next_page_table = paddr_to_kvaddr(page_table_paddr);
            arm64_mmu_unmap_pt(vaddr, vaddr_rem, chunk_size,
                               index_shift - (page_size_shift - 3),
                               page_size_shift,
                               next_page_table, asid);
            if (chunk_size == block_size ||
                page_table_is_clear(next_page_table, page_size_shift)) {
                LTRACEF("pte %p[0x%lx] = 0 (was page table)\n", page_table, index);
                page_table[index] = MMU_PTE_DESCRIPTOR_INVALID;
                __asm__ volatile("dmb ishst" ::: "memory");
                free_page_table(next_page_table, page_table_paddr, page_size_shift);
            }
        } else if (pte) {
            LTRACEF("pte %p[0x%lx] = 0\n", page_table, index);
            page_table[index] = MMU_PTE_DESCRIPTOR_INVALID;
            CF;
            if (asid == MMU_ARM64_GLOBAL_ASID)
                ARM64_TLBI(vaae1is, vaddr >> 12);
            else
                ARM64_TLBI(vae1is, vaddr >> 12 | (vaddr_t)asid << 48);
        } else {
            LTRACEF("pte %p[0x%lx] already clear\n", page_table, index);
        }
        vaddr += chunk_size;
        vaddr_rel += chunk_size;
        size -= chunk_size;
    }
}

static int arm64_mmu_map_pt(vaddr_t vaddr_in, vaddr_t vaddr_rel_in,
                            paddr_t paddr_in,
                            size_t size_in, pte_t attrs,
                            uint index_shift, uint page_size_shift,
                            pte_t *page_table, uint asid)
{
    int ret;
    pte_t *next_page_table;
    vaddr_t index;
    vaddr_t vaddr = vaddr_in;
    vaddr_t vaddr_rel = vaddr_rel_in;
    paddr_t paddr = paddr_in;
    size_t size = size_in;
    size_t chunk_size;
    vaddr_t vaddr_rem;
    vaddr_t block_size;
    vaddr_t block_mask;
    pte_t pte;

    LTRACEF("vaddr 0x%lx, vaddr_rel 0x%lx, paddr 0x%lx, size 0x%lx, attrs 0x%llx, index shift %d, page_size_shift %d, page_table %p\n",
            vaddr, vaddr_rel, paddr, size, attrs,
            index_shift, page_size_shift, page_table);

    if ((vaddr_rel | paddr | size) & ((1UL << page_size_shift) - 1)) {
        TRACEF("not page aligned\n");
        return ERR_INVALID_ARGS;
    }

    while (size) {
        block_size = 1UL << index_shift;
        block_mask = block_size - 1;
        vaddr_rem = vaddr_rel & block_mask;
        chunk_size = MIN(size, block_size - vaddr_rem);
        index = vaddr_rel >> index_shift;

        if (((vaddr_rel | paddr) & block_mask) ||
            (chunk_size != block_size) ||
            (index_shift > MMU_PTE_DESCRIPTOR_BLOCK_MAX_SHIFT)) {
            next_page_table = arm64_mmu_get_page_table(index, page_size_shift,
                                                       page_table);
            if (!next_page_table)
                goto err;

            ret = arm64_mmu_map_pt(vaddr, vaddr_rem, paddr, chunk_size, attrs,
                                   index_shift - (page_size_shift - 3),
                                   page_size_shift, next_page_table, asid);
            if (ret)
                goto err;
        } else {
            pte = page_table[index];
            if (pte) {
                TRACEF("page table entry already in use, index 0x%lx, 0x%llx\n",
                       index, pte);
                goto err;
            }

            pte = paddr | attrs;
            if (index_shift > page_size_shift)
                pte |= MMU_PTE_L012_DESCRIPTOR_BLOCK;
            else
                pte |= MMU_PTE_L3_DESCRIPTOR_PAGE;

            LTRACEF("pte %p[0x%lx] = 0x%llx\n", page_table, index, pte);
            page_table[index] = pte;
        }
        vaddr += chunk_size;
        vaddr_rel += chunk_size;
        paddr += chunk_size;
        size -= chunk_size;
    }

    return 0;

err:
    arm64_mmu_unmap_pt(vaddr_in, vaddr_rel_in, size_in - size,
                       index_shift, page_size_shift, page_table, asid);
    DSB;
    return ERR_GENERIC;
}

int arm64_mmu_map(vaddr_t vaddr, paddr_t paddr, size_t size, pte_t attrs,
                  vaddr_t vaddr_base, uint top_size_shift,
                  uint top_index_shift, uint page_size_shift,
                  pte_t *top_page_table, uint asid)
{
    int ret;
    vaddr_t vaddr_rel = vaddr - vaddr_base;
    vaddr_t vaddr_rel_max = 1UL << top_size_shift;

    LTRACEF("vaddr 0x%lx, paddr 0x%lx, size 0x%lx, attrs 0x%llx, asid 0x%x\n",
            vaddr, paddr, size, attrs, asid);

    if (vaddr_rel > vaddr_rel_max - size || size > vaddr_rel_max) {
        TRACEF("vaddr 0x%lx, size 0x%lx out of range vaddr 0x%lx, size 0x%lx\n",
               vaddr, size, vaddr_base, vaddr_rel_max);
        return ERR_INVALID_ARGS;
    }

    if (!top_page_table) {
        TRACEF("page table is NULL\n");
        return ERR_INVALID_ARGS;
    }

    ret = arm64_mmu_map_pt(vaddr, vaddr_rel, paddr, size, attrs,
                           top_index_shift, page_size_shift, top_page_table, asid);
    DSB;
    return ret;
}

int arm64_mmu_unmap(vaddr_t vaddr, size_t size,
                    vaddr_t vaddr_base, uint top_size_shift,
                    uint top_index_shift, uint page_size_shift,
                    pte_t *top_page_table, uint asid)
{
    vaddr_t vaddr_rel = vaddr - vaddr_base;
    vaddr_t vaddr_rel_max = 1UL << top_size_shift;

    LTRACEF("vaddr 0x%lx, size 0x%lx, asid 0x%x\n", vaddr, size, asid);

    if (vaddr_rel > vaddr_rel_max - size || size > vaddr_rel_max) {
        TRACEF("vaddr 0x%lx, size 0x%lx out of range vaddr 0x%lx, size 0x%lx\n",
               vaddr, size, vaddr_base, vaddr_rel_max);
        return ERR_INVALID_ARGS;
    }

    if (!top_page_table) {
        TRACEF("page table is NULL\n");
        return ERR_INVALID_ARGS;
    }

    arm64_mmu_unmap_pt(vaddr, vaddr_rel, size,
                       top_index_shift, page_size_shift, top_page_table, asid);
    DSB;
    return 0;
}

int arch_mmu_map(vaddr_t vaddr, paddr_t paddr, uint count, uint flags)
{
    return arm64_mmu_map(vaddr, paddr, count * PAGE_SIZE,
                         mmu_flags_to_pte_attr(flags),
                         ~0UL << MMU_KERNEL_SIZE_SHIFT, MMU_KERNEL_SIZE_SHIFT,
                         MMU_KERNEL_TOP_SHIFT, MMU_KERNEL_PAGE_SIZE_SHIFT,
                         arm64_kernel_translation_table, MMU_ARM64_GLOBAL_ASID);
}

int arch_mmu_unmap(vaddr_t vaddr, uint count)
{
    return arm64_mmu_unmap(vaddr, count * PAGE_SIZE,
                           ~0UL << MMU_KERNEL_SIZE_SHIFT, MMU_KERNEL_SIZE_SHIFT,
                           MMU_KERNEL_TOP_SHIFT, MMU_KERNEL_PAGE_SIZE_SHIFT,
                           arm64_kernel_translation_table,
                           MMU_ARM64_GLOBAL_ASID);
}
