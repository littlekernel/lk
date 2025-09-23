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

#define PHYS_LOAD_ADDRESS (MEMBASE + KERNEL_LOAD_OFFSET)
#define PHYS_ADDR_DELTA (KERNEL_BASE + KERNEL_LOAD_OFFSET - PHYS_LOAD_ADDRESS)
#define PHYS(x) ((x) - PHYS_ADDR_DELTA)

.section ".text.boot"
.global _start
_start:
    jmp real_start

.align 4

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

    movw $DATA_SELECTOR, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %ss
    movw %ax, %gs
    movw %ax, %ss

    /* load initial stack pointer */
    movl $PHYS(_kstack + 4096), %esp

    /*We jumped here in protected mode in a code segment that migh not longer
      be valid , do a long jump to our code segment, we use retf instead of
      ljmp to be able to use relative labels */
    pushl $CODE_SELECTOR     /*Pushing our code segment */
    pushl $PHYS(.Lfarjump)   /*and jump address */
    retf    /*This instruction will jump to codesel:farjump */

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
#if X86_LEGACY
    /* map the first 16MB 1:1 with 4KB pages and again at 0x8000.0000 */

    /* set up 4 page tables worth of entries */
    movl $PHYS(kernel_pt), %edi
    movl $1024*4,%ecx
    movl $X86_KERNEL_PT_FLAGS, %eax

.Lfill_pt:
    movl %eax, (%edi)
    addl $4, %edi
    addl $4096, %eax
    loop .Lfill_pt

    /* set up the page dir with 4 entries at 0 and 0x8000.0000 pointing
     * to 4 page tables that will map physical address 0 - 16MB
     */
    movl $PHYS(kernel_pd), %esi
    movl $PHYS(kernel_pd) + 512*4, %edi
    movl $PHYS(kernel_pt) + X86_KERNEL_PT_FLAGS, %eax
    movl %eax, (%esi)
    movl %eax, (%edi)
    addl $4096, %eax
    movl %eax, 4(%esi)
    movl %eax, 4(%edi)
    addl $4096, %eax
    movl %eax, 8(%esi)
    movl %eax, 8(%edi)
    addl $4096, %eax
    movl %eax, 12(%esi)
    movl %eax, 12(%edi)
#else
    /* map the first 1GB 1:1 using 4MB pages */
    movl $PHYS(kernel_pd), %esi
    movl $0x100, %ecx
    xor  %eax, %eax

.Lfill_pd:
    mov  %eax, %edx
    orl  $X86_KERNEL_PD_LP_FLAGS, %edx
    movl %edx, (%esi)
    addl $4, %esi
    addl $0x00400000, %eax
    loop .Lfill_pd

    /* map the first 1GB to KERNEL_ASPACE_BASE */
    movl $(PHYS(kernel_pd) + 0x800), %esi
    movl $0x100, %ecx
    xor  %eax, %eax

.Lfill_pd2:
    mov  %eax, %edx
    orl  $X86_KERNEL_PD_LP_FLAGS, %edx
    movl %edx, (%esi)
    addl $4, %esi
    addl $0x00400000, %eax
    loop .Lfill_pd2

    /* enable PSE (4MB pages) */
    mov %cr4, %eax
    orl $(1<<4), %eax
    mov %eax, %cr4
#endif

    /* Set PD in CR3 */
    movl $PHYS(kernel_pd), %eax
    mov %eax, %cr3

    /* save a copy of the address of the kernel page directory */
    movl %eax, PHYS(kernel_pd_phys)

    /* Enabling Paging and from this point we are in */
    mov %cr0,  %eax
    btsl $(31), %eax
    mov %eax, %cr0

    /* load the high kernel stack */
    movl $(_kstack + 4096), %esp

    /* reload the high gdtr */
    lgdt PHYS(_gdtr)

    /* branch to the high address */
    movl $main_lk, %eax
    jmp *%eax

main_lk:
    /* set up the idt */
    call setup_idt

    /* set up the percpu data structure pointer for the boot cpu */
    /* NOTE: sets the first cpu as APIC id 0 for now, which may not be correct. Fixed later in boot. */
    pushl $0
    pushl $0
    call x86_configure_percpu_early

    /* call the main module */
    call lk_main
0:                          /* just sit around waiting for interrupts */
    hlt                     /* interrupts will unhalt the processor */
    pause
    jmp 0b                  /* so jump back to halt to conserve power */
