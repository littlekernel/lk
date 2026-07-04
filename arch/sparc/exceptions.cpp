//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//

#include <arch/sparc.h>
#include <stdio.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <platform/interrupts.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

/*
 * SPARC v8 Trap Types:
 *
 * Trap Type (tt) | Trap Name                    | Description
 * ---------------+------------------------------+-------------------------------------------
 * 0x00           | reset                        | Power-on reset / external reset
 * 0x01           | instruction_access_exception | Instruction fetch access error or MMU fault
 * 0x02           | illegal_instruction          | Unimplemented or illegal instruction
 * 0x03           | privileged_instruction       | Executing supervisor-only instruction in user mode
 * 0x04           | fp_disabled                  | FPU instruction executed while FPU disabled (PSR.EF=0)
 * 0x05           | window_overflow              | Handled in assembly (exceptions.S)
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
    __asm__ volatile("lda [ %1 ] 0x4, %0" : "=r"(val) : "r" (0x400) : "memory");

    return val;
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
        }
            goto unhandled;
        case SPARC_TRAP_IRQ_1 ... SPARC_TRAP_IRQ_15: // IRQs 1-15
            ret = platform_irq(exception - (SPARC_TRAP_IRQ_1 - 1)); // normalized as if IRQ0 existed
            break;
unhandled:
        default:
            // unhandled exception
            panic("unhandled exception");
    }

    if (ret == INT_RESCHEDULE) {
        LTRACEF("rescheduling\n");
        thread_preempt();
    } else {
        LTRACEF("no reschedule\n");
    }
}