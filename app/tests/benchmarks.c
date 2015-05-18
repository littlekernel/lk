/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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
#include <sys/types.h>
#include <stdio.h>
#include <rand.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <app/tests.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

const size_t BUFSIZE = (1024*1024);
const uint ITER = 1024;

__NO_INLINE static void bench_set_overhead(void)
{
    uint32_t *buf = malloc(BUFSIZE);

    uint count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        __asm__ volatile("");
    }
    count = arch_cycle_count() - count;

    printf("took %u cycles overhead to loop %u times\n",
           count, ITER);

    free(buf);
}

__NO_INLINE static void bench_memset(void)
{
    void *buf = malloc(BUFSIZE);

    uint count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        memset(buf, 0, BUFSIZE);
    }
    count = arch_cycle_count() - count;

    printf("took %u cycles to memset a buffer of size %u %d times (%u bytes), %f bytes/cycle\n",
           count, BUFSIZE, ITER, BUFSIZE * ITER, (BUFSIZE * ITER) / (float)count);

    free(buf);
}

#define bench_cset(type) \
__NO_INLINE static void bench_cset_##type(void) \
{ \
    type *buf = malloc(BUFSIZE); \
 \
    uint count = arch_cycle_count(); \
    for (uint i = 0; i < ITER; i++) { \
        for (uint j = 0; j < BUFSIZE / sizeof(*buf); j++) { \
            buf[j] = 0; \
        } \
    } \
    count = arch_cycle_count() - count; \
 \
    printf("took %u cycles to manually clear a buffer using wordsize %d of size %u %d times (%u bytes), %f bytes/cycle\n", \
           count, sizeof(*buf), BUFSIZE, ITER, BUFSIZE * ITER, (BUFSIZE * ITER) / (float)count); \
 \
    free(buf); \
}

bench_cset(uint8_t)
bench_cset(uint16_t)
bench_cset(uint32_t)
bench_cset(uint64_t)

__NO_INLINE static void bench_cset_wide(void)
{
    uint32_t *buf = malloc(BUFSIZE);

    uint count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        for (uint j = 0; j < BUFSIZE / sizeof(*buf) / 8; j++) {
            buf[j*8] = 0;
            buf[j*8+1] = 0;
            buf[j*8+2] = 0;
            buf[j*8+3] = 0;
            buf[j*8+4] = 0;
            buf[j*8+5] = 0;
            buf[j*8+6] = 0;
            buf[j*8+7] = 0;
        }
    }
    count = arch_cycle_count() - count;

    printf("took %u cycles to manually clear a buffer of size %u %d times 8 words at a time (%u bytes), %f bytes/cycle\n",
           count, BUFSIZE, ITER, BUFSIZE * ITER, (BUFSIZE * ITER) / (float)count);

    free(buf);
}

__NO_INLINE static void bench_memcpy(void)
{
    uint8_t *buf = malloc(BUFSIZE);

    uint count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        memcpy(buf, buf + BUFSIZE / 2, BUFSIZE / 2);
    }
    count = arch_cycle_count() - count;

    printf("took %u cycles to memcpy a buffer of size %u %d times (%u source bytes), %f source bytes/cycle\n",
           count, BUFSIZE / 2, ITER, BUFSIZE / 2 * ITER, (BUFSIZE / 2 * ITER) / (float)count);

    free(buf);
}

#if ARCH_ARM
__NO_INLINE static void arm_bench_cset_stm(void)
{
    uint32_t *buf = malloc(BUFSIZE);

    uint count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        for (uint j = 0; j < BUFSIZE / sizeof(*buf) / 8; j++) {
            __asm__ volatile(
                "stm    %0, {r0-r7};"
                :: "r" (&buf[j*8])
            );
        }
    }
    count = arch_cycle_count() - count;

    printf("took %u cycles to manually clear a buffer of size %u %d times 8 words at a time using stm (%u bytes), %f bytes/cycle\n",
           count, BUFSIZE, ITER, BUFSIZE * ITER, (BUFSIZE * ITER) / (float)count);

    free(buf);
}

__NO_INLINE static void arm_bench_multi_issue(void)
{
    uint32_t cycles;
    uint32_t a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0;

#define ITER 1000000
    uint count = ITER;
    cycles = arch_cycle_count();
    while (count--) {
        asm volatile ("");
        asm volatile ("add %0, %0, %0" : "=r" (a) : "r" (a));
        asm volatile ("add %0, %0, %0" : "=r" (b) : "r" (b));
        asm volatile ("and %0, %0, %0" : "=r" (c) : "r" (c));
        asm volatile ("mov %0, %0" : "=r" (d) : "r" (d));
        asm volatile ("orr %0, %0, %0" : "=r" (e) : "r" (e));
        asm volatile ("add %0, %0, %0" : "=r" (f) : "r" (f));
        asm volatile ("and %0, %0, %0" : "=r" (g) : "r" (g));
        asm volatile ("mov %0, %0" : "=r" (h) : "r" (h));
    }
    cycles = arch_cycle_count() - cycles;

    printf("took %u cycles to issue 8 integer ops (%f cycles/iteration)\n", cycles, (float)cycles / ITER);
#undef ITER
}
#endif // ARCH_ARM

#if WITH_LIB_LIBM
#include <math.h>

__NO_INLINE static void bench_sincos(void)
{
    printf("touching the floating point unit\n");
    __UNUSED volatile double _hole = sin(0);

    uint count = arch_cycle_count();
    __UNUSED double a = sin(2.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for sin()\n", count);

    count = arch_cycle_count();
    a = cos(2.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for cos()\n", count);

    count = arch_cycle_count();
    a = sinf(2.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for sinf()\n", count);

    count = arch_cycle_count();
    a = cosf(2.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for cosf()\n", count);

    count = arch_cycle_count();
    a = sqrt(1234567.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for sqrt()\n", count);

    count = arch_cycle_count();
    a = sqrtf(1234567.0f);
    count = arch_cycle_count() - count;
    printf("took %u cycles for sqrtf()\n", count);
}

#endif // WITH_LIB_LIBM

void benchmarks(void)
{
    bench_set_overhead();
    bench_memset();
    bench_memcpy();

    bench_cset_uint8_t();
    bench_cset_uint16_t();
    bench_cset_uint32_t();
    bench_cset_uint64_t();
    bench_cset_wide();

#if ARCH_ARM
    arm_bench_cset_stm();

    arm_bench_multi_issue();
#endif
#if WITH_LIB_LIBM
    bench_sincos();
#endif
}

