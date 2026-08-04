/* Copyright (c) 2008-2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

/*
 * Software fallback for the 64-bit divide/modulo compiler-rt routines.
 *
 * This file is only added to the build when no usable libgcc was found for
 * the target toolchain/multilib combination (see lib/libc/rules.mk and the
 * LIBGCC resolution in engine.mk). Building it unconditionally would let
 * these weak definitions satisfy the reference before the linker ever
 * consults libgcc.a on the link line, silently replacing libgcc's optimized
 * routines with this much slower bit-by-bit fallback.
 */

__attribute__((weak)) uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem) {
    if (den == 0) {
        if (rem) *rem = 0;
        return 0;
    }
    uint64_t q = 0;
    uint64_t r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((num >> i) & 1);
        if (r >= den) {
            r -= den;
            q |= (1ULL << i);
        }
    }
    if (rem) *rem = r;
    return q;
}

__attribute__((weak)) int64_t __divmoddi4(int64_t num, int64_t den, int64_t *rem) {
    bool neg_q = (num < 0) ^ (den < 0);
    bool neg_r = (num < 0);
    uint64_t un = (num < 0) ? -(uint64_t)num : (uint64_t)num;
    uint64_t ud = (den < 0) ? -(uint64_t)den : (uint64_t)den;

    uint64_t urem = 0;
    uint64_t uquot = __udivmoddi4(un, ud, &urem);

    if (rem) *rem = neg_r ? -(int64_t)urem : (int64_t)urem;
    return neg_q ? -(int64_t)uquot : (int64_t)uquot;
}

__attribute__((weak)) uint64_t __udivdi3(uint64_t num, uint64_t den) {
    return __udivmoddi4(num, den, NULL);
}

__attribute__((weak)) uint64_t __umoddi3(uint64_t num, uint64_t den) {
    uint64_t rem = 0;
    __udivmoddi4(num, den, &rem);
    return rem;
}

__attribute__((weak)) int64_t __divdi3(int64_t num, int64_t den) {
    return __divmoddi4(num, den, NULL);
}

__attribute__((weak)) int64_t __moddi3(int64_t num, int64_t den) {
    int64_t rem = 0;
    __divmoddi4(num, den, &rem);
    return rem;
}
