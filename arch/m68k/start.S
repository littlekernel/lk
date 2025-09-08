/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.section .text.boot
FUNCTION(_start)
    // load the first 4 args that were pushed on whatever stack we have
    // NOTE: assumes stack is pointing at at least readable memory
    movl    %sp@(4),%d4
    movl    %sp@(8),%d5
    movl    %sp@(12),%d6
    movl    %sp@(16),%d7

#if ARCH_DO_RELOCATION
    lea     %pc@(_start),%a0 // load the current address using PC relative addressing mode
    movl    #_start,%a1      // load the same symbol absolutely
    cmpal   %a0,%a1
    beqs    bss_clear

    // load the end address for loop termination
    movl    #_end,%a2

    // copy forwards
    // NOTE: assumes the source and target do not overlap
0:
    movel   %a0@+,%a1@+
    cmpal   %a1,%a2
    bne     0b

    // branch to the new location
    movl    #bss_clear,%a0
    jmp     %a0@
#endif

    // clear bss
bss_clear:
    lea     %pc@(__bss_start),%a0
    lea     %pc@(__bss_end),%a1
    cmpl    %a0,%a1
    beqs    1f
    // zero 4 bytes at a time
0:
    clrl    %a0@+
    cmpal   %a1,%a0
    bne     0b
1:

#if M68K_MMU == 68040
init_mmu_68040:
    // Set up DTTR0 and ITTR0 to map 0x00000000 - 0x3FFFFFFF (1GB) to 0x00000000
    // Logical address base: 0x00000000, mask 0x3f000000, enable, supervisor, cacheble, copyback
    movl    #0x003fa020,%d0
    movec   %d0,%dtt0
    movec   %d0,%itt0

    // Set up an mapping of [0, MEMSIZE) to [KERNEL_ASPACE_BASE, KERNEL_ASPACE_BASE + MEMSIZE)

    // Set up L0 entries
    lea    %pc@(_root_page_table),%a0
    addl   #(KERNEL_ASPACE_BASE / L0_ENTRY_RANGE * 4),%a0    // offset into the middle of the L0 table for KERNEL_ASPACE_BASE
    movl   #L0_ENTRIES,%d0
    lea    %pc@(_l1_tables),%a1   // get pointer to L1 tables
    addl   #0x00000003,%a1        // mark it valid
.Ll0_loop:
    movl   %a1,%a0@               // store it in the L0 table
    addl   #4,%a0                 // advance to next L0 entry
    addl   #L1_PGTABLE_ENTRIES * 4,%a1 // advance to next L1 table
    subl   #1,%d0
    bne    .Ll0_loop

    // Set up L1 entries
    lea    %pc@(_l1_tables),%a0
    movl   #L1_ENTRIES,%d0
    lea    %pc@(_l2_tables),%a1   // get pointer to L2 table
    addl   #0x00000003,%a1        // mark it valid

.Ll1_loop:
    movl   %a1,%a0@
    addl   #4,%a0
    addl   #L2_PGTABLE_ENTRIES * 4,%a1 // advance to next L1 table
    subl   #1,%d0
    bne    .Ll1_loop

    // Set up L2 entries
    lea    %pc@(_l2_tables),%a0
    movl   #L2_ENTRIES,%d0
    movl   #0x000000083,%d1       // address 0, supervisor, writable, present
.L2_loop:
    movl   %d1,%a0@               // read the current entry
    addl   #4,%a0                 // advance to next L2 entry
    addl   #PAGE_SIZE,%d1         // advance to next page
    subl   #1,%d0
    bne    .L2_loop

    // set the supervisor root pointer
    lea    %pc@(_root_page_table),%a0
    movec  %a0,%srp
    movec  %a0,%urp

    // enable the mmu
    movl   #(1<<15),%d0
    movec  %d0,%tc

    // Branch to the high memory area
    movl   #.Lhigh_target,%a0
    jmp     %a0@
.Lhigh_target:

    // Turn off DTTR0 and ITTR0
    clrl    %d0
    movec   %d0,%dtt0
    movec   %d0,%itt0
#endif

    // load the initial stack pointer
    lea     _default_stack_top,%sp

    // branch into C land with 4 args off the previous stack
    movl    %d7,%sp@-
    movl    %d6,%sp@-
    movl    %d5,%sp@-
    movl    %d4,%sp@-
    jsr     lk_main

    // if we return from main just loop forever
    bra     .
END_FUNCTION(_start)

.bss
.balign 4
_default_stack_base:
    .skip 4096
_default_stack_top:

#if M68K_MMU == 68040
// Define space for page tables to set up a mapping of MEMSIZE bytes of memory at KERNEL_ASPACE_BASE
.equ PAGE_SIZE, 4096
.equ L0_PGTABLE_ENTRIES, 128 // 7 bits
.equ L0_ENTRY_RANGE, (1<<25) // each L0 entry covers 32MB
.equ L1_PGTABLE_ENTRIES, 128 // 7 bits
.equ L1_ENTRY_RANGE, (1<<18) // each L1 entry covers 256KB
.equ L2_PGTABLE_ENTRIES, 64  // 6 bits

// Number of entries at each level to fill in order to cover MEMSIZE,
// rounded up to the next L0 entry range so all of the L1 and L2 page tables are fully used.
.equ MEMSIZE_ROUNDED, (MEMSIZE + L0_ENTRY_RANGE - 1) & ~(L0_ENTRY_RANGE - 1)
.equ L0_ENTRIES, MEMSIZE_ROUNDED / L0_ENTRY_RANGE
.equ L1_ENTRIES, MEMSIZE_ROUNDED / L1_ENTRY_RANGE
.equ L2_ENTRIES, MEMSIZE_ROUNDED / PAGE_SIZE

.balign 4096
_root_page_table:
    .skip L0_PGTABLE_ENTRIES * 4 // 128 entries, 4 bytes each
.balign 4096
_l1_tables:
    .skip L1_ENTRIES * 4 // 4 bytes each, one per 256KB section of memory
.balign 4096
_l2_tables:
    .skip L2_ENTRIES * 4 // 4 bytes each, one per page of memory

#endif // M68K_MMU == 68040
