/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 * Copyright (c) 2012, NVIDIA CORPORATION. All rights reserved
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
#ifndef __ARCH_ARM_MMU_H
#define __ARCH_ARM_MMU_H

#define KB                (1024UL)
#define MB                (1024UL*1024UL)
#define GB                (1024UL*1024UL*1024UL)

#define SECTION_SIZE      MB
#define SUPERSECTION_SIZE (16 * MB)
#define TT_ENTRY_COUNT    (4096)

#if defined(ARM_ISA_ARMV6) | defined(ARM_ISA_ARMV7)

#define MMU_MEMORY_L1_DESCRIPTOR_INVALID                 (0x0 << 0)
#define MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE              (0x1 << 0)
#define MMU_MEMORY_L1_DESCRIPTOR_SECTION                 (0x2 << 0)
#define MMU_MEMORY_L1_DESCRIPTOR_SUPERSECTION            ((0x2 << 0) | (0x1 << 18))
#define MMU_MEMORY_L1_DESCRIPTOR_MASK                    (0x3 << 0)

#define MMU_MEMORY_L2_DESCRIPTOR_INVALID                 (0x0 << 0)
#define MMU_MEMORY_L2_DESCRIPTOR_LARGE_PAGE              (0x1 << 0)
#define MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE              (0x2 << 0)
#define MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE_XN           (0x3 << 0)
#define MMU_MEMORY_L2_DESCRIPTOR_MASK                    (0x3 << 0)

/* C, B and TEX[2:0] encodings without TEX remap (for first level descriptors) */
/* TEX      |    CB    */
#define MMU_MEMORY_L1_TYPE_STRONGLY_ORDERED              ((0x0 << 12) | (0x0 << 2))
#define MMU_MEMORY_L1_TYPE_DEVICE_SHARED                 ((0x0 << 12) | (0x1 << 2))
#define MMU_MEMORY_L1_TYPE_DEVICE_NON_SHARED             ((0x2 << 12) | (0x0 << 2))
#define MMU_MEMORY_L1_TYPE_NORMAL                        ((0x1 << 12) | (0x0 << 2))
#define MMU_MEMORY_L1_TYPE_NORMAL_WRITE_THROUGH          ((0x0 << 12) | (0x2 << 2))
#define MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_NO_ALLOCATE ((0x0 << 12) | (0x3 << 2))
#define MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_ALLOCATE    ((0x1 << 12) | (0x3 << 2))
#define MMU_MEMORY_L1_TYPE_MASK                          ((0x7 << 12) | (0x3 << 2))

#define MMU_MEMORY_L1_TYPE_INNER_WRITE_BACK_ALLOCATE     ((0x4 << 12) | (0x1 << 2))

/* C, B and TEX[2:0] encodings without TEX remap (for second level descriptors) */
/* TEX     |    CB    */
#define MMU_MEMORY_L2_TYPE_STRONGLY_ORDERED              ((0x0 << 6) | (0x0 << 2))
#define MMU_MEMORY_L2_TYPE_DEVICE_SHARED                 ((0x0 << 6) | (0x1 << 2))
#define MMU_MEMORY_L2_TYPE_DEVICE_NON_SHARED             ((0x2 << 6) | (0x0 << 2))
#define MMU_MEMORY_L2_TYPE_NORMAL                        ((0x1 << 6) | (0x0 << 2))
#define MMU_MEMORY_L2_TYPE_NORMAL_WRITE_THROUGH          ((0x0 << 6) | (0x2 << 2))
#define MMU_MEMORY_L2_TYPE_NORMAL_WRITE_BACK_NO_ALLOCATE ((0x0 << 6) | (0x3 << 2))
#define MMU_MEMORY_L2_TYPE_NORMAL_WRITE_BACK_ALLOCATE    ((0x1 << 6) | (0x3 << 2))
#define MMU_MEMORY_L2_TYPE_MASK                          ((0x7 << 6) | (0x3 << 2))

#define MMU_MEMORY_DOMAIN_MEM                            (0)

/*
 * AP (Access Permissions)
 * +-------------------------+
 * | AP        P         U   |
 * +-------------------------+
 * |                         |
 * | 000      NA        NA   |
 * |                         |
 * | 001      RW        NA   |
 * |                         |
 * | 010      RW        R    |
 * |                         |
 * | 011      RW        RW   |
 * |                         |
 * | 101      R         NA   |
 * |                         |
 * | 111      R         R    |
 * |                         |
 * +-------------------------+
 *
 * NA = No Access
 * RW = Read/Write
 * R  = Read only
 *
 * P = Privileged modes
 * U = ~P
 *
 */
