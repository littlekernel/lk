/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/ops.h>
#include <inttypes.h>
#include <lib/version.h>
#include <limits.h>
#include <lk/compiler.h>
#include <lk/console_cmd.h>
#include <malloc.h>
#include <platform/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generic_string.h"

// Benchmarks the string routines libc links against the portable C in
// generic_string.c, which stays in the image whatever libc uses. Correctness is
// covered separately by the libc_string_tests unit tests.
//
// Each timing calls through a pointer loaded with a volatile read, so the compiler
// cannot inline or specialize the routine for the length or fill value, even
// under LTO.

typedef void (*fn_t)(void);
typedef void *(*copy_fn)(void *, const void *, size_t);
typedef void *(*set_fn)(void *, int, size_t);
typedef int (*cmp_fn)(const void *, const void *, size_t);
typedef void *(*chr_fn)(const void *, int, size_t);
typedef size_t (*strlen_fn)(const char *);
typedef size_t (*strnlen_fn)(const char *, size_t);
typedef int (*strcmp_fn)(const char *, const char *);
typedef int (*strncmp_fn)(const char *, const char *, size_t);
typedef char *(*strchr_fn)(const char *, int);
typedef char *(*strcpy_fn)(char *, const char *);

// What each routine is timed doing. The searches and compares always run the full
// length, so the per byte cost is comparable across sizes.
enum kind {
    OP_COPY,
    OP_MOVE_FWD, // dst below an overlapping src
    OP_MOVE_BWD, // dst above an overlapping src
    OP_SET,
    OP_CMP, // equal buffers
    OP_CHR, // byte not present
    OP_STRLEN,
    OP_STRNLEN, // limit past the terminator
    OP_STRCMP,  // equal strings
    OP_STRNCMP, // equal strings, limit past the terminators
    OP_STRCHR,  // char not present
    OP_STRRCHR, // char not present
    OP_STRCPY,
};

static bool two_pointer(enum kind kind) {
    switch (kind) {
        case OP_COPY:
        case OP_MOVE_FWD:
        case OP_MOVE_BWD:
        case OP_CMP:
        case OP_STRCMP:
        case OP_STRNCMP:
        case OP_STRCPY:
            return true;
        default:
            return false;
    }
}

struct impl {
    const char *name;
    fn_t fn;
};

#define MAX_IMPLS           4
#define IMPL(name, f)       { name, (fn_t)(f) }
#define GENERIC_AND_LIBC(f) { IMPL("generic", generic_##f), IMPL("libc", f) }

// To compare another implementation, add it to the routine's list. Speedups are
// relative to the first entry.
static const struct op {
    const char *name;
    enum kind kind;
    struct impl impls[MAX_IMPLS];
} ops[] = {
    { "memcpy", OP_COPY, GENERIC_AND_LIBC(memcpy) },
    { "memmove_fwd", OP_MOVE_FWD, GENERIC_AND_LIBC(memmove) },
    { "memmove_bwd", OP_MOVE_BWD, GENERIC_AND_LIBC(memmove) },
    { "memset", OP_SET, GENERIC_AND_LIBC(memset) },
    { "memcmp", OP_CMP, GENERIC_AND_LIBC(memcmp) },
    { "memchr", OP_CHR, GENERIC_AND_LIBC(memchr) },
    { "strlen", OP_STRLEN, GENERIC_AND_LIBC(strlen) },
    { "strnlen", OP_STRNLEN, GENERIC_AND_LIBC(strnlen) },
    { "strcmp", OP_STRCMP, GENERIC_AND_LIBC(strcmp) },
    { "strncmp", OP_STRNCMP, GENERIC_AND_LIBC(strncmp) },
    { "strchr", OP_STRCHR, GENERIC_AND_LIBC(strchr) },
    { "strrchr", OP_STRRCHR, GENERIC_AND_LIBC(strrchr) },
    { "strcpy", OP_STRCPY, GENERIC_AND_LIBC(strcpy) },
};

// Small sizes are where most kernel calls land and where implementations differ
// most; the large ones run out of each cache level in turn and then DRAM. Sizes
// past what could be allocated are skipped.
static const size_t sizes[] = {
    1, 4, 7, 16, 31, 64, 128, 256, 512, 1024, 4096, 16384, 65536, 262144, 1048576, 8388608,
};

