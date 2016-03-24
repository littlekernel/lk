/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <app.h>
#include <platform.h>
#include <kernel/thread.h>

static uint8_t *src;
static uint8_t *dst;

static uint8_t *src2;
static uint8_t *dst2;

#define BUFFER_SIZE (2*1024*1024)
#define ITERATIONS (256*1024*1024 / BUFFER_SIZE) // enough iterations to have to copy/set 256MB of memory

#if 1
static inline void *mymemcpy(void *dst, const void *src, size_t len) { return memcpy(dst, src, len); }
static inline void *mymemset(void *dst, int c, size_t len) { return memset(dst, c, len); }
#else
// if we're testing our own memcpy, use this
extern void *mymemcpy(void *dst, const void *src, size_t len);
extern void *mymemset(void *dst, int c, size_t len);
#endif

/* reference implementations of memmove/memcpy */
typedef long word;

#define lsize sizeof(word)
#define lmask (lsize - 1)

static void *c_memmove(void *dest, void const *src, size_t count)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;
    int len;

    if (count == 0 || dest == src)
        return dest;

    if ((long)d < (long)s) {
        if (((long)d | (long)s) & lmask) {
            // src and/or dest do not align on word boundary
            if ((((long)d ^ (long)s) & lmask) || (count < lsize))
                len = count; // copy the rest of the buffer with the byte mover
            else
                len = lsize - ((long)d & lmask); // move the ptrs up to a word boundary

            count -= len;
            for (; len > 0; len--)
                *d++ = *s++;
        }
        for (len = count / lsize; len > 0; len--) {
            *(word *)d = *(word *)s;
            d += lsize;
            s += lsize;
        }
        for (len = count & lmask; len > 0; len--)
            *d++ = *s++;
    } else {
        d += count;
        s += count;
        if (((long)d | (long)s) & lmask) {
            // src and/or dest do not align on word boundary
            if ((((long)d ^ (long)s) & lmask) || (count <= lsize))
                len = count;
            else
                len = ((long)d & lmask);

            count -= len;
            for (; len > 0; len--)
                *--d = *--s;
        }
        for (len = count / lsize; len > 0; len--) {
            d -= lsize;
            s -= lsize;
            *(word *)d = *(word *)s;
        }
        for (len = count & lmask; len > 0; len--)
            *--d = *--s;
    }

    return dest;
}

static void *c_memset(void *s, int c, size_t count)
{
    char *xs = (char *) s;
    size_t len = (-(size_t)s) & lmask;
    word cc = c & 0xff;

    if ( count > len ) {
        count -= len;
        cc |= cc << 8;
        cc |= cc << 16;
        if (sizeof(word) == 8)
            cc |= (uint64_t)cc << 32; // should be optimized out on 32 bit machines

        // write to non-aligned memory byte-wise
        for ( ; len > 0; len-- )
            *xs++ = c;

        // write to aligned memory dword-wise
        for ( len = count / lsize; len > 0; len-- ) {
            *((word *)xs) = (word)cc;
            xs += lsize;
        }

        count &= lmask;
    }

    // write remaining bytes
    for ( ; count > 0; count-- )
        *xs++ = c;

    return s;
}

static void *null_memcpy(void *dst, const void *src, size_t len)
{
    return dst;
}

static lk_time_t bench_memcpy_routine(void *memcpy_routine(void *, const void *, size_t), size_t srcalign, size_t dstalign)
{
    int i;
    lk_time_t t0;

    t0 = current_time();
    for (i=0; i < ITERATIONS; i++) {
        memcpy_routine(dst + dstalign, src + srcalign, BUFFER_SIZE);
    }
    return current_time() - t0;
}

static void bench_memcpy(void)
{
    lk_time_t null, c, libc, mine;
    size_t srcalign, dstalign;

    printf("memcpy speed test\n");
    thread_sleep(200); // let the debug string clear the serial port

    for (srcalign = 0; srcalign < 64; ) {
        for (dstalign = 0; dstalign < 64; ) {

            null = bench_memcpy_routine(&null_memcpy, srcalign, dstalign);
            c = bench_memcpy_routine(&c_memmove, srcalign, dstalign);
            libc = bench_memcpy_routine(&memcpy, srcalign, dstalign);
            mine = bench_memcpy_routine(&mymemcpy, srcalign, dstalign);

            printf("srcalign %zu, dstalign %zu: ", srcalign, dstalign);
            printf("   null memcpy %u msecs\n", null);
            printf("c memcpy %u msecs, %llu bytes/sec; ", c, (uint64_t)BUFFER_SIZE * ITERATIONS * 1000ULL / c);
            printf("libc memcpy %u msecs, %llu bytes/sec; ", libc, (uint64_t)BUFFER_SIZE * ITERATIONS * 1000ULL / libc);
            printf("my memcpy %u msecs, %llu bytes/sec; ", mine, (uint64_t)BUFFER_SIZE * ITERATIONS * 1000ULL / mine);
            printf("\n");

            if (dstalign < 8)
                dstalign++;
            else
                dstalign <<= 1;
        }
        if (srcalign < 8)
            srcalign++;
        else
            srcalign <<= 1;
    }
}