#define MMU_MEMORY_L1_AP_P_NA_U_NA          ((0x0 << 15) | (0x0 << 10))
#define MMU_MEMORY_L1_AP_P_RW_U_RO          ((0x0 << 15) | (0x2 << 10)) /* Obsolete */
#define MMU_MEMORY_L1_AP_P_RW_U_RW          ((0x0 << 15) | (0x3 << 10))
#define MMU_MEMORY_L1_AP_P_RW_U_NA          ((0x0 << 15) | (0x1 << 10))
#define MMU_MEMORY_L1_AP_P_RO_U_RO          ((0x1 << 15) | (0x3 << 10))
#define MMU_MEMORY_L1_AP_P_RO_U_NA          ((0x1 << 15) | (0x1 << 10))
#define MMU_MEMORY_L1_AP_MASK               ((0x1 << 15) | (0x3 << 10))

#define MMU_MEMORY_L2_AP_P_NA_U_NA          ((0x0 << 9) | (0x0 << 4))
#define MMU_MEMORY_L2_AP_P_RW_U_RO          ((0x0 << 9) | (0x2 << 4)) /* Obsolete */
#define MMU_MEMORY_L2_AP_P_RW_U_RW          ((0x0 << 9) | (0x3 << 4))
#define MMU_MEMORY_L2_AP_P_RW_U_NA          ((0x0 << 9) | (0x1 << 4))
#define MMU_MEMORY_L2_AP_P_RO_U_RO          ((0x1 << 9) | (0x3 << 4))
#define MMU_MEMORY_L2_AP_P_RO_U_NA          ((0x1 << 9) | (0x1 << 4))
#define MMU_MEMORY_L2_AP_MASK               ((0x1 << 9) | (0x3 << 4))

#define MMU_MEMORY_L1_PAGETABLE_NON_SECURE  (1 << 3)

#define MMU_MEMORY_L1_SECTION_NON_SECURE    (1 << 19)
#define MMU_MEMORY_L1_SECTION_SHAREABLE     (1 << 16)
#define MMU_MEMORY_L1_SECTION_NON_GLOBAL    (1 << 17)
#define MMU_MEMORY_L1_SECTION_XN            (1 << 4)

#define MMU_MEMORY_L1_CB_SHIFT              2
#define MMU_MEMORY_L1_TEX_SHIFT            12

#define MMU_MEMORY_SET_L1_INNER(val)        (((val) & 0x3) << MMU_MEMORY_L1_CB_SHIFT)
#define MMU_MEMORY_SET_L1_OUTER(val)        (((val) & 0x3) << MMU_MEMORY_L1_TEX_SHIFT)
#define MMU_MEMORY_SET_L1_CACHEABLE_MEM     (0x4 << MMU_MEMORY_L1_TEX_SHIFT)

#define MMU_MEMORY_L2_SHAREABLE             (1 << 10)
#define MMU_MEMORY_L2_NON_GLOBAL            (1 << 11)

#define MMU_MEMORY_L2_CB_SHIFT              2
#define MMU_MEMORY_L2_TEX_SHIFT             6

#define MMU_MEMORY_NON_CACHEABLE            0
#define MMU_MEMORY_WRITE_BACK_ALLOCATE      1
#define MMU_MEMORY_WRITE_THROUGH_NO_ALLOCATE 2
#define MMU_MEMORY_WRITE_BACK_NO_ALLOCATE   3

#define MMU_MEMORY_SET_L2_INNER(val)        (((val) & 0x3) << MMU_MEMORY_L2_CB_SHIFT)
#define MMU_MEMORY_SET_L2_OUTER(val)        (((val) & 0x3) << MMU_MEMORY_L2_TEX_SHIFT)
#define MMU_MEMORY_SET_L2_CACHEABLE_MEM     (0x4 << MMU_MEMORY_L2_TEX_SHIFT)

#define MMU_MEMORY_L1_SECTION_ADDR(x)       ((x) & ~((1<<20)-1))
#define MMU_MEMORY_L1_PAGE_TABLE_ADDR(x)    ((x) & ~((1<<10)-1))

#define MMU_MEMORY_L2_SMALL_PAGE_ADDR(x)    ((x) & ~((1<<12)-1))
#define MMU_MEMORY_L2_LARGE_PAGE_ADDR(x)    ((x) & ~((1<<16)-1))

#define MMU_MEMORY_TTBR_RGN(x)              (((x) & 0x3) << 3)
/* IRGN[1:0] is encoded as: IRGN[0] in TTBRx[6], and IRGN[1] in TTBRx[0] */
#define MMU_MEMORY_TTBR_IRGN(x)             ((((x) & 0x1) << 6) | \
                                            ((((x) >> 1) & 0x1) << 0))
#define MMU_MEMORY_TTBR_S                   (1 << 1)
#define MMU_MEMORY_TTBR_NOS                 (1 << 5)

