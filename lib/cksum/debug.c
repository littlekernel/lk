/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#if WITH_LIB_CONSOLE

#include <ctype.h>
#include <debug.h>
#include <stdlib.h>
#include <printf.h>
#include <kernel/thread.h>
#include <platform.h>
#include <lib/cksum.h>

#include <lib/console.h>

static int cmd_crc32(int argc, const cmd_args *argv);
static int cmd_adler32(int argc, const cmd_args *argv);
static int cmd_cksum_bench(int argc, const cmd_args *argv);

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 0
	{ "crc32", "crc32", &cmd_crc32 },
	{ "adler32", "adler32", &cmd_adler32 },
#endif
#if LK_DEBUGLEVEL > 1
	{ "bench_cksum", "benchmark the checksum routines", &cmd_cksum_bench },
#endif
STATIC_COMMAND_END(crc);

static int cmd_crc32(int argc, const cmd_args *argv)
{
	if (argc < 3) {
		printf("not enough arguments\n");
		printf("usage: %s <address> <size>\n", argv[0].str);
		return -1;
	}

	uint32_t crc = crc32(0, (void *)argv[1].u, argv[2].u);

	printf("0x%x\n", crc);

	return 0;
}

static int cmd_adler32(int argc, const cmd_args *argv)
{
	if (argc < 3) {
		printf("not enough arguments\n");
		printf("usage: %s <address> <size>\n", argv[0].str);
		return -1;
	}

	uint32_t crc = adler32(0, (void *)argv[1].u, argv[2].u);

	printf("0x%x\n", crc);

	return 0;
}

static int cmd_cksum_bench(int argc, const cmd_args *argv)
{

#define BUFSIZE 0x1000
#define ITER 16384
	void *buf = malloc(BUFSIZE);
	if (!buf)
		return -1;

	bigtime_t t;
	uint32_t crc;

	t = current_time_hires();
	crc = 0;
	for (int i = 0; i < ITER; i++) {
		crc = crc32(crc, buf, BUFSIZE);
	}
	t = current_time_hires() - t;

	printf("took %llu usecs to crc32 %d bytes (%lld bytes/sec)\n", t, BUFSIZE * ITER, (BUFSIZE * ITER) * 1000000ULL / t);
	thread_sleep(500);

	t = current_time_hires();
	crc = 0;
	for (int i = 0; i < ITER; i++) {
		crc = adler32(crc, buf, BUFSIZE);
	}
	t = current_time_hires() - t;

	printf("took %llu usecs to adler32 %d bytes (%lld bytes/sec)\n", t, BUFSIZE * ITER, (BUFSIZE * ITER) * 1000000ULL / t);

	free(buf);
	return 0;
}

#endif // WITH_LIB_CONSOLE

