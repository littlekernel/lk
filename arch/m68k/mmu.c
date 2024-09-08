/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "arch/m68k/mmu.h"

#include <assert.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdint.h>
#include <stdlib.h>

#define LOCAL_TRACE 1

#if M68K_MMU

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
//                           L0,    L1,           L2
// entries:                  128,   8*128 (1024), 16*64 (1024)
// usable entries per level: 128/8, 1024/16,      1024

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

static volatile root_ptp_t kernel_pgtable[L0_ENTRIES_RAW] __ALIGNED(L0_BYTES);

#else
// TODO: support 65030 in the future, probably using identical page table sizes
#error "unsupported m68k mmu"
#endif

static status_t alloc_pgtable(paddr_t *paddrp) {
#if WITH_KERNEL_VM
    vm_page_t *p = pmm_alloc_page();
    if (!p) {
        return ERR_NO_MEMORY;
    }
    *paddrp = vm_page_to_paddr(p);
#else
    // XXX hack for now before we allocate from PMM
    static uint32_t pgtables[L0_ENTRIES * L1_ENTRIES * L2_ENTRIES] __ALIGNED(PAGE_SIZE);
    static size_t next_pgtable = 0;
    *paddrp = (paddr_t)&pgtables[next_pgtable * L2_ENTRIES];
    next_pgtable++;
    LTRACEF("returning %#lx\n", *paddrp);
#endif
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

__NO_INLINE static void map_l2(volatile pte_t *table, vaddr_t vaddr, paddr_t addr) {
    const unsigned int idx = get_l2_index(vaddr);
    LTRACEF_LEVEL(2, "vaddr %#lx paddr %#lx, shifted idx: %u\n", vaddr, addr, idx);

    DEBUG_ASSERT(idx < L2_ENTRIES);

    const pte_t pte = {
        .page_address = addr >> 12,
        .g = 0,   // not global
        .s = 1,   // supervisor
        .cm = 0,  // cache mode, cacheable
        .m = 0,   // not modified
        .u = 0,   // not used
        .w = 0,   // not write protected
        .pdt = 1, // resident
    };
    table[idx] = pte;
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

void m68k_mmu_early_init(void) {}

static status_t map_range(vaddr_t va, paddr_t pa, size_t len_minus_one) {
    DEBUG_ASSERT(IS_ALIGNED(va, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(pa, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(len_minus_one + 1, PAGE_SIZE));

    const vaddr_t terminal_va = va + len_minus_one + 1;

    // iterate over the L0 level
    for (;;) {
        const root_ptp_t l0_entry = *get_l0_ptp_base_ptr(kernel_pgtable, va);
        volatile ptp_t *l1_pgtable;
        if (!is_l0_entry_valid(l0_entry)) {
            // allocate a page table
            paddr_t pgtable;
            status_t err = alloc_pgtable(&pgtable);
            if (err < 0) {
                TRACEF("error allocating L1 page table\n");
                return err;
            }
            map_l0(kernel_pgtable, va, pgtable);
            l1_pgtable = (volatile ptp_t *)pgtable;
        } else {
            l1_pgtable = (volatile ptp_t *)((uintptr_t)l0_entry.table_address << 9);
        }

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
                map_l1(l1_pgtable, va, pgtable);
                l2_pgtable = (volatile pte_t *)pgtable;
            } else {
                l2_pgtable = (volatile pte_t *)((uintptr_t)l1_entry.table_address << 8);
            }

            // for every L2 page table entry, map a page
            do {
                map_l2(l2_pgtable, va, pa);
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

void m68k_mmu_init(void) {
    LTRACE_ENTRY;

    // set up some helpful maps for qemu virt
    map_range(0, 0, 64 * 1024 * 1024 - 1);
    map_range(0xff000000, 0xff000000, 0 - 0xff000000 - 1);

    // a few test mappings to stress the mapper
    map_range(0xc0104000, 0x12345000, 0x268000 - 1);
    map_range(0xc0371000, 0x6789a000, 0x440000 - 1);

    // set the root pointers
    set_srp((uint32_t)(uintptr_t)kernel_pgtable);
    set_urp((uint32_t)(uintptr_t)kernel_pgtable);
    set_tc((1 << 15)); // enable, 4K pages

    dump_mmu_regs();

    LTRACE_EXIT;
}

#endif // M68K_MMU