/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#if ARM_WITH_CACHE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arch.h>
#include <arch/ops.h>
#include <lib/console.h>
#include <platform.h>

static void bench_cache(size_t bufsize)
{
    lk_bigtime_t t;

    uint8_t *buf = memalign(PAGE_SIZE, bufsize);
    printf("buf %p, size %zu\n", buf, bufsize);

    if (!buf)
        return;

    t = current_time_hires();
    arch_clean_cache_range((addr_t)buf, bufsize);
    t = current_time_hires() - t;

    printf("took %llu usecs to clean %d bytes (cold)\n", t, bufsize);

    memset(buf, 0x99, bufsize);

    t = current_time_hires();
    arch_clean_cache_range((addr_t)buf, bufsize);
    t = current_time_hires() - t;

    printf("took %llu usecs to clean %d bytes (hot)\n", t, bufsize);

    free(buf);
}


static int cache_tests(int argc, const cmd_args *argv)
{
    printf("testing cache\n");

    bench_cache(2*1024);
    bench_cache(64*1024);
    bench_cache(256*1024);
    bench_cache(1024*1024);
    bench_cache(16*1024*1024);

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("cache_tests", "test/bench the cpu cache", &cache_tests)
STATIC_COMMAND_END(cache_tests);

#endif
