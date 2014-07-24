/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Intel Corporation 
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

#define MSR_EFER 0xc0000080
#define EFER_LME 0x00000100

.section ".text.boot"
.code32
.global _start
_start:
	jmp real_start

.align 8

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
	.int __bss_start
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
	movw %bx, (%edi)		/* low word in IDT(n).low */
	shrl $16, %ebx
	movw %bx, 6(%edi)		/* high word in IDT(n).high */
	shrl $16, %ebx
	movl $0, %ebx 
	movl %ebx, 8(%edi)
	shrl $16, %ebx
	movl %ebx, 12(%edi)
	shrl $16, %ebx
	
	addl $isr_stub_len, %esi	/* index the next ISR stub */
	addl $16, %edi			/* index the next IDT entry */
		
	
	loop .Lloop

	lidt _idtr
	xorl %eax, %eax
	mov %eax, %cr3

	lgdt _gdtr
		
	movw $datasel, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %ss
	movw %ax, %gs
	movw %ax, %ss
	
	movl $_kstack, %esp

 	/* We need to jump to our sane 32 bit  CS
	 executing a far jump using retf */
	movl $codesel_32, %ecx   
        pushl %ecx
        movl $farjump, %ecx
        pushl %ecx
        xorl %ecx, %ecx
        retf

farjump:
	movl $tsssel, %eax 
	ltr %ax
	
	/* zero the bss section */
	movl $__bss_start, %edi	/* starting address of the bss */
	movl $__bss_end, %ecx	/* find the length of the bss in bytes */
	subl %edi, %ecx
	shrl $2, %ecx			/* convert to 32 bit words, since the bss is aligned anyway */
2:
	movl $0, (%edi)
	addl $4, %edi
	loop 2b


	 
        /* Preparing 64 bit paging, we will use 2MB pages covering 1GB
	   for initial bootstrap, this page table will be 1 to 1  */

	/* PAE bit must be enabled  for 64 bit paging*/
	mov %cr4, %eax
	btsl $(5), %eax
	mov %eax, %cr4  /*Enabling PAE*/
	
	movl $pml4, %eax
	mov %eax, %cr3

	/* Long Mode Enabled at this point*/
	movl $MSR_EFER ,%ecx
        rdmsr
        orl $EFER_LME,%eax
        wrmsr
 
        /* Setting the First PML4E with a PDP table reference*/
        xorl %eax,  %eax
	movl $pdp,   %eax
        orl  $0x3, %eax
        movl %eax, (pml4)
 
        /* Setting the First PDPTE with a Page table reference*/
        xorl %eax, %eax
        movl $pte, %eax
        orl  $0x03, %eax
        movl %eax, (pdp)
 
        movl $pte, %esi
        movl $0x1ff, %ecx

	/* We need 256 entries of 2MB each to cover 1GB */ 
fill_pte:
        movl $0x1ff, %eax
        subl %ecx, %eax
        shll $21,%eax
        orl  $0x83, %eax
	movl %eax, (%esi)
        addl $8,%esi
        loop fill_pte

        /* Enabling Paging and from this point we are in 
	   32 bit compatibility mode*/
        mov %cr0,  %eax
        btsl $(31), %eax
        mov %eax,  %cr0

	/* Flushing TLB's */
	mov %cr3,%eax
	mov %eax,%cr3

	/* Using another long jump to be on 64 bit mode
	   after this we will be on real 64 bit mode */
	movl $codesel_64, %ecx     /*Need to put it in a the right CS*/
        pushl %ecx
        movl $farjump64, %ecx
	pushl %ecx
	retf

.align 8
.code64
farjump64:
	lidt _idtr
	/* call the main module */
	call lk_main
	
0:							/* just sit around waiting for interrupts */
	hlt						/* interrupts will unhalt the processor */
	pause
	jmp 0b					/* so jump back to halt to conserve power */

/* interrupt service routine stubs */
_isr: 
.set i, 0
.rept NUM_INT

.set isr_stub_start, .

.if i == 8 || (i >= 10 && i <= 14) || i == 17
        nop                                             /* error code pushed by exception */
        nop                                             /* 2 nops are the same length as push byte */
        pushq $i                                /* interrupt number */
        jmp interrupt_common
.else
        pushq $0                                /* fill in error code in iframe */
        pushq $i                                /* interrupt number */
        jmp interrupt_common
.endif

/* figure out the length of a single isr stub (usually 6 or 9 bytes) */
.set isr_stub_len, . - isr_stub_start

.set i, i + 1
.endr

/* annoying, but force AS to use the same (longer) encoding of jmp for all of the stubs */
.fill 256

interrupt_common: 

	/* save general purpose registers */
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rax
	pushq %rcx
	pushq %rdx
	pushq %rbx
	pushq %rbp
	pushq %rsi
	pushq %rdi

	/* store stack switch pivot. 
           push rsp has errata on some cpus, so use mov/push */
	movq %rsp, %rax
	pushq %rax
	movq %rsp, %rdi		/* pass the  iframe using rdi */
	
	incl critical_section_count

	call platform_irq
	
	cmpq $0,%rax
	je 0f
	call thread_preempt

0:
	decl critical_section_count

	/* restore task_rsp, stack switch can occur here 
           if task_rsp is modified */
	popq %rax
	movq %rax, %rsp

	/* restore general purpose registers */
	popq %rdi
	popq %rsi
	popq %rbp
	popq %rbx
	popq %rdx
	popq %rcx
	popq %rax
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15

	/* drop vector number and error code*/
	addq $16, %rsp
	iretq