// byte offsets from a 64 byte aligned base: both aligned, both equally misaligned,
// and mutually misaligned, where a word at a time routine has to shift or fall back
struct align {
    uint8_t src, dst;
};
static const struct align two_ptr_aligns[] = { { 0, 0 }, { 3, 3 }, { 1, 6 } };
static const struct align one_ptr_aligns[] = { { 0, 0 }, { 0, 3 } };

#define MAX_ALIGN 16 // range of the sweep
#define MOVE_DIST 64 // src to dst distance for the overlapping memmoves
#define SLACK     (MOVE_DIST + 2 * MAX_ALIGN + 64)
// Extra distance between the two buffers. Separate allocations can land a large
// power of two apart, which puts src and dst in the same cache sets and TLB slots
// (and on x86 trips 4K store aliasing), so carve both from one allocation, an odd
// number of pages and a few cache lines apart.
#define SKEW (3 * 4096 + 5 * 64)

#define TARGET_US 4000 // length of one timed trial, at least
#define TRIALS    3    // the fastest is reported

static lk_bigtime_t trial_us = TARGET_US; // stretched for a coarse timer

struct bufs {
    uint8_t *base;
    uint8_t *a;
    uint8_t *b;
    size_t max_size;
};

struct args {
    void *p1; // destination, or the first operand
    void *p2; // source, or the second operand
    size_t n;
};

struct result {
    lk_bigtime_t us;
    ulong cycles;
    uint iters;
};

static void run(enum kind kind, fn_t fn, const struct args *a, uint iters) {
    switch (kind) {
        case OP_COPY:
        case OP_MOVE_FWD:
        case OP_MOVE_BWD:
            for (uint i = 0; i < iters; i++) {
                ((copy_fn)fn)(a->p1, a->p2, a->n);
            }
            break;
        case OP_SET:
            for (uint i = 0; i < iters; i++) {
                ((set_fn)fn)(a->p1, 0, a->n);
            }
            break;
        case OP_CMP:
            for (uint i = 0; i < iters; i++) {
                ((cmp_fn)fn)(a->p1, a->p2, a->n);
            }
            break;
        case OP_CHR:
            for (uint i = 0; i < iters; i++) {
                ((chr_fn)fn)(a->p1, 'z', a->n);
            }
            break;
        case OP_STRLEN:
            for (uint i = 0; i < iters; i++) {
                ((strlen_fn)fn)(a->p1);
            }
            break;
        case OP_STRNLEN:
            for (uint i = 0; i < iters; i++) {
                ((strnlen_fn)fn)(a->p1, a->n + 1);
            }
            break;
        case OP_STRCMP:
            for (uint i = 0; i < iters; i++) {
                ((strcmp_fn)fn)(a->p1, a->p2);
            }
            break;
        case OP_STRNCMP:
            for (uint i = 0; i < iters; i++) {
                ((strncmp_fn)fn)(a->p1, a->p2, a->n + 1);
            }
            break;
        case OP_STRCHR:
        case OP_STRRCHR:
            for (uint i = 0; i < iters; i++) {
                ((strchr_fn)fn)(a->p1, 'z');
            }
            break;
        case OP_STRCPY:
            for (uint i = 0; i < iters; i++) {
                ((strcpy_fn)fn)(a->p1, a->p2);
            }
            break;
    }
}

static void setup(enum kind kind, struct bufs *bufs, struct align al, size_t n, struct args *a) {
    uint8_t *src = bufs->a + al.src;
    uint8_t *dst = bufs->b + al.dst;
    a->n = n;

    switch (kind) {
        case OP_COPY:
            a->p1 = dst;
            a->p2 = src;
            break;
        case OP_MOVE_FWD:
            a->p1 = bufs->a + al.dst;
            a->p2 = bufs->a + MOVE_DIST + al.src;
            break;
        case OP_MOVE_BWD:
            a->p1 = bufs->a + MOVE_DIST + al.dst;
            a->p2 = bufs->a + al.src;
            break;
        case OP_SET:
        case OP_CHR:
        case OP_STRLEN:
        case OP_STRNLEN:
        case OP_STRCHR:
        case OP_STRRCHR:
            // one pointer routines use the dst offset
            memset(dst, 'a', n);
            dst[n] = 0;
            a->p1 = dst;
            a->p2 = NULL;
            break;
        case OP_CMP:
        case OP_STRCMP:
        case OP_STRNCMP:
            memset(src, 'a', n);
            memset(dst, 'a', n);
            src[n] = dst[n] = 0;
            a->p1 = src;
            a->p2 = dst;
            break;
        case OP_STRCPY:
            memset(src, 'a', n);
            src[n] = 0;
            a->p1 = dst;
            a->p2 = src;
            break;
    }
}

