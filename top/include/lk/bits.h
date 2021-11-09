/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/atomic.h>
#include <arch/ops.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

#define clz(x) __builtin_clz(x)
#define ctz(x) __builtin_ctz(x)
#define ffs(x) __builtin_ffs(x)

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

static inline int bitmap_set(unsigned long *bitmap, int bit) {
    unsigned long mask = 1UL << BITMAP_BIT_IN_INT(bit);
    return atomic_or(&((int *)bitmap)[BITMAP_INT(bit)], mask) & mask ? 1 : 0;
}

static inline int bitmap_clear(unsigned long *bitmap, int bit) {
    unsigned long mask = 1UL << BITMAP_BIT_IN_INT(bit);

    return atomic_and(&((int *)bitmap)[BITMAP_INT(bit)], ~mask) & mask ? 1:0;
}

static inline int bitmap_test(unsigned long *bitmap, int bit) {
    return BIT_SET(bitmap[BITMAP_WORD(bit)], BITMAP_BIT_IN_WORD(bit));
}

/* find first zero bit starting from LSB */
static inline unsigned long _ffz(unsigned long x) {
    return __builtin_ffsl(~x) - 1;
}

static inline int bitmap_ffz(unsigned long *bitmap, int numbits) {
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

__END_CDECLS
