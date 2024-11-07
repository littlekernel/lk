/*
 * Copyright (c) 2015 Google Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/arm64.h>
#include <kernel/thread.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

static struct fpstate *current_fpstate[SMP_MAX_CPUS];

static void arm64_fpu_load_state(struct thread *t) {
    uint cpu = arch_curr_cpu_num();
    struct fpstate *fpstate = &t->arch.fpstate;

    if (fpstate == current_fpstate[cpu] && fpstate->current_cpu == cpu) {
        LTRACEF("cpu %d, thread %s, fpstate already valid\n", cpu, t->name);
        return;
    }
    LTRACEF("cpu %d, thread %s, load fpstate %p, last cpu %d, last fpstate %p\n",
            cpu, t->name, fpstate, fpstate->current_cpu, current_fpstate[cpu]);
    fpstate->current_cpu = cpu;
    current_fpstate[cpu] = fpstate;


    STATIC_ASSERT(sizeof(fpstate->regs) == 16 * 32);
    __asm__ volatile(
        ".arch_extension fp\n"
        "ldp     q0, q1, [%0, #(0 * 32)]\n"
        "ldp     q2, q3, [%0, #(1 * 32)]\n"
        "ldp     q4, q5, [%0, #(2 * 32)]\n"
        "ldp     q6, q7, [%0, #(3 * 32)]\n"
        "ldp     q8, q9, [%0, #(4 * 32)]\n"
        "ldp     q10, q11, [%0, #(5 * 32)]\n"
        "ldp     q12, q13, [%0, #(6 * 32)]\n"
        "ldp     q14, q15, [%0, #(7 * 32)]\n"
        "ldp     q16, q17, [%0, #(8 * 32)]\n"
        "ldp     q18, q19, [%0, #(9 * 32)]\n"
        "ldp     q20, q21, [%0, #(10 * 32)]\n"
        "ldp     q22, q23, [%0, #(11 * 32)]\n"
        "ldp     q24, q25, [%0, #(12 * 32)]\n"
        "ldp     q26, q27, [%0, #(13 * 32)]\n"
        "ldp     q28, q29, [%0, #(14 * 32)]\n"
        "ldp     q30, q31, [%0, #(15 * 32)]\n"
        "msr     fpcr, %1\n"
        "msr     fpsr, %2\n"
        ".arch_extension nofp\n"
        :: "r"(fpstate), "r"((uint64_t)fpstate->fpcr), "r"((uint64_t)fpstate->fpsr));
}

void arm64_fpu_save_state(struct thread *t) {
    struct fpstate *fpstate = &t->arch.fpstate;
    uint64_t fpcr, fpsr;
    __asm__ volatile(
        ".arch_extension fp\n"
        "stp     q0, q1, [%2, #(0 * 32)]\n"
        "stp     q2, q3, [%2, #(1 * 32)]\n"
        "stp     q4, q5, [%2, #(2 * 32)]\n"
        "stp     q6, q7, [%2, #(3 * 32)]\n"
        "stp     q8, q9, [%2, #(4 * 32)]\n"
        "stp     q10, q11, [%2, #(5 * 32)]\n"
        "stp     q12, q13, [%2, #(6 * 32)]\n"
        "stp     q14, q15, [%2, #(7 * 32)]\n"
        "stp     q16, q17, [%2, #(8 * 32)]\n"
        "stp     q18, q19, [%2, #(9 * 32)]\n"
        "stp     q20, q21, [%2, #(10 * 32)]\n"
        "stp     q22, q23, [%2, #(11 * 32)]\n"
        "stp     q24, q25, [%2, #(12 * 32)]\n"
        "stp     q26, q27, [%2, #(13 * 32)]\n"
        "stp     q28, q29, [%2, #(14 * 32)]\n"
        "stp     q30, q31, [%2, #(15 * 32)]\n"
        "mrs     %0, fpcr\n"
        "mrs     %1, fpsr\n"
        ".arch_extension nofp\n"
        : "=r"(fpcr), "=r"(fpsr)
        : "r"(fpstate));
    fpstate->fpcr = (uint32_t)fpcr;
    fpstate->fpsr = (uint32_t)fpsr;

    LTRACEF("thread %s, fpcr %x, fpsr %x\n", t->name, fpstate->fpcr, fpstate->fpsr);
}

void arm64_fpu_exception(struct arm64_iframe_long *iframe) {
    uint32_t cpacr = ARM64_READ_SYSREG(cpacr_el1);
    if (((cpacr >> 20) & 3) != 3) {
        cpacr |= 3 << 20;
        ARM64_WRITE_SYSREG(cpacr_el1, cpacr);
        thread_t *t = get_current_thread();
        if (likely(t))
            arm64_fpu_load_state(t);
        return;
    }
}
