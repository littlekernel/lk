/*
 * Copyright (c) 2013-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if ARM_WITH_VFP || ARCH_ARM64 || X86_WITH_FPU

#include <stdio.h>
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

#if !ARM_WITH_VFP_SP_ONLY
#define FLOAT float
#else
#define FLOAT float
#endif

/* optimize this function to cause it to try to use a lot of registers */
__OPTIMIZE("O3")
static int float_thread(void *arg) {
    FLOAT *val = arg;
    uint i, j;

    FLOAT a[16];

    /* do a bunch of work with floating point to test context switching */
    a[0] = *val;
    for (i = 1; i < countof(a); i++) {
        a[i] = a[i-1] * 1.01f;
    }

    for (i = 0; i < 1000000; i++) {
        a[0] += 0.001f;
        for (j = 1; j < countof(a); j++) {
            a[j] += a[j-1] * 0.00001f;
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

static void float_tests(void) {
    printf("floating point test:\n");

    /* test lazy fpu load on separate thread */
    thread_t *t[8];
    FLOAT val[countof(t)];

    printf("creating %zu floating point threads\n", countof(t));
    for (uint i = 0; i < countof(t); i++) {
        val[i] = i;
        char name[32];
        snprintf(name, sizeof(name), "float %u", i);
        t[i] = thread_create(name, &float_thread, &val[i], LOW_PRIORITY, DEFAULT_STACK_SIZE);
        thread_resume(t[i]);
    }

    int res;
    for (uint i = 0; i < countof(t); i++) {
        thread_join(t[i], &res, INFINITE_TIME);
        printf("float thread %u returns %d, val %f\n", i, res, val[i]);
    }
    printf("the above values should be close\n");

#if ARCH_ARM && !ARM_ISA_ARMV7M
    /* test all the instruction traps */
    arm_float_instruction_trap_test();
#endif
}

STATIC_COMMAND_START
STATIC_COMMAND("float_tests", "floating point test", (console_cmd)&float_tests)
STATIC_COMMAND_END(float_tests);

#endif // ARM_WITH_VFP || ARCH_ARM64