static lk_bigtime_t time_run(enum kind kind, fn_t fn, const struct args *a, uint iters,
                             ulong *cycles) {
    ulong c0 = arch_cycle_count();
    lk_bigtime_t t0 = current_time_hires();
    run(kind, fn, a, iters);
    lk_bigtime_t t = current_time_hires() - t0;
    *cycles = arch_cycle_count() - c0;
    return t;
}

// Calibrates a count for each implementation, then runs the trials round robin so
// a clock that drifts (DVFS on the host or the board) affects every column alike.
static void measure(enum kind kind, const struct impl *impls, int n, const struct args *a,
                    struct result *r) {
    fn_t fns[MAX_IMPLS];
    int trials[MAX_IMPLS];
    ulong cycles;

    for (int i = 0; i < n; i++) {
        fns[i] = *(const volatile fn_t *)&impls[i].fn;

        // grow the count to about a quarter of a trial, then scale it to a whole one
        uint iters = 1;
        lk_bigtime_t t;
        while ((t = time_run(kind, fns[i], a, iters, &cycles)) < trial_us / 4 &&
               iters < UINT_MAX / 8) {
            iters *= 2;
        }
        const uint scaled = MAX(iters, MIN((uint64_t)iters * trial_us / MAX(t, 1u), UINT_MAX / 2));

        // a single call that already fills a trial counts as the first one
        trials[i] = 0;
        r[i].us = UINT64_MAX;
        r[i].cycles = 0;
        if (scaled == iters && t >= trial_us) {
            r[i].us = t;
            r[i].cycles = cycles;
            trials[i]++;
        }
        r[i].iters = scaled;
    }

    for (int round = 0; round < TRIALS; round++) {
        for (int i = 0; i < n; i++) {
            if (trials[i] >= TRIALS) {
                continue;
            }
            trials[i]++;
            lk_bigtime_t t = time_run(kind, fns[i], a, r[i].iters, &cycles);
            if (t < r[i].us) {
                r[i].us = t;
                r[i].cycles = cycles;
            }
        }
    }
}

// Lets the console drain so its interrupts stay out of the next timing. Spins rather
// than sleeps: an idle cpu lets a frequency governor clock it down.
static void drain_console(void) {
    spin(10000);
}

static int num_impls(const struct op *op) {
    int n = 0;
    while (n < MAX_IMPLS && op->impls[n].fn) {
        n++;
    }
    return n;
}

static const char *size_str(char *buf, size_t len, size_t size) {
    if (size >= 1024 * 1024 && size % (1024 * 1024) == 0) {
        snprintf(buf, len, "%zuM", size / (1024 * 1024));
    } else if (size >= 1024 && size % 1024 == 0) {
        snprintf(buf, len, "%zuK", size / 1024);
    } else {
        snprintf(buf, len, "%zu", size);
    }
    return buf;
}

// tenths of a unit, scaled up through the units until it fits in 4 digits
static const char *scaled_str(char *buf, size_t len, uint64_t tenths, const char *const *units,
                              int num_units) {
    int u = 0;
    while (tenths >= 100000 && u + 1 < num_units) {
        tenths /= 1000;
        u++;
    }
    snprintf(buf, len, "%" PRIu64 ".%" PRIu64 "%s", tenths / 10, tenths % 10, units[u]);
    return buf;
}

static const char *const time_units[] = { "ns", "us", "ms", "s" };
static const char *const cycle_units[] = { "cy", "Kcy", "Mcy", "Gcy" };

static bool have_cycles;

