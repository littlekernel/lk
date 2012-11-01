/*
 * Copyright (c) 2009 Corey Tabaka
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
#include <arch/x86.h>
#include <kernel/thread.h>

static void dump_fault_frame(struct x86_iframe *frame)
{
	dprintf(CRITICAL, " CS:     %04x EIP: %08x EFL: %08x CR2: %08x\n",
	        frame->cs, frame->eip, frame->eflags, x86_get_cr2());
	dprintf(CRITICAL, "EAX: %08x ECX: %08x EDX: %08x EBX: %08x\n",
	        frame->eax, frame->ecx, frame->edx, frame->ebx);
	dprintf(CRITICAL, "ESP: %08x EBP: %08x ESI: %08x EDI: %08x\n",
	        frame->esp, frame->ebp, frame->esi, frame->edi);
	dprintf(CRITICAL, " DS:     %04x  ES:     %04x  FS:     %04x  GS:     %04x\n",
	        frame->ds, frame->es, frame->fs, frame->gs);

	// dump the bottom of the current stack
	addr_t stack = (addr_t) frame; //(addr_t) (((uint32_t *) frame) + (sizeof(struct x86_iframe) / sizeof(uint32_t) - 1));

	if (stack != 0) {
		dprintf(CRITICAL, "bottom of stack at 0x%08x:\n", (unsigned int)stack);
		hexdump((void *)stack, 192);
	}
}

static void exception_die(struct x86_iframe *frame, const char *msg)
{
	inc_critical_section();
	dprintf(CRITICAL, msg);
	dump_fault_frame(frame);

	for (;;) {
		x86_cli();
		x86_hlt();
	}
}

void x86_syscall_handler(struct x86_iframe *frame)
{
	exception_die(frame, "unhandled syscall, halting\n");
}

void x86_gpf_handler(struct x86_iframe *frame)
{
	exception_die(frame, "unhandled gpf, halting\n");
}

void x86_invop_handler(struct x86_iframe *frame)
{
	exception_die(frame, "unhandled invalid op, halting\n");
}

void x86_unhandled_exception(struct x86_iframe *frame)
{
	exception_die(frame, "unhandled exception, halting\n");
}
