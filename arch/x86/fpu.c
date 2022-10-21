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

#include <lk/trace.h>
#include <lk/bits.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/fpu.h>
#include <string.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

#if X86_WITH_FPU

#define FPU_MASK_ALL_EXCEPTIONS 1

/* CPUID EAX = 1 return values */

static bool fp_supported;
static thread_t *fp_owner;

/* FXSAVE area comprises 512 bytes starting with 16-byte aligned */
static uint8_t __ALIGNED(16) fpu_init_states[512]= {0};

/* saved copy of some feature bits */
typedef struct {
    bool with_fpu;
    bool with_sse;
    bool with_sse2;
    bool with_sse3;
    bool with_ssse3;
    bool with_sse4_1;
    bool with_sse4_2;
    bool with_sse4a;
    bool with_fxsave;
    bool with_xsave;

    bool with_xsaveopt;
    bool with_xsavec;
    bool with_xsaves;
} fpu_features_t;

static fpu_features_t fpu_features;

/* called on the first cpu before the kernel is initialized. printfs may not work here */
void x86_fpu_early_init(void) {
    fp_supported = false;
    fp_owner = NULL;

    // test a bunch of fpu features
    fpu_features.with_fpu = x86_feature_test(X86_FEATURE_FPU);
    fpu_features.with_sse = x86_feature_test(X86_FEATURE_SSE);
    fpu_features.with_sse2 = x86_feature_test(X86_FEATURE_SSE2);
    fpu_features.with_sse3 = x86_feature_test(X86_FEATURE_SSE3);
    fpu_features.with_ssse3 = x86_feature_test(X86_FEATURE_SSSE3);
    fpu_features.with_sse4_1 = x86_feature_test(X86_FEATURE_SSE4_1);
    fpu_features.with_sse4_2 = x86_feature_test(X86_FEATURE_SSE4_2);
    fpu_features.with_sse4a = x86_feature_test(X86_FEATURE_SSE4A);
    fpu_features.with_fxsave = x86_feature_test(X86_FEATURE_FXSR);
    fpu_features.with_xsave = x86_feature_test(X86_FEATURE_XSAVE);

    // these are the mandatory ones to continue (for the moment)
    if (!fpu_features.with_fpu || !fpu_features.with_sse || !fpu_features.with_fxsave) {
        return;
    }

    fp_supported = true;

    // detect and save some xsave information
    // NOTE: currently unused
    fpu_features.with_xsaveopt = false;
    fpu_features.with_xsavec = false;
    fpu_features.with_xsaves = false;
    if (fpu_features.with_xsave) {
        LTRACEF("X86: XSAVE detected\n");
        struct x86_cpuid_leaf leaf;
        if (x86_get_cpuid_subleaf(X86_CPUID_XSAVE, 0, &leaf)) {
            fpu_features.with_xsaveopt = BIT(leaf.a, 0);
            fpu_features.with_xsavec = BIT(leaf.a, 1);
            fpu_features.with_xsaves = BIT(leaf.a, 3);
            LTRACEF("xsaveopt %u xsavec %u xsaves %u\n", fpu_features.with_xsaveopt, fpu_features.with_xsavec, fpu_features.with_xsaves);
            LTRACEF("xsave leaf 0: %#x %#x %#x %#x\n", leaf.a, leaf.b, leaf.c, leaf.d);
        }
        if (x86_get_cpuid_subleaf(X86_CPUID_XSAVE, 1, &leaf)) {
            LTRACEF("xsave leaf 1: %#x %#x %#x %#x\n", leaf.a, leaf.b, leaf.c, leaf.d);
        }

        for (int i = 2; i < 64; i++) {
            if (x86_get_cpuid_subleaf(X86_CPUID_XSAVE, i, &leaf)) {
                if (leaf.a > 0) {
                    LTRACEF("xsave leaf %d: %#x %#x %#x %#x\n", i, leaf.a, leaf.b, leaf.c, leaf.d);
                    LTRACEF("\tstate %d: size required %u offset %u\n", i, leaf.a, leaf.b);
                }
            }
        }
    }

    /* No x87 emul, monitor co-processor */
    ulong x = x86_get_cr0();
    x &= ~X86_CR0_EM;
    x |= X86_CR0_NE;
    x |= X86_CR0_MP;
    x86_set_cr0(x);

    /* Init x87 */
    uint16_t fcw;
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
    x |= X86_CR4_OSXMMEXPT; // supports exceptions
    x |= X86_CR4_OSFXSR;    // supports fxsave
    x &= ~X86_CR4_OSXSAVE;  // no support for xsave (currently)
    x86_set_cr4(x);

    uint32_t mxcsr;
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

void x86_fpu_init(void) {
    dprintf(SPEW, "X86: fpu %u sse %u sse2 %u sse3 %u ssse3 %u sse4.1 %u sse4.2 %u sse4a %u\n",
            fpu_features.with_fpu, fpu_features.with_sse, fpu_features.with_sse2,
            fpu_features.with_sse3, fpu_features.with_ssse3, fpu_features.with_sse4_1,
            fpu_features.with_sse4_2, fpu_features.with_sse4a);
    dprintf(SPEW, "X86: fxsave %u xsave %u\n", fpu_features.with_fxsave, fpu_features.with_xsave);

    if (!fp_supported) {
        dprintf(SPEW, "no usable FPU detected (requires SSE + FXSAVE)\n");
    }

    if (fpu_features.with_fxsave) {
        dprintf(SPEW, "X86: FXSAVE detected\n");
    }

    if (fpu_features.with_xsave) {
        dprintf(SPEW, "X86: XSAVE detected\n");
        dprintf(SPEW, "\txsaveopt %u xsavec %u xsaves %u\n", fpu_features.with_xsaveopt, fpu_features.with_xsavec, fpu_features.with_xsaves);

        struct x86_cpuid_leaf leaf;
        if (x86_get_cpuid_subleaf(X86_CPUID_XSAVE, 0, &leaf)) {
            dprintf(SPEW, "\txsave leaf 0: %#x %#x %#x %#x\n", leaf.a, leaf.b, leaf.c, leaf.d);
        }
        if (x86_get_cpuid_subleaf(X86_CPUID_XSAVE, 1, &leaf)) {
            dprintf(SPEW, "\txsave leaf 1: %#x %#x %#x %#x\n", leaf.a, leaf.b, leaf.c, leaf.d);
        }

        for (int i = 2; i < 64; i++) {
            if (x86_get_cpuid_subleaf(X86_CPUID_XSAVE, i, &leaf)) {
                if (leaf.a > 0) {
                    dprintf(SPEW, "\txsave leaf %d: %#x %#x %#x %#x\n", i, leaf.a, leaf.b, leaf.c, leaf.d);
                    dprintf(SPEW, "\t\tstate %d: size required %u offset %u\n", i, leaf.a, leaf.b);
                }
            }
        }
    }

}

void fpu_init_thread_states(thread_t *t) {
    t->arch.fpu_states = (vaddr_t *)ROUNDUP(((vaddr_t)t->arch.fpu_buffer), 16);
    memcpy(t->arch.fpu_states, fpu_init_states, sizeof(fpu_init_states));
}

void fpu_context_switch(thread_t *old_thread, thread_t *new_thread) {
    if (!fp_supported)
        return;

    if (new_thread != fp_owner)
        x86_set_cr0(x86_get_cr0() | X86_CR0_TS);
    else
        x86_set_cr0(x86_get_cr0() & ~X86_CR0_TS);

    return;
}

void fpu_dev_na_handler(void) {
    thread_t *self;

    x86_set_cr0(x86_get_cr0() & ~X86_CR0_TS);

    if (!fp_supported)
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
