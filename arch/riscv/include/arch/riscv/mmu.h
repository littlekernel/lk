/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#if RISCV_MMU

#ifndef ASSEMBLY
#include <stdint.h>
#include <arch/defines.h>

// RISC-V specific mmu #defines and structures here
typedef uintptr_t riscv_pte_t;
#endif

#define KB                (1024UL)
#define MB                (1024UL*1024UL)
#define GB                (1024UL*1024UL*1024UL)

// some constants based on our particular implementation
#if RISCV_MMU == 48
#define RISCV_MMU_PT_LEVELS 4
#define RISCV_MMU_PT_SHIFT  9
#define RISCV_MMU_PT_ENTRIES 512 // 1 << PT_SHIFT
#define RISCV_MMU_CANONICAL_MASK ((1UL << 48) - 1)
#define RISCV_MMU_PPN_BITS 56
#define RISCV_MMU_PHYSMAP_BASE_VIRT (KERNEL_ASPACE_BASE)
#define RISCV_MMU_PHYSMAP_PAGE_SIZE (1UL << 39)
#define RISCV_MMU_PHYSMAP_PAGE_COUNT 1
#elif RISCV_MMU == 39
#define RISCV_MMU_PT_LEVELS 3
#define RISCV_MMU_PT_SHIFT  9
#define RISCV_MMU_PT_ENTRIES 512 // 1 << PT_SHIFT
#define RISCV_MMU_CANONICAL_MASK ((1UL << 39) - 1)
#define RISCV_MMU_PPN_BITS 56
#define RISCV_MMU_PHYSMAP_BASE_VIRT (KERNEL_ASPACE_BASE)
#define RISCV_MMU_PHYSMAP_PAGE_SIZE (1UL << 30)
#define RISCV_MMU_PHYSMAP_PAGE_COUNT 64
#elif RISCV_MMU == 32
#define RISCV_MMU_PT_LEVELS 2
#define RISCV_MMU_PT_SHIFT  10
#define RISCV_MMU_PT_ENTRIES 1024 // 1 << PT_SHIFT
#define RISCV_MMU_CANONICAL_MASK UINT32_MASK
#define RISCV_MMU_PPN_BITS 32
#define RISCV_MMU_PHYSMAP_BASE_VIRT (KERNEL_ASPACE_BASE)
#define RISCV_MMU_PHYSMAP_PAGE_SIZE (1UL << 30)
#define RISCV_MMU_PHYSMAP_PAGE_COUNT 1
#else
#error implement
#endif

#define RISCV_MMU_PHYSMAP_SIZE      (RISCV_MMU_PHYSMAP_PAGE_SIZE * RISCV_MMU_PHYSMAP_PAGE_COUNT)

// number of page table entries for the kernel and user half
// TODO: compute directly from KERNEL/USER_ASPACE_SIZE
#define RISCV_MMU_USER_PT_ENTRIES 256
#define RISCV_MMU_KERNEL_PT_ENTRIES 256

// page table bits
#define RISCV_PTE_V         (1 << 0) // valid
#define RISCV_PTE_R         (1 << 1) // read
#define RISCV_PTE_W         (1 << 2) // write
#define RISCV_PTE_X         (1 << 3) // execute
#define RISCV_PTE_PERM_MASK (0x7 << 1)
#define RISCV_PTE_U         (1 << 4) // user
#define RISCV_PTE_G         (1 << 5) // global
#define RISCV_PTE_A         (1 << 6) // accessed
#define RISCV_PTE_D         (1 << 7) // dirty
#define RISCV_PTE_RSW_MASK  (3 << 8) // reserved for software
#define RISCV_PTE_PPN_SHIFT (10)
#define RISCV_PTE_PPN_MASK  (((1UL << (RISCV_MMU_PPN_BITS - PAGE_SIZE_SHIFT)) - 1) << RISCV_PTE_PPN_SHIFT)

// riscv PPN is stored shifed over 2 from the natural alignment
#define RISCV_PTE_PPN(pte) (((pte) & RISCV_PTE_PPN_MASK) << (PAGE_SIZE_SHIFT - RISCV_PTE_PPN_SHIFT))
#define RISCV_PTE_PPN_TO_PTE(paddr) (((paddr) >> PAGE_SIZE_SHIFT) << RISCV_PTE_PPN_SHIFT)

// SATP register, contains the current mmu mode, address space id, and
// pointer to root page table
#define RISCV_SATP_MODE_NONE (0UL)
#define RISCV_SATP_MODE_SV32 (1UL)
#define RISCV_SATP_MODE_SV39 (8UL)
#define RISCV_SATP_MODE_SV48 (9UL)
#define RISCV_SATP_MODE_SV57 (10UL)
#define RISCV_SATP_MODE_SV64 (11UL)

#if __riscv_xlen == 32
#define RISCV_SATP_MODE_SHIFT (31)
#define RISCV_SATP_ASID_SHIFT (22)
#define RISCV_SATP_ASID_SIZE  (9)
#define RISCV_SATP_ASID_MASK  ((1 << RISCV_SATP_ASID_SIZE) - 1)

#elif __riscv_xlen == 64
#define RISCV_SATP_MODE_SHIFT (60)
#define RISCV_SATP_ASID_SHIFT (44)
#define RISCV_SATP_ASID_SIZE  (16)
#define RISCV_SATP_ASID_MASK  ((1UL << RISCV_SATP_ASID_SIZE) - 1)
#endif

#endif // RISCV_MMU

