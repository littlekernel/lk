/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/unittest.h>
#include <lk/compiler.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if WITH_KERNEL_VM
#include <arch/mmu.h>
#include <kernel/vm.h>
#endif

// Correctness tests for the libc string routines, checked against byte at a time
// expectations. They sweep alignment and length across the thresholds where an
// optimized routine changes strategy, and surround every destination with guard
// bytes. Each routine is called through a volatile pointer so the test exercises
// the linked routine rather than a compiler builtin. Targets with the RAM for it
// also check sizes past the bulk-copy thresholds, and with an MMU, ranges that end
// against an unmapped page.

static void *(*volatile memcpy_fn)(void *, const void *, size_t) = memcpy;
static void *(*volatile memmove_fn)(void *, const void *, size_t) = memmove;
static void *(*volatile memset_fn)(void *, int, size_t) = memset;
static int (*volatile memcmp_fn)(const void *, const void *, size_t) = memcmp;
static void *(*volatile memchr_fn)(const void *, int, size_t) = memchr;
static size_t (*volatile strlen_fn)(const char *) = strlen;
static size_t (*volatile strnlen_fn)(const char *, size_t) = strnlen;
static int (*volatile strcmp_fn)(const char *, const char *) = strcmp;
static int (*volatile strncmp_fn)(const char *, const char *, size_t) = strncmp;
static char *(*volatile strchr_fn)(const char *, int) = strchr;
static char *(*volatile strrchr_fn)(const char *, int) = strrchr;
static char *(*volatile strcpy_fn)(char *, const char *) = strcpy;

// every length up to 32, then either side of likely unroll and block thresholds
static const size_t extra_sizes[] = {
    63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 512, 513, 1031,
};
#define DENSE_SIZES 33
#define NUM_SIZES   (DENSE_SIZES + countof(extra_sizes))
#define MAX_SIZE    1031

static size_t test_size(size_t i) {
    return i < DENSE_SIZES ? i : extra_sizes[i - DENSE_SIZES];
}

#define GUARD     32
#define MAX_ALIGN 16
#define MAX_DELTA 128 // memmove overlap distance
#define BUF_LEN   (MAX_DELTA + GUARD + MAX_ALIGN + MAX_SIZE + MAX_DELTA + GUARD)

// deterministic fill byte for index i, so expectations need no reference copy
static uint8_t pat(uint32_t seed, size_t i) {
    uint32_t x = ((uint32_t)i + 1) * 2654435761u ^ seed;
    x ^= x >> 15;
    x *= 0x2c1b3c6du;
    x ^= x >> 12;
    return (uint8_t)x;
}

static void fill(uint8_t *buf, size_t len, uint32_t seed) {
    for (size_t i = 0; i < len; i++) {
        buf[i] = pat(seed, i);
    }
}

// fill with nonzero bytes, none equal to avoid
static void fill_str(uint8_t *buf, size_t len, uint32_t seed, uint8_t avoid) {
    for (size_t i = 0; i < len; i++) {
        uint8_t c = pat(seed, i);
        if (c == 0) {
            c = 0x80;
        }
        if (c == avoid) {
            c = (avoid == 0x80) ? 0x7f : 0x80;
        }
        buf[i] = c;
    }
}

static int sign(int x) {
    return (x > 0) - (x < 0);
}

// first index in [start, end) where buf differs from pat(seed, i), or end
static size_t check_pat(const uint8_t *buf, size_t start, size_t end, uint32_t seed) {
    for (size_t i = start; i < end; i++) {
        if (buf[i] != pat(seed, i)) {
            return i;
        }
    }
    return end;
}

struct bufs {
    uint8_t *a;
    uint8_t *b;
};

static bool alloc_bufs(struct bufs *bufs) {
    // 64 byte aligned so offset 0 is aligned for any word or vector size
    bufs->a = memalign(64, BUF_LEN);
    bufs->b = memalign(64, BUF_LEN);
    return bufs->a && bufs->b;
}

static void free_bufs(struct bufs *bufs) {
    free(bufs->a);
    free(bufs->b);
}

