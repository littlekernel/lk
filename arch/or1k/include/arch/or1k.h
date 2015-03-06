/*
 * Copyright (c) 2015 Stefan Kristiansson
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
