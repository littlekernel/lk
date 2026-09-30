/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "arch/m68k/mmu.h"

#if M68K_MMU

#include <arch/mmu.h>
#include <arch/interrupts.h>
#include <arch/spinlock.h>
#include <assert.h>
#include <kernel/vm.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

// initial mappings set up in start.S
// NOTE: synchronize with logic in start.S
struct mmu_initial_mapping mmu_initial_mappings[] = {
    // all of memory
    {
        .phys = 0,
        .virt = KERNEL_ASPACE_BASE,
        .size = ROUNDUP(MEMSIZE, 512 * 1024 * 1024), // round up to next 512MB
        .flags = 0,
        .name = "physmap"},

    // null entry to terminate the list
    {}};

#if M68K_MMU == 68040

// 68040's layout is
// 4 or 8K pages. only affects the bottom level
// 32 bit entries at all levels
//              L0,  L1,  L2
// bits:        7,   7,   6,  12 (4K pages)
// entries:     128, 128, 64
// bytes/table: 512, 512, 256
//
// if using 4K page tables for L1 and L2, and using a small root table (L0):
//                           L0,         L1,           L2
// entries:                  128,        8*128 (1024), 16*64 (1024)
// usable entries per level: 128/8 (16), 1024/16 (64), 1024
//
// Each usable entry at L0 and L1 is a run of 8 or 16 hardware entries that point
// at consecutive tables in one 4K page, so the first entry of a run points at
// the page itself.

// aspace coverage per level with this scheme:
// L0: 16 * 256MB = 4GB
// L1: 64 * 4MB = 256MB
// L2: 1024 * 4KB = 4MB

// Descriptor bits (68040 UM section 3.2). A root or pointer descriptor is
// resident when UDT is 2 or 3, a page descriptor when PDT is 1 or 3; PDT 2 is
// an indirect descriptor, which is never made here. New ones get 3.
#define DESC_DT_MASK        3u
#define DESC_DT_RESIDENT    3u
#define DESC_W              (1u << 2)   // write protected
#define PTE_CM_SHIFT        5
#define PTE_CM_MASK         (3u << PTE_CM_SHIFT)
#define PTE_S               (1u << 7)   // supervisor only
#define PTE_PAGE_MASK       0xfffff000u

static inline bool udt_resident(uint32_t desc) {
    return desc & 2;
}

static inline bool pdt_resident(uint32_t desc) {
    return desc & 1;
}

// cache modes
#define CM_CACHED_WRITETHROUGH  0
#define CM_CACHED_COPYBACK      1
#define CM_UNCACHED_SERIALIZED  2
#define CM_UNCACHED             3

// some constants based on this
#define L0_SHIFT_RAW 7
#define L1_SHIFT_RAW 7
#define L2_SHIFT_RAW 6

// number of entries to repeat per level to get our emulated tables
#define L0_REPEAT_SHIFT 3
#define L1_REPEAT_SHIFT 4

#define L0_REPEATS (1 << L0_REPEAT_SHIFT)
#define L1_REPEATS (1 << L1_REPEAT_SHIFT)
static_assert(L0_REPEATS == 8, "");
static_assert(L1_REPEATS == 16, "");

// number of entries per level
#define L0_ENTRIES_RAW (1 << L0_SHIFT_RAW)
#define L1_ENTRIES_RAW (1 << (L1_SHIFT_RAW + L0_REPEAT_SHIFT))
#define L2_ENTRIES_RAW (1 << (L2_SHIFT_RAW + L1_REPEAT_SHIFT))
static_assert(L0_ENTRIES_RAW == 128, "");
static_assert(L1_ENTRIES_RAW == 1024, "");
static_assert(L2_ENTRIES_RAW == 1024, "");

