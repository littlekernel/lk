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
#ifndef __BITS_H
#define __BITS_H

#include <compiler.h>
#include <arch/ops.h>

__BEGIN_CDECLS;

#define clz(x) __builtin_clz(x)
#define ctz(x) __builtin_ctz(x)

#define BIT(x, bit) ((x) & (1UL << (bit)))
#define BIT_SHIFT(x, bit) (((x) >> (bit)) & 1)
#define BITS(x, high, low) ((x) & (((1UL<<((high)+1))-1) & ~((1UL<<(low))-1)))
#define BITS_SHIFT(x, high, low) (((x) >> (low)) & ((1UL<<((high)-(low)+1))-1))
#define BIT_SET(x, bit) (((x) & (1UL << (bit))) ? 1 : 0)

#define BITMAP_BITS_PER_WORD (sizeof(unsigned long) * 8)
#define BITMAP_NUM_WORDS(x) (((x) + BITMAP_BITS_PER_WORD - 1) / BITMAP_BITS_PER_WORD)
#define BITMAP_WORD(x) ((x) / BITMAP_BITS_PER_WORD)
#define BITMAP_BIT_IN_WORD(x) ((x) & (BITMAP_BITS_PER_WORD - 1))

#define BITMAP_BITS_PER_INT (sizeof(unsigned int) * 8)
#define BITMAP_BIT_IN_INT(x) ((x) & (BITMAP_BITS_PER_INT - 1))
#define BITMAP_INT(x) ((x) / BITMAP_BITS_PER_INT)

#define BIT_MASK(x) (((x) >= sizeof(unsigned long) * 8) ? (0UL-1) : ((1UL << (x)) - 1))

static inline int bitmap_set(unsigned long *bitmap, int bit)
{
    unsigned long mask = 1UL << BITMAP_BIT_IN_INT(bit);
    return atomic_or(&((int *)bitmap)[BITMAP_INT(bit)], mask) & mask ? 1 : 0;
}

static inline int bitmap_clear(unsigned long *bitmap, int bit)
{
    unsigned long mask = 1UL << BITMAP_BIT_IN_INT(bit);

    return atomic_and(&((int *)bitmap)[BITMAP_INT(bit)], ~mask) & mask ? 1:0;
}

static inline int bitmap_test(unsigned long *bitmap, int bit)
{
    return BIT_SET(bitmap[BITMAP_WORD(bit)], BITMAP_BIT_IN_WORD(bit));
}

/* find first zero bit starting from LSB */
static inline unsigned long _ffz(unsigned long x)
{
    return __builtin_ffsl(~x) - 1;
}

static inline int bitmap_ffz(unsigned long *bitmap, int numbits)
{
    uint i;
    int bit;

    for (i = 0; i < BITMAP_NUM_WORDS(numbits); i++) {
        if (bitmap[i] == ~0UL)
            continue;
        bit = i * BITMAP_BITS_PER_WORD + _ffz(bitmap[i]);
        if (bit < numbits)
            return bit;
        return -1;
    }
    return -1;
}

__END_CDECLS;

#endif
