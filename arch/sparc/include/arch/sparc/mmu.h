//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#pragma once

// SPARC v8 Reference MMU (SRMMU), as described in Appendix H, "SPARC
// Reference MMU Architecture," of The SPARC Architecture Manual, Version 8.
// This is the MMU implemented by sun4m-class machines.
//
// The SRMMU translates a 32-bit virtual address to a 36-bit physical
// address. Translation information is held in tables in main memory,
// indexed by a per-address-space Context number and up to three levels of
// page tables:
//
//   Context Table[ctx] -> Level 1 table[idx1] -> Level 2 table[idx2] -> Level 3 table[idx3]
//
// Every entry in the Context Table and in a Level 1 or Level 2 table is
// either a Page Table Descriptor (PTD), which points at the next level of
// table, or a Page Table Entry (PTE), which terminates the walk early and
// maps a large, linearly aligned region of memory (4G, 16M, or 256K bytes,
// respectively). Every entry in a Level 3 table is a PTE mapping a single
// 4K page.
//
// All entries (PTDs and PTEs, at every level) are a single 32-bit word, and
// share a 2-bit Entry Type (ET) field in bits [1:0] that identifies what
// kind of entry it is.

#define KB (1024UL)
#define MB (1024UL * 1024UL)
#define GB (1024UL * 1024UL * 1024UL)

// A virtual address is divided into four fields:
//
//  31        24 23      18 17      12 11               0
// |   Index 1  |  Index 2 |  Index 3 |      Offset      |
//
// Index 1 selects an entry in the Level 1 table (pointed to by the
// context's Root Pointer PTD in the Context Table), Index 2 an entry in the
// Level 2 table, and Index 3 an entry in the Level 3 table.
#define SRMMU_VA_INDEX1_SHIFT 24
#define SRMMU_VA_INDEX1_BITS  8
#define SRMMU_VA_INDEX2_SHIFT 18
#define SRMMU_VA_INDEX2_BITS  6
#define SRMMU_VA_INDEX3_SHIFT 12
#define SRMMU_VA_INDEX3_BITS  6

#define SRMMU_VA_INDEX1(va) (((va) >> SRMMU_VA_INDEX1_SHIFT) & ((1u << SRMMU_VA_INDEX1_BITS) - 1))
#define SRMMU_VA_INDEX2(va) (((va) >> SRMMU_VA_INDEX2_SHIFT) & ((1u << SRMMU_VA_INDEX2_BITS) - 1))
#define SRMMU_VA_INDEX3(va) (((va) >> SRMMU_VA_INDEX3_SHIFT) & ((1u << SRMMU_VA_INDEX3_BITS) - 1))

// number of entries in, and byte size of, each level of page table.
// entries are a single 32bit word.
#define SRMMU_PTE_SIZE 4

#define SRMMU_L1_ENTRIES (1u << SRMMU_VA_INDEX1_BITS) // 256
#define SRMMU_L2_ENTRIES (1u << SRMMU_VA_INDEX2_BITS) // 64
#define SRMMU_L3_ENTRIES (1u << SRMMU_VA_INDEX3_BITS) // 64

#define SRMMU_L1_TABLE_SIZE (SRMMU_L1_ENTRIES * SRMMU_PTE_SIZE) // 1024 bytes
#define SRMMU_L2_TABLE_SIZE (SRMMU_L2_ENTRIES * SRMMU_PTE_SIZE) // 256 bytes
#define SRMMU_L3_TABLE_SIZE (SRMMU_L3_ENTRIES * SRMMU_PTE_SIZE) // 256 bytes

// a page table (at any level) must be physically aligned to its own size
#define SRMMU_L1_TABLE_ALIGN SRMMU_L1_TABLE_SIZE
#define SRMMU_L2_TABLE_ALIGN SRMMU_L2_TABLE_SIZE
#define SRMMU_L3_TABLE_ALIGN SRMMU_L3_TABLE_SIZE