// number of bytes per level
#define L0_BYTES (L0_ENTRIES_RAW * sizeof(uint32_t))
#define L1_BYTES (L1_ENTRIES_RAW * sizeof(uint32_t))
#define L2_BYTES (L2_ENTRIES_RAW * sizeof(uint32_t))
static_assert(L0_BYTES == 512, "");
static_assert(L1_BYTES == 4096, "");
static_assert(L2_BYTES == 4096, "");

// number of unique entries per level
// pow2 4+6+10
#define L0_ENTRIES (1u << (L0_SHIFT_RAW - L0_REPEAT_SHIFT))
#define L1_ENTRIES (1u << (L1_SHIFT_RAW - L1_REPEAT_SHIFT + L0_REPEAT_SHIFT))
#define L2_ENTRIES (1u << (L2_SHIFT_RAW + L1_REPEAT_SHIFT))
static_assert(L0_ENTRIES == 16, "");
static_assert(L1_ENTRIES == 64, "");
static_assert(L2_ENTRIES == 1024, "");

// for a given virtual address, which bits correspond to what layer of the table
#define L0_VADDR_SHIFT (32 - L0_SHIFT_RAW)
#define L1_VADDR_SHIFT (L0_VADDR_SHIFT - L1_SHIFT_RAW)
#define L2_VADDR_SHIFT (L1_VADDR_SHIFT - L2_SHIFT_RAW)

static_assert(L0_VADDR_SHIFT == 25, "");
static_assert(L1_VADDR_SHIFT == 18, "");
static_assert(L2_VADDR_SHIFT == 12, "");

// statically allocated kernel page table, initialized in start.S
// NOTE: smaller than a full 4K page, but is actually all hardware cares
// about at the root level. (128 * 4 = 512 bytes)
volatile uint32_t kernel_pgtable[L0_ENTRIES_RAW] __ALIGNED(L0_BYTES);
paddr_t kernel_pgtable_phys;

#else
// TODO: support 65030 in the future, probably using identical page table sizes
#error "unsupported m68k mmu"
#endif

static status_t alloc_pgtable(paddr_t *paddrp) {
    if (!vm_ptable_budget_take()) {
        return ERR_NO_MEMORY;
    }

    vm_page_t *p = pmm_alloc_page();
    if (!p) {
        return ERR_NO_MEMORY;
    }
    *paddrp = vm_page_to_paddr(p);
    return NO_ERROR;
}

// The span one usable entry covers at a level (0 and 1; 2 holds the pages),
// how many hardware entries it repeats over, and the table size they step by.
static const vaddr_t level_span[2] = { 1u << (L0_VADDR_SHIFT + L0_REPEAT_SHIFT),
                                       1u << (L1_VADDR_SHIFT + L1_REPEAT_SHIFT) };
static const uint level_repeats[2] = { L0_REPEATS, L1_REPEATS };
static const uint level_next_bytes[2] = { L1_ENTRIES_RAW / L0_REPEATS * sizeof(uint32_t),
                                          L2_ENTRIES_RAW / L1_REPEATS * sizeof(uint32_t) };

// the first hardware entry of the run for vaddr at level 0 or 1
static volatile uint32_t *level_entry(volatile uint32_t *table, int level, vaddr_t vaddr) {
    const uint index = (vaddr / level_span[level]) & (level == 0 ? L0_ENTRIES - 1 : L1_ENTRIES - 1);
    return &table[index * level_repeats[level]];
}

static volatile uint32_t *next_table(uint32_t entry) {
    return (volatile uint32_t *)paddr_to_kvaddr(entry & ~(PAGE_SIZE - 1));
}

static volatile uint32_t *pte_entry(volatile uint32_t *table, vaddr_t vaddr) {
    return &table[(vaddr >> L2_VADDR_SHIFT) & (L2_ENTRIES - 1)];
}

static void link_table(volatile uint32_t *entry, int level, paddr_t table) {
    for (uint i = 0; i < level_repeats[level]; i++) {
        entry[i] = (table + i * level_next_bytes[level]) | DESC_DT_RESIDENT;
    }
}

