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

#define NUM_INT 0x31
#define NUM_EXC 0x14

.text

/* interrupt service routine stubs */
_isr:
.set i, 0
.rept NUM_INT

.set isr_stub_start, .

.if i == 8 || (i >= 10 && i <= 14) || i == 17
        nop                                     /* error code pushed by exception */
        nop                                     /* 2 nops are the same length as push byte */
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

    movq %rsp, %rdi     /* pass the  iframe using rdi */

    call x86_exception_handler

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

FUNCTION(setup_idt)
    /* setup isr stub descriptors in the idt */
    mov  $_isr, %rsi
    mov  $_idt, %rdi
    movl $NUM_INT, %ecx

.Lloop:
    mov  %rsi, %rbx
    movw %bx, (%rdi)        /* offset [0:15] in IDT(n).low */
    shr  $16, %rbx
    movw %bx, 6(%rdi)       /* offset [16:31] in IDT(n).high */
    shr  $16, %rbx
    movl %ebx, 8(%rdi)      /* offset [32:63] */

    add  $isr_stub_len, %rsi    /* index the next ISR stub */
    add  $16, %rdi          /* index the next IDT entry */

    loop .Lloop

    lidt _idtr

    ret

.data

.align 8
DATA(_idtr)
    .short _idt_end - _idt - 1  /* IDT limit */
    .quad _idt
.fill 8

.align 8
/* interrupt descriptor table (IDT) */
DATA(_idt)

.set i, 0
.rept NUM_INT-1
    .short 0        /* low 16 bits of ISR offset (_isr#i & 0FFFFh) */
    .short CODE_64_SELECTOR   /* selector */
    .byte  0
    .byte  0x8e     /* present, ring 0, 64-bit interrupt gate */
    .short  0       /* high 16 bits of ISR offset (_isr#i / 65536) */
    .short  0       /* ISR offset */
    .short  0       /* ISR offset */
    .short  0       /* 32bits Reserved */
    .short  0       /* 32bits Reserved */


.set i, i + 1
.endr

.global _idt_end
_idt_end:

