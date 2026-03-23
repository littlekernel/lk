/*
 * Copyright (c) 2012-2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/arm/cm.h>
#include <kernel/thread.h>
#include <lk/bits.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <platform.h>
#include <stdint.h>
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wmissing-declarations"

static void dump_frame(const struct arm_cm_exception_frame *frame) {

    dprintf(INFO, "exception frame at %p\n", frame);
    dprintf(INFO, "\tr0  0x%08x r1  0x%08x r2  0x%08x r3 0x%08x r4 0x%08x\n",
            frame->r0, frame->r1, frame->r2, frame->r3, frame->r4);
    dprintf(INFO, "\tr5  0x%08x r6  0x%08x r7  0x%08x r8 0x%08x r9 0x%08x\n",
            frame->r5, frame->r6, frame->r7, frame->r8, frame->r9);
    dprintf(INFO, "\tr10 0x%08x r11 0x%08x r12 0x%08x\n",
            frame->r10, frame->r11, frame->r12);
    dprintf(INFO, "\tlr  0x%08x pc  0x%08x psr 0x%08x\n",
            frame->lr, frame->pc, frame->psr);
}

void hardfault(struct arm_cm_exception_frame *frame) {
    printf("hardfault\n");
    dump_frame(frame);

#if ARM_ISA_ARMV7M || ARM_ISA_ARMV8M
    if (LK_DEBUGLEVEL >= 1) {
        uint32_t hfsr = SCB->HFSR;

        printf("HFSR 0x%x\n", hfsr);
        if (hfsr & SCB_HFSR_VECTTBL_Msk) {
            printf("vector table read fault\n");
        }
        if (hfsr & SCB_HFSR_FORCED_Msk) {
            printf("escalated configurable fault, CFSR 0x%x\n", SCB->CFSR);
        }
        if (hfsr & SCB_HFSR_DEBUGEVT_Msk) {
            printf("debug event, DFSR 0x%x\n", SCB->DFSR);
        }
    }
#endif

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

#if ARM_ISA_ARMV7M || ARM_ISA_ARMV8M
void memmanage(struct arm_cm_exception_frame *frame) {
    printf("memmanage\n");
    dump_frame(frame);

    if (LK_DEBUGLEVEL >= 1) {
        uint32_t cfsr = SCB->CFSR;
        uint32_t mmfsr = cfsr & 0xff;

        printf("MMFSR 0x%x\n", mmfsr);

        if (cfsr & SCB_CFSR_IACCVIOL_Msk) {
            printf("instruction fault\n");
        }
        if (cfsr & SCB_CFSR_DACCVIOL_Msk) {
            printf("data fault\n");
        }
        if (cfsr & SCB_CFSR_MUNSTKERR_Msk) {
            printf("fault on exception return\n");
        }
        if (cfsr & SCB_CFSR_MSTKERR_Msk) {
            printf("fault on exception entry\n");
        }
#if ARM_ISA_ARMV8M
        if (cfsr & SCB_CFSR_MLSPERR_Msk) {
            printf("fault on lazy fpu preserve\n");
        }
#endif
        if (cfsr & SCB_CFSR_MMARVALID_Msk) {
            printf("fault address 0x%x\n", SCB->MMFAR);
        }
    }
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void usagefault(struct arm_cm_exception_frame *frame) {
    printf("usagefault\n");
    dump_frame(frame);

    if (LK_DEBUGLEVEL >= 1) {
        uint32_t cfsr = SCB->CFSR;
        uint32_t ufsr = BITS_SHIFT(cfsr, 31, 16);
        printf("UFSR 0x%x\n", ufsr);

        if (cfsr & SCB_CFSR_UNDEFINSTR_Msk) {
            printf("undefined instruction\n");
        }
        if (cfsr & SCB_CFSR_INVSTATE_Msk) {
            printf("invalid state\n");
        }
        if (cfsr & SCB_CFSR_INVPC_Msk) {
            printf("invalid PC or EXC_RETURN\n");
        }
        if (cfsr & SCB_CFSR_NOCP_Msk) {
            printf("coprocessor access error\n");
        }
#if ARM_ISA_ARMV8M
        if (cfsr & SCB_CFSR_STKOF_Msk) {
            printf("stack overflow\n");
        }
#endif
        if (cfsr & SCB_CFSR_UNALIGNED_Msk) {
            printf("unaligned error\n");
        }
        if (cfsr & SCB_CFSR_DIVBYZERO_Msk) {
            printf("division by zero\n");
        }
    }

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}
#endif

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
void securefault(struct arm_cm_exception_frame *frame) {
    printf("securefault\n");
    dump_frame(frame);

    if (LK_DEBUGLEVEL >= 1) {
        uint32_t sfsr = SAU->SFSR;
        printf("SFSR 0x%x\n", sfsr);

        if (sfsr & SAU_SFSR_INVEP_Msk) {
            printf("invalid entry point\n");
        }
        if (sfsr & SAU_SFSR_INVIS_Msk) {
            printf("invalid integrity signature\n");
        }
        if (sfsr & SAU_SFSR_INVER_Msk) {
            printf("invalid exception return\n");
        }
        if (sfsr & SAU_SFSR_AUVIOL_Msk) {
            printf("attribution unit violation\n");
        }
        if (sfsr & SAU_SFSR_INVTRAN_Msk) {
            printf("invalid transition\n");
        }
        if (sfsr & SAU_SFSR_LSPERR_Msk) {
            printf("lazy state preservation error\n");
        }
        if (sfsr & SAU_SFSR_LSERR_Msk) {
            printf("lazy state error\n");
        }
        if (sfsr & SAU_SFSR_SFARVALID_Msk) {
            printf("fault address 0x%x\n", SAU->SFAR);
        }
    }

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}
#endif

#if ARM_ISA_ARMV7M || ARM_ISA_ARMV8M
void busfault(struct arm_cm_exception_frame *frame) {
    printf("busfault\n");
    dump_frame(frame);

    if (LK_DEBUGLEVEL >= 1) {
        uint32_t cfsr = SCB->CFSR;
        uint32_t bfsr = BITS_SHIFT(cfsr, 15, 8);

        printf("BFSR 0x%x\n", bfsr);

        if (cfsr & SCB_CFSR_IBUSERR_Msk) {
            printf("instruction bus error\n");
        }
        if (cfsr & SCB_CFSR_PRECISERR_Msk) {
            printf("precise data bus error\n");
        }
        if (cfsr & SCB_CFSR_IMPRECISERR_Msk) {
            printf("imprecise data bus error\n");
        }
        if (cfsr & SCB_CFSR_UNSTKERR_Msk) {
            printf("fault on exception return\n");
        }
        if (cfsr & SCB_CFSR_STKERR_Msk) {
            printf("fault on exception entry\n");
        }
#if ARM_ISA_ARMV8M
        if (cfsr & SCB_CFSR_LSPERR_Msk) {
            printf("fault on lazy floating point state preservation\n");
        }
#endif
        if (cfsr & SCB_CFSR_BFARVALID_Msk) {
            printf("fault address 0x%x\n", SCB->BFAR);
        }
    }

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}
#endif

/* raw exception vectors */