static void print_header(lk_bigtime_t res) {
    printf("string routine benchmark: %s, %s, compiler %s, %s\n", lk_version.arch,
           lk_version.project, __VERSION__, GLOBAL_OPTFLAGS);
    printf("time%s and MB/s per call, fastest of %d trials of %" PRIu64 " ms each; speedup is "
           "relative to the first column\n",
           have_cycles ? ", cycles" : "", TRIALS, (uint64_t)trial_us / 1000);
    printf("timer resolution %" PRIu64 " us\n", (uint64_t)res);
}

// smallest step current_time_hires() takes
static lk_bigtime_t timer_resolution(void) {
    lk_bigtime_t res = UINT64_MAX;
    for (int i = 0; i < 4; i++) {
        lk_bigtime_t t0 = current_time_hires(), t1;
        while ((t1 = current_time_hires()) == t0) {
        }
        res = MIN(res, t1 - t0);
    }
    return res;
}

static void print_op_header(const struct op *op) {
    printf("\n%-12s %5s", op->name, two_pointer(op->kind) ? "s/d" : "align");
    // as wide as the ns, cycles, MB/s and speedup fields bench_one prints
    for (int i = 0; i < num_impls(op); i++) {
        printf(" %*s%s", have_cycles ? 31 : 21, op->impls[i].name, i > 0 ? "        " : "");
    }
    printf("\n");
}

static void bench_one(const struct op *op, struct bufs *bufs, struct align al, size_t size) {
    struct args args;
    struct result r[MAX_IMPLS];
    const int n = num_impls(op);

    setup(op->kind, bufs, al, size, &args);
    measure(op->kind, op->impls, n, &args, r);

    char sbuf[16];
    printf("%12s ", size_str(sbuf, sizeof(sbuf), size));
    if (two_pointer(op->kind)) {
        printf("%2u/%-2u", al.src, al.dst);
    } else {
        printf("%5u", al.dst);
    }
    for (int i = 0; i < n; i++) {
        uint64_t us = MAX(r[i].us, 1u);
        char buf[16];
        printf(" %8s", scaled_str(buf, sizeof(buf), us * 10000 / r[i].iters, time_units,
                                  countof(time_units)));
        if (have_cycles) {
            printf(" %9s", scaled_str(buf, sizeof(buf), (uint64_t)r[i].cycles * 10 / r[i].iters,
                                      cycle_units, countof(cycle_units)));
        }
        printf(" %7" PRIu64 "MB/s", (uint64_t)size * r[i].iters / us);
        if (i > 0) {
            // per call time of the first column over this one
            uint64_t x100 = (uint64_t)r[0].us * r[i].iters * 100 / (us * r[0].iters);
            printf(" %3" PRIu64 ".%02" PRIu64 "x", x100 / 100, x100 % 100);
        }
    }
    printf("\n");

    drain_console();
}

static bool alloc_bufs(struct bufs *bufs) {
    const size_t largest = sizes[countof(sizes) - 1];
    for (size_t size = largest; size >= 4096; size /= 2) {
        const size_t len = 2 * (size + SLACK) + SKEW;
        bufs->base = memalign(64, len);
        if (bufs->base) {
            bufs->a = bufs->base;
            bufs->b = bufs->base + size + SLACK + SKEW;
            bufs->max_size = size;
            // touch everything once up front
            memset(bufs->base, 0, len);
            return true;
        }
    }
    return false;
}

static void free_bufs(struct bufs *bufs) {
    free(bufs->base);
}

static bool op_matches(const struct op *op, const char *name) {
    return !name || !strcmp(name, "all") || !strncmp(op->name, name, strlen(name));
}

static void bench(struct bufs *bufs, const char *name, size_t only_size) {
    for (size_t o = 0; o < countof(ops); o++) {
        const struct op *op = &ops[o];
        if (!op_matches(op, name)) {
            continue;
        }

        const bool two = two_pointer(op->kind);
        const struct align *aligns = two ? two_ptr_aligns : one_ptr_aligns;
        const size_t num_aligns = two ? countof(two_ptr_aligns) : countof(one_ptr_aligns);

        print_op_header(op);
        for (size_t a = 0; a < num_aligns; a++) {
            for (size_t s = 0; s < countof(sizes); s++) {
                size_t size = only_size ? only_size : sizes[s];
                if (size > bufs->max_size) {
                    break;
                }
                bench_one(op, bufs, aligns[a], size);
                if (only_size) {
                    break;
                }
            }
        }
    }
}

