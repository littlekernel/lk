/*
 * Copyright (c) 2013-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * Tests that floating point register state survives context switching. Several
 * threads each churn through a long chain of floating point arithmetic in a
 * deliberately register hungry loop; if the architecture's lazy FPU save and
 * restore drops or mixes up any state, the results come out wrong.
 *
 * The expected values below are exact bit patterns. That is only reproducible
 * because arch/test/rules.mk builds this file with -ffp-contract=off, so the
 * compiler cannot fold the multiply-add pairs into fma instructions.
 */
/*
 * TODO: m68k does not save or restore the FPU registers across a context
 * switch. arch/m68k/asm.S:m68k_context_switch() only preserves %d2-%d7/%a2-%a6,
 * yet the default M68K_CPU is a 68040, which has an FPU. Floating point state
 * is therefore silently corrupted whenever more than one thread uses it, and
 * this test reliably produces garbage and NaNs there, with different values
 * from run to run. It fails the same way on builds from before these tests
 * moved here, so it is a pre-existing kernel bug rather than a test problem.
 * The whole file is compiled out on m68k rather than registering a test case
 * that runs nothing and reports success. Either implement FPU context
 * switching for m68k or set WITH_NO_FP=1 for the FPU bearing CPUs, then drop
 * the ARCH_M68K term below.
 */
#if !WITH_NO_FP && !ARCH_M68K

#include <lib/unittest.h>

#include <stdio.h>
#include <inttypes.h>
#include <rand.h>
#include <string.h>
#include <lk/err.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

#if ARM_WITH_VFP_SP_ONLY
#define FLOAT float
#else
#define FLOAT double
#endif

/* The reference results below are compared with a relative tolerance rather
 * than as exact bit patterns. Some FPUs keep intermediate results in a wider
 * format than the destination -- x87 on 32 bit x86 and the 68881/68040 on m68k
 * both compute in 80 bit extended precision -- so over a million iterations the
 * low bits legitimately drift. That drift is around 1e-14 relative, while
 * losing or mixing up whole registers across a context switch lands orders of
 * magnitude away (or on a NaN), so this still catches what the test is for.
 *
 * Note this tolerance was calibrated against the double precision path, which
 * is what every qemu target in CI exercises. The ARM_WITH_VFP_SP_ONLY path
 * carries only about 1e-7 relative precision to begin with, and test_results_32
 * has never been run on hardware, so expect to have to widen the tolerance and
 * regenerate that table the first time this runs on a single precision only
 * target. */
#define FLOAT_TOLERANCE 1e-9

static bool value_close(double got, double expected) {
    if (got != got) // NaN never compares close
        return false;

    double diff = got - expected;
    if (diff < 0)
        diff = -diff;
    double mag = (expected < 0) ? -expected : expected;

    return diff <= mag * FLOAT_TOLERANCE;
}

/* optimize this function to cause it to try to use a lot of registers */
__OPTIMIZE("O3")
static int float_thread(void *arg) {
    FLOAT *val = arg;

    FLOAT a[16];

    /* do a bunch of work with floating point to test context switching */
    a[0] = *val;
    for (size_t i = 1; i < countof(a); i++) {
        a[i] = a[i-1] * (FLOAT)1.01f;
    }

    for (size_t i = 0; i < 1000000; i++) {
        a[0] += (FLOAT)0.001f;
        for (size_t j = 1; j < countof(a); j++) {
            a[j] += a[j-1] * (FLOAT)0.00001f;
        }
    }

    *val = a[countof(a) - 1];

    return 1;
}

#if ARCH_ARM && !ARM_ISA_ARMV7M && !ARM_ISA_ARMV8M
extern void float_vfp_arm_instruction_test(void);
extern void float_vfp_thumb_instruction_test(void);
extern void float_neon_arm_instruction_test(void);
extern void float_neon_thumb_instruction_test(void);

/* There is nothing to assert here beyond not taking an undefined instruction
 * exception: each routine executes a pile of VFP and NEON encodings with the
 * FPU disabled, so the kernel has to decode and emulate the trap for every one
 * of them. Reaching the end means they all decoded. */
static bool arm_float_instruction_trap_test(void) {
    BEGIN_TEST;

#if !ARM_ONLY_THUMB
    float_vfp_arm_instruction_test();
    float_neon_arm_instruction_test();
#endif
    float_vfp_thumb_instruction_test();
    float_neon_thumb_instruction_test();

    END_TEST;
}
#endif

static bool float_test(void) {
    BEGIN_TEST;

    /* test lazy fpu load on separate thread */
    thread_t *t[8];
    volatile FLOAT val[countof(t)];
    /* Exact results of the loop in float_thread(), which is pure IEEE754
     * arithmetic and so is bit reproducible on any architecture as long as the
     * compiler does not contract the multiply-adds into fma (see the
     * -ffp-contract=off in rules.mk). Cross checked against a host build. */
    const uint32_t test_results_32[8] = {
        0x473aced4,
        0x47889742,
        0x47b3c708,
        0x47def6b4,
        0x4805139b,
        0x481aaab4,
        0x4830432a,
        0x4845da11,
    };
    const uint64_t test_results_64[8] = {
        0x40e7570fc8092db9,
        0x40f1117b2a41e1dd,
        0x40f6776e707f2b8a,
        0x40fbdd61b6bc75d0,
        0x4100a1aa7e7cdfa2,
        0x410354a4219b8562,
        0x4106079dc4ba29ff,
        0x4108ba9767d8cf08,
    };


    for (uint i = 0; i < countof(t); i++) {
        val[i] = i;
        char name[32];
        snprintf(name, sizeof(name), "float %u", i);
        t[i] = thread_create(name, &float_thread, (void *)&val[i], LOW_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(t[i], "could not create float thread");
        thread_resume(t[i]);
    }

    int res;
    for (uint i = 0; i < countof(t); i++) {
        thread_join(t[i], &res, INFINITE_TIME);
        EXPECT_EQ(1, res, "float thread returned the wrong value");

        if (sizeof(FLOAT) == 4) {
            float result = val[i];
            uint32_t result_u32;
            memcpy(&result_u32, &result, sizeof(result_u32));
            float expected;
            memcpy(&expected, &test_results_32[i], sizeof(expected));
            if (!value_close((double)result, (double)expected)) {
                UNITTEST_FAIL_TRACEF("float thread %u: got %#" PRIx32 ", expected %#" PRIx32 "\n",
                                     i, result_u32, test_results_32[i]);
                all_ok = false;
            }
        } else {
            double result = val[i];
            uint64_t result_u64;
            memcpy(&result_u64, &result, sizeof(result_u64));
            double expected;
            memcpy(&expected, &test_results_64[i], sizeof(expected));
            if (!value_close(result, expected)) {
                UNITTEST_FAIL_TRACEF("float thread %u: got %#" PRIx64 ", expected %#" PRIx64 "\n",
                                     i, result_u64, test_results_64[i]);
                all_ok = false;
            }
        }
    }

    END_TEST;
}

BEGIN_TEST_CASE(float_tests)
RUN_TEST(float_test);
#if ARCH_ARM && !ARM_ISA_ARMV7M && !ARM_ISA_ARMV8M
RUN_TEST(arm_float_instruction_trap_test);
#endif
END_TEST_CASE(float_tests)

#endif // !WITH_NO_FP
