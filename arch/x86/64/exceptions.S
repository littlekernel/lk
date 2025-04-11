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

#define NUM_INT 0x100
#define ISR_STUB_LEN 16

.text

/* interrupt service routine stubs */

/*
 * pushq $i occupies 5 bytes when i >= 0x80 compare to
 * 2 bytes when i < 0x80, use align to fill the gap
 * to make sure isr_stub_len correct for each interrupts
 */
.balign ISR_STUB_LEN
LOCAL_FUNCTION(_isr_vectors)
_isr:
.set i, 0
.rept NUM_INT

.balign ISR_STUB_LEN
.if i == 8 || (i >= 10 && i <= 14) || i == 17
    /* error code pushed by exception */
    push $i        /* interrupt number */
    jmp  interrupt_common
.else
    push $0        /* fill in error code in iframe */
    push $i        /* interrupt number */
    jmp  interrupt_common
.endif

.set i, i + 1
.endr
END_FUNCTION(_isr_vectors)

.balign 16
LOCAL_FUNCTION(interrupt_common)
    /* clear the direction bit */
    cld

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

    /* TODO: deal with swapgs if coming from user space */

    /* pass the  iframe using rdi */
    movq %rsp, %rdi

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
END_FUNCTION(interrupt_common)

FUNCTION(setup_idt)
    /* setup isr stub descriptors in the idt */
    mov  $_isr_vectors, %rsi
    mov  $_idt, %rdi
    movl $NUM_INT, %ecx

.Lloop:
    mov  %rsi, %rbx
    movw %bx, (%rdi)        /* offset [0:15] in IDT(n).low */
    shr  $16, %rbx
    movw %bx, 6(%rdi)       /* offset [16:31] in IDT(n).high */
    shr  $16, %rbx
    movl %ebx, 8(%rdi)      /* offset [32:63] */

    add  $ISR_STUB_LEN, %rsi    /* index the next ISR stub */
    add  $16, %rdi          /* index the next IDT entry */

    loop .Lloop

    ret
END_FUNCTION(setup_idt)

.data

.balign 8
DATA(_idtr)
    .short _idt_end - _idt - 1  /* IDT limit */
    .quad _idt
END_DATA(_idtr)

.balign 16
/* interrupt descriptor table (IDT) */
DATA(_idt)

.set i, 0
.rept NUM_INT
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

END_DATA(_idt)

DATA(_idt_end)


