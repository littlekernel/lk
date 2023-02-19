/*
 * Copyright (c) 2013-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if ARM_WITH_VFP || ARCH_ARM64 || X86_WITH_FPU || (ARCH_RISCV && RISCV_FPU)

#include <stdio.h>
#include <inttypes.h>
#include <rand.h>
#include <lk/err.h>
#include <lk/console_cmd.h>
#include <app/tests.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

extern void float_vfp_arm_instruction_test(void);
extern void float_vfp_thumb_instruction_test(void);
extern void float_neon_arm_instruction_test(void);
extern void float_neon_thumb_instruction_test(void);

#if ARM_WITH_VFP_SP_ONLY
#define FLOAT float
#else
#define FLOAT double
#endif

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

#if ARCH_ARM && !ARM_ISA_ARMV7M
static void arm_float_instruction_trap_test(void) {
    printf("testing fpu trap\n");

#if !ARM_ONLY_THUMB
    float_vfp_arm_instruction_test();
    float_neon_arm_instruction_test();
#endif
    float_vfp_thumb_instruction_test();
    float_neon_thumb_instruction_test();

    printf("if we got here, we probably decoded everything properly\n");
}
#endif

static int float_tests(int argc, const console_cmd_args *argv) {
    printf("floating point test:\n");

    /* test lazy fpu load on separate thread */
    thread_t *t[8];
    volatile FLOAT val[countof(t)];

    printf("creating %zu floating point threads\n", countof(t));
    for (uint i = 0; i < countof(t); i++) {
        val[i] = i;
        char name[32];
        snprintf(name, sizeof(name), "float %u", i);
        t[i] = thread_create(name, &float_thread, (void *)&val[i], LOW_PRIORITY, DEFAULT_STACK_SIZE);
        thread_resume(t[i]);
    }

    int res;
    for (uint i = 0; i < countof(t); i++) {
        thread_join(t[i], &res, INFINITE_TIME);
        double result = (double)val[i];
        printf("float thread %u returns %d, hex val %a\n", i, res, result);
        //hexdump8(&result, 8);
    }
    printf("the above values should be valid and generally increasing, but close\n");

#if ARCH_ARM && !ARM_ISA_ARMV7M
    /* test all the instruction traps */
    arm_float_instruction_trap_test();
#endif

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("float_tests", "floating point test", &float_tests)
STATIC_COMMAND_END(float_tests);

#endif // ARM_WITH_VFP || ARCH_ARM64
