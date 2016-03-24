/*
 * Copyright (c) 2013-2015 Travis Geiselbrecht
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
#include <arch/arm.h>
#include <assert.h>
#include <trace.h>
#include <stdbool.h>
#include <string.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

static inline bool is_16regs(void)
{
    uint32_t mvfr0;
    __asm__ volatile("vmrs	%0, MVFR0" : "=r"(mvfr0));

    return (mvfr0 & 0xf) == 1;
}

static inline uint32_t read_fpexc(void)
{
    uint32_t val;
    /* use legacy encoding of vmsr reg, fpexc */
    __asm__("mrc  p10, 7, %0, c8, c0, 0" : "=r" (val));
    return val;
}

static inline void write_fpexc(uint32_t val)
{
    /* use legacy encoding of vmrs fpexc, reg */
    __asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));
}

void arm_fpu_set_enable(bool enable)
{
    /* set enable bit in fpexc */
    write_fpexc(enable ? (1<<30) : 0);
}

#if ARM_WITH_VFP
void arm_fpu_undefined_instruction(struct arm_iframe *frame)
{
    thread_t *t = get_current_thread();

    if (unlikely(arch_in_int_handler())) {
        panic("floating point code in irq context. pc 0x%x\n", frame->pc);
    }

    LTRACEF("enabling fpu on thread %p\n", t);

    t->arch.fpused = true;
    arm_fpu_thread_swap(NULL, t);

    /* make sure the irq glue leaves the floating point unit enabled on the way out */
    frame->fpexc |= (1<<30);
}

void arm_fpu_thread_initialize(struct thread *t)
{
    /* zero the fpu register state */
    memset(t->arch.fpregs, 0, sizeof(t->arch.fpregs));

    t->arch.fpexc = (1<<30);
    t->arch.fpscr = 0;
    t->arch.fpused = false;
}

void arm_fpu_thread_swap(struct thread *oldthread, struct thread *newthread)
{
    LTRACEF("old %p (%d), new %p (%d)\n",
            oldthread, oldthread ? oldthread->arch.fpused : 0,
            newthread, newthread ? newthread->arch.fpused : 0);

    if (oldthread) {
        if (oldthread->arch.fpused) {
            /* save the old state */
            uint32_t fpexc;
            fpexc = read_fpexc();

            oldthread->arch.fpexc = fpexc;

            /* make sure that the fpu is enabled, so the next instructions won't fault */
            arm_fpu_set_enable(true);

            __asm__ volatile("vmrs  %0, fpscr" : "=r" (oldthread->arch.fpscr));
            __asm__ volatile("vstm   %0, { d0-d15 }" :: "r" (&oldthread->arch.fpregs[0]));
            if (!is_16regs()) {
                __asm__ volatile("vstm   %0, { d16-d31 }" :: "r" (&oldthread->arch.fpregs[16]));
            }

            arm_fpu_set_enable(false);
        }
    }

    if (newthread) {
        if (newthread->arch.fpused) {
            // load the new state
            arm_fpu_set_enable(true);
            __asm__ volatile("vmsr  fpscr, %0" :: "r" (newthread->arch.fpscr));

            __asm__ volatile("vldm   %0, { d0-d15 }" :: "r" (&newthread->arch.fpregs[0]));
            if (!is_16regs()) {
                __asm__ volatile("vldm   %0, { d16-d31 }" :: "r" (&newthread->arch.fpregs[16]));
            }
            write_fpexc(newthread->arch.fpexc);
        } else {
            arm_fpu_set_enable(false);
        }
    }
}
#endif