static void unlink_table(volatile uint32_t *entry, int level) {
    for (uint i = 0; i < level_repeats[level]; i++) {
        entry[i] = 0;
    }
}

static bool table_is_empty(const volatile uint32_t *table) {
    for (uint i = 0; i < PAGE_SIZE / sizeof(uint32_t); i++) {
        if (table[i] & DESC_DT_MASK) {
            return false;
        }
    }
    return true;
}

// the boot tables start.S fills in for the physmap, which live in the image's bss
static bool table_is_static(paddr_t table) {
    extern uint8_t __bss_start[], __bss_end[];
    const uint8_t *va = paddr_to_kvaddr(table);
    return va >= __bss_start && va < __bss_end;
}

// The 68040 ATC also caches a walk that ended at an invalid descriptor, so a
// new mapping needs the flush as much as a removed or changed one.
static inline void tlb_flush_all(void) {
    asm volatile("pflusha" ::: "memory");
}

static inline void tlb_flush_user(void) {
    // Fallback: flush all translations; selective user flush requires proper pflush encoding
    asm volatile("pflusha" ::: "memory");
}

// The page descriptor for vaddr, or NULL if a table on the way is missing.
static volatile uint32_t *lookup_pte(volatile uint32_t *root, vaddr_t vaddr) {
    volatile uint32_t *table = root;
    for (int level = 0; level < 2; level++) {
        const uint32_t entry = *level_entry(table, level, vaddr);
        if (!udt_resident(entry)) {
            return NULL;
        }
        table = next_table(entry);
    }
    return pte_entry(table, vaddr);
}

// Point vaddr's page descriptor at pte, linking in zeroed tables on the way.
static status_t map_page(volatile uint32_t *root, vaddr_t vaddr, uint32_t pte) {
    volatile uint32_t *table = root;
    for (int level = 0; level < 2; level++) {
        volatile uint32_t *entry = level_entry(table, level, vaddr);
        if (!udt_resident(*entry)) {
            paddr_t pa;
            status_t err = alloc_pgtable(&pa);
            if (err != NO_ERROR) {
                LTRACEF("out of page tables at level %d\n", level + 1);
                return err;
            }
            memset(paddr_to_kvaddr(pa), 0, PAGE_SIZE);
            link_table(entry, level, pa);
        }
        table = next_table(*entry);
    }

    volatile uint32_t *entry = pte_entry(table, vaddr);
    if (*entry & DESC_DT_MASK) {
        return ERR_ALREADY_EXISTS;
    }
    *entry = pte;
    return NO_ERROR;
}

// Clear [vaddr, last] (last inclusive) under table at level, stepping over
// holes. A table below that ends up empty is unlinked and put on freed, unless
// it is one of the boot tables.
static void unmap_table(volatile uint32_t *table, int level, vaddr_t vaddr, vaddr_t last,
                        struct list_node *freed) {
    if (level == 2) {
        for (;;) {
            *pte_entry(table, vaddr) = 0;
            if (vaddr == ROUNDDOWN(last, PAGE_SIZE)) {
                return;
            }
            vaddr += PAGE_SIZE;
        }
    }

    for (;;) {
        volatile uint32_t *entry = level_entry(table, level, vaddr);
        const vaddr_t entry_last = MIN(ROUNDDOWN(vaddr, level_span[level]) + (level_span[level] - 1), last);
        const uint32_t e = *entry;

        if (udt_resident(e)) {
            volatile uint32_t *next = next_table(e);
            unmap_table(next, level + 1, vaddr, entry_last, freed);

            const paddr_t next_pa = e & ~(PAGE_SIZE - 1);
            if (table_is_empty(next) && !table_is_static(next_pa)) {
                LTRACEF_LEVEL(2, "unlinking empty table %#lx at level %d\n", next_pa, level + 1);
                unlink_table(entry, level);
                vm_page_t *page = paddr_to_vm_page(next_pa);
                DEBUG_ASSERT(page);
                list_add_tail(freed, &page->node);
            }
        }

        if (entry_last == last) {
            return;
        }
        vaddr = entry_last + 1;
    }
}

