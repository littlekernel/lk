/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <debug.h>
#include <string.h>
#include <malloc.h>
#include <app.h>
#include <platform.h>
#include <kernel/thread.h>

static uint8_t *src;
static uint8_t *dst;

static uint8_t *src2;
static uint8_t *dst2;

#define BUFFER_SIZE (1024*1024)
#define ITERATIONS 16

extern void *mymemcpy(void *dst, const void *src, size_t len);
extern void *mymemset(void *dst, int c, size_t len);

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
	lk_time_t null, libc, mine;
	size_t srcalign, dstalign;

	printf("memcpy speed test\n");
	thread_sleep(200); // let the debug string clear the serial port

	for (srcalign = 0; srcalign < 64; ) {
		for (dstalign = 0; dstalign < 64; ) {

			null = bench_memcpy_routine(&null_memcpy, srcalign, dstalign);
			libc = bench_memcpy_routine(&memcpy, srcalign, dstalign);
			mine = bench_memcpy_routine(&mymemcpy, srcalign, dstalign);

			printf("srcalign %zu, dstalign %zu\n", srcalign, dstalign);
			printf("   null memcpy %lu msecs\n", null);
			printf("   libc memcpy %lu msecs, %llu bytes/sec\n", libc, BUFFER_SIZE * ITERATIONS * 1000ULL / libc);
			printf("   my   memcpy %lu msecs, %llu bytes/sec\n", mine, BUFFER_SIZE * ITERATIONS * 1000ULL / mine);

			if (dstalign == 0)
				dstalign = 1;
			else
				dstalign <<= 1;
		}
		if (srcalign == 0)
			srcalign = 1;
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
		for (dstalign = 0; dstalign < 64; dstalign++) {
//			printf("srcalign %zu, dstalign %zu\n", srcalign, dstalign);
			for (size = 0; size < maxsize; size++) {

//				printf("srcalign %zu, dstalign %zu, size %zu\n", srcalign, dstalign, size);

				fillbuf(src, maxsize * 2, 567);
				fillbuf(src2, maxsize * 2, 567);
				fillbuf(dst, maxsize * 2, 123514);
				fillbuf(dst2, maxsize * 2, 123514);

				memcpy(dst + dstalign, src + srcalign, size);
				mymemcpy(dst2 + dstalign, src2 + srcalign, size);

				int comp = memcmp(dst, dst2, maxsize * 2);
				if (comp != 0) {
					printf("error! srcalign %zu, dstalign %zu, size %zu\n", srcalign, dstalign, size);
				}
			}
		}
	}
}

static lk_time_t bench_memset_routine(void *memset_routine(void *, int, size_t), size_t dstalign)
{
	int i;
	lk_time_t t0;

	t0 = current_time();
	for (i=0; i < ITERATIONS; i++) {
		memset_routine(dst + dstalign, 0, BUFFER_SIZE);
	}
	return current_time() - t0;
}

static void bench_memset(void)
{
	lk_time_t libc, mine;
	size_t dstalign;

	printf("memset speed test\n");
	thread_sleep(200); // let the debug string clear the serial port

	for (dstalign = 0; dstalign < 64; dstalign++) {

		libc = bench_memset_routine(&memset, dstalign);
		mine = bench_memset_routine(&mymemset, dstalign);

		printf("dstalign %zu\n", dstalign);
		printf("   libc memset %lu msecs, %llu bytes/sec\n", libc, BUFFER_SIZE * ITERATIONS * 1000ULL / libc);
		printf("   my   memset %lu msecs, %llu bytes/sec\n", mine, BUFFER_SIZE * ITERATIONS * 1000ULL / mine);
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
			for (c = 0; c < 256; c++) {

				fillbuf(dst, maxsize * 2, 123514);
				fillbuf(dst2, maxsize * 2, 123514);

				memset(dst + dstalign, c, size);
				mymemset(dst2 + dstalign, c, size);

				int comp = memcmp(dst, dst2, maxsize * 2);
				if (comp != 0) {
					printf("error! align %zu, c %d, size %zu\n", dstalign, c, size);
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
{ "string", NULL, &string_tests },
STATIC_COMMAND_END(stringtests);

#endif

APP_START(stringtests)
APP_END