/* Default configuration for main kernel page table:
 *    - section mappings for memory
 *    - do cached translation walks
 */

/* Enable cached page table walks:
 * inner/outer (IRGN/RGN): write-back + write-allocate
 * (select inner sharable on smp)
 */
#if WITH_SMP
#define MMU_TTBRx_SHARABLE_FLAGS (MMU_MEMORY_TTBR_S | MMU_MEMORY_TTBR_NOS)
#else
#define MMU_TTBRx_SHARABLE_FLAGS (0)
#endif
#define MMU_TTBRx_FLAGS \
    (MMU_MEMORY_TTBR_RGN(MMU_MEMORY_WRITE_BACK_ALLOCATE) |\
     MMU_MEMORY_TTBR_IRGN(MMU_MEMORY_WRITE_BACK_ALLOCATE) | \
     MMU_TTBRx_SHARABLE_FLAGS)

/* Section mapping, TEX[2:0]=001, CB=11, S=1, AP[2:0]=001 */
#if WITH_SMP
#define MMU_KERNEL_L1_PTE_FLAGS \
    (MMU_MEMORY_L1_DESCRIPTOR_SECTION | \
     MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_ALLOCATE | \
     MMU_MEMORY_L1_AP_P_RW_U_NA | \
     MMU_MEMORY_L1_SECTION_SHAREABLE)
#else
#define MMU_KERNEL_L1_PTE_FLAGS \
    (MMU_MEMORY_L1_DESCRIPTOR_SECTION | \
     MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_ALLOCATE | \
     MMU_MEMORY_L1_AP_P_RW_U_NA)
#endif

#define MMU_INITIAL_MAP_STRONGLY_ORDERED \
    (MMU_MEMORY_L1_DESCRIPTOR_SECTION | \
    MMU_MEMORY_L1_TYPE_STRONGLY_ORDERED | \
    MMU_MEMORY_L1_AP_P_RW_U_NA)

#define MMU_INITIAL_MAP_DEVICE \
    (MMU_MEMORY_L1_DESCRIPTOR_SECTION | \
    MMU_MEMORY_L1_TYPE_DEVICE_SHARED | \
    MMU_MEMORY_L1_AP_P_RW_U_NA)

#endif // armv6 | armv7

#ifndef ASSEMBLY

#include <sys/types.h>
#include <assert.h>
#include <compiler.h>
#include <arch/arm.h>

__BEGIN_CDECLS

void arm_mmu_early_init(void);
void arm_mmu_init(void);
status_t arm_vtop(addr_t va, addr_t *pa);

/* tlb routines */

static inline void arm_after_invalidate_tlb_barrier(void)
{
#if WITH_SMP
    arm_write_bpiallis(0);
#else
    arm_write_bpiall(0);
#endif
    DSB;
    ISB;
}

static inline void arm_invalidate_tlb_global_no_barrier(void)
{
#if WITH_SMP
    arm_write_tlbiallis(0);
#else
    arm_write_tlbiall(0);
#endif
}

static inline void arm_invalidate_tlb_global(void)
{
    DSB;
    arm_invalidate_tlb_global_no_barrier();
    arm_after_invalidate_tlb_barrier();
}

static inline void arm_invalidate_tlb_mva_no_barrier(vaddr_t va)
{
#if WITH_SMP
    arm_write_tlbimvaais(va & 0xfffff000);
#else
    arm_write_tlbimvaa(va & 0xfffff000);
#endif
}

static inline void arm_invalidate_tlb_mva(vaddr_t va)
{
    DSB;
    arm_invalidate_tlb_mva_no_barrier(va);
    arm_after_invalidate_tlb_barrier();
}


static inline void arm_invalidate_tlb_asid_no_barrier(uint8_t asid)
{
#if WITH_SMP
    arm_write_tlbiasidis(asid);
#else
    arm_write_tlbiasid(asid);
#endif
}

static inline void arm_invalidate_tlb_asid(uint8_t asid)
{
    DSB;
    arm_invalidate_tlb_asid_no_barrier(asid);
    arm_after_invalidate_tlb_barrier();
}

static inline void arm_invalidate_tlb_mva_asid_no_barrier(vaddr_t va, uint8_t asid)
{
#if WITH_SMP
    arm_write_tlbimvais((va & 0xfffff000) | asid);
#else
    arm_write_tlbimva((va & 0xfffff000) | asid);
#endif
}

static inline void arm_invalidate_tlb_mva_asid(vaddr_t va, uint8_t asid)
{
    DSB;
    arm_invalidate_tlb_mva_asid_no_barrier(va, asid);
    arm_after_invalidate_tlb_barrier();
}

__END_CDECLS

#endif /* ASSEMBLY */

#endif
