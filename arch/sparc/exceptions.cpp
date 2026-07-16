//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//

#include <arch/sparc.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <platform/interrupts.h>
#include <stdio.h>

#define LOCAL_TRACE 0

/*
 * SPARC v8 Trap Types:
 *
 * Trap Type (tt) | Trap Name                    | Description
 * ---------------+------------------------------+-------------------------------------------
 * 0x00           | reset                        | Power-on reset / external reset
 * 0x01           | instruction_access_exception | Instruction fetch access error or MMU fault
 * 0x02           | illegal_instruction          | Unimplemented or illegal instruction
 * 0x03           | privileged_instruction       | Executing supervisor-only instruction in user
 * mode 0x04           | fp_disabled                  | FPU instruction executed while FPU disabled
 * (PSR.EF=0) 0x05           | window_overflow              | Handled in assembly (exceptions.S)
 * 0x06           | window_underflow             | Handled in assembly (exceptions.S)
 * 0x07           | mem_address_not_aligned      | Memory access address not aligned to data size
 * 0x08           | fp_exception                 | Floating-point coprocessor exception
 * 0x09           | data_access_exception        | Data access error or MMU fault (address in SFAR)
 * 0x0a           | tag_overflow                 | Taddcctv or Tsubcctv instruction overflowed
 * 0x0b           | watchpoint_detected          | Data access matched watchpoint address
 * 0x11 - 0x1f    | interrupt_level_1 - 15       | External hardware interrupts (IRQs 1-15)
 * 0x24           | cp_disabled                  | Coprocessor instruction while disabled
 * 0x28           | cp_exception                 | Coprocessor exception
 * 0x80 - 0xff    | software_trap                | Software traps (e.g., system calls)
 */
__NO_INLINE uint32_t sparc_read_sfar() {
    uint32_t val = 0;
    __asm__ volatile("lda [ %1 ] 0x4, %0" : "=r"(val) : "r"(0x400) : "memory");

    return val;
}

extern "C" void sparc_dump_registers(uint32_t pc, uint32_t npc, uint32_t psr) {
    printf("Exception info: PC 0x%08x, nPC 0x%08x, PSR 0x%08x\n", pc, npc, psr);

    uint32_t r2[16], r3[16], r4[16], r5[16];

    __asm__ volatile("std %%l0, [%0 + 0]\n"
                     "std %%l2, [%0 + 8]\n"
                     "std %%l4, [%0 + 16]\n"
                     "std %%l6, [%0 + 24]\n"
                     "std %%i0, [%0 + 32]\n"
                     "std %%i2, [%0 + 40]\n"
                     "std %%i4, [%0 + 48]\n"
                     "std %%i6, [%0 + 56]\n"
                     :
                     : "r"(r2)
                     : "memory");

    __asm__ volatile("restore\n"
                     "std %%l0, [%0 + 0]\n"
                     "std %%l2, [%0 + 8]\n"
                     "std %%l4, [%0 + 16]\n"
                     "std %%l6, [%0 + 24]\n"
                     "std %%i0, [%0 + 32]\n"
                     "std %%i2, [%0 + 40]\n"
                     "std %%i4, [%0 + 48]\n"
                     "std %%i6, [%0 + 56]\n"
                     "save\n"
                     :
                     : "r"(r3)
                     : "memory");

    __asm__ volatile("restore\n"
                     "restore\n"
                     "std %%l0, [%0 + 0]\n"
                     "std %%l2, [%0 + 8]\n"
                     "std %%l4, [%0 + 16]\n"
                     "std %%l6, [%0 + 24]\n"
                     "std %%i0, [%0 + 32]\n"
                     "std %%i2, [%0 + 40]\n"
                     "std %%i4, [%0 + 48]\n"
                     "std %%i6, [%0 + 56]\n"
                     "save\n"
                     "save\n"
                     :
                     : "r"(r4)
                     : "memory");

    __asm__ volatile("restore\n"
                     "restore\n"
                     "restore\n"
                     "std %%l0, [%0 + 0]\n"
                     "std %%l2, [%0 + 8]\n"
                     "std %%l4, [%0 + 16]\n"
                     "std %%l6, [%0 + 24]\n"
                     "std %%i0, [%0 + 32]\n"
                     "std %%i2, [%0 + 40]\n"
                     "std %%i4, [%0 + 48]\n"
                     "std %%i6, [%0 + 56]\n"
                     "save\n"
                     "save\n"
                     "save\n"
                     :
                     : "r"(r5)
                     : "memory");

    int cwp = psr & 0x1f;

    printf("Window CWP=%d (exception handler window):\n", (cwp - 1 + 8) % 8);
    printf("  l0-l3: %08x %08x %08x %08x\n", r2[0], r2[1], r2[2], r2[3]);
    printf("  l4-l7: %08x %08x %08x %08x\n", r2[4], r2[5], r2[6], r2[7]);
    printf("  i0-i3: %08x %08x %08x %08x\n", r2[8], r2[9], r2[10], r2[11]);
    printf("  i4-i7: %08x %08x %08x %08x\n", r2[12], r2[13], r2[14], r2[15]);

    printf("Window CWP=%d (trap window):\n", cwp);
    printf("  l0-l3: %08x %08x %08x %08x\n", r3[0], r3[1], r3[2], r3[3]);
    printf("  l4-l7: %08x %08x %08x %08x\n", r3[4], r3[5], r3[6], r3[7]);
    printf("  i0-i3: %08x %08x %08x %08x\n", r3[8], r3[9], r3[10], r3[11]);
    printf("  i4-i7: %08x %08x %08x %08x\n", r3[12], r3[13], r3[14], r3[15]);

    printf("Window CWP=%d (interrupted window):\n", (cwp + 1) % 8);
    printf("  l0-l3: %08x %08x %08x %08x\n", r4[0], r4[1], r4[2], r4[3]);
    printf("  l4-l7: %08x %08x %08x %08x\n", r4[4], r4[5], r4[6], r4[7]);
    printf("  i0-i3: %08x %08x %08x %08x\n", r4[8], r4[9], r4[10], r4[11]);
    printf("  i4-i7: %08x %08x %08x %08x\n", r4[12], r4[13], r4[14], r4[15]);

    printf("Window CWP=%d (caller of interrupted window):\n", (cwp + 2) % 8);
    printf("  l0-l3: %08x %08x %08x %08x\n", r5[0], r5[1], r5[2], r5[3]);
    printf("  l4-l7: %08x %08x %08x %08x\n", r5[4], r5[5], r5[6], r5[7]);
    printf("  i0-i3: %08x %08x %08x %08x\n", r5[8], r5[9], r5[10], r5[11]);
    printf("  i4-i7: %08x %08x %08x %08x\n", r5[12], r5[13], r5[14], r5[15]);
}