// Unmap [vaddr, vaddr + count pages), flush, and only then hand back the
// tables that were unlinked.
static void unmap_range(volatile uint32_t *root, vaddr_t vaddr, uint count) {
    struct list_node freed = LIST_INITIAL_VALUE(freed);
    unmap_table(root, 0, vaddr, vaddr + ((vaddr_t)count * PAGE_SIZE - 1), &freed);
    tlb_flush_all();
    pmm_free(&freed);
}

// Give every page in [vaddr, last] the write protect bit wp. With check_only
// nothing is written, and the walk only reports ERR_INVALID_ARGS on an uncached
// page when execute is asked for.
static status_t protect_table(volatile uint32_t *table, int level, vaddr_t vaddr, vaddr_t last,
                              uint32_t wp, bool execute, bool check_only) {
    if (level == 2) {
        for (;;) {
            volatile uint32_t *entry = pte_entry(table, vaddr);
            const uint32_t e = *entry;
            if (pdt_resident(e)) {
                if (check_only) {
                    const uint cm = (e & PTE_CM_MASK) >> PTE_CM_SHIFT;
                    if (execute && (cm == CM_UNCACHED || cm == CM_UNCACHED_SERIALIZED)) {
                        return ERR_INVALID_ARGS;
                    }
                } else {
                    *entry = (e & ~DESC_W) | wp;
                }
            }
            if (vaddr == ROUNDDOWN(last, PAGE_SIZE)) {
                return NO_ERROR;
            }
            vaddr += PAGE_SIZE;
        }
    }

    for (;;) {
        const uint32_t e = *level_entry(table, level, vaddr);
        const vaddr_t entry_last = MIN(ROUNDDOWN(vaddr, level_span[level]) + (level_span[level] - 1), last);

        if (udt_resident(e)) {
            status_t err = protect_table(next_table(e), level + 1, vaddr, entry_last, wp, execute,
                                         check_only);
            if (err != NO_ERROR) {
                return err;
            }
        }

        if (entry_last == last) {
            return NO_ERROR;
        }
        vaddr = entry_last + 1;
    }
}

#define MMU_REG_ACCESSOR(reg)                                       \
    static uint32_t get_##reg(void) {                               \
        uint32_t reg;                                               \
        asm volatile("movec %%" #reg ", %0" : "=r"(reg)::"memory"); \
        return reg;                                                 \
    }                                                               \
    static void set_##reg(uint32_t val) {                           \
        asm volatile("movec %0, %%" #reg ::"r"(val) : "memory");    \
    }

// Control register accessors
MMU_REG_ACCESSOR(tc);
MMU_REG_ACCESSOR(itt0);
MMU_REG_ACCESSOR(itt1);
MMU_REG_ACCESSOR(dtt0);
MMU_REG_ACCESSOR(dtt1);
MMU_REG_ACCESSOR(mmusr);
MMU_REG_ACCESSOR(urp);
MMU_REG_ACCESSOR(srp);

static void dump_mmu_regs(void) {
    // Dump all the registers
    printf("TC %#x\n", get_tc());
    printf("ITT0 %#x\n", get_itt0());
    printf("ITT1 %#x\n", get_itt1());
    printf("DTT0 %#x\n", get_dtt0());
    printf("DTT1 %#x\n", get_dtt1());
    printf("MMUSR %#x\n", get_mmusr());
    printf("URP %#x\n", get_urp());
    printf("SRP %#x\n", get_srp());
}

bool arch_mmu_supports_nx_mappings(void) {
    return false;
}

bool arch_mmu_supports_ns_mappings(void) {
    return false;
}

bool arch_mmu_supports_user_aspaces(void) {
    return false;
}

