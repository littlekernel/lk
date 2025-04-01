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
.balign ISR_STUB_LEN
LOCAL_FUNCTION(_isr_vectors)
.set i, 0
.rept NUM_INT

.balign ISR_STUB_LEN
.if i == 8 || (i >= 10 && i <= 14) || i == 17
    /* error code pushed by exception */
    push $i                 /* interrupt number */
    jmp interrupt_common
.else
    push $0                 /* fill in error code in iframe */
    push $i                 /* interrupt number */
    jmp interrupt_common
.endif

.set i, i + 1
.endr
END_FUNCTION(_isr_vectors)

.balign 16
LOCAL_FUNCTION(interrupt_common)
    cld
    pushl %gs               /* save segment registers */
    pushl %fs
    pushl %es
    pushl %ds
    pusha                   /* save general purpose registers */
    movl $DATA_SELECTOR, %eax /* put known good value in segment registers */
    // do not reset %gs, as it is used by the kernel
    // TODO: when dealing with user space, we need to reset %gs here
    movl %eax, %fs
    movl %eax, %es
    movl %eax, %ds

    movl %esp, %eax         /* store pointer to iframe */
    pushl %eax

    call x86_exception_handler

    popl %eax               /* drop pointer to iframe */

    popa                    /* restore general purpose registers */
    popl %ds                /* restore segment registers */
    popl %es
    popl %fs
    addl $12, %esp          /* drop gs, exception number, and error code */
    iret
END_FUNCTION(interrupt_common)

FUNCTION(setup_idt)
    /* setup isr stub descriptors in the idt */
    movl $_isr_vectors, %esi
    movl $_idt, %edi
    movl $NUM_INT, %ecx

.Lloop:
    movl %esi, %ebx
    movw %bx, (%edi)        /* low word in IDT(n).low */
    shrl $16, %ebx
    movw %bx, 6(%edi)       /* high word in IDT(n).high */

    addl $ISR_STUB_LEN, %esi/* index the next ISR stub */
    addl $8, %edi           /* index the next IDT entry */

    loop .Lloop

    ret
END_FUNCTION(setup_idt)

.data

.balign 8
DATA(_idtr)
    .short _idt_end - _idt - 1  /* IDT limit */
    .int _idt
END_DATA(_idtr)

/* interrupt descriptor table (IDT) */
.balign 8
DATA(_idt)
.set i, 0
.rept NUM_INT-1
    .short 0                /* low 16 bits of ISR offset (_isr#i & 0FFFFh) */
    .short CODE_SELECTOR    /* selector */
    .byte  0
    .byte  0x8e             /* present, ring 0, 32-bit interrupt gate */
    .short 0                /* high 16 bits of ISR offset (_isr#i / 65536) */

.set i, i + 1
.endr

END_DATA(_idt)

DATA(_idt_end)