// size of the region of virtual address space mapped by a single PTE found
// at each level of the table walk (Table H-1)
#define SRMMU_ROOT_MAP_SIZE (4 * GB)
#define SRMMU_L1_MAP_SIZE   (16 * MB)
#define SRMMU_L2_MAP_SIZE   (256 * KB)
#define SRMMU_L3_MAP_SIZE   (4 * KB)

// ---- Entry Type (ET) field, common to every Context Table / L1 / L2 / L3 entry ----
//
//  31                              2  1     0
// |         (varies by ET)          |  ET   |
//
#define SRMMU_ET_SHIFT 0
#define SRMMU_ET_BITS  2
#define SRMMU_ET_MASK  (0x3u << SRMMU_ET_SHIFT)

#define SRMMU_ET_INVALID  0x0u // va range is unmapped
#define SRMMU_ET_PTD      0x1u // entry points at the next level of page table
#define SRMMU_ET_PTE      0x2u // entry is a valid translation
#define SRMMU_ET_RESERVED 0x3u

// Every PTD and PTE stores a physical address shifted right by 4 bits in
// its upper bits, with the low bits of the shifted value used for entry
// type and (for a PTE) permission/status flags. Because the target of a
// PTD (a page table) and the target of a PTE (a mapped page) are always
// aligned to at least 64 bytes, the low bits used for flags are guaranteed
// to be zero in the shifted address, and can simply be OR'd in.
#define SRMMU_PADDR_SHIFT 4

// Physical addresses are 36 bits wide, wider than a 32-bit pointer. These
// macros are usable from assembly (no casts), so callers are responsible
// for passing/receiving values of an appropriately wide type.
#define SRMMU_PADDR_TO_FIELD(paddr) ((paddr) >> SRMMU_PADDR_SHIFT)
#define SRMMU_FIELD_TO_PADDR(field) (((field) & ~SRMMU_ET_MASK) << SRMMU_PADDR_SHIFT)

// ---- Page Table Descriptor (PTD): Context Table, L1, and L2 entries that
//      point at a further level of page table ----
//
//  31                              2  1   0
// |             PTP                 | ET  |
//
// PTP holds physical address bits [35:6] of the next level table.
#define SRMMU_PTD_PTP_SHIFT 2
#define SRMMU_PTD_PTP_MASK  (0xfffffffcu)

#define SRMMU_MAKE_PTD(paddr) (SRMMU_PADDR_TO_FIELD(paddr) | SRMMU_ET_PTD)
#define SRMMU_PTD_PADDR(ptd)  SRMMU_FIELD_TO_PADDR(ptd)

// ---- Page Table Entry (PTE): a translation, found at any level ----
//
//  31              8  7   6   5   4     2  1   0
// |      PPN        | C | M | R |  ACC   | ET  |
//
// PPN holds physical address bits [35:12] of the mapped page (physical
// address bits [11:0] within the mapped region come from the virtual
// address / page table index instead).
#define SRMMU_PTE_PPN_SHIFT 8
#define SRMMU_PTE_PPN_MASK  (0xffffff00u)

#define SRMMU_PTE_C (1u << 7) // cacheable
#define SRMMU_PTE_M (1u << 6) // modified (dirty); set by hardware on a write
#define SRMMU_PTE_R (1u << 5) // referenced (accessed); set by hardware on any access

#define SRMMU_PTE_ACC_SHIFT 2
#define SRMMU_PTE_ACC_BITS  3
#define SRMMU_PTE_ACC_MASK  (0x7u << SRMMU_PTE_ACC_SHIFT)

// ACC field encoding: permissions granted to user and supervisor accesses
#define SRMMU_ACC_URO_SRO   0 // user read-only,      supervisor read-only
#define SRMMU_ACC_URW_SRW   1 // user read/write,     supervisor read/write
#define SRMMU_ACC_URX_SRX   2 // user read/execute,   supervisor read/execute
#define SRMMU_ACC_URWX_SRWX 3 // user read/write/execute, supervisor read/write/execute
#define SRMMU_ACC_UX_SX     4 // user execute-only,   supervisor execute-only
#define SRMMU_ACC_URO_SRW   5 // user read-only,      supervisor read/write
#define SRMMU_ACC_NONE_SRX  6 // user no access,      supervisor read/execute
#define SRMMU_ACC_NONE_SRWX 7 // user no access,      supervisor read/write/execute

#define SRMMU_MAKE_PTE(paddr, acc, flags)                                                          \
    (SRMMU_PADDR_TO_FIELD(paddr) | (((acc) << SRMMU_PTE_ACC_SHIFT) & SRMMU_PTE_ACC_MASK) |         \
     (flags) | SRMMU_ET_PTE)
#define SRMMU_PTE_PADDR(pte) SRMMU_FIELD_TO_PADDR(pte)

// ---- MMU Control Register ----
//
//  31    28 27    24 23        8 7     6         2 1    0
// | IMPL  |  VER   |    SC      | PSO |  reserved | NF | E |
//
#define SRMMU_CTRL_IMPL_SHIFT 28
#define SRMMU_CTRL_IMPL_MASK  (0xfu << SRMMU_CTRL_IMPL_SHIFT)
#define SRMMU_CTRL_VER_SHIFT  24
#define SRMMU_CTRL_VER_MASK   (0xfu << SRMMU_CTRL_VER_SHIFT)
#define SRMMU_CTRL_SC_SHIFT   8
#define SRMMU_CTRL_SC_MASK    (0xffffu << SRMMU_CTRL_SC_SHIFT)
#define SRMMU_CTRL_PSO        (1u << 7) // 1 = Partial Store Ordering, 0 = Total Store Ordering
#define SRMMU_CTRL_NF         (1u << 1) // No Fault mode
#define SRMMU_CTRL_E          (1u << 0) // MMU Enable

// ---- Context Table Pointer Register ----
//
//  31                              2  1     0
// |     Context Table Pointer       | reserved |
//
// Same physical-address-shifted-by-4 encoding as a PTD's PTP field; points
// at the base of the Context Table.
#define SRMMU_CTX_TABLE_PTR_MASK (0xfffffffcu)

#define SRMMU_MAKE_CTX_TABLE_PTR(paddr) SRMMU_PADDR_TO_FIELD(paddr)
#define SRMMU_CTX_TABLE_PTR_PADDR(reg)  SRMMU_FIELD_TO_PADDR(reg)

// ---- Context Register ----
//
// Holds the Context Number of the currently active address space, used to
// index the Context Table. The number of implemented bits is
// implementation-defined.
//
//  31                                                    0
// |                   Context Number                      |
//

// ---- Fault Status Register (FSR) ----
//
//  31         18 17        10 9   8 7    5 4    2 1     0
// |  reserved   |   EBE      |  L  |  AT  |  FT  | FAV | OW |
//
#define SRMMU_FSR_EBE_SHIFT 10
#define SRMMU_FSR_EBE_MASK  (0xffu << SRMMU_FSR_EBE_SHIFT)
#define SRMMU_FSR_L_SHIFT   8
#define SRMMU_FSR_L_MASK    (0x3u << SRMMU_FSR_L_SHIFT)
#define SRMMU_FSR_AT_SHIFT  5
#define SRMMU_FSR_AT_MASK   (0x7u << SRMMU_FSR_AT_SHIFT)
#define SRMMU_FSR_FT_SHIFT  2
#define SRMMU_FSR_FT_MASK   (0x7u << SRMMU_FSR_FT_SHIFT)
#define SRMMU_FSR_FAV       (1u << 1) // Fault Address Register contents are valid
#define SRMMU_FSR_OW        (1u << 0) // Overwrite: FSR was written more than once since last read

// FSR.L: page table level of the entry that caused the fault
#define SRMMU_FSR_L_CONTEXT_TABLE 0
#define SRMMU_FSR_L_LEVEL1        1
#define SRMMU_FSR_L_LEVEL2        2
#define SRMMU_FSR_L_LEVEL3        3

// FSR.AT: type of access that caused the fault
#define SRMMU_FSR_AT_LOAD_USER_DATA        0
#define SRMMU_FSR_AT_LOAD_SUPERVISOR_DATA  1
#define SRMMU_FSR_AT_LOAD_USER_INSN        2
#define SRMMU_FSR_AT_LOAD_SUPERVISOR_INSN  3
#define SRMMU_FSR_AT_STORE_USER_DATA       4
#define SRMMU_FSR_AT_STORE_SUPERVISOR_DATA 5
#define SRMMU_FSR_AT_STORE_USER_INSN       6
#define SRMMU_FSR_AT_STORE_SUPERVISOR_INSN 7

// FSR.FT: fault type
#define SRMMU_FSR_FT_NONE                0
#define SRMMU_FSR_FT_INVALID_ADDRESS     1
#define SRMMU_FSR_FT_PROTECTION          2
#define SRMMU_FSR_FT_PRIVILEGE_VIOLATION 3
#define SRMMU_FSR_FT_TRANSLATION_ERROR   4
#define SRMMU_FSR_FT_ACCESS_BUS_ERROR    5
#define SRMMU_FSR_FT_INTERNAL_ERROR      6

// ---- Fault Address Register (FAR) ----
// Holds the faulting virtual address; valid for data faults and
// translation errors, per FSR.FAV.

// ---- Address Space Identifiers (ASIs) used to access the MMU ----
// Suggested assignments per Appendix I, "Suggested ASI Assignments for
// SPARC Systems"; used as-is by sun4m.
#define SRMMU_ASI_FLUSH_PROBE      0x03
#define SRMMU_ASI_REGS             0x04
#define SRMMU_ASI_DIAG_INSN_PDC    0x05
#define SRMMU_ASI_DIAG_DATA_PDC    0x06
#define SRMMU_ASI_DIAG_IO_PDC      0x07
#define SRMMU_ASI_USER_INSN        0x08
#define SRMMU_ASI_SUPERVISOR_INSN  0x09
#define SRMMU_ASI_USER_DATA        0x0a
#define SRMMU_ASI_SUPERVISOR_DATA  0x0b
#define SRMMU_ASI_PASSTHROUGH_BASE 0x20 // 0x20-0x2f: MMU physical address pass-through

// MMU internal register "addresses", selected by VA[11:8] within
// SRMMU_ASI_REGS (Table H-5); VA[7:0] is ignored and should be zero.
#define SRMMU_REG_CONTROL       0x00000000
#define SRMMU_REG_CTX_TABLE_PTR 0x00000100
#define SRMMU_REG_CONTEXT       0x00000200
#define SRMMU_REG_FSR           0x00000300
#define SRMMU_REG_FAR           0x00000400

// flush/probe type, selected by VA[11:8] within SRMMU_ASI_FLUSH_PROBE
// (Figure H-9); VA[31:12] holds the virtual address to flush/probe and
// VA[7:0] is reserved and should be zero.
#define SRMMU_FLUSH_PROBE_TYPE_SHIFT 8
#define SRMMU_FLUSH_PROBE_PAGE       0 // level-3 entry
#define SRMMU_FLUSH_PROBE_SEGMENT    1 // level-2 entry
#define SRMMU_FLUSH_PROBE_REGION     2 // level-1 entry
#define SRMMU_FLUSH_PROBE_CONTEXT    3 // level-0 (context table) entry
#define SRMMU_FLUSH_PROBE_ENTIRE     4 // entire PDC

#if !ASSEMBLY

#include <stdint.h>

typedef uint32_t srmmu_pte_t;

static inline uint32_t sparc_srmmu_read_reg(uint32_t reg_offset) {
    uint32_t val;
    __asm__ volatile("lda [%1] %2, %0"
                     : "=r"(val)
                     : "r"(reg_offset), "i"(SRMMU_ASI_REGS)
                     : "memory");
    return val;
}

static inline void sparc_srmmu_write_reg(uint32_t reg_offset, uint32_t val) {
    __asm__ volatile("sta %0, [%1] %2"
                     :
                     : "r"(val), "r"(reg_offset), "i"(SRMMU_ASI_REGS)
                     : "memory");
}

#endif // !ASSEMBLY