.data
.align 8

.global _multiboot_info
_multiboot_info:
	.int 0
	.int 0
.align 8
.global _gdtr
_gdtr:
	.short _gdt_end - _gdt - 1
	.int _gdt
.align 8
.global _gdt
_gdt:
	.int 0
	.int 0

/* ring 0 descriptors */
.set codesel_32, . - _gdt
_code_32_gde:
	.short 0xffff			/* limit 15:00 */
	.short 0x0000			/* base 15:00 */
	.byte  0x00			/* base 23:16 */
	.byte  0b10011010		/* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
	.byte  0b11001111		/* G(1) D(1) 0 0 limit 19:16 */
	.byte  0x0			/* base 31:24 */
	
.set codesel_64, . - _gdt
_code_64_gde:
	.short 0x0000			/* limit 15:00 */
	.short 0x0000			/* base 15:00 */
	.byte  0x00			/* base 23:16 */
	.byte  0b10011010		/* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
	.byte  0b10100000		/* G(1) D(0) L(1) AVL(0) limit 19:16 */
	.byte  0x0			/* base 31:24 */
	
.set datasel, . - _gdt
_data_gde:
	.short 0xffff			/* limit 15:00 */
	.short 0x0000			/* base 15:00 */
	.byte  0x00			/* base 23:16 */
	.byte  0b10010010		/* P(1) DPL(00) S(1) 0 E(0) W(1) A(0) */
	.byte  0b11001111		/* G(1) B(1) 0 0 limit 19:16 */
	.byte  0x0			/* base 31:24 */
	
.set videosel, . - _gdt
_video_gde:
	.short 0xffff			/* limit 15:00 */
	.short 0x8000			/* base 15:00 */
	.byte  0x0b			/* base 23:16 */
	.byte  0b10010010		/* P(1) DPL(00) S(1) 0 E(0) W(1) A(0) */
	.byte  0b11001111		/* G(1) B(1) 0 0 limit 19:16 */
	.byte  0x0			/* base 31:24 */
	
.if 1
/* ring 3 descriptors */
.set user_codesel_32, . - _gdt
_user_code_32_gde:
	.short 0xffff			/* limit 15:00 */
	.short 0x0000			/* base 15:00 */
	.byte  0x00			/* base 23:16 */
	.byte  0b11111010		/* P(1) DPL(11) S(1) 1 C(0) R(1) A(0) */
	.byte  0b11001111		/* G(1) D(1) 0 0 limit 19:16 */
	.byte  0x0			/* base 31:24 */
	
.set user_codesel_64, . - _gdt
_user_code_64_gde:
	.short 0x0000			/* limit 15:00 */
	.short 0x0000			/* base 15:00 */
	.byte  0x00			/* base 23:16 */
	.byte  0b11111010		/* P(1) DPL(11) S(1) 1 C(0) R(1) A(0) */
	.byte  0b10100000		/* G(1) D(1) L(0) AVL(0) limit 19:16 */
	.byte  0x0			/* base 31:24 */
	
.set user_datasel, . - _gdt
_user_data_gde:
	.short 0xffff			/* limit 15:00 */
	.short 0x0000			/* base 15:00 */
	.byte  0x00			/* base 23:16 */
	.byte  0b11110010		/* P(1) DPL(11) S(1) 0 E(0) W(1) A(0) */
	.byte  0b11001111		/* G(1) B(1) 0 0 limit 19:16 */
	.byte  0x0			/* base 31:24 */
.endif

/* TSS descriptor */
.if 1
.set tsssel, . - _gdt
_tss_gde:
	.short 0				/* limit 15:00 */
	.short 0				/* base 15:00 */
	.byte  0				/* base 23:16 */
	.byte  0x89				/* P(1) DPL(11) 0 10 B(0) 1 */
	.byte  0x80				/* G(0) 0 0 AVL(0) limit 19:16 */
	.byte  0				/* base 31:24 */
	.quad 0x0000000000000000
.endif

.global _gdt_end
_gdt_end:

.align 8
.global _idtr
_idtr:
	.short _idt_end - _idt - 1	/* IDT limit */
	.long _idt
.fill 8
.align 8
/* interrupt descriptor table (IDT) */
.global _idt
_idt:

.set i, 0
.rept NUM_INT-1
	.short 0		/* low 16 bits of ISR offset (_isr#i & 0FFFFh) */
	.short codesel_64	/* selector */
	.byte  0
	.byte  0x8e		/* present, ring 0, 64-bit interrupt gate */
	.short  0		/* high 16 bits of ISR offset (_isr#i / 65536) */
	.short  0		/* ISR offset */
	.short  0		/* ISR offset */
	.short  0		/* 32bits Reserved */
	.short  0		/* 32bits Reserved */

	
.set i, i + 1
.endr

/* syscall int (ring 3) */
_idt30:
	.short 0				/* low 16 bits of ISR offset (_isr#i & 0FFFFh) */
	.short codesel_64			/* selector */
	.byte  0
	.byte  0xee				/* present, ring 3, 32-bit interrupt gate */
	.short 0				/* high 16 bits of ISR offset (_isr#i / 65536) */

.global _idt_end
_idt_end:

/* Memory for the initial page table, we will use 3 pages for a
   1 to 1 mapping that covers 1GB of physycal memory */ 
.align 4096
pml4:
.fill 4096
pdp:
.fill 4096
pte:
.fill 4096

.bss
.align 4096

.global _kstack
.fill 4096
_kstack:
