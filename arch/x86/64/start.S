/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/x86/descriptor.h>
#include <arch/x86/mmu.h>
#include <hw/multiboot.h>

#define MSR_EFER 0xc0000080
#define EFER_LME 0x00000100

#define PHYS_LOAD_ADDRESS (MEMBASE + KERNEL_LOAD_OFFSET)
#define PHYS_ADDR_DELTA (KERNEL_BASE + KERNEL_LOAD_OFFSET - PHYS_LOAD_ADDRESS)
#define PHYS(x) ((x) - PHYS_ADDR_DELTA)

.section ".text.boot"
.code32
.global _start
_start:
    jmp real_start

.align 8

/* flags for multiboot header */
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE)

.type multiboot_header,STT_OBJECT
multiboot_header:
    /* magic */
    .int MULTIBOOT_HEADER_MAGIC
    /* flags */
    .int MULTIBOOT_HEADER_FLAGS
    /* checksum */
    .int -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

    /* header_addr */
    .int PHYS(multiboot_header)
    /* load_addr */
    .int PHYS(_start)
    /* load_end_addr */
    .int PHYS(__data_end)
    /* bss_end_addr */
    .int PHYS(__bss_end)
    /* entry_addr */
    .int PHYS(real_start)

real_start:
    cmpl $MULTIBOOT_BOOTLOADER_MAGIC, %eax
    jne 0f
    movl %ebx, PHYS(_multiboot_info)

0:
    /* load our new gdt by physical pointer */
    lgdt PHYS(_gdtr_phys)

    /* load our data selectors */
    movw $DATA_SELECTOR, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %ss
    movw %ax, %gs
    movw %ax, %ss

    /* load initial stack pointer */
    movl $PHYS(_kstack + 4096), %esp

    /* far jump to load the CS from our GDT */
    pushl $CODE_SELECTOR
    pushl $PHYS(.Lfarjump)
    lret

.Lfarjump:

    /* zero the bss section */
bss_setup:
    movl $PHYS(__bss_start), %edi /* starting address of the bss */
    movl $PHYS(__bss_end), %ecx   /* find the length of the bss in bytes */
    subl %edi, %ecx
    shrl $2, %ecx       /* convert to 32 bit words, since the bss is aligned anyway */
2:
    movl $0, (%edi)
    addl $4, %edi
    loop 2b

paging_setup:
    /* Preparing 64 bit paging. We will use 2MB pages covering 1GB
     * for initial bootstrap, this page table will be 1 to 1.
     */

    /* PAE bit must be enabled  for 64 bit paging*/
    mov %cr4, %eax
    or  $(1<<5), %eax
    mov %eax, %cr4

    /* load the physical pointer to the top level page table */
    movl $PHYS(kernel_pml4), %eax
    mov %eax, %cr3

    /* save it into a global variable that is used by the kernel */
    movl %eax, PHYS(kernel_pml4_phys)

    /* Long Mode Enabled at this point*/
    movl $MSR_EFER ,%ecx
    rdmsr
    orl $EFER_LME,%eax
    wrmsr

    /* Setting the First PML4E with a PDP table reference at index 0 */
    movl $PHYS(kernel_pdp), %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax
    movl %eax, PHYS(kernel_pml4)

    /* Setting the First PDPTE with a Page table reference at index 0 */
    movl $PHYS(kernel_pte), %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax
    movl %eax, PHYS(kernel_pdp)

    /* point the pml4e at the second high PDP (for -2GB mapping) at index 511 */
    movl $PHYS(kernel_pdp_high),   %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax
    movl %eax, PHYS(kernel_pml4 + 8*511)

    /* point the second pdp at the same low level page table */
    movl $PHYS(kernel_pte), %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax
    movl %eax, PHYS(kernel_pdp_high + 8*510)

    /* map the first 1GB in this table */
    movl $PHYS(kernel_pte), %esi
    movl $0x200, %ecx               /* 512 entries */
    xor  %eax, %eax                 /* start off at address 0 */

0:
    mov  %eax, %ebx
    shll $21, %ebx
    orl  $X86_KERNEL_PD_LP_FLAGS, %ebx
    movl %ebx, (%esi)
    addl $8,%esi
    inc  %eax
    loop 0b                         /* dec ecx and loop while > 0 */

    /* physical [0 ... 1GB] is now be mapped to -2GB (0xffffffff80000000)
     * using 2MB pages, where the kernel image will reside.
     */

    /* set up a linear map of the first 64GB at 0xffffff8000000000 */
    movl $PHYS(kernel_linear_map_pdp), %esi
    movl $32768, %ecx
    xor  %eax, %eax

    /* loop across these page tables, incrementing the address by 2MB */
0:
    mov  %eax, %ebx
    shll $21, %ebx
    orl  $X86_KERNEL_PD_LP_FLAGS, %ebx    # lower word of the entry
    movl %ebx, (%esi)
    mov  %eax, %ebx
    shrl $11, %ebx      # upper word of the entry
    movl %ebx, 4(%esi)
    addl $8,%esi
    inc  %eax
    loop 0b

    /* point the high pdp at our linear mapping page tables */
    movl $PHYS(kernel_pdp_high), %esi
    movl $64, %ecx
    movl $PHYS(kernel_linear_map_pdp), %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax

0:
    movl %eax, (%esi)
    add  $8, %esi
    addl $4096, %eax
    loop 0b

    /* Enabling Paging and from this point we are in 32 bit compatibility mode */
    mov %cr0,  %eax
    btsl $(31), %eax
    mov %eax,  %cr0

    /* Use a far jump to get into 64bit mode */
    pushl $CODE_64_SELECTOR
    pushl $PHYS(farjump64)
    lret

.align 8
.code64
farjump64:
    /* branch to our high address */
    mov  $highaddr, %rax
    jmp  *%rax

highaddr:
    /* load the high kernel stack */
    mov  $(_kstack + 4096), %rsp

    /* reload the gdtr */
    lgdt _gdtr

    /* zero out the data selectors */
    xor  %eax, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /* set up the idt */
    call setup_idt

    /* set up the percpu data structure pointer for the boot cpu */
    /* NOTE: sets the first cpu as APIC id 0 for now, which may not be correct. Fixed later in boot. */
    xor %edi, %edi
    xor %esi, %esi
    call x86_configure_percpu_early

    /* call the main module */
    call lk_main

0:                          /* just sit around waiting for interrupts */
    hlt                     /* interrupts will unhalt the processor */
    pause
    jmp 0b                  /* so jump back to halt to conserve power */
