/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// x87 setup for CPU=legacy builds (386 + optional 387, 486SX/DX).
//
// There is no per thread x87 state: the coprocessor is shared by every thread, so x87
// code (libm, the float parts of libc) must only ever run on one thread at a time.
//
// Error reporting differs by generation:
// - 486: CR0.NE is set and errors arrive as #MF, like on any later cpu.
// - 386 + 387: there is no CR0.NE. The 387's ERROR# line is wired through the
//   motherboard to IRQ13 and also holds the cpu's BUSY# input, which only a write to
//   port 0xf0 releases. The platform registers an IRQ13 handler that calls
//   x86_x87_irq13_error() when x86_x87_errors_on_irq13() says so.
// With no coprocessor at all CR0.EM is set, so any x87 instruction raises #NM, which
// is fatal.

#include <arch/fpu.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <lk/debug.h>
#include <lk/err.h>

#define X87_BUSY_LATCH_PORT 0xf0

void x86_x87_legacy_init(void) {
    ulong cr0 = x86_get_cr0();
    cr0 &= ~(X86_CR0_EM | X86_CR0_TS | X86_CR0_MP | X86_CR0_NE);

    if (x86_feature_test(X86_FEATURE_FPU)) {
        // MP makes WAIT honour TS, the setting for a real coprocessor. ET selects the 387
        // protocol on a 386 (a 287 would want it clear) and is hardwired on a 486.
        cr0 |= X86_CR0_MP | X86_CR0_ET;
        if (x86_get_cpu_level() >= X86_CPU_LEVEL_486) {
            cr0 |= X86_CR0_NE;
        }
        x86_set_cr0(cr0);
        __asm__ volatile("fninit");
    } else {
        cr0 |= X86_CR0_EM;
        x86_set_cr0(cr0);
    }
}

bool x86_x87_errors_on_irq13(void) {
    return x86_feature_test(X86_FEATURE_FPU) && x86_get_cpu_level() < X86_CPU_LEVEL_486;
}

void x86_x87_irq13_error(void) {
    // release BUSY# first: the cpu stalls every coprocessor instruction while it is held
    outp(X87_BUSY_LATCH_PORT, 0);

    uint16_t fsw;
    __asm__ volatile("fnstsw %0" : "=m"(fsw));

    panic("x87 math fault (irq13), fsw %#hx\n", fsw);
}

#if WITH_LIB_CONSOLE
#include <lk/console_cmd.h>
#include <stdio.h>
#include <string.h>

static int cmd_x87(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
    usage:
        printf("usage: %s info|fault\n", argv[0].str);
        printf("  fault: unmask divide by zero and divide 1 by 0, which is fatal\n");
        return ERR_INVALID_ARGS;
    }

    if (!strcmp(argv[1].str, "info")) {
        const bool fpu = x86_feature_test(X86_FEATURE_FPU);
        printf("x87 %spresent, cr0 %#lx, errors via %s\n", fpu ? "" : "not ", x86_get_cr0(),
               !fpu ? "none" : (x86_x87_errors_on_irq13() ? "irq13" : "#MF"));
    } else if (!strcmp(argv[1].str, "fault")) {
        // control word: default 0x37f with ZM (bit 2) clear
        const uint16_t cw = 0x37b;
        __asm__ volatile("fninit\n\t"
                         "fldcw %0\n\t"
                         "fld1\n\t"
                         "fldz\n\t"
                         "fdivrp\n\t"
                         "fwait\n\t"
                         "fstp %%st(0)" ::"m"(cw));
        printf("no x87 fault was raised\n");
    } else {
        goto usage;
    }
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("x87", "legacy x87 state and fault test", &cmd_x87)
STATIC_COMMAND_END(x87);
#endif