void _nmi(void) {
    printf("nmi\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

/* Declare two versions of the assembly to push the extra registers
 * not already saved by the exception delivery hardware. For armv6-m
 * based hardware we cannot directly push the higher registers so we
 * need to move them into lower registers before pushing.
 */
#if ARM_ISA_ARMV7M || ARM_ISA_ARMV8M
#define PUSH_REGS \
    "push   {r4-r11, lr};" /* 9 words on the stack */
#else
#define PUSH_REGS                       \
    "push   {r4-r7, lr};" /* 5 words */ \
    "mov    r4, r8;"                    \
    "mov    r5, r9;"                    \
    "mov    r6, r10;"                   \
    "mov    r7, r11;"                   \
    "push   {r4-r7};" /* 4 more words */
#endif

__NAKED void _hardfault(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      hardfault;");
}

#if ARM_ISA_ARMV7M || ARM_ISA_ARMV8M
__NAKED void _memmanage(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      memmanage;");
}

__NAKED void _busfault(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      busfault;");
}

__NAKED void _usagefault(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      usagefault;");
}
#endif

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
__NAKED void _securefault(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      securefault;");
}
#endif

#undef PUSH_REGS

/* declared weak so these can be overridden elsewhere */

/* systick handler */
void __WEAK _systick(void) {
    printf("systick\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

#if ARM_ISA_ARMV7M || ARM_ISA_ARMV8M
void __WEAK _debugmonitor(void) {
    printf("debugmonitor\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}
#endif

void __WEAK _svc(void) {
    printf("svc\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}
