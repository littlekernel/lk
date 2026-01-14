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

// aspace coverage per level with this scheme:
// L0: 16 * 512MB = 4GB
// L1: 64 * 4MB = 512MB
// L2: 1024 * 4KB = 4MB

// 68040 L2 table entry
typedef struct pte {
    uint32_t page_address : 20;
    uint32_t ur : 1;
    uint32_t g : 1;
    uint32_t u1 : 1;
    uint32_t u0 : 1;
    uint32_t s : 1;
    uint32_t cm : 2;
    uint32_t m : 1;
    uint32_t u : 1;
    uint32_t w : 1;
    uint32_t pdt : 2;
} pte_t;
static_assert(sizeof(pte_t) == 4, "");

// 68040 L1 table entry
typedef struct ptp {
    uint32_t table_address : 24;
    uint32_t _1 : 4;
    uint32_t u : 1;
    uint32_t w : 1;
    uint32_t udt : 2;
} ptp_t;
static_assert(sizeof(ptp_t) == 4, "");

// 68040 L0 table entry
typedef struct root_ptp {
    uint32_t table_address : 23;
    uint32_t _1 : 5;
    uint32_t u : 1;
    uint32_t w : 1;
    uint32_t udt : 2;
} root_ptp_t;
static_assert(sizeof(root_ptp_t) == 4, "");

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
#define L0_BYTES (L0_ENTRIES_RAW * sizeof(root_ptp_t))
#define L1_BYTES (L1_ENTRIES_RAW * sizeof(ptp_t))
#define L2_BYTES (L2_ENTRIES_RAW * sizeof(pte_t))
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
volatile root_ptp_t kernel_pgtable[L0_ENTRIES_RAW] __ALIGNED(L0_BYTES);
paddr_t kernel_pgtable_phys;

#else
// TODO: support 65030 in the future, probably using identical page table sizes
#error "unsupported m68k mmu"
#endif

static status_t alloc_pgtable(paddr_t *paddrp) {
    vm_page_t *p = pmm_alloc_page();
    if (!p) {
        return ERR_NO_MEMORY;
    }
    *paddrp = vm_page_to_paddr(p);
    return NO_ERROR;
}

static status_t free_pgtable(paddr_t ptable) {
    vm_page_t *page = paddr_to_vm_page(ptable);
    if (!page) {
        return ERR_NOT_FOUND;
    }
    pmm_free_page(page);
    return NO_ERROR;
}

// given a vaddr, generate the virtual index of the page table at that level.
// Needs to be shifted by Ln_REPEAT_SHIFT to get the raw entry
static uint get_l0_index(vaddr_t vaddr) {
    return (vaddr >> (L0_VADDR_SHIFT + L0_REPEAT_SHIFT)) & (L0_ENTRIES - 1);
}

static uint get_l1_index(vaddr_t vaddr) {
    return (vaddr >> (L1_VADDR_SHIFT + L1_REPEAT_SHIFT)) & (L1_ENTRIES - 1);
}

static uint get_l2_index(vaddr_t vaddr) {
    return (vaddr >> L2_VADDR_SHIFT) & (L2_ENTRIES - 1);
}

// Return a pointer to the first, possibly repeated, page table pointer in this level.
// Any updates will need to be repeated L0_REPEATS
static volatile root_ptp_t *get_l0_ptp_base_ptr(volatile root_ptp_t *table, vaddr_t vaddr) {
    const unsigned int idx = get_l0_index(vaddr) << L0_REPEAT_SHIFT;
    LTRACEF_LEVEL(3, "vaddr %#lx shifted idx: %u\n", vaddr, idx);

    return &table[idx];
}

// Return a pointer to the first, possibly repeated, page table pointer in this level.
// Any updates will need to be repeated L1_REPEATS
static volatile ptp_t *get_l1_ptp_base_ptr(volatile ptp_t *table, vaddr_t vaddr) {
    const unsigned int idx = get_l1_index(vaddr) << L1_REPEAT_SHIFT;
    LTRACEF_LEVEL(3, "vaddr %#lx shifted idx: %u\n", vaddr, idx);

    return &table[idx];
}

