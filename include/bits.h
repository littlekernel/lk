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

#include <arch/ops.h>

#define clz(x) __builtin_clz(x)

#define BIT(x, bit) ((x) & (1 << (bit)))
#define BIT_SHIFT(x, bit) (((x) >> (bit)) & 1)
#define BITS(x, high, low) ((x) & (((1<<((high)+1))-1) & ~((1<<(low))-1)))
#define BITS_SHIFT(x, high, low) (((x) >> (low)) & ((1<<((high)-(low)+1))-1))
#define BIT_SET(x, bit) (((x) & (1 << (bit))) ? 1 : 0)

#define BITMAP_BITS_PER_WORD (sizeof(unsigned long) * 8)
#define BITMAP_NUM_WORDS(x) (((x) / BITMAP_BITS_PER_WORD) + 1)
#define BITMAP_WORD(x) ((x) / BITMAP_BITS_PER_WORD)
#define BITMAP_BIT_IN_WORD(x) ((x) & (BITMAP_BITS_PER_WORD - 1))

static inline int bitmap_set(unsigned long *bitmap, int bit)
{
	unsigned long mask = 1 << BITMAP_BIT_IN_WORD(bit);
	return atomic_or((int*)&bitmap[BITMAP_WORD(bit)], mask) & mask ? 1 : 0;
}

static inline int bitmap_clear(unsigned long *bitmap, int bit)
{
	unsigned long mask = 1 << BITMAP_BIT_IN_WORD(bit);

	return atomic_and((int*)&bitmap[BITMAP_WORD(bit)], ~mask) & mask ? 1:0;
}

static inline int bitmap_test(unsigned long *bitmap, int bit)
{
	return BIT_SET(bitmap[BITMAP_WORD(bit)], BITMAP_BIT_IN_WORD(bit));
}

#endif
