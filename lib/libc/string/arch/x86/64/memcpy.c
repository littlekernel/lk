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

#define LONG_SIZE sizeof(long)
#define LONG_MASK (LONG_SIZE - 1)
#define BYTE_SIZE 1

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

static inline void memcpy_inc(char *dest, const char *src, size_t len)
{
    __asm__ __volatile__ (
            "cld        \n\t"
            "rep movsb  \n\t"
            :: "D" (dest), "S" (src), "c" (len)
            );
}

static inline void memcpy64_inc(char *dest, const char *src, size_t len)
{
    __asm__ __volatile__ (
            "cld        \n\t"
            "rep movsq  \n\t"
            :: "D" (dest), "S" (src), "c" (len >> 3)
            );
}

static inline void memcpy_dec(char *dest, const char *src, size_t len)
{
    __asm__ __volatile__ (
            "std        \n\t"
            "rep movsb  \n\t"
            "cld        \n\t"
            :: "D" (dest + len - BYTE_SIZE),
            "S" (src + len - BYTE_SIZE),
            "c" (len)
            );
}

static inline void memcpy64_dec(char *dest, const char *src, size_t len)
{
    __asm__ __volatile__ (
            "std        \n\t"
            "rep movsq  \n\t"
            "cld        \n\t"
            :: "D" (dest + len - LONG_SIZE),
            "S" (src + len - LONG_SIZE),
            "c" (len >> 3)
            );
}

static inline int para_available(char *d, const char *s, size_t c)
{
    if ((NULL == d) || (NULL == s) || (0 == c) || (d == s))
        return FALSE;

    return TRUE;
}

static inline int para_with_different_offset(char *d, const char *s)
{
    if (((long)d ^ (long)s) & LONG_MASK)
        return TRUE;

    return FALSE;
}

static inline int para_count_too_small(size_t count)
{
    if (count < LONG_SIZE)
        return TRUE;

    return FALSE;
}

/*
 * Two types of copy methods provided by Intel x86 architecutre:
 * increment and decrement copy, DF flag controls copy direction.
 * If source is larger than destination, increment copy selected,
 * otherwise, decrement copy selected (since if increment copy used
 * in this scenario, destination and source memory might overlapped
 * for some region: src < dest < src + count, copy to destination may
 * overwrite original source data, decrement copy sloves this issue).
 *
 * When decrement copy selected, processor copys data first, and then
 * decreases RDI/RSI/RCX. In this case, we need to adjust RDI/RSI before
 * copy to right address.
 *
 * More details about MOVS/MOVSB/MOVSW/MOVSD/MOVSQ please refer to
 * <Intel 64 and IA-32 Architectures Software Developer's Manaual>
 * Volume 2: Instruction Set Reference
 */
void *memcpy(void *dest, const void *src, size_t count)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;
    int len[3] = {0, 0, 0};

    if (!para_available(dest, src, count))
        return dest;

    if (para_with_different_offset(d, s) || para_count_too_small(count)) {
        /*
         * dest and src to be aligned with different offsets,
         * or, count is smaller than LONG_SIZE.
         */
        len[0] = count;
        len[1] = 0;
        len[2] = 0;
    } else {
        /*
         * dest and src to be aligned with same offsets,
         * non-align part need to be copy by bytes
         */
        len[0] = (-(size_t)d) & LONG_MASK;
        len[1] = (count - len[0]) & ~LONG_MASK;
        len[2] = (count - len[0]) & LONG_MASK;
    }


    if (s > d) {
        /* Increment copy */
        if (len[0])
            memcpy_inc(d, s, len[0]);

        if (len[1])
            memcpy64_inc(d + len[0], s + len[0], len[1]);

        if (len[2])
            memcpy_inc(d + len[0] + len[1], s + len[0] + len[1], len[2]);
    } else {
        /* Decrement copy */
        if (len[2])
            memcpy_dec(d + len[0] + len[1], s + len[0] + len[1], len[2]);

        if (len[1])
            memcpy64_dec(d + len[0], s + len[0], len[1]);

        if (len[0])
            memcpy_dec(d, s, len[0]);
    }

    return dest;
}