#define FAIL(fmt, ...)                                                                             \
    do {                                                                                           \
        unittest_printf("\n        %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);          \
        return false;                                                                              \
    } while (0)

static bool check_memcpy(struct bufs *bufs, size_t salign, size_t dalign, size_t size) {
    const uint32_t sseed = 1, dseed = 2;
    uint8_t *src = bufs->a, *dst = bufs->b;
    const size_t end = GUARD + dalign + size + GUARD;
    fill(src, GUARD + salign + size, sseed);
    fill(dst, end, dseed);

    void *ret = memcpy_fn(dst + GUARD + dalign, src + GUARD + salign, size);
    if (ret != dst + GUARD + dalign) {
        FAIL("memcpy salign %zu dalign %zu size %zu: returned %p, expected %p", salign, dalign,
             size, ret, dst + GUARD + dalign);
    }
    size_t i = check_pat(dst, 0, GUARD + dalign, dseed);
    if (i == GUARD + dalign) {
        for (; i < GUARD + dalign + size; i++) {
            if (dst[i] != pat(sseed, i - dalign + salign)) {
                break;
            }
        }
        if (i == GUARD + dalign + size) {
            i = check_pat(dst, i, end, dseed);
        }
    }
    if (i != end) {
        FAIL("memcpy salign %zu dalign %zu size %zu: wrong byte at dst offset %zd", salign, dalign,
             size, (ssize_t)i - (ssize_t)(GUARD + dalign));
    }
    return true;
}

static bool memcpy_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    for (size_t salign = 0; salign < MAX_ALIGN && all_ok; salign++) {
        for (size_t dalign = 0; dalign < MAX_ALIGN && all_ok; dalign++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                all_ok = check_memcpy(&bufs, salign, dalign, test_size(s));
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

static bool check_memmove(uint8_t *buf, size_t align, ssize_t delta, size_t size) {
    const uint32_t seed = 3;
    const size_t src = MAX_DELTA + GUARD + align;
    const size_t dst = src + delta;
    const size_t lo = MIN(src, dst) - GUARD;
    const size_t hi = MAX(src, dst) + size + GUARD;
    for (size_t i = lo; i < hi; i++) {
        buf[i] = pat(seed, i);
    }

    void *ret = memmove_fn(buf + dst, buf + src, size);
    if (ret != buf + dst) {
        FAIL("memmove align %zu delta %zd size %zu: returned %p, expected %p", align, delta, size,
             ret, buf + dst);
    }
    size_t i = check_pat(buf, lo, dst, seed);
    if (i == dst) {
        for (; i < dst + size; i++) {
            if (buf[i] != pat(seed, i - delta)) {
                break;
            }
        }
        if (i == dst + size) {
            i = check_pat(buf, i, hi, seed);
        }
    }
    if (i != hi) {
        FAIL("memmove align %zu delta %zd size %zu: wrong byte at dst offset %zd", align, delta,
             size, (ssize_t)i - (ssize_t)dst);
    }
    return true;
}

static bool memmove_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    // dst - src: every small overlap in both directions, then block sized ones
    static const ssize_t far_deltas[] = { 31, 32, 33, 63, 64, 65, MAX_DELTA };
    const size_t num_deltas = 2 * (17 + countof(far_deltas));

    for (size_t align = 0; align < 8 && all_ok; align++) {
        for (size_t d = 0; d < num_deltas && all_ok; d++) {
            size_t n = d / 2;
            ssize_t delta = n <= 16 ? (ssize_t)n : far_deltas[n - 17];
            if (d & 1) {
                delta = -delta;
            }
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                all_ok = check_memmove(bufs.a, align, delta, test_size(s));
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

static bool check_memset(uint8_t *buf, size_t align, int c, size_t size) {
    const uint32_t seed = 4;
    const size_t start = GUARD + align;
    const size_t end = start + size + GUARD;
    fill(buf, end, seed);

    void *ret = memset_fn(buf + start, c, size);
    if (ret != buf + start) {
        FAIL("memset align %zu c %#x size %zu: returned %p, expected %p", align, c, size, ret,
             buf + start);
    }
    size_t i = check_pat(buf, 0, start, seed);
    if (i == start) {
        for (; i < start + size; i++) {
            if (buf[i] != (uint8_t)c) {
                break;
            }
        }
        if (i == start + size) {
            i = check_pat(buf, i, end, seed);
        }
    }
    if (i != end) {
        FAIL("memset align %zu c %#x size %zu: wrong byte at offset %zd", align, c, size,
             (ssize_t)i - (ssize_t)start);
    }
    return true;
}

static bool memset_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    // only the low byte of c counts
    static const int values[] = { 0, 0x5a, 0xff, 0x1a5, -1 };

    for (size_t align = 0; align < MAX_ALIGN && all_ok; align++) {
        for (size_t v = 0; v < countof(values) && all_ok; v++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                all_ok = check_memset(bufs.a, align, values[v], test_size(s));
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

// Lay down two equal runs of size bytes at the given alignments, then optionally make
// them differ at pos: a > b there, and b > a at the next byte so a compare that does not
// stop at the first difference gets the sign wrong. The bytes past size always differ.
static void setup_cmp(struct bufs *bufs, size_t a1, size_t a2, size_t size, ssize_t pos,
                      bool swap) {
    uint8_t *s1 = bufs->a + GUARD + a1;
    uint8_t *s2 = bufs->b + GUARD + a2;
    for (size_t i = 0; i < size; i++) {
        s1[i] = s2[i] = pat(5, i);
    }
    s1[size] = 0x01;
    s2[size] = 0xfe;
    if (pos >= 0) {
        uint8_t *hi = swap ? s2 : s1;
        uint8_t *lo = swap ? s1 : s2;
        hi[pos] = 0xff;
        lo[pos] = 0x01;
        if ((size_t)pos + 1 < size) {
            hi[pos + 1] = 0x00;
            lo[pos + 1] = 0xff;
        }
    }
}

static bool check_memcmp(struct bufs *bufs, size_t a1, size_t a2, size_t size) {
    const uint8_t *s1 = bufs->a + GUARD + a1;
    const uint8_t *s2 = bufs->b + GUARD + a2;

    setup_cmp(bufs, a1, a2, size, -1, false);
    int r = memcmp_fn(s1, s2, size);
    if (r != 0) {
        FAIL("memcmp a1 %zu a2 %zu size %zu equal: returned %d", a1, a2, size, r);
    }
    if (size == 0) {
        return true;
    }

    const size_t positions[] = { 0, size / 2, size - 1 };
    for (size_t p = 0; p < countof(positions); p++) {
        for (int swap = 0; swap < 2; swap++) {
            setup_cmp(bufs, a1, a2, size, positions[p], swap);
            r = memcmp_fn(s1, s2, size);
            if (sign(r) != (swap ? -1 : 1)) {
                FAIL("memcmp a1 %zu a2 %zu size %zu diff at %zu: returned %d, expected %s", a1, a2,
                     size, positions[p], r, swap ? "< 0" : "> 0");
            }
        }
    }
    return true;
}

static bool memcmp_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    for (size_t a1 = 0; a1 < 8 && all_ok; a1++) {
        for (size_t a2 = 0; a2 < 8 && all_ok; a2++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                all_ok = check_memcmp(&bufs, a1, a2, test_size(s));
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

static bool check_memchr(uint8_t *buf, size_t align, int c, size_t size) {
    const uint8_t target = (uint8_t)c;
    uint8_t *s = buf + GUARD + align;
    fill_str(buf, GUARD + align + size + GUARD, 6, target);

    // just past the end only
    s[size] = target;
    void *r = memchr_fn(s, c, size);
    if (r != NULL) {
        FAIL("memchr align %zu c %#x size %zu absent: returned %p", align, c, size, r);
    }
    if (size == 0) {
        return true;
    }

    // each one ahead of the last, so it is the first occurrence
    const size_t positions[] = { size - 1, size / 2, 0 };
    for (size_t p = 0; p < countof(positions); p++) {
        size_t pos = positions[p];
        s[pos] = target;
        r = memchr_fn(s, c, size);
        if (r != s + pos) {
            FAIL("memchr align %zu c %#x size %zu at %zu: returned %p, expected %p", align, c, size,
                 pos, r, s + pos);
        }
    }
    return true;
}

static bool memchr_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    // the comparison is against (unsigned char)c
    static const int values[] = { 0, 0x01, 0x80, 0xa5, 0x1a5, -1 };

    for (size_t align = 0; align < MAX_ALIGN && all_ok; align++) {
        for (size_t v = 0; v < countof(values) && all_ok; v++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                all_ok = check_memchr(bufs.a, align, values[v], test_size(s));
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

// String contents to sweep. Word at a time zero detection has to cope with bytes
// that have the high bit set or that borrow, so beyond the random pattern try
// runs of the bytes most likely to produce a false match.
static const uint8_t str_fills[] = { 0, 0x01, 0x7f, 0x80, 0x81, 0xff };

// a string of len bytes at buf + GUARD + align, with nonzero bytes either side
static char *setup_str(uint8_t *buf, size_t align, size_t len, uint8_t fill_byte, uint8_t avoid) {
    fill_str(buf, GUARD + align + len + GUARD, 7, avoid);
    uint8_t *s = buf + GUARD + align;
    if (fill_byte && fill_byte != avoid) {
        memset(s, fill_byte, len);
    }
    s[len] = 0;
    return (char *)s;
}

static bool strlen_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    for (size_t align = 0; align < MAX_ALIGN && all_ok; align++) {
        for (size_t f = 0; f < countof(str_fills) && all_ok; f++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                size_t len = test_size(s);
                const char *str = setup_str(bufs.a, align, len, str_fills[f], 0);
                size_t r = strlen_fn(str);
                if (r != len) {
                    unittest_printf("\n        strlen align %zu fill %#x len %zu: returned %zu\n",
                                    align, str_fills[f], len, r);
                    all_ok = false;
                }
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

static bool strnlen_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    for (size_t align = 0; align < MAX_ALIGN && all_ok; align++) {
        for (size_t f = 0; f < countof(str_fills) && all_ok; f++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                size_t len = test_size(s);
                const char *str = setup_str(bufs.a, align, len, str_fills[f], 0);
                // a limit far past the buffer must not be added to the pointer
                const size_t limits[] = { 0, len ? len - 1 : 0, len, len + 1, SIZE_MAX };
                for (size_t l = 0; l < countof(limits) && all_ok; l++) {
                    size_t expected = MIN(len, limits[l]);
                    size_t r = strnlen_fn(str, limits[l]);
                    if (r != expected) {
                        unittest_printf("\n        strnlen align %zu fill %#x len %zu limit %zu: "
                                        "returned %zu\n",
                                        align, str_fills[f], len, limits[l], r);
                        all_ok = false;
                    }
                }
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

// Two equal strings of len bytes, with different garbage past the terminators.
// If pos >= 0 they differ there as in setup_cmp, with the a side greater.
static void setup_strcmp(struct bufs *bufs, size_t a1, size_t a2, size_t len, ssize_t pos,
                         bool swap) {
    setup_cmp(bufs, a1, a2, len, pos, swap);
    uint8_t *s1 = bufs->a + GUARD + a1;
    uint8_t *s2 = bufs->b + GUARD + a2;
    for (size_t i = 0; i < len; i++) {
        // keep the strings free of terminators, but keep the planted difference
        if (s1[i] == 0) {
            s1[i] = 0x80;
        }
        if (s2[i] == 0) {
            s2[i] = 0x80;
        }
    }
    if (pos >= 0 && (size_t)pos + 1 < len) {
        // setup_cmp put a 0x00 after the difference; make it the lowest nonzero byte
        uint8_t *hi = swap ? s2 : s1;
        hi[pos + 1] = 0x01;
    }
    s1[len] = s2[len] = 0;
    s1[len + 1] = 0x01;
    s2[len + 1] = 0xfe;
}

static bool check_strcmp(struct bufs *bufs, size_t a1, size_t a2, size_t len) {
    const char *s1 = (const char *)bufs->a + GUARD + a1;
    const char *s2 = (const char *)bufs->b + GUARD + a2;

    setup_strcmp(bufs, a1, a2, len, -1, false);
    int r = strcmp_fn(s1, s2);
    if (r != 0) {
        FAIL("strcmp a1 %zu a2 %zu len %zu equal: returned %d", a1, a2, len, r);
    }
    r = strncmp_fn(s1, s2, SIZE_MAX);
    if (r != 0) {
        FAIL("strncmp a1 %zu a2 %zu len %zu equal: returned %d", a1, a2, len, r);
    }

    // one string a prefix of the other
    ((uint8_t *)s2)[len] = 0xa5;
    r = strcmp_fn(s1, s2);
    if (sign(r) != -1) {
        FAIL("strcmp a1 %zu a2 %zu len %zu prefix: returned %d, expected < 0", a1, a2, len, r);
    }
    r = strncmp_fn(s1, s2, len + 1);
    if (sign(r) != -1) {
        FAIL("strncmp a1 %zu a2 %zu len %zu prefix: returned %d, expected < 0", a1, a2, len, r);
    }
    r = strncmp_fn(s1, s2, len);
    if (r != 0) {
        FAIL("strncmp a1 %zu a2 %zu len %zu limit at prefix: returned %d", a1, a2, len, r);
    }
    if (len == 0) {
        return true;
    }

    const size_t positions[] = { 0, len / 2, len - 1 };
    for (size_t p = 0; p < countof(positions); p++) {
        size_t pos = positions[p];
        for (int swap = 0; swap < 2; swap++) {
            const int expected = swap ? -1 : 1;
            setup_strcmp(bufs, a1, a2, len, pos, swap);
            r = strcmp_fn(s1, s2);
            if (sign(r) != expected) {
                FAIL("strcmp a1 %zu a2 %zu len %zu diff at %zu: returned %d, expected %s", a1, a2,
                     len, pos, r, swap ? "< 0" : "> 0");
            }
            r = strncmp_fn(s1, s2, pos + 1);
            if (sign(r) != expected) {
                FAIL("strncmp a1 %zu a2 %zu len %zu diff at %zu: returned %d, expected %s", a1, a2,
                     len, pos, r, swap ? "< 0" : "> 0");
            }
            r = strncmp_fn(s1, s2, pos);
            if (r != 0) {
                FAIL("strncmp a1 %zu a2 %zu len %zu limit at diff %zu: returned %d", a1, a2, len,
                     pos, r);
            }
        }
    }
    return true;
}

static bool strcmp_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    for (size_t a1 = 0; a1 < 8 && all_ok; a1++) {
        for (size_t a2 = 0; a2 < 8 && all_ok; a2++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                all_ok = check_strcmp(&bufs, a1, a2, test_size(s));
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

static bool check_strchr(uint8_t *buf, size_t align, int c, size_t len) {
    const uint8_t target = (uint8_t)c;
    char *s = setup_str(buf, align, len, 0, target);

    // past the terminator only
    s[len + 1] = (char)target;
    char *r = strchr_fn(s, c);
    char *rr = strrchr_fn(s, c);
    if (r != NULL || rr != NULL) {
        FAIL("strchr/strrchr align %zu c %#x len %zu absent: returned %p/%p", align, c, len, r, rr);
    }

    // the terminator itself is part of the string
    r = strchr_fn(s, 0);
    rr = strrchr_fn(s, 0);
    if (r != s + len || rr != s + len) {
        FAIL("strchr/strrchr align %zu len %zu c 0: returned %p/%p, expected %p", align, len, r, rr,
             s + len);
    }
    if (len == 0) {
        return true;
    }

    // the first and last of a growing set of occurrences
    const size_t positions[] = { len / 2, len - 1, 0 };
    size_t first = SIZE_MAX, last = 0;
    for (size_t p = 0; p < countof(positions); p++) {
        s[positions[p]] = (char)target;
        first = MIN(first, positions[p]);
        last = MAX(last, positions[p]);
        r = strchr_fn(s, c);
        rr = strrchr_fn(s, c);
        if (r != s + first || rr != s + last) {
            FAIL("strchr/strrchr align %zu c %#x len %zu: returned %p/%p, expected %p/%p", align, c,
                 len, r, rr, s + first, s + last);
        }
    }
    return true;
}

static bool strchr_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    // c converts to char, so 0xa5, 0x1a5 and (signed char)0xa5 all find 0xa5
    static const int values[] = { 0x01, 0x7f, 0x80, 0xa5, 0x1a5, -91, 0xff };

    for (size_t align = 0; align < MAX_ALIGN && all_ok; align++) {
        for (size_t v = 0; v < countof(values) && all_ok; v++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                all_ok = check_strchr(bufs.a, align, values[v], test_size(s));
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

static bool check_strcpy(struct bufs *bufs, size_t salign, size_t dalign, size_t len) {
    const uint32_t dseed = 8;
    char *src = setup_str(bufs->a, salign, len, 0, 0);
    uint8_t *dst = bufs->b;
    const size_t start = GUARD + dalign;
    const size_t end = start + len + 1 + GUARD;
    fill(dst, end, dseed);

    char *ret = strcpy_fn((char *)dst + start, src);
    if (ret != (char *)dst + start) {
        FAIL("strcpy salign %zu dalign %zu len %zu: returned %p, expected %p", salign, dalign, len,
             ret, dst + start);
    }
    size_t i = check_pat(dst, 0, start, dseed);
    if (i == start) {
        for (; i < start + len + 1; i++) {
            if (dst[i] != (uint8_t)src[i - start]) {
                break;
            }
        }
        if (i == start + len + 1) {
            i = check_pat(dst, i, end, dseed);
        }
    }
    if (i != end) {
        FAIL("strcpy salign %zu dalign %zu len %zu: wrong byte at dst offset %zd", salign, dalign,
             len, (ssize_t)i - (ssize_t)start);
    }
    return true;
}

static bool strcpy_test(void) {
    BEGIN_TEST;
    struct bufs bufs;
    ASSERT_TRUE(alloc_bufs(&bufs), "allocating buffers");

    for (size_t salign = 0; salign < 8 && all_ok; salign++) {
        for (size_t dalign = 0; dalign < 8 && all_ok; dalign++) {
            for (size_t s = 0; s < NUM_SIZES && all_ok; s++) {
                all_ok = check_strcpy(&bufs, salign, dalign, test_size(s));
            }
        }
    }

    free_bufs(&bufs);
    END_TEST;
}

#if !LK_EMBEDDED
// Sizes past the point where an optimized routine switches to its bulk loop (block
// copies, prefetching, cache zeroing), checked byte for byte with a cheaper pattern.
static const size_t large_sizes[] = { 65536 + 7, 1048576 + 13 };
#define LARGE_MAX   (1048576 + 13)
#define LARGE_SLACK (2 * 4096 + 64)
#define LARGE_LEN   (LARGE_MAX + 2 * LARGE_SLACK)

static uint8_t lpat(uint32_t seed, size_t i) {
    return (uint8_t)((((uint32_t)i + seed) * 0x9e3779b1u) >> 24);
}

static void lfill(uint8_t *buf, size_t start, size_t end, uint32_t seed) {
    for (size_t i = start; i < end; i++) {
        buf[i] = lpat(seed, i);
    }
}

// first index in [start, end) where buf differs from lpat(seed, i - shift), or end
static size_t lcheck(const uint8_t *buf, size_t start, size_t end, uint32_t seed, ssize_t shift) {
    for (size_t i = start; i < end; i++) {
        if (buf[i] != lpat(seed, i - shift)) {
            return i;
        }
    }
    return end;
}

struct large_bufs {
    uint8_t *a;
    uint8_t *b;
};

static bool large_alloc(struct large_bufs *bufs) {
    bufs->a = memalign(64, LARGE_LEN);
    bufs->b = memalign(64, LARGE_LEN);
    return bufs->a && bufs->b;
}

static void large_free(struct large_bufs *bufs) {
    free(bufs->a);
    free(bufs->b);
}

static bool large_memcpy_test(void) {
    BEGIN_TEST;
    struct large_bufs bufs;
    ASSERT_TRUE(large_alloc(&bufs), "allocating buffers");

    static const struct {
        uint8_t s, d;
    } aligns[] = { { 0, 0 }, { 3, 3 }, { 1, 6 } };
    for (size_t z = 0; z < countof(large_sizes) && all_ok; z++) {
        const size_t size = large_sizes[z];
        for (size_t a = 0; a < countof(aligns) && all_ok; a++) {
            const size_t src = LARGE_SLACK + aligns[a].s;
            const size_t dst = LARGE_SLACK + aligns[a].d;
            const size_t end = dst + size + GUARD;
            lfill(bufs.a, src, src + size, 1);
            lfill(bufs.b, dst - GUARD, end, 2);
            void *ret = memcpy_fn(bufs.b + dst, bufs.a + src, size);
            size_t i = lcheck(bufs.b, dst - GUARD, dst, 2, 0);
            if (i == dst) {
                i = lcheck(bufs.b, dst, dst + size, 1, (ssize_t)dst - (ssize_t)src);
            }
            if (i == dst + size) {
                i = lcheck(bufs.b, dst + size, end, 2, 0);
            }
            if (ret != bufs.b + dst || i != end) {
                unittest_printf("\n        memcpy size %zu salign %u dalign %u: returned %p, "
                                "wrong byte at dst offset %zd\n",
                                size, aligns[a].s, aligns[a].d, ret, (ssize_t)i - (ssize_t)dst);
                all_ok = false;
            }
        }
    }

    large_free(&bufs);
    END_TEST;
}

static bool large_memmove_test(void) {
    BEGIN_TEST;
    struct large_bufs bufs;
    ASSERT_TRUE(large_alloc(&bufs), "allocating buffers");

    // dst - src, small and just past a page, both directions
    static const ssize_t deltas[] = { 1, -1, 64, -64, 4096 + 8, -(4096 + 8) };
    uint8_t *buf = bufs.a;
    for (size_t z = 0; z < countof(large_sizes) && all_ok; z++) {
        const size_t size = large_sizes[z];
        for (size_t d = 0; d < countof(deltas) && all_ok; d++) {
            const ssize_t delta = deltas[d];
            const size_t src = LARGE_SLACK + 3;
            const size_t dst = src + delta;
            const size_t lo = MIN(src, dst) - GUARD;
            const size_t hi = MAX(src, dst) + size + GUARD;
            lfill(buf, lo, hi, 3);
            void *ret = memmove_fn(buf + dst, buf + src, size);
            size_t i = lcheck(buf, lo, dst, 3, 0);
            if (i == dst) {
                i = lcheck(buf, dst, dst + size, 3, delta);
            }
            if (i == dst + size) {
                i = lcheck(buf, dst + size, hi, 3, 0);
            }
            if (ret != buf + dst || i != hi) {
                unittest_printf("\n        memmove size %zu delta %zd: returned %p, wrong byte at "
                                "dst offset %zd\n",
                                size, delta, ret, (ssize_t)i - (ssize_t)dst);
                all_ok = false;
            }
        }
    }

    large_free(&bufs);
    END_TEST;
}

static bool large_memset_test(void) {
    BEGIN_TEST;
    struct large_bufs bufs;
    ASSERT_TRUE(large_alloc(&bufs), "allocating buffers");

    // 0 separately, since some routines zero whole cache lines for it
    static const int values[] = { 0, 0xa5 };
    static const uint8_t aligns[] = { 0, 3, 61 };
    uint8_t *buf = bufs.a;
    for (size_t z = 0; z < countof(large_sizes) && all_ok; z++) {
        const size_t size = large_sizes[z];
        for (size_t v = 0; v < countof(values) && all_ok; v++) {
            for (size_t a = 0; a < countof(aligns) && all_ok; a++) {
                const size_t start = LARGE_SLACK + aligns[a];
                const size_t end = start + size;
                lfill(buf, start - GUARD, end + GUARD, 4);
                void *ret = memset_fn(buf + start, values[v], size);
                size_t i = lcheck(buf, start - GUARD, start, 4, 0);
                if (i == start) {
                    while (i < end && buf[i] == (uint8_t)values[v]) {
                        i++;
                    }
                }
                if (i == end) {
                    i = lcheck(buf, end, end + GUARD, 4, 0);
                }
                if (ret != buf + start || i != end + GUARD) {
                    unittest_printf("\n        memset size %zu c %#x align %u: returned %p, "
                                    "wrong byte at offset %zd\n",
                                    size, values[v], aligns[a], ret, (ssize_t)i - (ssize_t)start);
                    all_ok = false;
                }
            }
        }
    }

    large_free(&bufs);
    END_TEST;
}

static bool large_scan_test(void) {
    BEGIN_TEST;
    struct large_bufs bufs;
    ASSERT_TRUE(large_alloc(&bufs), "allocating buffers");

    for (size_t z = 0; z < countof(large_sizes) && all_ok; z++) {
        const size_t size = large_sizes[z];
        uint8_t *s1 = bufs.a + LARGE_SLACK + 1;
        uint8_t *s2 = bufs.b + LARGE_SLACK + 6;

        // memcmp: equal, then a difference near the end and in the middle, both ways
        for (size_t i = 0; i < size; i++) {
            s1[i] = s2[i] = lpat(5, i) | 0x01;
        }
        int r = memcmp_fn(s1, s2, size);
        EXPECT_EQ(0, r, "memcmp equal");
        const size_t positions[] = { size - 1, size / 2 };
        for (size_t p = 0; p < countof(positions); p++) {
            const size_t pos = positions[p];
            const uint8_t saved = s2[pos];
            s2[pos] = 0x00;
            s1[pos] = 0xff;
            EXPECT_GT(memcmp_fn(s1, s2, size), 0, "memcmp large diff");
            EXPECT_LT(memcmp_fn(s2, s1, size), 0, "memcmp large diff swapped");
            s1[pos] = s2[pos] = saved;
        }

        // the bytes are all nonzero and odd, so 0x02 and the terminator are absent
        s1[size] = 0;
        EXPECT_EQ(size, strlen_fn((const char *)s1), "strlen large");
        EXPECT_TRUE(memchr_fn(s1, 0x02, size) == NULL, "memchr large absent");
        s1[size - 1] = 0x02;
        EXPECT_TRUE(memchr_fn(s1, 0x02, size) == s1 + size - 1, "memchr large at end");
        if (!all_ok) {
            unittest_printf("\n        size %zu\n", size);
        }
    }

    large_free(&bufs);
    END_TEST;
}
#endif // !LK_EMBEDDED

#if WITH_KERNEL_VM
// One mapped page with an unmapped page either side, carved from a reserved range so
// nothing else maps there. A routine that reads or writes past either edge faults,
// which panics: there is no fault recovery in the kernel, so the boot test fails.
struct guard_page {
    vaddr_t base;
    vm_page_t *page;
    uint8_t *p;
};

static bool guard_alloc(struct guard_page *g) {
    vmm_aspace_t *aspace = vmm_get_kernel_aspace();
    if (vmm_reserve_range(aspace, "string_guard", 3 * PAGE_SIZE, PAGE_SIZE_SHIFT, &g->base) < 0) {
        return false;
    }
    g->page = pmm_alloc_page();
    if (!g->page) {
        vmm_free_region(aspace, g->base);
        return false;
    }
    vmm_aspace_lock(aspace);
    status_t err =
        arch_mmu_map(&aspace->arch_aspace, g->base + PAGE_SIZE, vm_page_to_paddr(g->page), 1,
                     ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE);
    vmm_aspace_unlock(aspace);
    if (err < 0) {
        vmm_free_region(aspace, g->base);
        pmm_free_page(g->page);
        return false;
    }
    g->p = (uint8_t *)(g->base + PAGE_SIZE);
    return true;
}

static void guard_free(struct guard_page *g) {
    vmm_free_region(vmm_get_kernel_aspace(), g->base);
    pmm_free_page(g->page);
}

#define GUARD_MAX_LEN 80

// Runs every routine on n bytes at p, which sits against one edge of the guard page.
// The bytes are nonzero and avoid 'z'; for the string routines p[n] is the terminator.
// ref is an ordinary heap buffer holding the same bytes, for the two pointer routines.
static bool guard_check(uint8_t *p, size_t n, uint8_t *ref, bool string) {
    char *s = (char *)p;
    const char *rs = (const char *)ref;

    if (string) {
        if (strlen_fn(s) != n) {
            FAIL("strlen len %zu", n);
        }
        if (strnlen_fn(s, SIZE_MAX) != n) {
            FAIL("strnlen unbounded len %zu", n);
        }
        if (strchr_fn(s, 'z') != NULL) {
            FAIL("strchr absent len %zu", n);
        }
        if (strrchr_fn(s, 'z') != NULL) {
            FAIL("strrchr absent len %zu", n);
        }
        if (strchr_fn(s, 0) != s + n) {
            FAIL("strchr terminator len %zu", n);
        }
        if (strrchr_fn(s, 0) != s + n) {
            FAIL("strrchr terminator len %zu", n);
        }
        if (strcmp_fn(s, rs) != 0 || strcmp_fn(rs, s) != 0) {
            FAIL("strcmp equal len %zu", n);
        }
        if (strncmp_fn(s, rs, SIZE_MAX) != 0) {
            FAIL("strncmp equal len %zu", n);
        }
        if (n > 0) {
            // the heap copy one byte longer, so the guarded string ends first
            ref[n] = 'a';
            ref[n + 1] = 0;
            if (strcmp_fn(s, rs) >= 0 || strcmp_fn(rs, s) <= 0) {
                FAIL("strcmp prefix len %zu", n);
            }
            ref[n] = 0;
        }
        // copying out of the page, then into it
        uint8_t *tmp = ref + GUARD_MAX_LEN + 16;
        if (strcpy_fn((char *)tmp, s) != (char *)tmp || memcmp(tmp, p, n + 1) != 0) {
            FAIL("strcpy out len %zu", n);
        }
        if (strcpy_fn(s, (const char *)tmp) != s || memcmp(tmp, p, n + 1) != 0) {
            FAIL("strcpy in len %zu", n);
        }
    } else {
        // exactly n bytes, no terminator anywhere near: limits must be honored
        if (strnlen_fn(s, n) != n) {
            FAIL("strnlen at limit len %zu", n);
        }
        if (strncmp_fn(s, rs, n) != 0) {
            FAIL("strncmp at limit len %zu", n);
        }
        if (memchr_fn(p, 'z', n) != NULL) {
            FAIL("memchr absent len %zu", n);
        }
        if (n > 0 && memchr_fn(p, p[n - 1], n) == NULL) {
            FAIL("memchr present len %zu", n);
        }
        if (memcmp_fn(p, ref, n) != 0 || memcmp_fn(ref, p, n) != 0) {
            FAIL("memcmp len %zu", n);
        }
        uint8_t *tmp = ref + GUARD_MAX_LEN + 16;
        if (memcpy_fn(tmp, p, n) != tmp || memcmp(tmp, ref, n) != 0) {
            FAIL("memcpy out len %zu", n);
        }
        if (memcpy_fn(p, tmp, n) != p || memcmp(p, ref, n) != 0) {
            FAIL("memcpy in len %zu", n);
        }
        if (memmove_fn(tmp, p, n) != tmp || memmove_fn(p, tmp, n) != p || memcmp(p, ref, n) != 0) {
            FAIL("memmove len %zu", n);
        }
        if (n > 1) {
            // overlapping moves inside the page, both directions
            memmove_fn(p, p + 1, n - 1);
            memmove_fn(p + 1, p, n - 1);
            if (memcmp(p + 1, ref + 1, n - 1) != 0) {
                FAIL("memmove overlap len %zu", n);
            }
        }
        if (memset_fn(p, 'x', n) != p) {
            FAIL("memset len %zu", n);
        }
        for (size_t i = 0; i < n; i++) {
            if (p[i] != 'x') {
                FAIL("memset len %zu at %zu", n, i);
            }
        }
    }
    return true;
}

static void guard_fill(uint8_t *p, uint8_t *ref, size_t n, uint32_t seed) {
    for (size_t i = 0; i < n; i++) {
        uint8_t c = pat(seed, i);
        if (c == 0 || c == 'z') {
            c = 'a';
        }
        p[i] = ref[i] = c;
    }
}

static bool guard_page_test(void) {
    BEGIN_TEST;
    struct guard_page g;
    ASSERT_TRUE(guard_alloc(&g), "setting up the guard page");
    uint8_t *ref = malloc(2 * (GUARD_MAX_LEN + 16) + 16);
    ASSERT_TRUE(ref != NULL, "allocating the reference buffer");
    uint8_t *const page_end = g.p + PAGE_SIZE;

    for (size_t n = 0; n <= GUARD_MAX_LEN && all_ok; n++) {
        for (size_t t = 0; t < 16 && all_ok; t++) {
            for (int edge = 0; edge < 2 && all_ok; edge++) {
                for (int string = 0; string < 2 && all_ok; string++) {
                    // the last byte (or the terminator) t bytes before the end of the
                    // page, or the first byte t bytes after its start
                    uint8_t *p = edge ? page_end - t - n - string : g.p + t;
                    guard_fill(p, ref, n, 9 + edge);
                    if (string) {
                        p[n] = ref[n] = 0;
                    }
                    if (!guard_check(p, n, ref, string)) {
                        unittest_printf("        %s, len %zu, %zu bytes from the %s of the page\n",
                                        string ? "string" : "bytes", n, t, edge ? "end" : "start");
                        all_ok = false;
                    }
                }
            }
        }
    }

    free(ref);
    guard_free(&g);
    END_TEST;
}
#endif // WITH_KERNEL_VM

BEGIN_TEST_CASE(libc_string_tests)
RUN_TEST(memcpy_test)
RUN_TEST(memmove_test)
RUN_TEST(memset_test)
RUN_TEST(memcmp_test)
RUN_TEST(memchr_test)
RUN_TEST(strlen_test)
RUN_TEST(strnlen_test)
RUN_TEST(strcmp_test)
RUN_TEST(strchr_test)
RUN_TEST(strcpy_test)
#if !LK_EMBEDDED
RUN_TEST(large_memcpy_test)
RUN_TEST(large_memmove_test)
RUN_TEST(large_memset_test)
RUN_TEST(large_scan_test)
#endif
#if WITH_KERNEL_VM
RUN_TEST(guard_page_test)
#endif
END_TEST_CASE(libc_string_tests)
