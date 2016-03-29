/*
* Copyright (c) 2015 Intel Corporation
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

#include <trace.h>
#include <arch/x86.h>
#include <arch/fpu.h>
#include <string.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

#if X86_WITH_FPU

#define FPU_MASK_ALL_EXCEPTIONS 1

/* CPUID EAX = 1 return values */

#define ECX_SSE3    (0x00000001 << 0)
#define ECX_SSSE3   (0x00000001 << 9)
#define ECX_SSE4_1  (0x00000001 << 19)
#define ECX_SSE4_2  (0x00000001 << 20)
#define EDX_FXSR    (0x00000001 << 24)
#define EDX_SSE     (0x00000001 << 25)
#define EDX_SSE2    (0x00000001 << 26)
#define EDX_FPU     (0x00000001 << 0)

#define FPU_CAP(ecx, edx) ((edx & EDX_FPU) != 0)

#define SSE_CAP(ecx, edx) ( \
    ((ecx & (ECX_SSE3 | ECX_SSSE3 | ECX_SSE4_1 | ECX_SSE4_2)) != 0) || \
    ((edx & (EDX_SSE | EDX_SSE2)) != 0) \
    )

#define FXSAVE_CAP(ecx, edx) ((edx & EDX_FXSR) != 0)

static int fp_supported;
static thread_t *fp_owner;

/* FXSAVE area comprises 512 bytes starting with 16-byte aligned */
static uint8_t __ALIGNED(16) fpu_init_states[512]= {0};

static void get_cpu_cap(uint32_t *ecx, uint32_t *edx)
{
    uint32_t eax = 1;

    __asm__ __volatile__
    ("cpuid" : "=c" (*ecx), "=d" (*edx) : "a" (eax));
}

void fpu_init(void)
{
    uint32_t ecx = 0, edx = 0;
    uint16_t fcw;
    uint32_t mxcsr;

#ifdef ARCH_X86_64
    uint64_t x;
#else
    uint32_t x;
#endif

    fp_supported = 0;
    fp_owner = NULL;

    get_cpu_cap(&ecx, &edx);

    if (!FPU_CAP(ecx, edx) || !SSE_CAP(ecx, edx) || !FXSAVE_CAP(ecx, edx))
        return;

    fp_supported = 1;

    /* No x87 emul, monitor co-processor */

    x = x86_get_cr0();
    x &= ~X86_CR0_EM;
    x |= X86_CR0_NE;
    x |= X86_CR0_MP;
    x86_set_cr0(x);

    /* Init x87 */
    __asm__ __volatile__ ("finit");
    __asm__ __volatile__("fstcw %0" : "=m" (fcw));
#if FPU_MASK_ALL_EXCEPTIONS
    /* mask all exceptions */
    fcw |= 0x3f;
#else
    /* unmask all exceptions */
    fcw &= 0xffc0;
#endif
    __asm__ __volatile__("fldcw %0" : : "m" (fcw));

    /* Init SSE */
    x = x86_get_cr4();
    x |= X86_CR4_OSXMMEXPT;
    x |= X86_CR4_OSFXSR;
    x &= ~X86_CR4_OSXSAVE;
    x86_set_cr4(x);

    __asm__ __volatile__("stmxcsr %0" : "=m" (mxcsr));
#if FPU_MASK_ALL_EXCEPTIONS
    /* mask all exceptions */
    mxcsr = (0x3f << 7);
#else
    /* unmask all exceptions */
    mxcsr &= 0x0000003f;
#endif
    __asm__ __volatile__("ldmxcsr %0" : : "m" (mxcsr));

    /* save fpu initial states, and used when new thread creates */
    __asm__ __volatile__("fxsave %0" : "=m" (fpu_init_states));

    x86_set_cr0(x86_get_cr0() | X86_CR0_TS);
    return;
}

void fpu_init_thread_states(thread_t *t)
{
    t->arch.fpu_states = (vaddr_t *)ROUNDUP(((vaddr_t)t->arch.fpu_buffer), 16);
    memcpy(t->arch.fpu_states, fpu_init_states, sizeof(fpu_init_states));
}

void fpu_context_switch(thread_t *old_thread, thread_t *new_thread)
{
    if (fp_supported == 0)
        return;

    if (new_thread != fp_owner)
        x86_set_cr0(x86_get_cr0() | X86_CR0_TS);
    else
        x86_set_cr0(x86_get_cr0() & ~X86_CR0_TS);

    return;
}

void fpu_dev_na_handler(void)
{
    thread_t *self;

    x86_set_cr0(x86_get_cr0() & ~X86_CR0_TS);

    if (fp_supported == 0)
        return;

    self = get_current_thread();

    LTRACEF("owner %p self %p\n", fp_owner, self);
    if ((fp_owner != NULL) && (fp_owner != self)) {
        __asm__ __volatile__("fxsave %0" : "=m" (*fp_owner->arch.fpu_states));
        __asm__ __volatile__("fxrstor %0" : : "m" (*self->arch.fpu_states));
    }

    fp_owner = self;
    return;
}
#endif

/* End of file */
