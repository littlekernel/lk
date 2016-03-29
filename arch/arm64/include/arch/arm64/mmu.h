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

#ifndef __ARCH_ARM64_MMU_H
#define __ARCH_ARM64_MMU_H

#include <arch/defines.h>

#define IFTE(c,t,e) (!!(c) * (t) | !(c) * (e))
#define NBITS01(n)      IFTE(n, 1, 0)
#define NBITS02(n)      IFTE((n) >>  1,  1 + NBITS01((n) >>  1), NBITS01(n))
#define NBITS04(n)      IFTE((n) >>  2,  2 + NBITS02((n) >>  2), NBITS02(n))
#define NBITS08(n)      IFTE((n) >>  4,  4 + NBITS04((n) >>  4), NBITS04(n))
#define NBITS16(n)      IFTE((n) >>  8,  8 + NBITS08((n) >>  8), NBITS08(n))
#define NBITS32(n)      IFTE((n) >> 16, 16 + NBITS16((n) >> 16), NBITS16(n))
#define NBITS(n)        IFTE((n) >> 32, 32 + NBITS32((n) >> 32), NBITS32(n))

#ifndef MMU_KERNEL_SIZE_SHIFT
#define KERNEL_ASPACE_BITS (NBITS(0xffffffffffffffff-KERNEL_ASPACE_BASE))
#define KERNEL_BASE_BITS (NBITS(0xffffffffffffffff-KERNEL_BASE))
#if KERNEL_BASE_BITS > KERNEL_ASPACE_BITS
#define KERNEL_ASPACE_BITS KERNEL_BASE_BITS /* KERNEL_BASE should not be below KERNEL_ASPACE_BASE */
#endif

#if KERNEL_ASPACE_BITS < 25
#define MMU_KERNEL_SIZE_SHIFT (25)
#else
#define MMU_KERNEL_SIZE_SHIFT (KERNEL_ASPACE_BITS)
#endif
#endif

#ifndef MMU_USER_SIZE_SHIFT
#define MMU_USER_SIZE_SHIFT 48
#endif

#ifndef MMU_IDENT_SIZE_SHIFT
#define MMU_IDENT_SIZE_SHIFT 42 /* Max size supported by block mappings */
#endif

#define MMU_KERNEL_PAGE_SIZE_SHIFT      (PAGE_SIZE_SHIFT)
#define MMU_USER_PAGE_SIZE_SHIFT        (USER_PAGE_SIZE_SHIFT)

#if MMU_IDENT_SIZE_SHIFT < 25
#error MMU_IDENT_SIZE_SHIFT too small
#elif MMU_IDENT_SIZE_SHIFT <= 29 /* Use 2MB block mappings (4K page size) */
#define MMU_IDENT_PAGE_SIZE_SHIFT       (SHIFT_4K)
#elif MMU_IDENT_SIZE_SHIFT <= 30 /* Use 512MB block mappings (64K page size) */
#define MMU_IDENT_PAGE_SIZE_SHIFT       (SHIFT_64K)
#elif MMU_IDENT_SIZE_SHIFT <= 39 /* Use 1GB block mappings (4K page size) */
#define MMU_IDENT_PAGE_SIZE_SHIFT       (SHIFT_4K)
#elif MMU_IDENT_SIZE_SHIFT <= 42 /* Use 512MB block mappings (64K page size) */
#define MMU_IDENT_PAGE_SIZE_SHIFT       (SHIFT_64K)
#else
#error MMU_IDENT_SIZE_SHIFT too large
#endif

/*
 * TCR TGx values
 *
 * Page size:   4K      16K     64K
 * TG0:         0       2       1
 * TG1:         2       1       3
 */

#define MMU_TG0(page_size_shift) ((((page_size_shift == 14) & 1) << 1) | \
                                  ((page_size_shift == 16) & 1))

#define MMU_TG1(page_size_shift) ((((page_size_shift == 12) & 1) << 1) | \
                                  ((page_size_shift == 14) & 1) | \
                                  ((page_size_shift == 16) & 1) | \
                                  (((page_size_shift == 16) & 1) << 1))

#define MMU_LX_X(page_shift, level) ((4 - (level)) * ((page_shift) - 3) + 3)

#if MMU_USER_SIZE_SHIFT > MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 0)
#define MMU_USER_TOP_SHIFT MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 0)
#elif MMU_USER_SIZE_SHIFT > MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 1)
#define MMU_USER_TOP_SHIFT MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 1)
#elif MMU_USER_SIZE_SHIFT > MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 2)
#define MMU_USER_TOP_SHIFT MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 2)
#elif MMU_USER_SIZE_SHIFT > MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 3)
#define MMU_USER_TOP_SHIFT MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 3)
#else
#error User address space size must be larger than page size
#endif
#define MMU_USER_PAGE_TABLE_ENTRIES_TOP (0x1 << (MMU_USER_SIZE_SHIFT - MMU_USER_TOP_SHIFT))