extern "C" {
enum handler_return platform_irq(uint32_t irq);
void sparc_exception(uint32_t exception, uint32_t pc, uint32_t npc, uint32_t psr);
}

extern "C" void sparc_exception(uint32_t exception, uint32_t pc, uint32_t npc, uint32_t psr) {
    LTRACEF("exc %#x at PC %#x, nPC %#x, PSR %#x\n", exception, pc, npc, psr);

    handler_return ret = INT_NO_RESCHEDULE;
    switch (exception) {
        case SPARC_TRAP_INSTRUCTION_ACCESS_EXCEPTION:
            dprintf(INFO, "instruction access exception at PC %#x\n", pc);
            break;
        case SPARC_TRAP_ILLEGAL_INSTRUCTION:
            dprintf(INFO, "illegal instruction at PC %#x\n", pc);
            goto unhandled;
        case SPARC_TRAP_FP_DISABLED:
            dprintf(INFO, "fp disabled at PC %#x\n", pc);
            goto unhandled;
        case SPARC_TRAP_MEM_ADDRESS_NOT_ALIGNED: {
            uint32_t fault_addr = sparc_read_sfar();
            dprintf(INFO, "memory address not aligned at PC %#x at address %#x\n", pc, fault_addr);
            goto unhandled;
        }
        case SPARC_TRAP_DATA_ACCESS_EXCEPTION: {
            uint32_t fault_addr = sparc_read_sfar();
            dprintf(INFO, "data access exception at PC %#x at address %#x\n", pc, fault_addr);
            goto unhandled;
        }
        case SPARC_TRAP_IRQ_1 ... SPARC_TRAP_IRQ_15:                // IRQs 1-15
            ret = platform_irq(exception - (SPARC_TRAP_IRQ_1 - 1)); // normalized as if IRQ0 existed
            break;
unhandled:
        default:
            // unhandled exception
            sparc_dump_registers(pc, npc, psr);
            panic("unhandled exception");
    }

    if (ret == INT_RESCHEDULE) {
        LTRACEF("rescheduling\n");
        thread_preempt();
    } else {
        LTRACEF("no reschedule\n");
    }
}