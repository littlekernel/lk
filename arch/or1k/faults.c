/*
 * Copyright (c) 2015 Stefan Kristiansson
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
#include <debug.h>
#include <arch/or1k.h>
#include <kernel/thread.h>
#include <platform.h>

static void dump_fault_frame(struct or1k_iframe *frame)
{
    addr_t stack = (addr_t)((char *)frame + 128 + sizeof(frame));

    dprintf(CRITICAL, "r0:  0x%08x r1:  0x%08x: r2:  0x%08x r3:  0x%08x\n",
            0, (uint32_t)stack, frame->r2, frame->r3);
    dprintf(CRITICAL, "r4:  0x%08x r5:  0x%08x: r6:  0x%08x r7:  0x%08x\n",
            frame->r4, frame->r5, frame->r6, frame->r7);
    dprintf(CRITICAL, "r8:  0x%08x r9:  0x%08x: r10: 0x%08x r11: 0x%08x\n",
            frame->r8, frame->r9, frame->r10, frame->r11);
    dprintf(CRITICAL, "r12: 0x%08x r13: 0x%08x: r14: 0x%08x r15: 0x%08x\n",
            frame->r12, frame->r13, frame->r14, frame->r15);
    dprintf(CRITICAL, "r16: 0x%08x r17: 0x%08x: r18: 0x%08x r19: 0x%08x\n",
            frame->r16, frame->r17, frame->r18, frame->r19);
    dprintf(CRITICAL, "r20: 0x%08x r21: 0x%08x: r22: 0x%08x r23: 0x%08x\n",
            frame->r20, frame->r21, frame->r22, frame->r23);
    dprintf(CRITICAL, "r24: 0x%08x r25: 0x%08x: r26: 0x%08x r27: 0x%08x\n",
            frame->r24, frame->r25, frame->r26, frame->r27);
    dprintf(CRITICAL, "r28: 0x%08x r29: 0x%08x: r30: 0x%08x r31: 0x%08x\n",
            frame->r28, frame->r29, frame->r30, frame->r31);
    dprintf(CRITICAL, "PC:  0x%08x SR:  0x%08x\n",
            frame->pc, frame->sr);

    dprintf(CRITICAL, "bottom of stack at 0x%08x:\n", (unsigned int)stack);
    hexdump((void *)stack, 128);

}

static void exception_die(struct or1k_iframe *frame, const char *msg)
{
    inc_critical_section();
    dprintf(CRITICAL, msg);
    dump_fault_frame(frame);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
    for (;;);
}

void or1k_busfault_handler(struct or1k_iframe *frame, uint32_t addr)
{
    dprintf(CRITICAL, "unhandled busfault (EEAR: 0x%08x)", addr);
    exception_die(frame, ", halting\n");
}

void or1k_data_pagefault_handler(struct or1k_iframe *frame, uint32_t addr)
{
    dprintf(CRITICAL, "unhandled data pagefault (EEAR: 0x%08x)", addr);
    exception_die(frame, ", halting\n");
}

void or1k_instruction_pagefault_handler(struct or1k_iframe *frame, uint32_t addr)
{
    dprintf(CRITICAL, "unhandled instruction pagefault (EEAR: 0x%08x)", addr);
    exception_die(frame, ", halting\n");
}

void or1k_alignment_handler(struct or1k_iframe *frame, uint32_t addr)
{
    dprintf(CRITICAL, "unhandled unaligned access (EEAR: 0x%08x)", addr);
    exception_die(frame, ", halting\n");
}

void or1k_illegal_instruction_handler(struct or1k_iframe *frame, uint32_t addr)
{
    dprintf(CRITICAL, "unhandled illegal instruction (EEAR: 0x%08x)", addr);
    exception_die(frame, ", halting\n");
}

void or1k_syscall_handler(struct or1k_iframe *frame)
{
    exception_die(frame, "unhandled syscall, halting\n");
}

void or1k_unhandled_exception(struct or1k_iframe *frame, uint32_t vector)
{
    dprintf(CRITICAL, "unhandled exception (vector: 0x%08x)", vector);
    exception_die(frame, ", halting\n");
}
