/*
 * Copyright (c) 2013-2014 Travis Geiselbrecht
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
#if ARM_WITH_VFP

#include <stdio.h>
#include <rand.h>
#include <err.h>
#include <app/tests.h>
#include <arch/arm.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

extern void float_vfp_arm_instruction_test(void);
extern void float_vfp_thumb_instruction_test(void);
extern void float_neon_arm_instruction_test(void);
extern void float_neon_thumb_instruction_test(void);

/* optimize this function to cause it to try to use a lot of registers */
__OPTIMIZE("O3")
static int float_thread(void *arg)
{
    uint64_t *val = arg;
    uint i, j;

    double a[16];

    /* do a bunch of work with floating point to test context switching */
    a[0] = *val;
    for (i = 1; i < countof(a); i++) {
        a[i] = a[i-1] * 1.01;
    }

    for (i = 0; i < 1000000; i++) {
        a[0] += i;
        for (j = 1; j < countof(a); j++) {
            a[j] += a[j-1] * 0.00001;
        }
    }

    *val = a[countof(a) - 1];

    return 1;
}

static void float_instruction_trap_test(void)
{
    printf("testing fpu trap\n");

#if !ARM_ONLY_THUMB
    float_vfp_arm_instruction_test();
    float_neon_arm_instruction_test();
#endif
    float_vfp_thumb_instruction_test();
    float_neon_thumb_instruction_test();
}

void float_tests(void)
{
    printf("floating point test:\n");

    /* test lazy fpu load on separate thread */
    printf("creating floating point threads\n");
    thread_t *t[8];
    uint64_t val[countof(t)];

    for (uint i = 0; i < countof(t); i++) {
        val[i] = i;
        t[i] = thread_create("float", &float_thread, &val[i], DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        thread_resume(t[i]);
    }

    int res;
    for (uint i = 0; i < countof(t); i++) {
        thread_join(t[i], &res, INFINITE_TIME);
        printf("float thread %u returns %d, val 0x%llx\n", i, res, val[i]);
    }

    /* test all the instruction traps */
    float_instruction_trap_test();

    printf("if we got here, we probably decoded everything properly\n");
}

#endif // ARM_WITH_VFP