__NO_INLINE static void map_l0(volatile root_ptp_t *root_table, vaddr_t vaddr, paddr_t paddr) {
    LTRACEF("vaddr %#lx paddr %#lx\n", vaddr, paddr);

    volatile root_ptp_t *entry = get_l0_ptp_base_ptr(root_table, vaddr);
    for (uint i = 0; i < L0_REPEATS; i++) {
        const paddr_t pa = paddr + i * (1u << L1_SHIFT_RAW) * sizeof(ptp_t);

        const root_ptp_t ptp = {
            .table_address = pa >> 9,
            .u = 0,   // not used
            .w = 0,   // not write protected
            .udt = 3, // resident
        };
        entry[i] = ptp;
        LTRACEF_LEVEL(2, "real addr: %lx, index %u, shifted index %u\n", pa, get_l0_index(vaddr), (get_l0_index(vaddr) << L0_REPEAT_SHIFT) + i);
    }
}

__NO_INLINE static void map_l1(volatile ptp_t *table, vaddr_t vaddr, paddr_t paddr) {
    LTRACEF("vaddr %#lx paddr %#lx\n", vaddr, paddr);

    volatile ptp_t *entry = get_l1_ptp_base_ptr(table, vaddr);
    for (unsigned int i = 0; i < L1_REPEATS; i++) {
        const paddr_t pa = paddr + i * (1u << L2_SHIFT_RAW) * sizeof(ptp_t);

        const ptp_t ptp = {
            .table_address = pa >> 8,
            .u = 0,   // not used
            .w = 0,   // not write protected
            .udt = 3, // resident
        };
        entry[i] = ptp;
        LTRACEF_LEVEL(2, "real addr: %lx, index %u, shifted index %u\n", pa, get_l1_index(vaddr), (get_l1_index(vaddr) << L1_REPEAT_SHIFT) + i);
    }
}

__NO_INLINE static void map_l2(volatile pte_t *table, vaddr_t vaddr, paddr_t addr, uint flags) {
    const unsigned int idx = get_l2_index(vaddr);
    LTRACEF_LEVEL(2, "vaddr %#lx paddr %#lx, shifted idx: %u\n", vaddr, addr, idx);

    DEBUG_ASSERT(idx < L2_ENTRIES);

    // derive permissions from ARCH_MMU flags
    const bool user = (flags & ARCH_MMU_FLAG_PERM_USER) != 0;
    const bool ro = (flags & ARCH_MMU_FLAG_PERM_RO) != 0;
    uint cm;
    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_UNCACHED:
            cm = 3; // uncached
            break;
        case ARCH_MMU_FLAG_UNCACHED_DEVICE:
            cm = 2; // device
            break;
        case ARCH_MMU_FLAG_CACHED:
        default:
            cm = 0; // cacheable
            break;
    }

    const pte_t pte = {
        .page_address = addr >> 12,
        .g = 0,            // not global
        .s = user ? 0 : 1, // supervisor bit: 1=kernel, 0=user
        .cm = cm,          // cache mode
        .m = 0,            // not modified
        .u = 0,            // not used
        .w = ro ? 1 : 0,   // write protect (1=RO)
        .pdt = 1,          // resident
    };
    table[idx] = pte;
    LTRACEF_LEVEL(2, "mapped l2 idx %u vaddr %#lx to paddr %#lx flags 0x%x (user %u ro %u cm %u)\n",
                  idx, vaddr, addr, flags, user, ro, cm);
}

__NO_INLINE static void unmap_l0(volatile root_ptp_t *root_table, vaddr_t vaddr) {
    LTRACEF("vaddr %#lx\n", vaddr);

    volatile root_ptp_t *entry = get_l0_ptp_base_ptr(root_table, vaddr);
    for (uint i = 0; i < L0_REPEATS; i++) {
        entry[i] = (root_ptp_t){0};
        LTRACEF_LEVEL(2, "index %u, shifted index %u\n", get_l0_index(vaddr), (get_l0_index(vaddr) << L0_REPEAT_SHIFT) + i);
    }
}

