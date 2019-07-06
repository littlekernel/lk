/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/or1k-sprs.h>

#define mtspr(_spr, _val)                       \
    __asm__ __volatile__(                       \
        "l.mtspr r0, %1, %0"                    \
        :                                       \
        : "K" (_spr), "r" (_val)                \
    )

#define mtspr_off(_spr, _off, _val)             \
    __asm__ __volatile__ (                      \
        "l.mtspr %0, %1, %2"                    \
        :                                       \
        : "r" (_off), "r" (_val), "K" (_spr)    \
    )

#define mfspr(_spr)                             \
({                                              \
    uint32_t _val;                              \
    __asm__ __volatile__(                       \
        "l.mfspr %0, r0, %1"                    \
        : "=r"(_val)                            \
        : "K" (_spr)                            \
        );                                      \
    _val;                                       \
})

#define mfspr_off(_spr, _off)                   \
({                                              \
    uint32_t _val;                              \
    __asm__ __volatile__ (                      \
        "l.mfspr %0, %1, %2"                    \
        : "=r" (_val)                           \
        : "r" (_off), "K" (_spr)                \
        );                                      \
    _val;                                       \
})

#ifndef ASSEMBLY
struct or1k_iframe {
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t r13;
    uint32_t r14;
    uint32_t r15;
    uint32_t r16;
    uint32_t r17;
    uint32_t r18;
    uint32_t r19;
    uint32_t r20;
    uint32_t r21;
    uint32_t r22;
    uint32_t r23;
    uint32_t r24;
    uint32_t r25;
    uint32_t r26;
    uint32_t r27;
    uint32_t r28;
    uint32_t r29;
    uint32_t r30;
    uint32_t r31;
    uint32_t pc;
    uint32_t sr;
};
#endif // !ASSEMBLY