#if MMU_KERNEL_SIZE_SHIFT > MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 0)
#define MMU_KERNEL_TOP_SHIFT MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 0)
#elif MMU_KERNEL_SIZE_SHIFT > MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 1)
#define MMU_KERNEL_TOP_SHIFT MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 1)
#elif MMU_KERNEL_SIZE_SHIFT > MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 2)
#define MMU_KERNEL_TOP_SHIFT MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 2)
#elif MMU_KERNEL_SIZE_SHIFT > MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 3)
#define MMU_KERNEL_TOP_SHIFT MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 3)
#else
#error Kernel address space size must be larger than page size
#endif
#define MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP (0x1 << (MMU_KERNEL_SIZE_SHIFT - MMU_KERNEL_TOP_SHIFT))

#if MMU_IDENT_SIZE_SHIFT > MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 0)
#define MMU_IDENT_TOP_SHIFT MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 0)
#elif MMU_IDENT_SIZE_SHIFT > MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 1)
#define MMU_IDENT_TOP_SHIFT MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 1)
#elif MMU_IDENT_SIZE_SHIFT > MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 2)
#define MMU_IDENT_TOP_SHIFT MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 2)
#elif MMU_IDENT_SIZE_SHIFT > MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 3)
#define MMU_IDENT_TOP_SHIFT MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 3)
#else
#error Ident address space size must be larger than page size
#endif
#define MMU_PAGE_TABLE_ENTRIES_IDENT_SHIFT (MMU_IDENT_SIZE_SHIFT - MMU_IDENT_TOP_SHIFT)
#define MMU_PAGE_TABLE_ENTRIES_IDENT (0x1 << MMU_PAGE_TABLE_ENTRIES_IDENT_SHIFT)

#define MMU_PTE_DESCRIPTOR_BLOCK_MAX_SHIFT      (30)

#ifndef ASSEMBLY
#define BM(base, count, val) (((val) & ((1UL << (count)) - 1)) << (base))
#else
#define BM(base, count, val) (((val) & ((0x1 << (count)) - 1)) << (base))
#endif

#define MMU_SH_NON_SHAREABLE                    (0)
#define MMU_SH_OUTER_SHAREABLE                  (2)
#define MMU_SH_INNER_SHAREABLE                  (3)

#define MMU_RGN_NON_CACHEABLE                   (0)
#define MMU_RGN_WRITE_BACK_ALLOCATE             (1)
#define MMU_RGN_WRITE_THROUGH_NO_ALLOCATE       (2)
#define MMU_RGN_WRITE_BACK_NO_ALLOCATE          (3)

#define MMU_TCR_TBI1                            BM(38, 1, 1)
#define MMU_TCR_TBI0                            BM(37, 1, 1)
#define MMU_TCR_AS                              BM(36, 1, 1)
#define MMU_TCR_IPS(size)                       BM(32, 3, (size))
#define MMU_TCR_TG1(granule_size)               BM(30, 2, (granule_size))
#define MMU_TCR_SH1(shareability_flags)         BM(28, 2, (shareability_flags))
#define MMU_TCR_ORGN1(cache_flags)              BM(26, 2, (cache_flags))
#define MMU_TCR_IRGN1(cache_flags)              BM(24, 2, (cache_flags))
#define MMU_TCR_EPD1                            BM(23, 1, 1)
#define MMU_TCR_A1                              BM(22, 1, 1)
#define MMU_TCR_T1SZ(size)                      BM(16, 6, (size))
#define MMU_TCR_TG0(granule_size)               BM(14, 2, (granule_size))
#define MMU_TCR_SH0(shareability_flags)         BM(12, 2, (shareability_flags))
#define MMU_TCR_ORGN0(cache_flags)              BM(10, 2, (cache_flags))
#define MMU_TCR_IRGN0(cache_flags)              BM( 8, 2, (cache_flags))
#define MMU_TCR_EPD0                            BM( 7, 1, 1)
#define MMU_TCR_T0SZ(size)                      BM( 0, 6, (size))

#define MMU_MAIR_ATTR(index, attr)              BM(index * 8, 8, (attr))


/* L0/L1/L2/L3 descriptor types */
#define MMU_PTE_DESCRIPTOR_INVALID              BM(0, 2, 0)
#define MMU_PTE_DESCRIPTOR_MASK                 BM(0, 2, 3)

/* L0/L1/L2 descriptor types */
#define MMU_PTE_L012_DESCRIPTOR_BLOCK           BM(0, 2, 1)
#define MMU_PTE_L012_DESCRIPTOR_TABLE           BM(0, 2, 3)