static void fillbuf(void *ptr, size_t len, uint32_t seed)
{
    size_t i;

    for (i = 0; i < len; i++) {
        ((char *)ptr)[i] = seed;
        seed *= 0x1234567;
    }
}

static void validate_memcpy(void)
{
    size_t srcalign, dstalign, size;
    const size_t maxsize = 256;

    printf("testing memcpy for correctness\n");

    /*
     * do the simple tests to make sure that memcpy doesn't color outside
     * the lines for all alignment cases
     */
    for (srcalign = 0; srcalign < 64; srcalign++) {
        printf("srcalign %zu\n", srcalign);
        for (dstalign = 0; dstalign < 64; dstalign++) {
            //printf("\tdstalign %zu\n", dstalign);
            for (size = 0; size < maxsize; size++) {

                //printf("srcalign %zu, dstalign %zu, size %zu\n", srcalign, dstalign, size);

                fillbuf(src, maxsize * 2, 567);
                fillbuf(src2, maxsize * 2, 567);
                fillbuf(dst, maxsize * 2, 123514);
                fillbuf(dst2, maxsize * 2, 123514);

                c_memmove(dst + dstalign, src + srcalign, size);
                memcpy(dst2 + dstalign, src2 + srcalign, size);

                int comp = memcmp(dst, dst2, maxsize * 2);
                if (comp != 0) {
                    printf("error! srcalign %zu, dstalign %zu, size %zu\n", srcalign, dstalign, size);
                }
            }
        }
    }
}

static lk_time_t bench_memset_routine(void *memset_routine(void *, int, size_t), size_t dstalign, size_t len)
{
    int i;
    lk_time_t t0;

    t0 = current_time();
    for (i=0; i < ITERATIONS; i++) {
        memset_routine(dst + dstalign, 0, len);
    }
    return current_time() - t0;
}

static void bench_memset(void)
{
    lk_time_t c, libc, mine;
    size_t dstalign;

    printf("memset speed test\n");
    thread_sleep(200); // let the debug string clear the serial port

    for (dstalign = 0; dstalign < 64; dstalign++) {

        c = bench_memset_routine(&c_memset, dstalign, BUFFER_SIZE);
        libc = bench_memset_routine(&memset, dstalign, BUFFER_SIZE);
        mine = bench_memset_routine(&mymemset, dstalign, BUFFER_SIZE);

        printf("dstalign %zu: ", dstalign);
        printf("c memset %u msecs, %llu bytes/sec; ", c, (uint64_t)BUFFER_SIZE * ITERATIONS * 1000ULL / c);
        printf("libc memset %u msecs, %llu bytes/sec; ", libc, (uint64_t)BUFFER_SIZE * ITERATIONS * 1000ULL / libc);
        printf("my memset %u msecs, %llu bytes/sec; ", mine, (uint64_t)BUFFER_SIZE * ITERATIONS * 1000ULL / mine);
        printf("\n");
    }
}

static void validate_memset(void)
{
    size_t dstalign, size;
    int c;
    const size_t maxsize = 256;

    printf("testing memset for correctness\n");

    for (dstalign = 0; dstalign < 64; dstalign++) {
        printf("align %zd\n", dstalign);
        for (size = 0; size < maxsize; size++) {
            for (c = -1; c < 257; c++) {

                fillbuf(dst, maxsize * 2, 123514);
                fillbuf(dst2, maxsize * 2, 123514);

                c_memset(dst + dstalign, c, size);
                memset(dst2 + dstalign, c, size);

                int comp = memcmp(dst, dst2, maxsize * 2);
                if (comp != 0) {
                    printf("error! align %zu, c 0x%hhx, size %zu\n", dstalign, c, size);
                }
            }
        }
    }
}

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

static int string_tests(int argc, const cmd_args *argv)
{
    src = memalign(64, BUFFER_SIZE + 256);
    dst = memalign(64, BUFFER_SIZE + 256);
    src2 = memalign(64, BUFFER_SIZE + 256);
    dst2 = memalign(64, BUFFER_SIZE + 256);

    printf("src %p, dst %p\n", src, dst);
    printf("src2 %p, dst2 %p\n", src2, dst2);

    if (!src || !dst || !src2 || !dst2) {
        printf("failed to allocate all the buffers\n");
        goto out;
    }

    if (argc < 3) {
        printf("not enough arguments:\n");
usage:
        printf("%s validate <routine>\n", argv[0].str);
        printf("%s bench <routine>\n", argv[0].str);
        goto out;
    }

    if (!strcmp(argv[1].str, "validate")) {
        if (!strcmp(argv[2].str, "memcpy")) {
            validate_memcpy();
        } else if (!strcmp(argv[2].str, "memset")) {
            validate_memset();
        }
    } else if (!strcmp(argv[1].str, "bench")) {
        if (!strcmp(argv[2].str, "memcpy")) {
            bench_memcpy();
        } else if (!strcmp(argv[2].str, "memset")) {
            bench_memset();
        }
    } else {
        goto usage;
    }

out:
    free(src);
    free(dst);
    free(src2);
    free(dst2);

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("string", "memcpy tests", &string_tests)
STATIC_COMMAND_END(stringtests);

#endif

APP_START(stringtests)
APP_END

