/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
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

#define NUM_INT 0x31
#define NUM_EXC 0x14

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
    .int multiboot_header
    /* load_addr */
    .int _start
    /* load_end_addr */
    .int __data_end
    /* bss_end_addr */
    .int __bss_end
    /* entry_addr */
    .int real_start
#endif

real_start:
    cmpl $MULTIBOOT_BOOTLOADER_MAGIC, %eax
    jne 0f
    movl %ebx, (_multiboot_info)
0:
    /* setup isr stub descriptors in the idt */
    movl $_isr, %esi
    movl $_idt, %edi
    movl $NUM_INT, %ecx

.Lloop:
    movl %esi, %ebx
    movw %bx, (%edi)        /* low word in IDT(n).low */
    shrl $16, %ebx
    movw %bx, 6(%edi)       /* high word in IDT(n).high */

    addl $isr_stub_len, %esi/* index the next ISR stub */
    addl $8, %edi           /* index the next IDT entry */

    loop .Lloop

    lidt _idtr
    xorl %eax, %eax
    movl %eax, %cr3

    lgdt _gdtr

    movw $datasel, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %ss
    movw %ax, %gs
    movw %ax, %ss

    movl $_kstack, %esp
    /*We jumped here in protected mode in a code segment that migh not longer
      be valid , do a long jump to our code segment, we use retf instead of
      ljmp to be able to use relative labels */
    movl $codesel_32, %ecx     /*Pushing our code segment */
    push %ecx
    movl $farjump, %ecx /*and jump address */
    push %ecx
    xorl %ecx, %ecx
    retf    /*This instruction will jump to codesel:farjump */
farjump:
    /* zero the bss section */
    movl $__bss_start, %edi /* starting address of the bss */
    movl $__bss_end, %ecx   /* find the length of the bss in bytes */
    subl %edi, %ecx
    shrl $2, %ecx       /* convert to 32 bit words, since the bss is aligned anyway */
2:
    movl $0, (%edi)
    addl $4, %edi
    loop 2b

#ifdef PAE_MODE_ENABLED

    /* Preparing PAE paging, we will use 2MB pages covering 1GB
    for initial bootstrap, this page table will be 1 to 1 */

    /* Setting the First PDPTE with a PD table reference*/
    xorl %eax,  %eax
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
    /* Set PD in CR3 */
    movl $pd, %eax
    mov %eax, %cr3

    movl $pd, %esi
    movl $0x100, %ecx

fill_pd:
    xor %eax, %eax
    mov $0x100, %eax
    sub %ecx, %eax
    shll $22,%eax
    orl  $0x87, %eax
    movl %eax, (%esi)
    addl $4,%esi
    loop fill_pd

    /* Enabling Paging and from this point we are in */
    xorl %eax, %eax
    mov %cr4, %eax
    orl $0x10, %eax
    mov %eax, %cr4
    xorl %eax, %eax
    mov %cr0,  %eax
    btsl $(31), %eax
    mov %eax, %cr0
#endif

    /* Flushing TLB's */
        mov %cr3,%eax
        mov %eax,%cr3

main_lk:
    /* call the main module */
    call lk_main
0:                          /* just sit around waiting for interrupts */
    hlt                     /* interrupts will unhalt the processor */
    pause
    jmp 0b                  /* so jump back to halt to conserve power */

/* interrupt service routine stubs */
_isr:

.set i, 0
.rept NUM_INT

.set isr_stub_start, .

.if i == 8 || (i >= 10 && i <= 14) || i == 17
    nop                     /* error code pushed by exception */
    nop                     /* 2 nops are the same length as push byte */
    pushl $i                /* interrupt number */
    jmp interrupt_common
.else
    pushl $0                /* fill in error code in iframe */
    pushl $i                /* interrupt number */
    jmp interrupt_common
.endif

/* figure out the length of a single isr stub (usually 6 or 9 bytes) */
.set isr_stub_len, . - isr_stub_start

.set i, i + 1
.endr

/* annoying, but force AS to use the same (longer) encoding of jmp for all of the stubs */
.fill 256

interrupt_common:
    pushl %gs               /* save segment registers */
    pushl %fs
    pushl %es
    pushl %ds
    pusha                   /* save general purpose registers */
    movl $datasel, %eax     /* put known good value in segment registers */
    movl %eax, %gs
    movl %eax, %fs
    movl %eax, %es
    movl %eax, %ds
    movl %esp, %eax         /* store stack switch pivot. push esp has errata on some cpus, so use mov/push */
    pushl %eax
    movl %esp, %eax         /* store pointer to iframe, using same method */
    pushl %eax


    call platform_irq

    cmpl $0,%eax
    je 0f
    call thread_preempt

