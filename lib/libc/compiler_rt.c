/* Copyright (c) 2008-2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unwind.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

void *__dso_handle;



/* needed by some piece of EABI */
void raise(void) {
}

int __cxa_atexit(void (*destructor)(void *), void *arg, void *dso_handle) {
    return 0;
}

int __aeabi_atexit(void *arg, void (*func)(void *), void *d) {
    return __cxa_atexit(func, arg, d);
}

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
