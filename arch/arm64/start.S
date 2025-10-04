#include <lk/asm.h>
#include <arch/arm64/mmu.h>
#include <arch/asm_macros.h>
#include <kernel/vm.h>

/*
 * Register use:
 *  x0-x3   Arguments
 *  x9-x15  Scratch
 *  x18     Off-limits (percpu pointer))
 *  x19-x28 Globals
 */
tmp                     .req x9
tmp2                    .req x10
wtmp2                   .req w10
idx                     .req x11
idx_shift               .req x12
page_table              .req x13
new_page_table          .req x14
phys_offset             .req x15

page_table1             .req x19
mmu_initial_mapping     .req x20
vaddr                   .req x21
paddr                   .req x22
mapping_size            .req x23
size                    .req x24
attr                    .req x25
boot_el                 .req x26

.macro setup_cpu
    /* if we came in at higher than EL1, drop down to EL1 */
    bl      arm64_elX_to_el1

    /* enable caches so atomics and spinlocks work */
    mrs     tmp, sctlr_el1
    bic     tmp, tmp, (1<<19) /* Disable WXN */
    orr     tmp, tmp, (1<<12) /* Enable icache */
    orr     tmp, tmp, (1<<3) | /* Enable Stack Alignment Check EL1 */ \
                      (1<<2) /* Enable dcache/ucache */
    bic     tmp, tmp, (1<<1)  /* Disable Alignment Checking for EL1 EL0 */
    msr     sctlr_el1, tmp

    /* make sure SP_ELx is being used */
    msr     spsel, #1
.endm

.section .text.boot
// Entry point on the boot CPU
FUNCTION(arm64_reset)
#if WITH_SMP
    b       .Larm_reset_primary
    // The second word of the binary is the entry point for secondary CPUs
    // Just a quick solution to make it easy to compute the starting address.
    b       arm64_secondary_phys_entry
.Larm_reset_primary:
#endif

    /* keep track of the boot EL */
    mrs     boot_el, currentel

    // set up the cpu and drop out of EL2 if needed
    setup_cpu

    /* save a copy of the boot args so x0-x3 are available for use */
    adr_global tmp, arm64_boot_args
    stp     x0, x1, [tmp]
    stp     x2, x3, [tmp, #16]

    /* save the boot EL */
    adrp    tmp, arm64_boot_el
    str     boot_el, [tmp, #:lo12:arm64_boot_el]

#if WITH_KERNEL_VM
    /* load the base of the translation table */
    adr_global page_table1, arm64_kernel_translation_table

    /* set up the mmu according to mmu_initial_mappings */
    /* walk through all the entries in the translation table, setting them up */
    mov     tmp, #0
.Lclear_top_page_table_loop:
    str     xzr, [page_table1, tmp, lsl #3]
    add     tmp, tmp, #1
    cmp     tmp, #MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP
    bne     .Lclear_top_page_table_loop

    /* load the address of the mmu_initial_mappings table and start processing */
    adr_global mmu_initial_mapping, mmu_initial_mappings

.Linitial_mapping_loop:
/* Read entry of mmu_initial_mappings (likely defined in platform.c) */
    ldp     paddr, vaddr, [mmu_initial_mapping, #__MMU_INITIAL_MAPPING_PHYS_OFFSET]
    ldp     size, tmp, [mmu_initial_mapping, #__MMU_INITIAL_MAPPING_SIZE_OFFSET]

    tbzmask tmp, MMU_INITIAL_MAPPING_FLAG_DYNAMIC, .Lnot_dynamic
    adr     paddr, _start
    mov     size, x0 /* use the arg passed through from platform_reset */
    str     paddr, [mmu_initial_mapping, #__MMU_INITIAL_MAPPING_PHYS_OFFSET]
    str     size, [mmu_initial_mapping, #__MMU_INITIAL_MAPPING_SIZE_OFFSET]

.Lnot_dynamic:
    /* if size == 0, end of list, done with initial mapping */
    cbz     size, .Linitial_mapping_done
    mov     mapping_size, size

    /* set up the flags */
    tbzmask tmp, MMU_INITIAL_MAPPING_FLAG_UNCACHED, .Lnot_uncached
    ldr     attr, =MMU_INITIAL_MAP_STRONGLY_ORDERED
    b       .Lmem_type_done

.Lnot_uncached:
    /* is this memory mapped to device/peripherals? */
    tbzmask tmp, MMU_INITIAL_MAPPING_FLAG_DEVICE, .Lnot_device
    ldr     attr, =MMU_INITIAL_MAP_DEVICE
    b       .Lmem_type_done
.Lnot_device:

/* Determine the segment in which the memory resides and set appropriate
 *  attributes.  In order to handle offset kernels, the following rules are
 *  implemented below:
 *      KERNEL_BASE    to __code_start             -read/write (see note below)
 *      __code_start   to __rodata_start (.text)   -read only
 *      __rodata_start to __data_start   (.rodata) -read only, execute never
 *      __data_start   to .....          (.data)   -read/write
 *
 *  The space below __code_start is presently left as read/write (same as .data)
 *   mainly as a workaround for the raspberry pi boot process.  Boot vectors for
 *   secondary CPUs are in this area and need to be updated by cpu0 once the system
 *   is ready to boot the secondary processors.
 *   TODO: handle this via mmu_initial_mapping entries, which may need to be
 *         extended with additional flag types
 */
.Lmapping_size_loop:
    movlit  attr, MMU_PTE_KERNEL_DATA_FLAGS

    /* If page is below  the entry point (__code_start) mark as kernel data */
    ldr     tmp, =__code_start
    subs    size, tmp, vaddr
    b.hi    .Lmem_type_done

    /* If the page is between __code_start and __rodata_start mark as RO */
    movlit  attr, MMU_PTE_KERNEL_RO_FLAGS
    ldr     tmp, =__rodata_start
    subs    size, tmp, vaddr
    b.hi    .Lmem_type_done

    /* If the page is between __rodata_start and __data_start mark as RO + XN */
    orr     attr, attr, #MMU_PTE_ATTR_PXN
    ldr     tmp, =__data_start
    subs    size, tmp, vaddr
    b.hi    .Lmem_type_done

    /* > __data_start, mark as kernel data (RW + XN) */
    movlit  attr, MMU_PTE_KERNEL_DATA_FLAGS
    ldr     tmp, =_end
    subs    size, tmp, vaddr
    b.lo    . /* Error: _end < vaddr */
    cmp     mapping_size, size
    b.lo    . /* Error: mapping_size < size => RAM size too small for data/bss */
    mov     size, mapping_size

.Lmem_type_done:
    subs    mapping_size, mapping_size, size
    b.lo    . /* Error: mapping_size < size (RAM size too small for code/rodata?) */

    /* Check that paddr, vaddr and size are page aligned */
    orr     tmp, vaddr, paddr
    orr     tmp, tmp, size
    tst     tmp, #(1 << MMU_KERNEL_PAGE_SIZE_SHIFT) - 1
    bne     . /* Error: not page aligned */

    /* Clear top bits of virtual address (should be all set) */
    eor     vaddr, vaddr, #(~0 << MMU_KERNEL_SIZE_SHIFT)

    /* Check that top bits were all set */
    tst     vaddr, #(~0 << MMU_KERNEL_SIZE_SHIFT)
    bne     . /* Error: vaddr out of range */

.Lmap_range_top_loop:
    /* Select top level page table */
    mov     page_table, page_table1
    mov     idx_shift, #MMU_KERNEL_TOP_SHIFT

    lsr     idx, vaddr, idx_shift


/* determine the type of page table entry to use given alignment and size
 *  of the chunk of memory we are mapping
 */
.Lmap_range_one_table_loop:
    /* Check if current level allow block descriptors */
    cmp     idx_shift, #MMU_PTE_DESCRIPTOR_BLOCK_MAX_SHIFT
    b.hi    .Lmap_range_need_page_table

    /* Check if paddr and vaddr alignment allows a block descriptor */
    orr     tmp2, vaddr, paddr
    lsr     tmp, tmp2, idx_shift
    lsl     tmp, tmp, idx_shift
    cmp     tmp, tmp2
    b.ne    .Lmap_range_need_page_table

    /* Check if size is large enough for a block mapping */
    lsr     tmp, size, idx_shift
    cbz     tmp, .Lmap_range_need_page_table

    /* Select descriptor type, page for level 3, block for level 0-2 */
    orr     tmp, attr, #MMU_PTE_L3_DESCRIPTOR_PAGE
    cmp     idx_shift, MMU_KERNEL_PAGE_SIZE_SHIFT
    beq     .Lmap_range_l3
    orr     tmp, attr, #MMU_PTE_L012_DESCRIPTOR_BLOCK
.Lmap_range_l3:

    /* Write page table entry */
    orr     tmp, tmp, paddr
    str     tmp, [page_table, idx, lsl #3]

    /* Move to next page table entry */
    mov     tmp, #1
    lsl     tmp, tmp, idx_shift
    add     vaddr, vaddr, tmp
    add     paddr, paddr, tmp
    subs    size, size, tmp
    /* TODO: add local loop if next entry is in the same page table */
    b.ne    .Lmap_range_top_loop /* size != 0 */

    /* Restore top bits of virtual address (should be all set) */
    eor     vaddr, vaddr, #(~0 << MMU_KERNEL_SIZE_SHIFT)
    /* Move to next subtype of ram mmu_initial_mappings entry */
    cbnz    mapping_size, .Lmapping_size_loop

    /* Move to next mmu_initial_mappings entry */
    add     mmu_initial_mapping, mmu_initial_mapping, __MMU_INITIAL_MAPPING_SIZE
    b       .Linitial_mapping_loop

.Lmap_range_need_page_table:
    /* Check if page table entry is unused */
    ldr     new_page_table, [page_table, idx, lsl #3]
    cbnz    new_page_table, .Lmap_range_has_page_table

    /* Calculate phys offset (needed for memory allocation) */
.Lphys_offset:
    adr     phys_offset, .Lphys_offset /* phys */
    ldr     tmp, =.Lphys_offset /* virt */
    sub     phys_offset, tmp, phys_offset

    /* Allocate new page table */
    calloc_bootmem_aligned new_page_table, tmp, tmp2, MMU_KERNEL_PAGE_SIZE_SHIFT, phys_offset

    /* Write page table entry (with allocated page table) */
    orr     new_page_table, new_page_table, #MMU_PTE_L012_DESCRIPTOR_TABLE
    str     new_page_table, [page_table, idx, lsl #3]

.Lmap_range_has_page_table:
    /* Check descriptor type */
    and     tmp, new_page_table, #MMU_PTE_DESCRIPTOR_MASK
    cmp     tmp, #MMU_PTE_L012_DESCRIPTOR_TABLE
    b.ne    . /* Error: entry already in use (as a block entry) */

    /* switch to next page table level */
    bic     page_table, new_page_table, #MMU_PTE_DESCRIPTOR_MASK
    mov     tmp, #~0
    lsl     tmp, tmp, idx_shift
    bic     tmp, vaddr, tmp
    sub     idx_shift, idx_shift, #(MMU_KERNEL_PAGE_SIZE_SHIFT - 3)
    lsr     idx, tmp, idx_shift

    b       .Lmap_range_one_table_loop

.Linitial_mapping_done:
    /* compute the base TCR configuration and save away in a global for future use */
    /* inner sharable write-back write-allocate */
    movlit  tmp, MMU_TCR_FLAGS_BASE

    /* Set TCR_EL1.IPS to ID_AA64MMFR0_EL1.PARange */
    mrs     tmp2, id_aa64mmfr0_el1
    and     tmp2, tmp2, #0xf
    /*
     * Give up if we see a reserved value. 52-bit PAs have a different translation
     * table format that we don't support, so use 48-bit PAs in that case.
     */
    cmp     tmp2, #6
    b.hi    .
    b.lo    1f
    mov     tmp2, #5
1:
    orr     tmp, tmp, tmp2, lsl #32
    adrp    tmp2, arm64_mmu_tcr_flags
    str     tmp, [tmp2, #:lo12:arm64_mmu_tcr_flags]

    // Turn on the mmu
    bl      arm64_enable_mmu

    // Running in high kernel space virtual address from here on out
#endif /* WITH_KERNEL_VM */

    /* load the stack pointer */
    ldr tmp, =__stack_end
    mov sp, tmp

    /* clear bss */
.L__do_bss:
    /* clear out the bss excluding the stack and kernel translation table  */
    /* NOTE: relies on __post_prebss_bss_start and __bss_end being 8 byte aligned */
    ldr     tmp, =__post_prebss_bss_start
    ldr     tmp2, =__bss_end
    sub     tmp2, tmp2, tmp
    cbz     tmp2, .L__bss_loop_done
.L__bss_loop:
    sub     tmp2, tmp2, #8
    str     xzr, [tmp], #8
    cbnz    tmp2, .L__bss_loop
.L__bss_loop_done:

    /* set up per-cpu area for the boot cpu */
    bl      arm64_init_boot_percpu

    /* load the boot args we had saved previously */
    adr_global tmp, arm64_boot_args
    ldp     x0, x1, [tmp], #16
    ldp     x2, x3, [tmp]

    bl      lk_main
    b       .
END_FUNCTION(arm64_reset)

#if WITH_SMP
LOCAL_FUNCTION(arm64_secondary_phys_entry)
    // Entry point for secondary CPUs.
    // argument: x0 = cpu number from PSCI
    // TODO: more cleanly handle other boot paths unlike PSCI

    // set up the cpu
    setup_cpu

    // enable the mmu
    bl arm64_enable_mmu

    // Running in high kernel space virtual address from here on out

    cmp     x0, #SMP_MAX_CPUS
    bge     .Lunsupported_cpu_trap

    // Pick a local stack for this cpu out of an array of stacks.
    ldr     tmp, =__stack_end
    mov     tmp2, #ARCH_DEFAULT_STACK_SIZE
    mul     tmp2, tmp2, x0
    sub     sp, tmp, tmp2

    bl      arm64_secondary_entry

.Lunsupported_cpu_trap:
    wfe
    b       .Lunsupported_cpu_trap
END_FUNCTION(arm64_secondary_phys_entry)
#endif

#if WITH_KERNEL_VM
    // Enable the mmu on the current cpu
LOCAL_FUNCTION(arm64_enable_mmu)
    // Compute the difference of virtual and physical addresses and tweak our
    // return address to return to the correct virtual address after enabling
    // the mmu.
    adr     tmp, .Lphys_offset /* phys */
    ldr     tmp2, =.Lphys_offset /* virt */
    sub     tmp, tmp2, tmp
    add     x30, x30, tmp

    /* Invalidate TLB */
    tlbi    vmalle1is
    dsb     sy
    isb

    /* Initialize Memory Attribute Indirection Register */
    movlit  tmp, MMU_MAIR_VAL
    msr     mair_el1, tmp

    /* Initialize TCR_EL1 */
    /* set cacheable attributes on translation walk */
    /* (SMP extensions) non-shareable, inner write-back write-allocate */
    ldr_global tmp, arm64_mmu_tcr_flags
    msr     tcr_el1, tmp

    isb

    /* load the base of the translation table */
    adr_global page_table1, arm64_kernel_translation_table

    /* Write ttbr with phys addr of the translation table */
    msr     ttbr0_el1, xzr
    msr     ttbr1_el1, page_table1
    isb

    /* Set VBAR to the virtual address of the trampoline VBAR */
    ldr     tmp, =trampoline_vbar
    msr     vbar_el1, tmp
    isb

    /* Read SCTLR */
    mrs     tmp, sctlr_el1

    /* Turn on the MMU */
    orr     tmp, tmp, #0x1

    /*
     * Write back SCTLR. This instruction will cause an exception when fetching
     * the following instruction, as the PC will contain an unmapped physical
     * address. This will be handled by the trampoline VBAR which will branch
     * to that instruction's virtual address.
     */
    msr     sctlr_el1, tmp
.Lmmu_on_pc:
    isb

    /* Disable the trampoline VBAR */
    msr     vbar_el1, xzr
    isb

    /* Invalidate TLB */
    tlbi    vmalle1
    dsb     sy
    isb

    ret
END_FUNCTION(arm64_enable_mmu)

.section .text.boot.vectab
/*
 * The only type of exception that we expect with the trampoline VBAR active is
 * sync to current EL. All other exceptions result in infinite loops.
 */
LOCAL_FUNCTION(trampoline_vbar)
.p2align 11
.org 0x00
    wfe
    b       .-4
.org 0x80
    wfe
    b       .-4
.org 0x100
    wfe
    b       .-4
.org 0x180
    wfe
    b       .-4
    /* exception vector for synchronous exceptions from current EL -> current EL */
.org 0x200
    adr_global tmp, .Lmmu_on_pc
    br      tmp
.org 0x280
    wfe
    b       .-4
.org 0x300
    wfe
    b       .-4
.org 0x380
    wfe
    b       .-4
.org 0x400
    wfe
    b       .-4
.org 0x480
    wfe
    b       .-4
.org 0x500
    wfe
    b       .-4
.org 0x580
    wfe
    b       .-4
.org 0x600
    wfe
    b       .-4
.org 0x680
    wfe
    b       .-4
.org 0x700
    wfe
    b       .-4
.org 0x780
    wfe
    b       .-4
END_FUNCTION(trampoline_vbar)
#endif

.data
    .balign 8
LOCAL_DATA(arm64_boot_args)
    .skip (4 * 8)
END_DATA(arm64_boot_args)
DATA(arm64_boot_el)
    .skip 8
END_DATA(arm64_boot_el)

.section .bss.prebss.stack
    .align 4
DATA(__stack)
    .skip ARCH_DEFAULT_STACK_SIZE * SMP_MAX_CPUS
END_DATA(__stack)
DATA(__stack_end)
