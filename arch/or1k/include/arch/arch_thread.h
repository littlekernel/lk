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

#include <sys/types.h>

struct or1k_context_switch_frame {
    uint32_t r1; // stack pointer
    uint32_t r2; // frame pointer

    uint32_t r9; // link register

    /* callee saved */
    uint32_t r10;
    uint32_t r14;
    uint32_t r16;
    uint32_t r18;
    uint32_t r20;
    uint32_t r22;
    uint32_t r24;
    uint32_t r26;
    uint32_t r28;
    uint32_t r30;
};

struct arch_thread {
    struct or1k_context_switch_frame cs_frame;
};

void or1k_context_switch(struct or1k_context_switch_frame *oldcs,
                         struct or1k_context_switch_frame *newcs);
