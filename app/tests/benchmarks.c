/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <app/tests.h>
#include <inttypes.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/thread.h>
#include <lk/err.h>
#include <platform.h>
#include <rand.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// quickly guess how big of a buffer we can try to allocate
#if !defined(MEMSIZE) || MEMSIZE > (1024 * 1024)
static const size_t BUFSIZE = (size_t)1024 * 1024;
static const uint ITER = 1024;
#else
static const size_t BUFSIZE = (4 * 1024);
static const uint ITER = 1024 * 32;
#endif
// Have to use a define to work around gcc 7.x bug where it thinks
// BUFSIZE is not constant.
#define TOTAL_SIZE ((uint64_t)BUFSIZE * ITER)

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
    if (count == 0) {
        count = 1;
    }

    uint64_t bytes_cycle = (TOTAL_SIZE * 1000) / count;
    printf("took %lu cycles to memset a buffer of size %zu %u times"
           "(%" PRIu64 " bytes), %" PRIu64 ".%03" PRIu64 " bytes/cycle\n",
           count, BUFSIZE, ITER, TOTAL_SIZE, bytes_cycle / 1000, bytes_cycle % 1000);

    free(buf);
}

#define bench_cset(type)                                                                                \
    __NO_INLINE static void bench_cset_##type(void) {                                                   \
        type *buf = malloc(BUFSIZE);                                                                    \
        if (!buf) {                                                                                     \
            printf("failed to allocate buffer\n");                                                      \
            return;                                                                                     \
        }                                                                                               \
                                                                                                        \
        ulong count = arch_cycle_count();                                                               \
        for (uint i = 0; i < ITER; i++) {                                                               \
            for (uint j = 0; j < BUFSIZE / sizeof(*buf); j++) {                                         \
                buf[j] = 0;                                                                             \
            }                                                                                           \
        }                                                                                               \
        count = arch_cycle_count() - count;                                                             \
        if (count == 0) {                                                                               \
            count = 1;                                                                                  \
        }                                                                                               \
                                                                                                        \
        uint64_t bytes_cycle = (TOTAL_SIZE * 1000) / count;                                             \
        printf("took %lu cycles to manually clear a buffer using wordsize %zu of size %zu %u times "    \
               "(%" PRIu64 " bytes), %" PRIu64 ".%03" PRIu64 " bytes/cycle\n",                          \
               count, sizeof(*buf), BUFSIZE, ITER, TOTAL_SIZE, bytes_cycle / 1000, bytes_cycle % 1000); \
        free(buf);                                                                                      \
    }

// clang-format off
bench_cset(uint8_t)
bench_cset(uint16_t)
bench_cset(uint32_t)
bench_cset(uint64_t)
// clang-format on

__NO_INLINE static void bench_cset_wide(void) {
    uint32_t *buf = malloc(BUFSIZE);
    if (!buf) {
        printf("failed to allocate buffer\n");
        return;
    }

    ulong count = arch_cycle_count();
    for (uint i = 0; i < ITER; i++) {
        for (size_t j = 0; j < BUFSIZE / sizeof(*buf) / 8; j++) {
            buf[j * 8] = 0;
            buf[j * 8 + 1] = 0;
            buf[j * 8 + 2] = 0;
            buf[j * 8 + 3] = 0;
            buf[j * 8 + 4] = 0;
            buf[j * 8 + 5] = 0;
            buf[j * 8 + 6] = 0;
            buf[j * 8 + 7] = 0;
        }
    }
    count = arch_cycle_count() - count;
    if (count == 0) {
        count = 1;
    }

    uint64_t bytes_cycle = (TOTAL_SIZE * 1000) / count;
    printf("took %lu cycles to manually clear a buffer of size %zu %d times 8 words at a time "
           "(%" PRIu64 " bytes), %" PRIu64 ".%03" PRIu64 " bytes/cycle\n",
           count, BUFSIZE, ITER, TOTAL_SIZE, bytes_cycle / 1000, bytes_cycle % 1000);

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
    if (count == 0) {
        count = 1;
    }

    uint64_t total_bytes = TOTAL_SIZE / 2;
    uint64_t bytes_cycle = (total_bytes * 1000) / count;
    printf("took %lu cycles to memcpy a buffer of size %zu %d times (%" PRIu64 " source bytes), "
           "%" PRIu64 ".%03" PRIu64 " source bytes/cycle\n",
           count, BUFSIZE / 2, ITER, total_bytes, bytes_cycle / 1000, bytes_cycle % 1000);

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
                "stm    %0, {r0-r7};" ::"r"(&buf[j * 8]));
        }
    }
    count = arch_cycle_count() - count;
    if (count == 0) {
        count = 1;
    }

    uint64_t bytes_cycle = (TOTAL_SIZE * 1000) / count;
    printf("took %lu cycles to manually clear a buffer of size %zu %d times 8 words at a time using stm "
           "(%" PRIu64 " bytes), %" PRIu64 ".%03" PRIu64 " bytes/cycle\n",
           count, BUFSIZE, ITER, TOTAL_SIZE, bytes_cycle / 1000, bytes_cycle % 1000);

    free(buf);
}

#if (__CORTEX_M >= 0x03)
__NO_INLINE static void arm_bench_multi_issue(void) {
    ulong cycles;
    uint32_t a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0;
#define ITER 1000000
    uint count = ITER;
    cycles = arch_cycle_count();
    while (count--) {
        asm volatile("");
        asm volatile("add %0, %0, %0" : "=r"(a) : "r"(a));
        asm volatile("add %0, %0, %0" : "=r"(b) : "r"(b));
        asm volatile("and %0, %0, %0" : "=r"(c) : "r"(c));
        asm volatile("mov %0, %0" : "=r"(d) : "r"(d));
        asm volatile("orr %0, %0, %0" : "=r"(e) : "r"(e));
        asm volatile("add %0, %0, %0" : "=r"(f) : "r"(f));
        asm volatile("and %0, %0, %0" : "=r"(g) : "r"(g));
        asm volatile("mov %0, %0" : "=r"(h) : "r"(h));
    }
    cycles = arch_cycle_count() - cycles;

    ulong cycles_per_iter = (cycles * 1000) / ITER;
    printf("took %lu cycles to issue 8 integer ops (%lu.%03lu cycles/iteration)\n", cycles,
           cycles_per_iter / 1000, cycles_per_iter % 1000);
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

#if (__CORTEX_M >= 0x03)
    arm_bench_multi_issue();
#endif
#endif
#if WITH_LIB_LIBM
    bench_sincos();
#endif

    return NO_ERROR;
}