/* L3 descriptor types */
#define MMU_PTE_L3_DESCRIPTOR_PAGE              BM(0, 2, 3)

/* Output address mask */
#define MMU_PTE_OUTPUT_ADDR_MASK                BM(12, 36, 0xfffffffff)

/* Table attrs */
#define MMU_PTE_ATTR_NS_TABLE                   BM(63, 1, 1)
#define MMU_PTE_ATTR_AP_TABLE_NO_WRITE          BM(62, 1, 1)
#define MMU_PTE_ATTR_AP_TABLE_NO_EL0            BM(61, 1, 1)
#define MMU_PTE_ATTR_UXN_TABLE                  BM(60, 1, 1)
#define MMU_PTE_ATTR_PXN_TABLE                  BM(59, 1, 1)

/* Block/Page attrs */
#define MMU_PTE_ATTR_RES_SOFTWARE               BM(55, 4, 0xf)
#define MMU_PTE_ATTR_UXN                        BM(54, 1, 1)
#define MMU_PTE_ATTR_PXN                        BM(53, 1, 1)
#define MMU_PTE_ATTR_CONTIGUOUS                 BM(52, 1, 1)

#define MMU_PTE_ATTR_NON_GLOBAL                 BM(11, 1, 1)
#define MMU_PTE_ATTR_AF                         BM(10, 1, 1)

#define MMU_PTE_ATTR_SH_NON_SHAREABLE           BM(8, 2, 0)
#define MMU_PTE_ATTR_SH_OUTER_SHAREABLE         BM(8, 2, 2)
#define MMU_PTE_ATTR_SH_INNER_SHAREABLE         BM(8, 2, 3)

#define MMU_PTE_ATTR_AP_P_RW_U_NA               BM(6, 2, 0)
#define MMU_PTE_ATTR_AP_P_RW_U_RW               BM(6, 2, 1)
#define MMU_PTE_ATTR_AP_P_RO_U_NA               BM(6, 2, 2)
#define MMU_PTE_ATTR_AP_P_RO_U_RO               BM(6, 2, 3)
#define MMU_PTE_ATTR_AP_MASK                    BM(6, 2, 3)

#define MMU_PTE_ATTR_NON_SECURE                 BM(5, 1, 1)

#define MMU_PTE_ATTR_ATTR_INDEX(attrindex)      BM(2, 3, attrindex)
#define MMU_PTE_ATTR_ATTR_INDEX_MASK            MMU_PTE_ATTR_ATTR_INDEX(7)

/* Default configuration for main kernel page table:
 *    - do cached translation walks
 */

/* Device-nGnRnE memory */
#define MMU_MAIR_ATTR0                  MMU_MAIR_ATTR(0, 0x00)
#define MMU_PTE_ATTR_STRONGLY_ORDERED   MMU_PTE_ATTR_ATTR_INDEX(0)

/* Device-nGnRE memory */
#define MMU_MAIR_ATTR1                  MMU_MAIR_ATTR(1, 0x04)
#define MMU_PTE_ATTR_DEVICE             MMU_PTE_ATTR_ATTR_INDEX(1)

/* Normal Memory, Outer Write-back non-transient Read/Write allocate,
 * Inner Write-back non-transient Read/Write allocate
 */
#define MMU_MAIR_ATTR2                  MMU_MAIR_ATTR(2, 0xff)
#define MMU_PTE_ATTR_NORMAL_MEMORY      MMU_PTE_ATTR_ATTR_INDEX(2)

#define MMU_MAIR_ATTR3                  (0)
#define MMU_MAIR_ATTR4                  (0)
#define MMU_MAIR_ATTR5                  (0)
#define MMU_MAIR_ATTR6                  (0)
#define MMU_MAIR_ATTR7                  (0)

#define MMU_MAIR_VAL                    (MMU_MAIR_ATTR0 | MMU_MAIR_ATTR1 | \
                                         MMU_MAIR_ATTR2 | MMU_MAIR_ATTR3 | \
                                         MMU_MAIR_ATTR4 | MMU_MAIR_ATTR5 | \
                                         MMU_MAIR_ATTR6 | MMU_MAIR_ATTR7 )

#define MMU_TCR_IPS_DEFAULT MMU_TCR_IPS(2) /* TODO: read at runtime, or configure per platform */

/* Enable cached page table walks:
 * inner/outer (IRGN/ORGN): write-back + write-allocate
 */