status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) {
    DEBUG_ASSERT(aspace);

    LTRACEF("aspace %p, base %#lx, size %#zx, flags %#x\n", aspace, base, size, flags);

    aspace->base = base;
    aspace->size = size;
    aspace->flags = flags;

    // Kernel aspace uses the statically allocated kernel root table
    if (flags & ARCH_ASPACE_FLAG_KERNEL) {
        aspace->pgtable_virt = (void *)kernel_pgtable;
        aspace->pgtable_phys = kernel_pgtable_phys;
    } else {
        // Allocate a page to serve as the user root table (L0)
        // TODO: consider just mallocing this since it only really needs to be
        // 512 bytes long.
        paddr_t pgtable;
        status_t err = alloc_pgtable(&pgtable);
        if (err < 0) {
            return err;
        }

        aspace->pgtable_phys = pgtable;
        aspace->pgtable_virt = paddr_to_kvaddr(pgtable);

        // Zero the new page table.
        memset(aspace->pgtable_virt, 0, PAGE_SIZE);
    }

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    DEBUG_ASSERT(aspace);

    LTRACEF("aspace %p\n", aspace);

    // Nothing to do for kernel aspace
    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        return NO_ERROR;
    }

    // nothing mapped means no table below the root
    const volatile uint32_t *root = aspace->pgtable_virt;
    for (uint i = 0; i < L0_ENTRIES_RAW; i++) {
        DEBUG_ASSERT_MSG(root[i] == 0, "aspace %p still has a table at root entry %u\n", aspace, i);
    }

    vm_page_t *page = paddr_to_vm_page(aspace->pgtable_phys);
    DEBUG_ASSERT(page);
    pmm_free_page(page);

    aspace->pgtable_phys = 0;
    aspace->pgtable_virt = NULL;

    return NO_ERROR;
}

status_t arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) {
    LTRACEF("aspace %p, vaddr %#lx, paddr %#lx, count %u, flags %#x\n",
            aspace, vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);

    // Leaving out EXECUTE is accepted, but the 68040 has no NX so the page
    // stays executable.
    if (!IS_ALIGNED(vaddr, PAGE_SIZE) || !IS_ALIGNED(paddr, PAGE_SIZE) ||
        !arch_mmu_paddr_range_valid(paddr, count, 32) || !arch_mmu_flags_valid(flags)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    uint cm;
    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_UNCACHED:
            cm = CM_UNCACHED;
            break;
        case ARCH_MMU_FLAG_UNCACHED_DEVICE:
            cm = CM_UNCACHED_SERIALIZED;
            break;
        default:
            cm = CM_CACHED_WRITETHROUGH;
            break;
    }
    const uint32_t pte_flags = (cm << PTE_CM_SHIFT) |
                               ((flags & ARCH_MMU_FLAG_PERM_USER) ? 0 : PTE_S) |
                               ((flags & ARCH_MMU_FLAG_PERM_WRITE) ? 0 : DESC_W) |
                               DESC_DT_RESIDENT;

    volatile uint32_t *root = aspace->pgtable_virt;
    status_t err = NO_ERROR;
    for (uint i = 0; i < count; i++) {
        const vaddr_t offset = (vaddr_t)i * PAGE_SIZE;
        err = map_page(root, vaddr + offset, (paddr + offset) | pte_flags);
        if (err != NO_ERROR) {
            // Take back what went in: the pages before a collision, or through
            // the page a table was missing for, so an empty table linked for it
            // goes as well.
            const uint done = i + (err == ERR_NO_MEMORY ? 1 : 0);
            if (done > 0) {
                unmap_range(root, vaddr, done);
            }
            return err;
        }
    }

    tlb_flush_all();
    return NO_ERROR;
}

status_t arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    LTRACEF("aspace %p, vaddr %#lx, count %u\n", aspace, vaddr, count);

    DEBUG_ASSERT(aspace);

    if (!IS_ALIGNED(vaddr, PAGE_SIZE)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    unmap_range(aspace->pgtable_virt, vaddr, count);
    return NO_ERROR;
}