// every alignment combination at one size, as a grid of MB/s per implementation
static void sweep(struct bufs *bufs, const char *name, size_t size) {
    for (size_t o = 0; o < countof(ops); o++) {
        const struct op *op = &ops[o];
        if (!op_matches(op, name)) {
            continue;
        }

        const bool two = two_pointer(op->kind);
        const int n = num_impls(op);
        for (int i = 0; i < n; i++) {
            printf("\n%s %s, %zu bytes, MB/s, %s\n", op->name, op->impls[i].name, size,
                   two ? "rows src align, columns dst align" : "columns align");
            printf("    ");
            for (uint d = 0; d < MAX_ALIGN; d++) {
                printf(" %6u", d);
            }
            printf("\n");

            for (uint s = 0; s < (two ? MAX_ALIGN : 1u); s++) {
                printf("%4u", s);
                for (uint d = 0; d < MAX_ALIGN; d++) {
                    struct args args;
                    struct result r;
                    setup(op->kind, bufs, (struct align){ (uint8_t)s, (uint8_t)d }, size, &args);
                    measure(op->kind, &op->impls[i], 1, &args, &r);
                    printf(" %6" PRIu64, (uint64_t)size * r.iters / MAX(r.us, 1u));
                }
                printf("\n");
                drain_console();
            }
        }
    }
}

static void usage(const char *cmd) {
    printf("usage:\n");
    printf("%s bench [<routine>|all] [<size>]  time the routines, at <size> or a range\n", cmd);
    printf("%s sweep <routine> <size>          time every alignment at one size\n", cmd);
    printf("%s list                            list the routines and implementations\n", cmd);
    printf("<routine> matches by prefix, so memmove runs both directions\n");
    printf("correctness is checked by 'ut libc_string_tests'\n");
}

static int string_bench(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
        usage(argv[0].str);
        return -1;
    }

    if (!strcmp(argv[1].str, "list")) {
        for (size_t o = 0; o < countof(ops); o++) {
            printf("%-12s", ops[o].name);
            for (int i = 0; i < num_impls(&ops[o]); i++) {
                printf(" %s", ops[o].impls[i].name);
            }
            printf("\n");
        }
        return 0;
    }

    const bool is_sweep = !strcmp(argv[1].str, "sweep");
    if (!is_sweep && strcmp(argv[1].str, "bench")) {
        usage(argv[0].str);
        return -1;
    }
    if (is_sweep && argc < 4) {
        usage(argv[0].str);
        return -1;
    }

    struct bufs bufs;
    if (!alloc_bufs(&bufs)) {
        printf("failed to allocate buffers\n");
        return -1;
    }

    const char *name = argc >= 3 ? argv[2].str : NULL;
    size_t size = argc >= 4 ? argv[3].u : 0;
    bool any = false;
    for (size_t o = 0; o < countof(ops); o++) {
        any |= op_matches(&ops[o], name);
    }
    if (!any) {
        printf("no routine matches '%s'\n", name);
        free_bufs(&bufs);
        return -1;
    }
    if (size > bufs.max_size) {
        printf("size %zu is larger than the %zu byte buffers\n", size, bufs.max_size);
        free_bufs(&bufs);
        return -1;
    }

    // arm64 reads 0 when the PMU cycle counter is not available
    have_cycles = arch_cycle_count() != arch_cycle_count();
    // keep a coarse clock's quantization under 1% of each trial
    const lk_bigtime_t res = timer_resolution();
    trial_us = MAX(TARGET_US, 100 * res);
    print_header(res);
    // bring the clock up before the first timing
    spin(250000);
    printf("buffers %zu bytes\n", bufs.max_size);
    if (is_sweep) {
        sweep(&bufs, name, size);
    } else {
        bench(&bufs, name, size);
    }

    free_bufs(&bufs);
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("string", "string routine benchmarks", &string_bench)
STATIC_COMMAND_END(stringtests);
