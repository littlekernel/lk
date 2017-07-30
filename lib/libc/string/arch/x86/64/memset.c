/*
 * Copyright (c) 2017 Intel Corporation
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
#include <string.h>
#include <sys/types.h>

static inline void inner_memset(char **dest, uint8_t val, size_t count)
{
    __asm__ __volatile__ (
            "cld        \n\t"
            "rep stosb  \n\t"
            : "=D" (*dest)
            : "D" (*dest), "a" (val), "c" (count)
            );
    return;
}

static inline void inner_memset64(char **dest, uint8_t val, size_t count)
{
    uint64_t val64 = (uint64_t)val;
    val64 |= val64 << 8;
    val64 |= val64 << 16;
    val64 |= val64 << 32;

    __asm__ __volatile__ (
            "cld        \n\t"
            "rep stosq  \n\t"
            : "=D" (*dest)
            : "D" (*dest), "a" (val64), "c" (count >> 3)
            );
    return;
}

void * memset(void *s, int c, size_t count)
{
    char *xs = (char *) s;
    size_t len = (-(size_t)s) & (sizeof(size_t)-1);
    uint8_t cc = (uint8_t)c;

    if ( count > len ) {

        //non-aligned memory
        if(len) {
            inner_memset(&xs, cc, len);
            count -= len;
        }

        //aligned memory
        if (count >> 3) {
            inner_memset64(&xs, cc, count);
            count &= sizeof(size_t)-1;
        }

    }

    //remaining memory
    inner_memset(&xs, cc, count);

    return s;
}