status_t arch_mmu_protect(arch_aspace_t *aspace, vaddr_t vaddr, uint count, uint flags) {
    LTRACEF("aspace %p, vaddr %#lx, count %u, flags %#x\n", aspace, vaddr, count, flags);

    DEBUG_ASSERT(aspace);

    if (!IS_ALIGNED(vaddr, PAGE_SIZE) || !arch_mmu_protect_flags_valid(flags)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    // without NX, EXECUTE changes nothing but is still refused on uncached memory
    volatile uint32_t *root = aspace->pgtable_virt;
    const vaddr_t last = vaddr + ((vaddr_t)count * PAGE_SIZE - 1);
    const bool execute = flags & ARCH_MMU_FLAG_PERM_EXECUTE;
    const uint32_t wp = (flags & ARCH_MMU_FLAG_PERM_WRITE) ? 0 : DESC_W;

    if (execute) {
        status_t err = protect_table(root, 0, vaddr, last, wp, true, true);
        if (err != NO_ERROR) {
            return err;
        }
    }

    protect_table(root, 0, vaddr, last, wp, execute, false);
    tlb_flush_all();
    return NO_ERROR;
}

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) {
    LTRACEF("aspace %p, vaddr %#lx\n", aspace, vaddr);

    DEBUG_ASSERT(aspace);

    if (!arch_mmu_range_in_aspace(aspace, vaddr, 1)) {
        return ERR_OUT_OF_RANGE;
    }

    // the aspace's own tables, not the live MMU: the transparent translation
    // windows are not part of any aspace
    const volatile uint32_t *entry = lookup_pte(aspace->pgtable_virt, vaddr);
    if (!entry || !pdt_resident(*entry)) {
        return ERR_NOT_FOUND;
    }
    const uint32_t pte = *entry;
    LTRACEF("vaddr %#lx, pte %#x\n", vaddr, pte);

    if (paddr) {
        *paddr = (pte & PTE_PAGE_MASK) | (vaddr & (PAGE_SIZE - 1));
    }
    if (flags) {
        // without NX every page can be executed
        *flags = ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_EXECUTE;
        *flags |= (pte & DESC_W) ? 0 : ARCH_MMU_FLAG_PERM_WRITE;
        *flags |= (pte & PTE_S) ? 0 : ARCH_MMU_FLAG_PERM_USER;
        switch ((pte & PTE_CM_MASK) >> PTE_CM_SHIFT) {
            case CM_CACHED_WRITETHROUGH:
            case CM_CACHED_COPYBACK:
                *flags |= ARCH_MMU_FLAG_CACHED;
                break;
            case CM_UNCACHED_SERIALIZED:
                *flags |= ARCH_MMU_FLAG_UNCACHED_DEVICE;
                break;
            case CM_UNCACHED:
                *flags |= ARCH_MMU_FLAG_UNCACHED;
                break;
        }
    }
    return NO_ERROR;
}

void arch_mmu_context_switch(arch_aspace_t *old_aspace, arch_aspace_t *aspace) {
    // If unloading user space or switching to kernel, point URP at kernel root
    if (!aspace || (aspace->flags & ARCH_ASPACE_FLAG_KERNEL)) {
        set_urp((uint32_t)(uintptr_t)kernel_pgtable);
        // Flush all translations to avoid stale entries
        tlb_flush_all();
        return;
    }

    // Switch URP to the new user address space root table
    // Assume pgtable_phys is valid when aspace is present
    set_urp((uint32_t)aspace->pgtable_phys);

    // Flush translations to ensure new context takes effect
    tlb_flush_user();
}

// initialization
void m68k_mmu_init(void) {
    LTRACE_ENTRY;

    if (LOCAL_TRACE) {
        dump_mmu_regs();
    }

    LTRACE_EXIT;
}

void m68k_mmu_early_init(void) {
}

#endif // M68K_MMU