__NO_INLINE static void unmap_l1(volatile ptp_t *table, vaddr_t vaddr) {
    LTRACEF("vaddr %#lx\n", vaddr);

    volatile ptp_t *entry = get_l1_ptp_base_ptr(table, vaddr);
    for (unsigned int i = 0; i < L1_REPEATS; i++) {
        entry[i] = (ptp_t){0};
        LTRACEF_LEVEL(2, "index %u, shifted index %u\n", get_l1_index(vaddr), (get_l1_index(vaddr) << L1_REPEAT_SHIFT) + i);
    }
}

__NO_INLINE static void unmap_l2(volatile pte_t *table, vaddr_t vaddr) {
    const unsigned int idx = get_l2_index(vaddr);
    LTRACEF_LEVEL(2, "vaddr %#lx, shifted idx: %u\n", vaddr, idx);

    DEBUG_ASSERT(idx < L2_ENTRIES);

    table[idx] = (pte_t){0};
}

static bool is_l1_table_empty(const volatile ptp_t *table) {
    for (unsigned int i = 0; i < L1_ENTRIES_RAW; i++) {
        ptp_t entry = table[i];
        if (entry.udt != 0) {
            return false;
        }
    }
    return true;
}

static bool is_l2_table_empty(const volatile pte_t *table) {
    for (unsigned int i = 0; i < L2_ENTRIES_RAW; i++) {
        pte_t entry = table[i];
        if (entry.pdt != 0) {
            return false;
        }
    }
    return true;
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

static bool is_l0_entry_valid(root_ptp_t entry) {
    // 0, 1 == invalid
    // 2, 3 == valid
    return entry.udt > 2;
}

static bool is_l1_entry_valid(ptp_t entry) {
    // 0, 1 == invalid
    // 2, 3 == valid
    return entry.udt > 2;
}

static bool is_l2_entry_valid(pte_t entry) {
    // 0 == invalid
    // 1, 2 == valid
    // 3 == indirect pointer (unused)
    return entry.pdt == 1 || entry.pdt == 2;
}

static status_t map_range_table(volatile root_ptp_t *root_table, vaddr_t va, paddr_t pa, size_t len_minus_one, uint flags) {
    DEBUG_ASSERT(IS_ALIGNED(va, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(pa, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(len_minus_one + 1, PAGE_SIZE));

    LTRACEF("va %#lx pa %#lx len %#zx, root_table %p\n", va, pa, len_minus_one + 1, root_table);

    const vaddr_t terminal_va = va + len_minus_one + 1;

    // iterate over the L0 level
    for (;;) {
        const root_ptp_t l0_entry = *get_l0_ptp_base_ptr(root_table, va);
        volatile ptp_t *l1_pgtable;
        if (!is_l0_entry_valid(l0_entry)) {
            // allocate a page table
            paddr_t pgtable;
            status_t err = alloc_pgtable(&pgtable);
            if (err < 0) {
                TRACEF("error allocating L1 page table\n");
                return err;
            }
            // zero the new page table before linking it in
            l1_pgtable = (volatile ptp_t *)paddr_to_kvaddr(pgtable);
            memset((void *)l1_pgtable, 0, PAGE_SIZE);
            map_l0(root_table, va, pgtable);
        } else {
            l1_pgtable = (volatile ptp_t *)paddr_to_kvaddr((paddr_t)l0_entry.table_address << 9);
        }

        LTRACEF_LEVEL(2, "l1_pgtable %p\n", l1_pgtable);

        // iterate over the L1 level
        do {
            const ptp_t l1_entry = *get_l1_ptp_base_ptr(l1_pgtable, va);
            volatile pte_t *l2_pgtable;
            if (!is_l1_entry_valid(l1_entry)) {
                // allocate a page table
                paddr_t pgtable;
                status_t err = alloc_pgtable(&pgtable);
                if (err < 0) {
                    TRACEF("error allocating L2 page table\n");
                    return err;
                }
                // zero the new page table before linking it in
                l2_pgtable = (volatile pte_t *)paddr_to_kvaddr(pgtable);
                memset((void *)l2_pgtable, 0, PAGE_SIZE);
                map_l1(l1_pgtable, va, pgtable);
            } else {
                l2_pgtable = (volatile pte_t *)paddr_to_kvaddr((paddr_t)l1_entry.table_address << 8);
            }

            LTRACEF_LEVEL(2, "l2_pgtable %p\n", l2_pgtable);

            // for every L2 page table entry, map a page
            do {
                map_l2(l2_pgtable, va, pa, flags);
                va += PAGE_SIZE;
                pa += PAGE_SIZE;

                // If we hit the terminal address, stop
                if (va == terminal_va) {
                    return NO_ERROR;
                }
            } while (get_l2_index(va) != 0);
        } while (get_l1_index(va) != 0);
    }
    return NO_ERROR;
}

// Alignment helper functions
static inline vaddr_t align_up_to_block(vaddr_t va, unsigned int shift) {
    return (va + (1u << shift)) & ~((1u << shift) - 1);
}

static inline vaddr_t align_down_to_block(vaddr_t va, unsigned int shift) {
    return va & ~((1u << shift) - 1);
}

// TLB flush helpers
static inline void tlb_flush_all(void) {
    asm volatile("pflusha" ::: "memory");
}

static inline void tlb_flush_user(void) {
    // Fallback: flush all translations; selective user flush requires proper pflush encoding
    asm volatile("pflusha" ::: "memory");
}

static status_t unmap_range_table(volatile root_ptp_t *root_table, vaddr_t va, size_t len_minus_one) {
    DEBUG_ASSERT(IS_ALIGNED(va, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(len_minus_one + 1, PAGE_SIZE));

    size_t remaining = len_minus_one + 1;

    // iterate over the L0 level
    for (;;) {
        const root_ptp_t l0_entry = *get_l0_ptp_base_ptr(root_table, va);
        if (!is_l0_entry_valid(l0_entry)) {
            // If L0 entry is invalid, there's nothing to unmap, jump to next L0 block
            vaddr_t next_va = align_up_to_block(va, L0_VADDR_SHIFT);
            size_t skip_size = next_va - va;
            if (skip_size >= remaining) {
                return NO_ERROR;
            }
            va = next_va;
            remaining -= skip_size;
            continue;
        }

        volatile ptp_t *l1_pgtable = (volatile ptp_t *)paddr_to_kvaddr((paddr_t)l0_entry.table_address << 9);
        vaddr_t l1_block_start_va = align_down_to_block(va, L1_VADDR_SHIFT);

        // iterate over the L1 level
        do {
            const ptp_t l1_entry = *get_l1_ptp_base_ptr(l1_pgtable, va);
            if (!is_l1_entry_valid(l1_entry)) {
                // If L1 entry is invalid, there's nothing to unmap, jump to next L1 block
                vaddr_t next_va = align_up_to_block(va, L1_VADDR_SHIFT);
                size_t skip_size = next_va - va;
                if (skip_size >= remaining) {
                    return NO_ERROR;
                }
                va = next_va;
                remaining -= skip_size;
                l1_block_start_va = align_down_to_block(va, L1_VADDR_SHIFT);
                break;
            }

            volatile pte_t *l2_pgtable = (volatile pte_t *)paddr_to_kvaddr((paddr_t)l1_entry.table_address << 8);
            vaddr_t l2_block_start_va = va;

            // for every L2 page table entry, unmap a page
            do {
                unmap_l2(l2_pgtable, va);
                va += PAGE_SIZE;
                remaining -= PAGE_SIZE;

                // If we've unmapped everything, check for empty tables and return
                if (remaining == 0) {
                    // Check if L2 table is now empty
                    if (is_l2_table_empty(l2_pgtable)) {
                        unmap_l1(l1_pgtable, l2_block_start_va);
                        // Check if L1 table is now empty
                        if (is_l1_table_empty(l1_pgtable)) {
                            unmap_l0(root_table, l1_block_start_va);
                        }
                    }
                    return NO_ERROR;
                }
            } while (get_l2_index(va) != 0);

            // We've finished unmapping all L2 entries in this L1 block
            // Check if L2 table is now empty
            if (is_l2_table_empty(l2_pgtable)) {
                unmap_l1(l1_pgtable, va - PAGE_SIZE);
                // Free the L2 page table page back to PMM
                free_pgtable(l1_entry.table_address << 8);
            }
        } while (get_l1_index(va) != 0);

        // We've finished unmapping all L1 entries in this L0 block
        // Check if L1 table is now empty
        if (is_l1_table_empty(l1_pgtable)) {
            unmap_l0(root_table, va - PAGE_SIZE);
            // Free the L1 page table page back to PMM
            free_pgtable(l0_entry.table_address << 9);
        }
    }
    return NO_ERROR;
}

// Default stub implementations for arch_mmu routines

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

    // Template: clear pointers. Freeing physical memory may require PMM helpers.
    if (aspace->pgtable_virt) {
        // Clear the table content to avoid leaving stale entries
        memset(aspace->pgtable_virt, 0, PAGE_SIZE);
    }

    // Free the root page table page back to PMM if it was allocated
    if (aspace->pgtable_phys) {
        vm_page_t *page = paddr_to_vm_page(aspace->pgtable_phys);
        if (page) {
            pmm_free_page(page);
        }
    }

    aspace->pgtable_phys = 0;
    aspace->pgtable_virt = NULL;

    return NO_ERROR;
}

int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) {
    LTRACEF("aspace %p, vaddr %#lx, paddr %#lx, count %u, flags %#x\n",
            aspace, vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);
    if (count == 0) {
        return 0;
    }

    if (flags & (ARCH_MMU_FLAG_PERM_NO_EXECUTE | ARCH_MMU_FLAG_NS)) {
        return ERR_INVALID_ARGS;
    }

    // Only support page-aligned mappings
    if (!IS_ALIGNED(vaddr, PAGE_SIZE) || !IS_ALIGNED(paddr, PAGE_SIZE)) {
        return ERR_INVALID_ARGS;
    }

    size_t len_minus_one = (size_t)count * PAGE_SIZE - 1;

    volatile root_ptp_t *root = aspace->pgtable_virt;
    status_t st = map_range_table(root, vaddr, paddr, len_minus_one, flags);
    return st;
}

int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    LTRACEF("aspace %p, vaddr %#lx, count %u\n", aspace, vaddr, count);

    DEBUG_ASSERT(aspace);
    if (count == 0) {
        return 0;
    }

    if (!IS_ALIGNED(vaddr, PAGE_SIZE)) {
        return ERR_INVALID_ARGS;
    }

    size_t len_minus_one = (size_t)count * PAGE_SIZE - 1;
    volatile root_ptp_t *root = aspace->pgtable_virt;
    status_t st = unmap_range_table(root, vaddr, len_minus_one);
    if (st == NO_ERROR) {
        tlb_flush_all();
    }
    return st;
}

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) {
    LTRACEF("aspace %p, vaddr %#lx\n", aspace, vaddr);

    // Disable interrupts around the ptest instruction in case we get preempted
    spin_lock_saved_state_t state;
    arch_interrupt_save(&state, 0);

    // Use the PTEST instruction to probe the translation
    uint32_t mmusr;
    asm volatile(
        "ptestr (%1)\n"
        "movec %%mmusr, %0"
        : "=r"(mmusr) : "a"(vaddr) : "memory");

    arch_interrupt_restore(state, 0);

    LTRACEF("vaddr %#x, mmusr %#x\n", (uint32_t)vaddr, mmusr);
    if ((mmusr & 0x1) == 0) {
        return ERR_NOT_FOUND;
    }

    // extract the physical address from the mmusr
    if (paddr) {
        *paddr = (mmusr & 0xfffff000) | (vaddr & 0xfff);
    }
    if (flags) {
        *flags = 0;
        *flags |= (mmusr & (1 << 2)) ? ARCH_MMU_FLAG_PERM_RO : 0;
        *flags |= (mmusr & (1 << 7)) ? 0 : ARCH_MMU_FLAG_PERM_USER;
        uint32_t cm = mmusr & (3 << 5);
        switch (cm) {
            case 0:
            case 1:
                *flags |= ARCH_MMU_FLAG_CACHED;
                break;
            case 2:
                *flags |= ARCH_MMU_FLAG_UNCACHED_DEVICE;
                break;
            case 3:
                *flags |= ARCH_MMU_FLAG_UNCACHED;
                break;
        }
    }
    return NO_ERROR;
}

void arch_mmu_context_switch(arch_aspace_t *aspace) {
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