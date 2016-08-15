/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
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
#include <asm.h>
#include <arch/x86/descriptor.h>
#include <arch/x86/mmu.h>

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC 0x1BADB002

/* The flags for the Multiboot header. */
#if defined(__ELF__) && 0
#define MULTIBOOT_HEADER_FLAGS 0x00000002
#else
#define MULTIBOOT_HEADER_FLAGS 0x00010002
#endif

/* The magic number passed by a Multiboot-compliant boot loader. */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

#define PHYS_LOAD_ADDRESS (MEMBASE + KERNEL_LOAD_OFFSET)
#define PHYS_ADDR_DELTA (KERNEL_BASE + KERNEL_LOAD_OFFSET - PHYS_LOAD_ADDRESS)
#define PHYS(x) ((x) - PHYS_ADDR_DELTA)

.section ".text.boot"
.global _start
_start:
    jmp real_start

.align 4

.type multiboot_header,STT_OBJECT
multiboot_header:
    /* magic */
    .int MULTIBOOT_HEADER_MAGIC
    /* flags */
    .int MULTIBOOT_HEADER_FLAGS
    /* checksum */
    .int -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

#if !defined(__ELF__) || 1
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
#endif

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
#ifdef PAE_MODE_ENABLED
#error broken for now
    /* Preparing PAE paging, we will use 2MB pages covering 1GB
    for initial bootstrap, this page table will be 1 to 1 */

    /* Setting the First PDPTE with a PD table reference*/
    movl $pdp,   %eax
    orl  $0x01, %eax
    movl %eax, (pdpt)

    movl $pdp, %esi
    movl $0x1ff, %ecx

fill_pdp:
    movl $0x1ff, %eax
    subl %ecx, %eax
    shll $21,%eax
    orl  $0x83, %eax
    movl %eax, (%esi)
    addl $8,%esi
    loop fill_pdp

    /* Set PDPT in CR3 */
    movl $pdpt, %eax
    mov %eax, %cr3

    /* Enabling PAE*/
    mov %cr4, %eax
    btsl $(5), %eax
    mov %eax, %cr4

    /* Enabling Paging and from this point we are in
    32 bit compatibility mode */
    mov %cr0,  %eax
    btsl $(31), %eax
    mov %eax,  %cr0

#else
    /* map the first 1GB 1:1 */
    movl $PHYS(pd), %esi
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
    movl $(PHYS(pd) + 0x800), %esi
    movl $0x100, %ecx
    xor  %eax, %eax

.Lfill_pd2:
    mov  %eax, %edx
    orl  $X86_KERNEL_PD_LP_FLAGS, %edx
    movl %edx, (%esi)
    addl $4, %esi
    addl $0x00400000, %eax
    loop .Lfill_pd2

    /* Set PD in CR3 */
    movl $PHYS(pd), %eax
    mov %eax, %cr3

    /* Enabling Paging and from this point we are in */
    mov %cr4, %eax
    orl $0x10, %eax
    mov %eax, %cr4

    mov %cr0,  %eax
    btsl $(31), %eax
    mov %eax, %cr0
#endif

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

    /* call the main module */
    call lk_main
0:                          /* just sit around waiting for interrupts */
    hlt                     /* interrupts will unhalt the processor */
    pause
    jmp 0b                  /* so jump back to halt to conserve power */
