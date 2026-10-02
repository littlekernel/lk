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

// Correctness tests for the libc string routines, checked against byte at a time
// expectations. They sweep alignment and length across the thresholds where an
// optimized routine changes strategy, and surround every destination with guard
// bytes. Each routine is called through a volatile pointer so the test exercises
// the linked routine rather than a compiler builtin.

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
END_TEST_CASE(libc_string_tests)