0:

    popl %eax               /* drop pointer to iframe */
    popl %eax               /* restore task_esp, stack switch can occur here if task_esp is modified */
    movl %eax, %esp
    popa                    /* restore general purpose registers */
    popl %ds                /* restore segment registers */
    popl %es
    popl %fs
    popl %gs
    addl $8, %esp           /* drop exception number and error code */
    iret

.data
.align 4

.global _multiboot_info
_multiboot_info:
    .int 0

_gdtr:
    .short _gdt_end - _gdt - 1
    .int _gdt

.global _gdt
_gdt:
    .int 0
    .int 0

/* ring 0 descriptors */
.set codesel_32, . - _gdt
_code_32_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00         /* base 23:16 */
    .byte  0b10011010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) D(1) 0 0 limit 19:16 */
    .byte  0x0          /* base 31:24 */

.set datasel, . - _gdt
_data_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00         /* base 23:16 */
    .byte  0b10010010       /* P(1) DPL(00) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0          /* base 31:24 */

.set user_codesel_32, . - _gdt
_user_code_32_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00         /* base 23:16 */
    .byte  0b11111010       /* P(1) DPL(11) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) D(1) 0 0 limit 19:16 */
    .byte  0x0          /* base 31:24 */

.set user_datasel, . - _gdt
_user_data_32_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00         /* base 23:16 */
    .byte  0b11110010       /* P(1) DPL(11) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0          /* base 31:24 */

.set codesel_64, . - _gdt
_code_64_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00         /* base 23:16 */
    .byte  0b10011010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b10101111       /* G(1) D(0) L(1) AVL(0) limit 19:16 */
    .byte  0x0          /* base 31:24 */

.set datasel_64, . - _gdt
_data_64_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00         /* base 23:16 */
    .byte  0b10010010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 AVL(0) limit 19:16 */
    .byte  0x0          /* base 31:24 */

    .quad  0x0000000000000000
    .quad  0x0000000000000000

.set user_codesel_64, . - _gdt
_user_code_64_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00         /* base 23:16 */
    .byte  0b11111010       /* P(1) DPL(11) S(1) 1 C(0) R(1) A(0) */
    .byte  0b10101111       /* G(1) D(1) L(0) AVL(0) limit 19:16 */
    .byte  0x0          /* base 31:24 */

.set user_datasel_64, . - _gdt
_user_data_64_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00         /* base 23:16 */
    .byte  0b11110010       /* P(1) DPL(11) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0          /* base 31:24 */

.set null_2, . - _gdt
_null_2:
    .int 0
    .int 0

/* TSS descriptor */
.set tsssel, . - _gdt
_tss_gde:
    .short 0                /* limit 15:00 */
    .short 0                /* base 15:00 */
    .byte  0                /* base 23:16 */
    .byte  0x89             /* P(1) DPL(11) 0 10 B(0) 1 */
    .byte  0x80             /* G(0) 0 0 AVL(0) limit 19:16 */
    .short  0               /* base 31:24 */
.global _gdt_end
_gdt_end:

.align 8
.global _idtr
_idtr:
    .short _idt_end - _idt - 1  /* IDT limit */
    .int _idt

/* interrupt descriptor table (IDT) */
.global _idt
_idt:

.set i, 0
.rept NUM_INT-1
    .short 0                /* low 16 bits of ISR offset (_isr#i & 0FFFFh) */
    .short codesel_32           /* selector */
    .byte  0
    .byte  0x8e             /* present, ring 0, 32-bit interrupt gate */
    .short 0                /* high 16 bits of ISR offset (_isr#i / 65536) */

.set i, i + 1
.endr

/* syscall int (ring 3) */
_idt30:
    .short 0                /* low 16 bits of ISR offset (_isr#i & 0FFFFh) */
    .short codesel_32           /* selector */
    .byte  0
    .byte  0xee             /* present, ring 3, 32-bit interrupt gate */
    .short 0                /* high 16 bits of ISR offset (_isr#i / 65536) */

.global _idt_end
_idt_end:

/* Memory for the initial page table, we will use 2 pages for a
   1 to 1 mapping that covers 1GB of physical memory */
.align 4096
.fill 4096

#ifdef PAE_MODE_ENABLED
.align 4096
pdpt:
.fill 4096
pdp:
.fill 4096
#else
.align 4096
pd:
.fill 4096
#endif

.align 4096
.fill 4096

.bss
.align 4096

.global _kstack
.fill 4096
_kstack:
