/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <stdio.h>
#include <rand.h>
#include <lk/err.h>
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

__NO_INLINE static void bench_set_overhead(void) {
    uint32_t *buf = malloc(BUFSIZE);
    if (!buf) {
        printf("failed to allocate buffer\n");
        return;
    }

    ulong count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        __asm__ volatile("");
    }
    count = arch_cycle_count() - count;

    printf("took %lu cycles overhead to loop %u times\n", count, ITER);

    free(buf);
}

__NO_INLINE static void bench_memset(void) {
    void *buf = malloc(BUFSIZE);
    if (!buf) {
        printf("failed to allocate buffer\n");
        return;
    }

    ulong count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        memset(buf, 0, BUFSIZE);
    }
    count = arch_cycle_count() - count;

    size_t total_bytes = BUFSIZE * ITER;
    double bytes_cycle = total_bytes / (double)count;
    printf("took %lu cycles to memset a buffer of size %zu %d times (%zu bytes), %f bytes/cycle\n",
           count, BUFSIZE, ITER, total_bytes, bytes_cycle);

    free(buf);
}

#define bench_cset(type) \
__NO_INLINE static void bench_cset_##type(void) \
{ \
    type *buf = malloc(BUFSIZE); \
    if (!buf) { \
        printf("failed to allocate buffer\n"); \
        return; \
    } \
 \
    ulong count = arch_cycle_count(); \
    for (uint i = 0; i < ITER; i++) { \
        for (uint j = 0; j < BUFSIZE / sizeof(*buf); j++) { \
            buf[j] = 0; \
        } \
    } \
    count = arch_cycle_count() - count; \
 \
    size_t total_bytes = BUFSIZE * ITER; \
    double bytes_cycle = total_bytes / (double)count; \
    printf("took %lu cycles to manually clear a buffer using wordsize %zu of size %zu %u times (%zu bytes), %f bytes/cycle\n", \
           count, sizeof(*buf), BUFSIZE, ITER, total_bytes, bytes_cycle); \
 \
    free(buf); \
}

bench_cset(uint8_t)
bench_cset(uint16_t)
bench_cset(uint32_t)
bench_cset(uint64_t)

__NO_INLINE static void bench_cset_wide(void) {
    uint32_t *buf = malloc(BUFSIZE);
    if (!buf) {
        printf("failed to allocate buffer\n");
        return;
    }

    ulong count = arch_cycle_count();
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

    size_t total_bytes = BUFSIZE * ITER;
    double bytes_cycle = total_bytes / (double)count;
    printf("took %lu cycles to manually clear a buffer of size %zu %d times 8 words at a time (%zu bytes), %f bytes/cycle\n",
           count, BUFSIZE, ITER, total_bytes, bytes_cycle);

    free(buf);
}

__NO_INLINE static void bench_memcpy(void) {
    uint8_t *buf = malloc(BUFSIZE);
    if (!buf) {
        printf("failed to allocate buffer\n");
        return;
    }

    ulong count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        memcpy(buf, buf + BUFSIZE / 2, BUFSIZE / 2);
    }
    count = arch_cycle_count() - count;

    size_t total_bytes = (BUFSIZE / 2) * ITER;
    double bytes_cycle = total_bytes / (double)count;
    printf("took %lu cycles to memcpy a buffer of size %zu %d times (%zu source bytes), %f source bytes/cycle\n",
           count, BUFSIZE / 2, ITER, total_bytes, bytes_cycle);

    free(buf);
}

#if ARCH_ARM
__NO_INLINE static void arm_bench_cset_stm(void) {
    uint32_t *buf = malloc(BUFSIZE);
    if (!buf) {
        printf("failed to allocate buffer\n");
        return;
    }

    ulong count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        for (uint j = 0; j < BUFSIZE / sizeof(*buf) / 8; j++) {
            __asm__ volatile(
                "stm    %0, {r0-r7};"
                :: "r" (&buf[j*8])
            );
        }
    }
    count = arch_cycle_count() - count;

    size_t total_bytes = BUFSIZE * ITER;
    double bytes_cycle = total_bytes / (float)count;
    printf("took %lu cycles to manually clear a buffer of size %zu %d times 8 words at a time using stm (%zu bytes), %f bytes/cycle\n",
           count, BUFSIZE, ITER, total_bytes, bytes_cycle);

    free(buf);
}

#if       (__CORTEX_M >= 0x03)
__NO_INLINE static void arm_bench_multi_issue(void) {
    ulong cycles;
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

    double cycles_iter = (float)cycles / ITER;
    printf("took %lu cycles to issue 8 integer ops (%f cycles/iteration)\n", cycles, cycles_iter);
#undef ITER
}
#endif // __CORTEX_M
#endif // ARCH_ARM

#if WITH_LIB_LIBM
#include <math.h>

__NO_INLINE static void bench_sincos(void) {
    printf("touching the floating point unit\n");
    __UNUSED volatile double _hole = sin(0);
    volatile float input_f = 1234567.0f;
    volatile double input_d = 1234567.0;

    ulong count = arch_cycle_count();
    __UNUSED volatile double d = sin(input_d);
    count = arch_cycle_count() - count;
    printf("took %lu cycles for sin()\n", count);

    count = arch_cycle_count();
    d = cos(input_d);
    count = arch_cycle_count() - count;
    printf("took %lu cycles for cos()\n", count);

    count = arch_cycle_count();
    __UNUSED volatile float f = sinf(input_f);
    count = arch_cycle_count() - count;
    printf("took %lu cycles for sinf()\n", count);

    count = arch_cycle_count();
    f = cosf(input_f);
    count = arch_cycle_count() - count;
    printf("took %lu cycles for cosf()\n", count);

    count = arch_cycle_count();
    d = sqrt(input_d);
    count = arch_cycle_count() - count;
    printf("took %lu cycles for sqrt()\n", count);

    count = arch_cycle_count();
    f = sqrtf(input_f);
    count = arch_cycle_count() - count;
    printf("took %lu cycles for sqrtf()\n", count);
}

#endif // WITH_LIB_LIBM

int benchmarks(int argc, const console_cmd_args *argv) {
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

#if       (__CORTEX_M >= 0x03)
    arm_bench_multi_issue();
#endif
#endif
#if WITH_LIB_LIBM
    bench_sincos();
#endif

    return NO_ERROR;
}