#define MMU_TCR_FLAGS1 (MMU_TCR_TG1(MMU_TG1(MMU_KERNEL_PAGE_SIZE_SHIFT)) | \
                        MMU_TCR_SH1(MMU_SH_INNER_SHAREABLE) | \
                        MMU_TCR_ORGN1(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_IRGN1(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_T1SZ(64 - MMU_KERNEL_SIZE_SHIFT))
#define MMU_TCR_FLAGS0 (MMU_TCR_TG0(MMU_TG0(MMU_USER_PAGE_SIZE_SHIFT)) | \
                        MMU_TCR_SH0(MMU_SH_INNER_SHAREABLE) | \
                        MMU_TCR_ORGN0(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_IRGN0(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_T0SZ(64 - MMU_USER_SIZE_SHIFT))
#define MMU_TCR_FLAGS0_IDENT \
                       (MMU_TCR_TG0(MMU_TG0(MMU_IDENT_PAGE_SIZE_SHIFT)) | \
                        MMU_TCR_SH0(MMU_SH_INNER_SHAREABLE) | \
                        MMU_TCR_ORGN0(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_IRGN0(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_T0SZ(64 - MMU_IDENT_SIZE_SHIFT))
#define MMU_TCR_FLAGS_IDENT (MMU_TCR_IPS_DEFAULT | MMU_TCR_FLAGS1 | MMU_TCR_FLAGS0_IDENT)
#define MMU_TCR_FLAGS_KERNEL (MMU_TCR_IPS_DEFAULT | MMU_TCR_FLAGS1 | MMU_TCR_FLAGS0 | MMU_TCR_EPD0)
#define MMU_TCR_FLAGS_USER (MMU_TCR_IPS_DEFAULT | MMU_TCR_FLAGS1 | MMU_TCR_FLAGS0)


#if MMU_IDENT_SIZE_SHIFT > MMU_LX_X(MMU_IDENT_PAGE_SIZE_SHIFT, 2)
#define MMU_PTE_IDENT_DESCRIPTOR MMU_PTE_L012_DESCRIPTOR_BLOCK
#else
#define MMU_PTE_IDENT_DESCRIPTOR MMU_PTE_L3_DESCRIPTOR_PAGE
#endif
#define MMU_PTE_IDENT_FLAGS \
    (MMU_PTE_IDENT_DESCRIPTOR | \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_INNER_SHAREABLE | \
     MMU_PTE_ATTR_NORMAL_MEMORY | \
     MMU_PTE_ATTR_AP_P_RW_U_NA)

#define MMU_PTE_KERNEL_RO_FLAGS \
    (MMU_PTE_ATTR_UXN | \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_INNER_SHAREABLE | \
     MMU_PTE_ATTR_NORMAL_MEMORY | \
     MMU_PTE_ATTR_AP_P_RO_U_NA)

#define MMU_PTE_KERNEL_DATA_FLAGS \
    (MMU_PTE_ATTR_UXN | \
     MMU_PTE_ATTR_PXN | \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_INNER_SHAREABLE | \
     MMU_PTE_ATTR_NORMAL_MEMORY | \
     MMU_PTE_ATTR_AP_P_RW_U_NA)

#define MMU_INITIAL_MAP_STRONGLY_ORDERED \
    (MMU_PTE_ATTR_UXN | \
     MMU_PTE_ATTR_PXN | \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_STRONGLY_ORDERED | \
     MMU_PTE_ATTR_AP_P_RW_U_NA)

#define MMU_INITIAL_MAP_DEVICE \
    (MMU_PTE_ATTR_UXN | \
     MMU_PTE_ATTR_PXN | \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_DEVICE | \
     MMU_PTE_ATTR_AP_P_RW_U_NA)

#ifndef ASSEMBLY

#include <sys/types.h>
#include <assert.h>
#include <compiler.h>
#include <arch/arm64.h>

typedef uint64_t pte_t;

__BEGIN_CDECLS

#define ARM64_TLBI_NOADDR(op) \
({ \
    __asm__ volatile("tlbi " #op::); \
    ISB; \
})

#define ARM64_TLBI(op, val) \
({ \
    __asm__ volatile("tlbi " #op ", %0" :: "r" (val)); \
    ISB; \
})

#define MMU_ARM64_GLOBAL_ASID (~0U)
#define MMU_ARM64_USER_ASID (0U)
int arm64_mmu_map(vaddr_t vaddr, paddr_t paddr, size_t size, pte_t attrs,
                  vaddr_t vaddr_base, uint top_size_shift,
                  uint top_index_shift, uint page_size_shift,
                  pte_t *top_page_table, uint asid);
int arm64_mmu_unmap(vaddr_t vaddr, size_t size,
                    vaddr_t vaddr_base, uint top_size_shift,
                    uint top_index_shift, uint page_size_shift,
                    pte_t *top_page_table, uint asid);

__END_CDECLS
#endif /* ASSEMBLY */

#endif
